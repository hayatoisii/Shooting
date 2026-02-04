#include "Player.h"
#include "Enemy.h"
#include "RailCamera.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
// For window size constants
#include "base/WinApp.h"
#include <Windows.h>
#include <cstdio>
#include <vector>

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
	worldtransfrom_.translation_ = pos;
	input_ = KamataEngine::Input::GetInstance();
	audio_ = KamataEngine::Audio::GetInstance();
	if (audio_)
		hitPlayerSoundHandle_ = audio_->LoadWave("./sound/parry.wav");


	worldtransfrom_.Initialize();

	modelParticle_ = KamataEngine::Model::CreateFromOBJ("flare", true);
	engineExhaust_ = new ParticleEmitter();
	engineExhaust_->Initialize(modelParticle_);

	hp_ = 3;
	isDead_ = false;
	shotTimer_ = 0;

	hitShakePrevVerticalOffset_ = 0.0f;
	hitShakePrevHorizontalOffset_ = 0.0f;
}

void Player::OnCollision() {
	// isDead_ = true; // 即死
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
				audio_->playAudio(hitPlayerSound_, hitPlayerSoundHandle_, false, 0.2f);
		}

		// 連射の速度
		shotTimer_ = 5;
		isParry_ = false;
	}
}
}

KamataEngine::Vector3 Player::GetWorldPosition() {
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

void Player::SetParent(const KamataEngine::WorldTransform* parent) { worldtransfrom_.parent_ = parent; }

void Player::Update() {

	// Update bullets safely: copy pointers to a temporary vector so that
	// if bullets_ is modified during an update (e.g. bullets marked dead by collision)
	// we won't iterate invalidated iterators.
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

	// Remove and delete dead bullets
	bullets_.remove_if([](PlayerBullet* bullet) {
		if (!bullet)
			return true;
		if (bullet->IsDead()) {
			delete bullet;
			return true;
		}
		return false;
	});

	if (dodgeTimer_ > 0) {
		dodgeTimer_--;
	} else {
		if (input_->PushKey(DIK_LSHIFT)) {
			float dodgeDir = 0.0f;
			if (input_->PushKey(DIK_A))
				dodgeDir = -1.0f;
			else if (input_->PushKey(DIK_D))
				dodgeDir = 1.0f;

			if (dodgeDir != 0.0f && railCamera_) {
				railCamera_->Dodge(dodgeDir);

				isRolling_ = true;
				rollTimer_ = 0.0f;
				rollDirection_ = dodgeDir;

				dodgeTimer_ = 10; // クールタイム
			}
		}
	}

	Vector3 currentRotation = {0, 0, 0};
	currentRotation.x = 0;

	if (isRolling_) {
		// === 回避アクション中 ===
		rollTimer_ += 1.0f;
		float t = rollTimer_ / kRollDuration_;
		if (t >= 1.0f) {
			t = 1.0f;
			isRolling_ = false;
		}

		float easeT = 1.0f - std::pow(1.0f - t, 3.0f);
		float maxAngle = 2.0f * 3.14159265f; // 360度

		currentRotation.z = maxAngle * easeT * rollDirection_ * -1.0f;

		worldtransfrom_.rotation_ = currentRotation;

	} else {

		worldtransfrom_.rotation_ = currentRotation;

		if (railCamera_) {
			const float lerpFactor = 0.1f;

			// --- ロール（横の傾き） ---
			float rollVelocity = railCamera_->GetRotationVelocity().z;
			const float tiltFactor = 5.0f;
			float targetRoll = rollVelocity * tiltFactor;

			// ヨーによるロールへの影響
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
	}

	// -- 被弾時の揺れ適用 --
	worldtransfrom_.translation_.x -= hitShakePrevHorizontalOffset_;
	worldtransfrom_.translation_.y -= hitShakePrevVerticalOffset_;
	hitShakePrevHorizontalOffset_ = 0.0f;
	hitShakePrevVerticalOffset_ = 0.0f;

	if (hitShakeAmplitude_ > 0.001f || hitShakeVerticalAmplitude_ > 0.0001f || hitShakeHorizontalAmplitude_ > 0.0001f) {
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

	worldtransfrom_.UpdateMatrix();

	Attack();

	// 排気パーティクル
	if (engineExhaust_) {
		KamataEngine::Vector3 exhaustOffset = {0.0f, -0.3f, -3.0f};
		KamataEngine::Vector3 emitterPos = KamataEngine::MathUtility::Transform(exhaustOffset, worldtransfrom_.matWorld_);

		KamataEngine::Vector3 playerBackVector = {-worldtransfrom_.matWorld_.m[2][0], -worldtransfrom_.matWorld_.m[2][1], -worldtransfrom_.matWorld_.m[2][2]};
		playerBackVector = KamataEngine::MathUtility::Normalize(playerBackVector);

		const float exhaustSpeed = 0.5f; // 排気速度
		KamataEngine::Vector3 exhaustVelocity = playerBackVector * exhaustSpeed;

		engineExhaust_->Emit(emitterPos, exhaustVelocity);
		engineExhaust_->Update();
	}
}

void Player::Draw() {
	model_->Draw(worldtransfrom_, *camera_);
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

void Player::UpdateGameOver(float animationTime) {
	// 姿勢制御
	const float pitchDownAngle = 3.14159265f / 4.0f; // const float pitchDownAngle = KamataEngine::MathUtility::PI / 4.0f;これかわらない、かわらない
	worldtransfrom_.rotation_.x = pitchDownAngle;

	// 回転
	const float baseSpinSpeed = 0.01f;
	const float spinAcceleration = 0.0005f;
	float currentSpinSpeed = baseSpinSpeed + spinAcceleration * animationTime;
	worldtransfrom_.rotation_.y += currentSpinSpeed;

	// 移動
	const float baseFallSpeed = 0.02f;
	const float fallAcceleration = 0.001f;
	float currentFallSpeed = baseFallSpeed + fallAcceleration * animationTime;
	worldtransfrom_.translation_.y -= currentFallSpeed;

	worldtransfrom_.UpdateMatrix();

	// パーティクル放出
	if (engineExhaust_) {
		KamataEngine::Vector3 emitOffset = {0.8f, 0.0f, -0.8f};
		KamataEngine::Vector3 worldEmitPos = KamataEngine::MathUtility::Transform(emitOffset, worldtransfrom_.matWorld_);
		KamataEngine::Vector3 localVelocityDir = {1.0f, 1.0f, -0.5f};
		localVelocityDir = KamataEngine::MathUtility::Normalize(localVelocityDir);
		KamataEngine::Vector3 worldVelocityDir = KamataEngine::MathUtility::TransformNormal(localVelocityDir, worldtransfrom_.matWorld_);
		const float smokeSpeed = 0.5f;
		KamataEngine::Vector3 smokeVelocity = worldVelocityDir * smokeSpeed;
		engineExhaust_->Emit(worldEmitPos, smokeVelocity);
	}

	if (engineExhaust_) {
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

	if (isRolling_) {

		//	const float kJustEvasionRange = 50.0f; // すれ違い判定の距離
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
