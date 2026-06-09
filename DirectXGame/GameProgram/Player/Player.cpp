#include "Player.h"
#include "Enemy.h"
#include "PlayerState.h"
#include "RailCamera.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <limits>
// For window size constants
#include "base/WinApp.h"
#include <Windows.h>
#include <cstdio>
#include <vector>

Player::Player() { state_ = PlayerStateNormal::Instance(); }

Player::~Player() {
	delete modelbullet_;
	delete modelParticle_;
	delete engineExhaust_;
	for (PlayerBullet* bullet : bullets_) {
		delete bullet;
	}
}

void Player::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& pos) {
	assert(model);
	model_ = model;
	camera_ = camera;
	modelbullet_ = KamataEngine::Model::CreateFromOBJ("Bullet", true);
	input_ = KamataEngine::Input::GetInstance();
	audio_ = KamataEngine::Audio::GetInstance();
	if (audio_)
		hitPlayerSoundHandle_ = audio_->LoadWave("./sound/parry.wav");

	// Initialize() を先に呼んでからポジションを設定（逆順だと translation_ がリセットされる）
	worldtransfrom_.Initialize();
	worldtransfrom_.translation_ = pos;
	worldtransfrom_.UpdateMatrix();

	// パター・矢印の初期変換を初期化
	putterTransform_.Initialize();
	arrowTransform_.Initialize();
	arrowHeightTransform_.Initialize();
	// 軌跡 Draw 用は一度だけ初期化（parent_ なし）
	trailPointTf_.Initialize();
	trailPointTf_.parent_ = nullptr;
	ballTrail_.clear();
	trailSpawnTimer_ = 0;

	modelParticle_ = KamataEngine::Model::CreateFromOBJ("flare", true);
	engineExhaust_ = new ParticleEmitter();
	engineExhaust_->Initialize(modelParticle_);

	hp_ = kMaxHp_;
	isDead_ = false;
	shotTimer_ = 0;

	hitShakePrevVerticalOffset_ = 0.0f;
	hitShakePrevHorizontalOffset_ = 0.0f;

	velocity_ = {0.0f, 0.0f, 0.0f};
	swingTimer_ = 0.0f;

	// 初期状態は待機（地面に落下して止まる）
	ChangeState(PlayerStateNormal::Instance());
}

void Player::SetPosition(const KamataEngine::Vector3& position) {
	worldtransfrom_.translation_ = position;
}

void Player::ResetStats() {
	hp_ = kMaxHp_;
	isDead_ = false;
	gameOverAnimationTime_ = 0.0f;
	rollTimer_ = 0.0f;
	velocity_          = {0.0f, 0.0f, 0.0f};
	swingTimer_        = 0.0f;
	gaugeTimer_        = 0.0f;
	gaugePower_        = 0.0f;
	aimTimer_          = 0.0f;
	aimAngle_          = 0.0f;
	lockedArrowAngle_  = 0.0f;
	heightTimer_       = 0.0f;
	heightAngle_       = 3.14159265f * 30.0f / 180.0f;
	lockedHeightAngle_ = heightAngle_;
	ballTrail_.clear();
	trailSpawnTimer_   = 0;
	ChangeState(PlayerStateNormal::Instance());
}

// ゴルフボールの物理更新: 重力・バウンド・摩擦
void Player::UpdateGolfBall() {
	// 下向きの重力を速度に加える
	velocity_.y -= kGravity_;

	// 速度ぶんだけ位置を動かす
	worldtransfrom_.translation_.x += velocity_.x;
	worldtransfrom_.translation_.y += velocity_.y;
	worldtransfrom_.translation_.z += velocity_.z;

	// 地面より下に行かないようにする（バウンド）
	if (worldtransfrom_.translation_.y <= groundY_) {
		worldtransfrom_.translation_.y = groundY_;
		if (velocity_.y < -0.05f) {
			// バウンド: 上向きに反発
			velocity_.y = -velocity_.y * kBounceRestitution_;
		} else {
			// 十分小さければ完全停止
			velocity_.y = 0.0f;
		}
		// 転がり摩擦（地面接触中のみ）
		velocity_.x *= kRollFriction_;
		velocity_.z *= kRollFriction_;
	}

	// ボールの転がりを回転に反映（Z方向に転がるのでX軸回転）
	if (std::abs(velocity_.z) > 0.001f) {
		worldtransfrom_.rotation_.x -= velocity_.z * 0.3f;
	}

	worldtransfrom_.UpdateMatrix();

	// 飛翔中のみ軌跡ポイントを蓄積する（ランダムな散布で「軌跡が広がる」演出）
	if (state_ == PlayerStateFlying::Instance()) {
		--trailSpawnTimer_;
		if (trailSpawnTimer_ <= 0) {
			// -kSpread〜+kSpread のランダムオフセット
			const float kSpread = 0.35f;
			float ox = ((std::rand() % 201) - 100) * (kSpread / 100.0f);
			float oy = ((std::rand() % 201) - 100) * (kSpread / 100.0f);
			KamataEngine::Vector3 spawnPos = {
			    worldtransfrom_.translation_.x + ox,
			    worldtransfrom_.translation_.y + oy,
			    worldtransfrom_.translation_.z
			};
			ballTrail_.push_back({spawnPos, TrailPoint::kMaxLife});
			trailSpawnTimer_ = kTrailSpawnInterval_;
		}
	}

	// 軌跡ポイントを毎フレーム老化させ、寿命切れを削除
	for (auto& tp : ballTrail_) {
		tp.life -= 1.0f;
	}
	while (!ballTrail_.empty() && ballTrail_.front().life <= 0.0f) {
		ballTrail_.pop_front();
	}
}

// SPACE が今フレームで押されたか
bool Player::IsSpaceJustPressed() const {
	return input_ && input_->TriggerKey(DIK_SPACE);
}

// 速度がほぼゼロか
bool Player::IsVelocityNearZero() const {
	const float kThreshold = 0.005f;
	return std::abs(velocity_.x) < kThreshold &&
	       std::abs(velocity_.y) < kThreshold &&
	       std::abs(velocity_.z) < kThreshold;
}

// 照準開始: タイマーをリセットして Aiming 状態へ遷移
void Player::BeginAiming() {
	aimTimer_   = 0.0f;
	aimAngle_   = 0.0f;
	lockedArrowAngle_ = 0.0f;
	ChangeState(PlayerStateAiming::Instance());
}

// 矢印アニメを1フレーム進める（sin で ±kAimMaxAngle_ を往復）
void Player::UpdateAimArrow() {
	aimTimer_ += kAimSpeed_;
	aimAngle_ = kAimMaxAngle_ * std::sin(aimTimer_);

	// 矢印はボールの正面（Z+）方向に kArrowDist 離して配置
	const float kArrowDist = 2.5f;
	arrowTransform_.translation_ = {
		worldtransfrom_.translation_.x + std::sin(aimAngle_) * kArrowDist,
		worldtransfrom_.translation_.y,
		worldtransfrom_.translation_.z + std::cos(aimAngle_) * kArrowDist
	};
	// Y軸回転で方向を示す。細長く見せるため X スケールを小さく
	arrowTransform_.rotation_  = {0.0f, aimAngle_, 0.0f};
	arrowTransform_.scale_     = {0.3f, 0.3f, 1.8f};
	arrowTransform_.UpdateMatrix();
}

// 現在の照準角度を打撃方向として確定する
void Player::LockAimDirection() {
	lockedArrowAngle_ = aimAngle_;
}

// 照準状態かどうか
bool Player::IsAiming() const {
	return state_ == PlayerStateAiming::Instance();
}

// 高さ照準開始: Aiming → AimingHeight 遷移
void Player::BeginAimingHeight() {
	heightTimer_ = 0.0f;
	heightAngle_ = 3.14159265f * 30.0f / 180.0f; // 30° スタート
	ChangeState(PlayerStateAimingHeight::Instance());
}

// 高さ矢印アニメ: ロフト角を kHeightMinAngle_ ↔ kHeightMaxAngle_ で往復
void Player::UpdateAimHeight() {
	heightTimer_ += kHeightSpeed_;
	float mid  = (kHeightMaxAngle_ + kHeightMinAngle_) * 0.5f;
	float half = (kHeightMaxAngle_ - kHeightMinAngle_) * 0.5f;
	heightAngle_ = mid + half * std::sin(heightTimer_);

	// 横矢印と同じ: 中心をボールから 2.5 前方に置き、長さ 1.8
	const float kArrowLen  = 1.8f;
	const float kCenterDist = 2.5f; // 横矢印の kArrowDist と統一

	// 矢印が向く3次元方向ベクトル（Y回転=方向、X回転=仰角）
	float yaw   = lockedArrowAngle_;
	float pitch = heightAngle_;
	float dirX  = std::sin(yaw) * std::cos(pitch);
	float dirY  = std::sin(pitch);
	float dirZ  = std::cos(yaw) * std::cos(pitch);

	// 矢印の中心 = ボール位置 + 向き × kCenterDist
	// 根元は kCenterDist-kArrowLen/2 = 1.6 でボールの外側になる
	arrowHeightTransform_.translation_ = {
		worldtransfrom_.translation_.x + dirX * kCenterDist,
		worldtransfrom_.translation_.y + dirY * kCenterDist,
		worldtransfrom_.translation_.z + dirZ * kCenterDist
	};
	arrowHeightTransform_.rotation_ = {-heightAngle_, lockedArrowAngle_, 0.0f};
	arrowHeightTransform_.scale_    = {0.3f, 0.3f, kArrowLen};
	arrowHeightTransform_.UpdateMatrix();
}

// 高さ確定: 現在のロフト角を保存
void Player::LockAimHeight() {
	lockedHeightAngle_ = heightAngle_;
}

// 高さ照準状態かどうか
bool Player::IsAimingHeight() const {
	return state_ == PlayerStateAimingHeight::Instance();
}

// ゲージ開始: タイマーをリセットして Gauging 状態へ遷移
void Player::BeginGauging() {
	gaugeTimer_ = 0.0f;
	gaugePower_  = 0.0f;
	ChangeState(PlayerStateGauging::Instance());
}

// スイング開始: スイングタイマーをリセットして Swing 状態へ遷移
void Player::BeginSwing() {
	swingTimer_ = 0.0f;
	ChangeState(PlayerStateSwing::Instance());
}

// ゲージを1フレーム進める（sin で往復させる）
void Player::UpdateGauge() {
	gaugeTimer_ += kGaugeSpeed_;
	// sin を 0〜1 に正規化（0=底・最弱、1=頂・最強）
	gaugePower_ = (std::sin(gaugeTimer_) + 1.0f) * 0.5f;
}

// ゲージ状態かどうか
bool Player::IsGauging() const {
	return state_ == PlayerStateGauging::Instance();
}

// スイング演出の更新: パターが弧を描いてボールを打つ
// 完了したら true を返す
bool Player::UpdateSwingAnimation() {
	swingTimer_ += 1.0f;

	const float t = swingTimer_ / kSwingDuration_;

	// パターは斜め手前に配置（打つ方向と直交する位置）
	// lockedArrowAngle_ を使って打つ方向の左側から振り下ろす
	const float kPutterDist  = 2.2f;   // ボールからの距離
	const float kPutterOffsetY = 0.8f; // 少し上

	// 打つ方向に直交する左側（打つ方向角度 - 90度）に配置
	const float sideAngle = lockedArrowAngle_ - 3.14159265f * 0.5f;
	putterTransform_.translation_ = {
		worldtransfrom_.translation_.x + std::sin(sideAngle) * kPutterDist,
		worldtransfrom_.translation_.y + kPutterOffsetY,
		worldtransfrom_.translation_.z + std::cos(sideAngle) * kPutterDist
	};

	// X軸スイング + 左側からの振り下ろしY傾き
	const float swingAngle = 3.14159265f * 0.8f * std::sin(t * 3.14159265f);
	putterTransform_.rotation_ = {swingAngle, lockedArrowAngle_ - 3.14159265f * 0.5f, 0.0f};
	putterTransform_.UpdateMatrix();

	return t >= 1.0f;
}

// 打撃: ゲージ打力(0〜1) × ロフト角 × 水平方向角で3次元速度ベクトルを決定
void Player::LaunchBall() {
	float p          = gaugePower_;
	float totalSpeed = kShotSpeedZMin_ + (kShotSpeedZ_ - kShotSpeedZMin_) * p;
	float loft       = lockedHeightAngle_;

	// 水平速度 = totalSpeed * cos(loft)、垂直速度 = totalSpeed * sin(loft)
	float horizSpeed = totalSpeed * std::cos(loft);
	float vertSpeed  = totalSpeed * std::sin(loft);

	velocity_.x = horizSpeed * std::sin(lockedArrowAngle_);
	velocity_.z = horizSpeed * std::cos(lockedArrowAngle_);
	velocity_.y = vertSpeed;

	// 軌跡リセット（新しいショットの軌跡を新鮮に）
	ballTrail_.clear();
	trailSpawnTimer_ = 0;
}

void Player::ChangeState(PlayerState* newState) {
	if (newState == nullptr || newState == state_) {
		return;
	}
	state_ = newState;
}

bool Player::IsRolling() const {
	return state_ != nullptr && state_->IsRolling();
}

const char* Player::GetStateName() const {
	return state_ ? state_->GetStateName() : "None";
}

bool Player::IsFlying() const {
	return state_ == PlayerStateFlying::Instance();
}

// ポリモーフィズム: Player 固有の被弾処理（GameCharacter::OnCollision の override）
void Player::OnCollision() {
	// isDead_ = true; // 即死テスト用（通常は HP を減らす）
	hp_--;
	if (hp_ <= 0) {
		isDead_ = true;
	}

	// 被弾時に左右に揺れる
	hitShakeTime_ = 0.0f;
	hitShakeAmplitude_ = 0.6f;           // 最大振幅
	hitShakeVerticalAmplitude_ = 1.5f;   // 垂直
	hitShakeHorizontalAmplitude_ = 1.0f; // 水平
}

void Player::Attack() {

	if (shotTimer_ > 0) {
		shotTimer_--;
	}

	// これGameScene始まるまで撃たせないようにするやつ
	specialTimer--;
	if (specialTimer < 0) {
		if (input_->PushKey(DIK_SPACE) && shotTimer_ <= 0) {
			assert(railCamera_);

			// --- 弾発生位置の計算 ---
			// ワールド行列が最新であることを保証
			worldtransfrom_.UpdateMatrix();

			// プレイヤーのワールド位置とローカル軸を取得してスポーン位置を計算する
			KamataEngine::Vector3 playerWorldPos = GetWorldPosition();
			const KamataEngine::Matrix4x4& wm = worldtransfrom_.matWorld_;
			KamataEngine::Vector3 localForward = {wm.m[2][0], wm.m[2][1], wm.m[2][2]};
			KamataEngine::Vector3 localRight = {wm.m[0][0], wm.m[0][1], wm.m[0][2]};
			KamataEngine::Vector3 localUp = {wm.m[1][0], wm.m[1][1], wm.m[1][2]};
			localForward = KamataEngine::MathUtility::Normalize(localForward);
			localRight = KamataEngine::MathUtility::Normalize(localRight);
			localUp = KamataEngine::MathUtility::Normalize(localUp);

			// 揺れの影響を除いたクリーンな基準位置を使う（縦揺れで発射位置がズレるのを防ぐ）
			KamataEngine::Vector3 cleanPlayerPos = playerWorldPos;
			cleanPlayerPos.x -= hitShakePrevHorizontalOffset_; // 横揺れ分を除去
			cleanPlayerPos.y -= hitShakePrevVerticalOffset_;   // 縦揺れ分を除去

			// 逆方向（プレイヤーの後ろ／手前の反対）に大きくずらす
			float forwardOffset = -10.0f; // 大きめに移動させる（プレイヤーの向きの反対方向へ）
			float upOffset = -1.0f;
			float rightOffset = 0.0f;

			// もし localForward が不正（ゼロベクトル）ならカメラ前方向を使う
			KamataEngine::Vector3 cameraForward = {0.0f, 0.0f, 0.0f};
			KamataEngine::Vector3 cameraPosition = {0.0f, 0.0f, 0.0f};
			if (std::abs(localForward.x) < 1e-6f && std::abs(localForward.y) < 1e-6f && std::abs(localForward.z) < 1e-6f) {
				const KamataEngine::Matrix4x4& camMat = railCamera_->GetWorldTransform().matWorld_;
				cameraForward = {camMat.m[2][0], camMat.m[2][1], camMat.m[2][2]};
				cameraForward = KamataEngine::MathUtility::Normalize(cameraForward);
				cameraPosition = {camMat.m[3][0], camMat.m[3][1], camMat.m[3][2]};
			} else {
				// それでも念のためカメラ前方向も取得
				const KamataEngine::Matrix4x4& camMat = railCamera_->GetWorldTransform().matWorld_;
				cameraForward = {camMat.m[2][0], camMat.m[2][1], camMat.m[2][2]};
				cameraForward = KamataEngine::MathUtility::Normalize(cameraForward);
				cameraPosition = {camMat.m[3][0], camMat.m[3][1], camMat.m[3][2]};
			}

			// 優先: プレイヤーの向きの反対方向（後方）へ大きくずらす
			KamataEngine::Vector3 preferredMoveBullet = cleanPlayerPos - localForward * forwardOffset + localUp * upOffset + localRight * rightOffset;
			KamataEngine::Vector3 cameraBasedMoveBullet = cleanPlayerPos + cameraForward * forwardOffset + localUp * upOffset + localRight * rightOffset;

			// デバッグ出力: 座標を確認
			char dbgBuf[256];
			sprintf_s(
			    dbgBuf, "playerPos=(%.2f,%.2f,%.2f) localF=(%.2f,%.2f,%.2f) camF=(%.2f,%.2f,%.2f) prefBullet=(%.2f,%.2f,%.2f) camBullet=(%.2f,%.2f,%.2f)\\n", playerWorldPos.x, playerWorldPos.y,
			    playerWorldPos.z, localForward.x, localForward.y, localForward.z, cameraForward.x, cameraForward.y, cameraForward.z, preferredMoveBullet.x, preferredMoveBullet.y,
			    preferredMoveBullet.z, cameraBasedMoveBullet.x, cameraBasedMoveBullet.y, cameraBasedMoveBullet.z);
			OutputDebugStringA(dbgBuf);

			// プレイヤー基準（今回は後方へ大きくずらした位置）を優先して使う
			KamataEngine::Vector3 moveBullet = preferredMoveBullet;

			// --- ここまで弾発生位置の計算 ---

			const float kBulletSpeed = 60.0f; // 弾速
			KamataEngine::Vector3 velocity;

			float minDistanceSq = FLT_MAX;
			Enemy* nearestOnScreenEnemy = nullptr;
			const float maxHomingDistance = 1000.0f;

			if (enemies_) {
				for (Enemy* enemy : *enemies_) {
					if (!enemy || enemy->IsDead())
						continue;

					if (enemy->IsOnScreen()) {
						KamataEngine::Vector3 enemyPos = enemy->GetWorldPosition();
						KamataEngine::Vector3 toEnemy = enemyPos - moveBullet;
						float distanceSq = toEnemy.x * toEnemy.x + toEnemy.y * toEnemy.y + toEnemy.z * toEnemy.z;

						if (distanceSq < minDistanceSq && distanceSq < maxHomingDistance * maxHomingDistance) {
							float distance = sqrtf(distanceSq);
							if (distance > 0.001f) {
								minDistanceSq = distanceSq;
								nearestOnScreenEnemy = enemy;
							}
						}
					}
				}
			}

			{
				const KamataEngine::Matrix4x4& cameraWorldMatrix = railCamera_->GetWorldTransform().matWorld_;
				// reuse previously declared cameraPosition and cameraForward instead of redeclaring
				cameraPosition = {cameraWorldMatrix.m[3][0], cameraWorldMatrix.m[3][1], cameraWorldMatrix.m[3][2]};
				cameraForward = {cameraWorldMatrix.m[2][0], cameraWorldMatrix.m[2][1], cameraWorldMatrix.m[2][2]};
				cameraForward = KamataEngine::MathUtility::Normalize(cameraForward);
				KamataEngine::Vector3 targetPosition = cameraPosition + cameraForward * 1000.0f;
				velocity = targetPosition - moveBullet;
			}

			velocity = KamataEngine::MathUtility::Normalize(velocity);
			velocity = velocity * kBulletSpeed;

			PlayerBullet* newBullet = new PlayerBullet();
			newBullet->Initialize(modelbullet_, moveBullet, velocity);

			// ホーミング強度
			newBullet->SetHomingStrength(1.0f);

			// まず、アシストロック中の敵を優先して探す
			Enemy* assistLockedEnemy = nullptr;
			if (railCamera_ && enemies_) {
				const float kVisualRadius = 0.08f;
				// const float kDetectionRadius = 0.1f;
				const float kAspect = (float)KamataEngine::WinApp::kWindowWidth / (float)KamataEngine::WinApp::kWindowHeight;
				const float ndcVisualRadiusY = kVisualRadius * 2.0f;
				const float ndcVisualRadiusX = ndcVisualRadiusY / kAspect;
				// const float ndcDetectionRadiusY = kDetectionRadius * 2.0f;
				// const float ndcDetectionRadiusX = ndcDetectionRadiusY / kAspect;

				const KamataEngine::Matrix4x4& viewMatrix = railCamera_->GetViewProjection().matView;
				const KamataEngine::Matrix4x4& projMatrix = railCamera_->GetViewProjection().matProjection;

				// ロックオンされている敵（レティクルの円内）を探す
				for (Enemy* e : *enemies_) {
					if (!e || e->IsDead())
						continue;
					if (!e->IsOnScreen())
						continue;
					// ロックオンされている敵のみを対象にする
					if (!e->IsAssistLocked())
						continue;
					// world -> view
					KamataEngine::Vector3 worldPos = e->GetWorldPosition();
					KamataEngine::Vector3 viewPos;
					viewPos.x = worldPos.x * viewMatrix.m[0][0] + worldPos.y * viewMatrix.m[1][0] + worldPos.z * viewMatrix.m[2][0] + 1.0f * viewMatrix.m[3][0];
					viewPos.y = worldPos.x * viewMatrix.m[0][1] + worldPos.y * viewMatrix.m[1][1] + worldPos.z * viewMatrix.m[2][1] + 1.0f * viewMatrix.m[3][1];
					viewPos.z = worldPos.x * viewMatrix.m[0][2] + worldPos.y * viewMatrix.m[1][2] + worldPos.z * viewMatrix.m[2][2] + 1.0f * viewMatrix.m[3][2];
					if (viewPos.z <= 0.0f)
						continue;
					float clipX = viewPos.x * projMatrix.m[0][0] + viewPos.y * projMatrix.m[1][0] + viewPos.z * projMatrix.m[2][0] + 1.0f * projMatrix.m[3][0];
					float clipY = viewPos.x * projMatrix.m[0][1] + viewPos.y * projMatrix.m[1][1] + viewPos.z * projMatrix.m[2][1] + 1.0f * projMatrix.m[3][1];
					float w_clip = viewPos.x * projMatrix.m[0][3] + viewPos.y * projMatrix.m[1][3] + viewPos.z * projMatrix.m[2][3] + 1.0f * projMatrix.m[3][3];
					if (w_clip <= 0.0f)
						continue;
					float ndcX = clipX / w_clip;
					float ndcY = clipY / w_clip;
					float visualNormX = ndcX / ndcVisualRadiusX;
					float visualNormY = ndcY / ndcVisualRadiusY;
					float visualNormDistSq = (visualNormX * visualNormX) + (visualNormY * visualNormY);
					// レティクルの円内の敵のみを対象にする
					if (visualNormDistSq <= 1.0f) {
						assistLockedEnemy = e;
						break;
					}
				}
			}

			// ホーミング消したいときはここをコメントアウト
			// ロックオンされている敵（レティクルの円内）のみホーミングを有効化
			if (assistLockedEnemy && assistLockedEnemy->IsAssistLocked()) {
				// レティクル周辺の円内の敵に対してのみ即座にホーミングを有効化
				newBullet->SetHomingTarget(assistLockedEnemy);
				newBullet->SetHomingEnabled(true);
				newBullet->SetAimAssistHoming(true);
				newBullet->SetAssistLockId(assistLockedEnemy->GetAssistLockId());
			}

			bullets_.push_back(newBullet);

			if (audio_){
				audio_->playAudio(hitPlayerSound_, hitPlayerSoundHandle_, false, 0.5f);
		}

		// 連射の速度
		shotTimer_ = 5;
		isParry_ = false;
	}
}
}

bool Player::IsDead() const { return isDead_; }

int Player::GetHp() const { return hp_; }

int Player::GetMaxHp() const { return kMaxHp_; }

float Player::GetCollisionRadius() const { return 0.8f; }

const char* Player::GetKindName() const { return "Player"; }

KamataEngine::Vector3 Player::GetWorldPosition() const {
	KamataEngine::Vector3 worldPos;
	worldPos.x = worldtransfrom_.matWorld_.m[3][0];
	worldPos.y = worldtransfrom_.matWorld_.m[3][1];
	worldPos.z = worldtransfrom_.matWorld_.m[3][2];
	return worldPos;
}

AABB Player::GetAABB() {
	KamataEngine::Vector3 worldPos = GetWorldPosition();
	AABB aabb;
	aabb.min = {worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f};
	aabb.max = {worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f};
	return aabb;
}

void Player::SetParent(const KamataEngine::WorldTransform* parent) {
	worldtransfrom_.parent_ = parent;
	// パターも同じ親を持たせてワールド座標系を合わせる
	putterTransform_.parent_ = parent;
}

// State Pattern: 現在の状態オブジェクトに更新処理を委譲（ポリモーフィズム）
void Player::Update() {
	if (state_) {
		state_->Update(*this);
	}
}

void Player::UpdateBullets() {
	// 弾更新中にリストが変わっても安全なようスナップショットで回す
	std::vector<PlayerBullet*> bulletSnapshot;
	bulletSnapshot.reserve(bullets_.size());
	for (PlayerBullet* b : bullets_) {
		if (b)
			bulletSnapshot.push_back(b);
	}

	for (PlayerBullet* b : bulletSnapshot) {
		if (b)
			b->Update();
	}

	bullets_.remove_if([](PlayerBullet* bullet) {
		if (!bullet)
			return true;
		if (bullet->IsDead()) {
			delete bullet;
			return true;
		}
		return false;
	});
}

void Player::ProcessDodgeInput() {
	if (dodgeTimer_ > 0) {
		dodgeTimer_--;
		return;
	}

	if (!input_->PushKey(DIK_LSHIFT)) {
		return;
	}

	float dodgeDir = 0.0f;
	if (input_->PushKey(DIK_A))
		dodgeDir = -1.0f;
	else if (input_->PushKey(DIK_D))
		dodgeDir = 1.0f;

	if (dodgeDir != 0.0f && railCamera_) {
		BeginRolling(dodgeDir);
	}
}

void Player::BeginRolling(float direction) {
	railCamera_->Dodge(direction);
	rollTimer_ = 0.0f;
	rollDirection_ = direction;
	dodgeTimer_ = 10; // クールタイム
	// 状態遷移: 回避開始時に Rolling へ（Normal 状態クラスから呼ばれる）
	ChangeState(PlayerStateRolling::Instance());
}

void Player::UpdateRotationNormal() {
	Vector3 currentRotation = {0, 0, 0};
	worldtransfrom_.rotation_ = currentRotation;

	if (!railCamera_) {
		return;
	}

	const float lerpFactor = 0.1f;

	// ロール（横の傾き）
	float rollVelocity = railCamera_->GetRotationVelocity().z;
	const float tiltFactor = 5.0f;
	float targetRoll = rollVelocity * tiltFactor;

	float yawVelocity = railCamera_->GetRotationVelocity().y;
	const float yawTiltFactor = 50.0f;
	targetRoll -= yawVelocity * yawTiltFactor;

	const float maxRollAngle = 4.0f;
	targetRoll = std::clamp(targetRoll, -maxRollAngle, maxRollAngle);
	worldtransfrom_.rotation_.z += (targetRoll - worldtransfrom_.rotation_.z) * lerpFactor;

	float pitchVelocity = railCamera_->GetRotationVelocity().x;
	const float pitchFactor = 12.0f;
	float targetPitch = pitchVelocity * pitchFactor;
	const float maxPitchAngle = 1.5f;
	targetPitch = std::clamp(targetPitch, -maxPitchAngle, maxPitchAngle);
	worldtransfrom_.rotation_.x += (targetPitch - worldtransfrom_.rotation_.x) * lerpFactor;
}

bool Player::UpdateRotationRolling() {
	rollTimer_ += 1.0f;
	float t = rollTimer_ / kRollDuration_;
	const bool finished = t >= 1.0f;
	if (finished) {
		t = 1.0f;
	}

	float easeT = 1.0f - std::pow(1.0f - t, 3.0f);
	float maxAngle = 2.0f * 3.14159265f;

	Vector3 currentRotation = {0, 0, 0};
	currentRotation.z = maxAngle * easeT * rollDirection_ * -1.0f;
	worldtransfrom_.rotation_ = currentRotation;

	return finished;
}

void Player::UpdateHitShake() {
	worldtransfrom_.translation_.x -= hitShakePrevHorizontalOffset_;
	worldtransfrom_.translation_.y -= hitShakePrevVerticalOffset_;
	hitShakePrevHorizontalOffset_ = 0.0f;
	hitShakePrevVerticalOffset_ = 0.0f;

	if (hitShakeAmplitude_ <= 0.001f && hitShakeVerticalAmplitude_ <= 0.0001f && hitShakeHorizontalAmplitude_ <= 0.0001f) {
		return;
	}

	hitShakeTime_ += 1.0f;
	float damping = std::exp(-hitShakeDecay_ * hitShakeTime_);

	float angle = hitShakeAmplitude_ * damping * std::sin(hitShakeFrequency_ * hitShakeTime_ * 2.0f * 3.14159265f);
	worldtransfrom_.rotation_.y += angle;
	worldtransfrom_.rotation_.z += angle * 0.25f;

	float verticalOffset = hitShakeVerticalAmplitude_ * damping * std::sin(hitShakeFrequency_ * hitShakeTime_ * 2.0f * 3.14159265f);
	worldtransfrom_.translation_.y += verticalOffset;
	hitShakePrevVerticalOffset_ = verticalOffset;

	float horizontalOffset = hitShakeHorizontalAmplitude_ * damping * std::cos(hitShakeFrequency_ * hitShakeTime_ * 2.0f * 3.14159265f);
	worldtransfrom_.translation_.x += horizontalOffset;
	hitShakePrevHorizontalOffset_ = horizontalOffset;

	if (damping < 0.01f) {
		hitShakeAmplitude_ = 0.0f;
		hitShakeVerticalAmplitude_ = 0.0f;
		hitShakeHorizontalAmplitude_ = 0.0f;
		hitShakeTime_ = 0.0f;
		hitShakePrevVerticalOffset_ = 0.0f;
		hitShakePrevHorizontalOffset_ = 0.0f;
	}
}

void Player::FinalizeFrameUpdate() {
	worldtransfrom_.UpdateMatrix();
	Attack();

	if (engineExhaust_) {
		KamataEngine::Vector3 exhaustOffset = {0.0f, -0.3f, -3.0f};
		KamataEngine::Vector3 emitterPos = KamataEngine::MathUtility::Transform(exhaustOffset, worldtransfrom_.matWorld_);

		KamataEngine::Vector3 playerBackVector = {-worldtransfrom_.matWorld_.m[2][0], -worldtransfrom_.matWorld_.m[2][1], -worldtransfrom_.matWorld_.m[2][2]};
		playerBackVector = KamataEngine::MathUtility::Normalize(playerBackVector);

		const float exhaustSpeed = 0.5f;
		KamataEngine::Vector3 exhaustVelocity = playerBackVector * exhaustSpeed;

		engineExhaust_->Emit(emitterPos, exhaustVelocity);
		engineExhaust_->Update();
	}
}

void Player::Draw() {
	// 軌跡パーティクル（古い点ほど小さく描画してフェードアウト演出）
	// trailPointTf_ はメンバで一度だけ初期化済み。Draw コスト削減のため再利用する
	for (const auto& tp : ballTrail_) {
		float ratio = tp.life / TrailPoint::kMaxLife;
		float s     = 0.5f * ratio;
		trailPointTf_.translation_ = tp.pos;
		trailPointTf_.scale_       = {s, s, s};
		trailPointTf_.UpdateMatrix();
		model_->Draw(trailPointTf_, *camera_);
	}

	// ゴルフボール本体
	model_->Draw(worldtransfrom_, *camera_);

	// 水平方向矢印（照準中とゲージ中に表示）
	if (state_ == PlayerStateAiming::Instance() ||
	    state_ == PlayerStateAimingHeight::Instance() ||
	    state_ == PlayerStateGauging::Instance()) {
		model_->Draw(arrowTransform_, *camera_);
	}

	// 高さ矢印（高さ照準中とゲージ中に表示）
	if (state_ == PlayerStateAimingHeight::Instance() ||
	    state_ == PlayerStateGauging::Instance()) {
		model_->Draw(arrowHeightTransform_, *camera_);
	}

	// パター（スイング状態のときのみ描画）
	if (state_ == PlayerStateSwing::Instance()) {
		model_->Draw(putterTransform_, *camera_);
	}

	if (engineExhaust_) {
		engineExhaust_->Draw(*camera_);
	}
	for (PlayerBullet* bullet : bullets_) {
		bullet->Draw(*camera_);
	}
}

void Player::SetRailCamera(RailCamera* camera) { railCamera_ = camera; }

void Player::ResetRotation() {
	worldtransfrom_.rotation_ = {0.0f, 0.0f, 0.0f};
	worldtransfrom_.UpdateMatrix();
}

void Player::ResetParticles() {
	if (engineExhaust_) {
		engineExhaust_->Clear();
	}
}

// Dead 状態から呼ばれるゲームオーバー演出
void Player::UpdateGameOverAnimation() {
	const float animationTime = gameOverAnimationTime_;

	const float pitchDownAngle = 3.14159265f / 4.0f;
	worldtransfrom_.rotation_.x = pitchDownAngle;

	const float baseSpinSpeed = 0.01f;
	const float spinAcceleration = 0.0005f;
	float currentSpinSpeed = baseSpinSpeed + spinAcceleration * animationTime;
	worldtransfrom_.rotation_.y += currentSpinSpeed;

	const float baseFallSpeed = 0.02f;
	const float fallAcceleration = 0.001f;
	float currentFallSpeed = baseFallSpeed + fallAcceleration * animationTime;
	worldtransfrom_.translation_.y -= currentFallSpeed;

	worldtransfrom_.UpdateMatrix();

	if (engineExhaust_) {
		KamataEngine::Vector3 emitOffset = {0.8f, 0.0f, -0.8f};
		KamataEngine::Vector3 worldEmitPos = KamataEngine::MathUtility::Transform(emitOffset, worldtransfrom_.matWorld_);
		KamataEngine::Vector3 localVelocityDir = {1.0f, 1.0f, -0.5f};
		localVelocityDir = KamataEngine::MathUtility::Normalize(localVelocityDir);
		KamataEngine::Vector3 worldVelocityDir = KamataEngine::MathUtility::TransformNormal(localVelocityDir, worldtransfrom_.matWorld_);
		const float smokeSpeed = 0.5f;
		KamataEngine::Vector3 smokeVelocity = worldVelocityDir * smokeSpeed;
		engineExhaust_->Emit(worldEmitPos, smokeVelocity);
		engineExhaust_->Update();
	}
}

void Player::ResetBullets() {
	for (PlayerBullet* bullet : bullets_) {
		delete bullet;
	}
	bullets_.clear();
}

void Player::EvadeBullets(std::list<EnemyBullet*>& bullets) {

	if (IsRolling()) {

		// 回避中: 近距離のホーミング弾を無効化
		KamataEngine::Vector3 playerPos = GetWorldPosition();

		for (EnemyBullet* bullet : bullets) {
			if (!bullet)
				continue;
			if (!bullet->IsHoming())
				continue;

			KamataEngine::Vector3 bulletPos = bullet->GetWorldPosition();

			// 距離を計算
			float dx = playerPos.x - bulletPos.x;
			float dy = playerPos.y - bulletPos.y;
			float dz = playerPos.z - bulletPos.z;
			float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

			// 200以内の時に回避行動をしたらホーミングを失う
			const float kEvasionRange = 200.0f;
			if (dist < kEvasionRange) {
				bullet->OnEvaded();
			}
		}
	}
}
