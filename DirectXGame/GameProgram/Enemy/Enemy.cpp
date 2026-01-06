#include "Enemy.h"
#include "2d/Sprite.h"
#include "GaneScene.h"
#include "KamataEngine.h"
#include "Player.h"
#include "base/TextureManager.h"
#include "base/WinApp.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>

Enemy::~Enemy() {
	delete modelbullet_;
	delete targetSprite_;
	delete directionIndicatorSprite_;
	delete assistLockSprite_;
}

void Enemy::Initialize(KamataEngine::Model* model, const KamataEngine::Vector3& pos) {
	assert(model);
	model_ = model;
	modelbullet_ = KamataEngine::Model::CreateFromOBJ("cube", true);
	worldtransfrom_.Initialize();
	worldtransfrom_.translation_ = pos;

	initialWorldPos_ = pos;

	circleTimer_ = 0.0f;

	isFollowing_ = false;

	isFollowingFast_ = false;

	worldtransfrom_.UpdateMatrix();

	hp_ = 5;

	uint32_t indicatorHandle = TextureManager::Load("indicator.png");
	directionIndicatorSprite_ = Sprite::Create(indicatorHandle, {0, 0});
	if (directionIndicatorSprite_) {
		directionIndicatorSprite_->SetSize({40.0f, 40.0f});
		directionIndicatorSprite_->SetAnchorPoint({0.5f, 0.5f});
	}
	isOffScreen_ = false;

	assistLockTextureHandle_ = TextureManager::Load("lockongreen.png");
	assistLockSprite_ = Sprite::Create(assistLockTextureHandle_, {0, 0});
	if (assistLockSprite_) {
		assistLockSprite_->SetSize({1.0f, 1.0f});            // サイズは調整
		assistLockSprite_->SetColor({0.0f, 1.0f, 0.0f, 1.0f}); // 緑色
		assistLockSprite_->SetAnchorPoint({0.5f, 0.5f});
	}
	isAssistLocked_ = false;

	uint32_t texHandle = TextureManager::Load("redbox.png");
	targetSprite_ = Sprite::Create(texHandle, {0, 0});
	if (targetSprite_) {
		targetSprite_->SetSize({50.0f, 50.0f});
		targetSprite_->SetColor({1.0f, 0.0f, 0.0f, 1.0f});
		targetSprite_->SetAnchorPoint({0.5f, 0.5f});
	}
	isOnScreen_ = false;
	wasOnScreenLastFrame_ = false;
	lockOnAnimRotation_ = 0.0f;
	lockOnAnimScale_ = 1.0f;

	// 大航海のような広範囲移動の初期化（X軸とZ軸に散らばる）
	baseX_ = pos.x;
	baseZ_ = pos.z;
	// 初期にランダムにスポーンする処理
	const float kInitMaxOffset = 1000.0f; // spawn range -> 1000 per request
	currentOffsetX_ = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f) * kInitMaxOffset; // 初期位置をランダムに散らす
	currentOffsetZ_ = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f) * kInitMaxOffset; // 初期位置をランダムに散らす
	moveSpeedX_ = 1.0f + (static_cast<float>(rand()) / RAND_MAX) * 1.0f; // X軸方向の速度
	moveSpeedZ_ = 1.0f + (static_cast<float>(rand()) / RAND_MAX) * 0.8f; // Z軸方向の速度
	directionX_ = (rand() % 2 == 0) ? 1.0f : -1.0f; // ランダムな初期X方向
	directionZ_ = (rand() % 2 == 0) ? 1.0f : -1.0f; // ランダムな初期Z方向
	directionChangeIntervalX_ = static_cast<float>(rand() % 180 + 90); // 90-270フレームのランダムな間隔
	directionChangeIntervalZ_ = static_cast<float>(rand() % 200 + 100); // 100-300フレームのランダムな間隔
	directionChangeTimerX_ = 0.0f;
	directionChangeTimerZ_ = 0.0f;

	prevRenderedX_ = baseX_ + currentOffsetX_;
	prevRenderedZ_ = baseZ_ + currentOffsetZ_;
	smoothedForward_ = {0.0f, 0.0f, 1.0f};

	// ゆっくり大きく曲がる
	wanderAngle_ = (static_cast<float>(rand()) / RAND_MAX) * (2.0f * 3.14159265f);
	wanderJitter_ = 0.02f + (static_cast<float>(rand()) / RAND_MAX) * 0.03f;
	wanderRadius_ = 1200.0f + (static_cast<float>(rand()) / RAND_MAX) * 800.0f;
	wanderDistance_ = 900.0f + (static_cast<float>(rand()) / RAND_MAX) * 600.0f;
	desiredSpeed_ = (0.6f + (static_cast<float>(rand()) / RAND_MAX) * 0.8f) * 3.0f;

	posSmoothFactor_ = 0.06f;    //  小さくすると遅れて滑らか
	facingSmoothFactor_ = 0.04f; // 小さくするとゆっくり回る
	turnSmoothFactor_ = 0.04f;   // 方向変化の滑らかさ
}

KamataEngine::Vector3 Enemy::GetWorldPosition() {
	KamataEngine::Vector3 worldPos;
	worldPos.x = worldtransfrom_.matWorld_.m[3][0];
	worldPos.y = worldtransfrom_.matWorld_.m[3][1];
	worldPos.z = worldtransfrom_.matWorld_.m[3][2];
	return worldPos;
}

void Enemy::OnCollision() {
	hp_--;
	if (hp_ <= 0) {
		isDead_ = true;
		if (gameScene_) {
			gameScene_->RequestExplosion(GetWorldPosition());
		}
	};
}

void Enemy::Fire() {
	assert(player_);
	spawnTimer--;

	if (spawnTimer < 0.0f) {

		KamataEngine::Vector3 moveBullet = GetWorldPosition();
		const float kBulletSpeed = 10.0f;
		KamataEngine::Vector3 velocity(0, 0, 0);

		KamataEngine::Vector3 playerWorldtransform = player_->GetWorldPosition();
		KamataEngine::Vector3 enemyWorldtransform = GetWorldPosition();

		KamataEngine::Vector3 homingBullet = playerWorldtransform - enemyWorldtransform;

		homingBullet = KamataEngine::MathUtility::Normalize(homingBullet);

		velocity.x = kBulletSpeed * homingBullet.x;
		velocity.y = kBulletSpeed * homingBullet.y;
		velocity.z = kBulletSpeed * homingBullet.z;

		EnemyBullet* newBullet = new EnemyBullet();
		newBullet->Initialize(modelbullet_, moveBullet, velocity);

		newBullet->SetHomingEnabled(true);
		newBullet->SetHomingTarget(player_);
		newBullet->SetSpeed(kBulletSpeed);

		if (gameScene_) {
			gameScene_->AddEnemyBullet(newBullet);
		}

		spawnTimer = kFireInterval;
	}
}

void Enemy::Update() {

	// Fire();

	// 大航海のような広範囲移動処理（X軸とZ軸に散らばって移動し続ける）
	directionChangeTimerX_++;
	directionChangeTimerZ_++;
	
	// X軸方向の変更処理
	if (directionChangeTimerX_ >= directionChangeIntervalX_) {
		// ランダムに方向を変更（-1.0f または 1.0f）
		directionX_ = (rand() % 2 == 0) ? 1.0f : -1.0f;
		// 次の方向変更までの時間をランダムに設定（90-270フレーム）
		directionChangeTimerX_ = 0.0f;
		directionChangeIntervalX_ = static_cast<float>(rand() % 180 + 90);
	}

	// Z軸方向の変更処理
	if (directionChangeTimerZ_ >= directionChangeIntervalZ_) {
		// ランダムに方向を変更（-1.0f または 1.0f）
		directionZ_ = (rand() % 2 == 0) ? 1.0f : -1.0f;
		// 次の方向変更までの時間をランダムに設定（100-300フレーム）
		directionChangeTimerZ_ = 0.0f;
		directionChangeIntervalZ_ = static_cast<float>(rand() % 200 + 100);
	}

	// 滑らかに補間
	float targetX = baseX_ + currentOffsetX_;
	float targetZ = baseZ_ + currentOffsetZ_;

	// 位置を滑らかに補間
	float renderX = prevRenderedX_ + (targetX - prevRenderedX_) * posSmoothFactor_;
	float renderZ = prevRenderedZ_ + (targetZ - prevRenderedZ_) * posSmoothFactor_;

	worldtransfrom_.translation_.x = renderX;
	worldtransfrom_.translation_.z = renderZ;

	// 現在のレンダリング位置の移動ベクトル
	KamataEngine::Vector3 moveDir = { renderX - prevRenderedX_, 0.0f, renderZ - prevRenderedZ_ };
	float lenMove = std::sqrt(moveDir.x * moveDir.x + moveDir.z * moveDir.z);
	if (lenMove > 0.0001f) {
		moveDir.x /= lenMove;
		moveDir.z /= lenMove;

		smoothedForward_.x += (moveDir.x - smoothedForward_.x) * facingSmoothFactor_;
		smoothedForward_.z += (moveDir.z - smoothedForward_.z) * facingSmoothFactor_;

		// 計算した前方ベクトルからヨー角を求めてモデルに適用（モデル前方が Z+ の場合）
		float yaw = std::atan2(smoothedForward_.x, smoothedForward_.z);
		worldtransfrom_.rotation_.y = yaw;
	}

	prevRenderedX_ = renderX;
	prevRenderedZ_ = renderZ;

	// Y座標は固定

	worldtransfrom_.UpdateMatrix();

	if (camera_ && targetSprite_) {
		UpdateScreenPosition();
	}

	// ウォーカーステアリングによる大きな滑らかな曲線移動の実現
	KamataEngine::Vector3 currentVelocity = { smoothedVelocity_.x, 0.0f, smoothedVelocity_.z };

	KamataEngine::Vector3 forward = { smoothedForward_.x, 0.0f, smoothedForward_.z };
	KamataEngine::Vector3 wanderCenter = forward;
	// normalize
	{
		float lv = wanderCenter.x * wanderCenter.x + wanderCenter.z * wanderCenter.z;
		if (lv > 0.0001f) {
			float inv = 1.0f / std::sqrt(lv);
			wanderCenter.x *= inv;
			wanderCenter.z *= inv;
		}
	}
	wanderCenter.x *= wanderDistance_;
	wanderCenter.z *= wanderDistance_;
	
	// ジッターで角度を少しずつ動かす
	wanderAngle_ += ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f) * wanderJitter_;

	// wander 円上の点
	KamataEngine::Vector3 wanderPoint = { std::sin(wanderAngle_) * wanderRadius_, 0.0f, std::cos(wanderAngle_) * wanderRadius_ };

	// 目標ベロシティは wanderCenter + wanderPoint
	KamataEngine::Vector3 targetVelocity = { wanderCenter.x + wanderPoint.x, 0.0f, wanderCenter.z + wanderPoint.z };
	// 正規化して speed を与える
	{
		float lv = targetVelocity.x * targetVelocity.x + targetVelocity.z * targetVelocity.z;
		if (lv > 0.0001f) {
			float inv = 1.0f / std::sqrt(lv);
			targetVelocity.x *= inv * desiredSpeed_;
			targetVelocity.z *= inv * desiredSpeed_;
		}
	}

	// smoothedVelocity_ をゆっくり目に追従させる（turnSmoothFactor_ が小さいほどゆっくり曲がる）
	smoothedVelocity_.x += (targetVelocity.x - smoothedVelocity_.x) * turnSmoothFactor_;
	smoothedVelocity_.z += (targetVelocity.z - smoothedVelocity_.z) * turnSmoothFactor_;

	// 新しい方向を計算してオフセットに反映
	{
		float lv = smoothedVelocity_.x * smoothedVelocity_.x + smoothedVelocity_.z * smoothedVelocity_.z;
		if (lv > 0.0001f) {
			float inv = 1.0f / std::sqrt(lv);
			float vx = smoothedVelocity_.x * inv;
			float vz = smoothedVelocity_.z * inv;
			// オフセットを速度に合わせて更新
			currentOffsetX_ += vx * desiredSpeed_;
			currentOffsetZ_ += vz * desiredSpeed_;
			// 回転補間にも反映
			smoothedForward_.x += (vx - smoothedForward_.x) * facingSmoothFactor_;
			smoothedForward_.z += (vz - smoothedForward_.z) * facingSmoothFactor_;
			float yaw = std::atan2(smoothedForward_.x, smoothedForward_.z);
			worldtransfrom_.rotation_.y = yaw;
		}
	}

	// オフセットの値が大きくなりすぎないように制限
	// clamp by radius to enforce circular limit
	const float kMaxOffsetRadius = 1500.0f; // movement upper bound -> 1500 per request
	float distSq = currentOffsetX_ * currentOffsetX_ + currentOffsetZ_ * currentOffsetZ_;
	if (distSq > kMaxOffsetRadius * kMaxOffsetRadius) {
		float r = std::sqrt(distSq);
		if (r > 0.0001f) {
			currentOffsetX_ = (currentOffsetX_ / r) * kMaxOffsetRadius;
			currentOffsetZ_ = (currentOffsetZ_ / r) * kMaxOffsetRadius;
		}
	}
}

void Enemy::Draw(const KamataEngine::Camera& camera) { model_->Draw(worldtransfrom_, camera); }

void Enemy::DrawSprite() {
	if (isOnScreen_) {
		if (useGreenLock_) {
			if (assistLockSprite_) {
				assistLockSprite_->Draw();
			}
		} else {
			if (targetSprite_)
				targetSprite_->Draw();
		}
	}

	// 画面外 かつ インジケーター表示距離内の場合のみ、方向インジケーターを描画する
	if (isOffScreen_ && showDirectionIndicator_ && directionIndicatorSprite_) {
		directionIndicatorSprite_->Draw();
	}

	if (isAssistLocked_ && assistLockSprite_) {
		// アシストロックオン中はロックオンスプライトも描画する
		assistLockSprite_->Draw();
	}
}

void Enemy::UpdateScreenPosition() {

	if (!camera_ || !targetSprite_ || !directionIndicatorSprite_) {
		isOnScreen_ = false;
		isOffScreen_ = false;
		return;
	}
	const KamataEngine::Matrix4x4& viewMatrix = camera_->matView;
	const KamataEngine::Matrix4x4& projMatrix = camera_->matProjection;
	KamataEngine::Vector2 screenCenter = {KamataEngine::WinApp::kWindowWidth / 2.0f, KamataEngine::WinApp::kWindowHeight / 2.0f};

	KamataEngine::Vector3 worldPos = GetWorldPosition();
	KamataEngine::Vector3 viewPos;
	viewPos.x = worldPos.x * viewMatrix.m[0][0] + worldPos.y * viewMatrix.m[1][0] + worldPos.z * viewMatrix.m[2][0] + 1.0f * viewMatrix.m[3][0];
	viewPos.y = worldPos.x * viewMatrix.m[0][1] + worldPos.y * viewMatrix.m[1][1] + worldPos.z * viewMatrix.m[2][1] + 1.0f * viewMatrix.m[3][1];
	viewPos.z = worldPos.x * viewMatrix.m[0][2] + worldPos.y * viewMatrix.m[1][2] + worldPos.z * viewMatrix.m[2][2] + 1.0f * viewMatrix.m[3][2];

	// 距離に基づいて、緑ロックか赤ロックかを決定する
	const float kLockDistanceThreshold = 3000.0f;
	bool farForLock = (viewPos.z > kLockDistanceThreshold);

	// 距離に基づいて、画面外の方向インジケーターを表示するかきめる
	const float kIndicatorMaxDistance = 3000.0f; // 2500
	float viewDist = std::sqrt(viewPos.x * viewPos.x + viewPos.y * viewPos.y + viewPos.z * viewPos.z);
	showDirectionIndicator_ = (viewDist <= kIndicatorMaxDistance);

	if (viewPos.z > 0.0f) {
		float clipX = viewPos.x * projMatrix.m[0][0] + viewPos.y * projMatrix.m[1][0] + viewPos.z * projMatrix.m[2][0] + 1.0f * projMatrix.m[3][0];
		float clipY = viewPos.x * projMatrix.m[0][1] + viewPos.y * projMatrix.m[1][1] + viewPos.z * projMatrix.m[2][1] + 1.0f * projMatrix.m[3][1];
		float w_clip = viewPos.x * projMatrix.m[0][3] + viewPos.y * projMatrix.m[1][3] + viewPos.z * projMatrix.m[2][3] + 1.0f * projMatrix.m[3][3];

		if (w_clip > 0.0f) {
			float ndcX = clipX / w_clip;
			float ndcY = clipY / w_clip;

			if (ndcX >= -1.0f && ndcX <= 1.0f && ndcY >= -1.0f && ndcY <= 1.0f) {
				// 画面内
				isOnScreen_ = true;
				isOffScreen_ = false;
				float screenX = (ndcX + 1.0f) * 0.5f * KamataEngine::WinApp::kWindowWidth;
				float screenY = (1.0f - ndcY) * 0.5f * KamataEngine::WinApp::kWindowHeight;
				targetSprite_->SetPosition({screenX, screenY});
				if (assistLockSprite_) {
					assistLockSprite_->SetPosition({screenX, screenY});
				}

				// 距離に基づいて useGreenLock_ (緑ロックを使用するか) を設定する
				useGreenLock_ = farForLock;
			} else {
				// 画面外・前方
				isOnScreen_ = false;
				isOffScreen_ = true;
				float screenX = (ndcX + 1.0f) * 0.5f * KamataEngine::WinApp::kWindowWidth;
				float screenY = (1.0f - ndcY) * 0.5f * KamataEngine::WinApp::kWindowHeight;
				KamataEngine::Vector2 vecFromCenter = {screenX - screenCenter.x, screenY - screenCenter.y};
				float angle = std::atan2(vecFromCenter.y, vecFromCenter.x);

				const float kIndicatorRadius = 70.0f;
				float indicatorX = screenCenter.x + kIndicatorRadius * std::cos(angle);
				float indicatorY = screenCenter.y + kIndicatorRadius * std::sin(angle);

				const float kScreenMargin = 20.0f;
				indicatorX = std::clamp(indicatorX, kScreenMargin, KamataEngine::WinApp::kWindowWidth - kScreenMargin);
				indicatorY = std::clamp(indicatorY, kScreenMargin, KamataEngine::WinApp::kWindowHeight - kScreenMargin);

				directionIndicatorSprite_->SetPosition({indicatorX, indicatorY});
				directionIndicatorSprite_->SetRotation(angle + KamataEngine::MathUtility::PI / 2.0f);
			}
		} else {
			isOnScreen_ = false;
			isOffScreen_ = true;

			float angle = std::atan2(-viewPos.y, -viewPos.x);

			const float kIndicatorRadius = 70.0f;
			float indicatorX = screenCenter.x + kIndicatorRadius * std::cos(angle);
			float indicatorY = screenCenter.y + kIndicatorRadius * std::sin(angle);

			const float kScreenMargin = 20.0f;
			indicatorX = std::clamp(indicatorX, kScreenMargin, KamataEngine::WinApp::kWindowWidth - kScreenMargin);
			indicatorY = std::clamp(indicatorY, kScreenMargin, KamataEngine::WinApp::kWindowHeight - kScreenMargin);

			directionIndicatorSprite_->SetPosition({indicatorX, indicatorY});
			directionIndicatorSprite_->SetRotation(angle + 3.14159265f / 2.0f);// directionIndicatorSprite_->SetRotation(angle + KamataEngine::MathUtility::PI / 2.0f); どっちもかわらない
		}
	} else {
		isOnScreen_ = false;
		isOffScreen_ = true;

		float angle = std::atan2(-viewPos.y, -viewPos.x);

		const float kIndicatorRadius = 70.0f;
		float indicatorX = screenCenter.x + kIndicatorRadius * std::cos(angle);
		float indicatorY = screenCenter.y + kIndicatorRadius * std::sin(angle);

		const float kScreenMargin = 20.0f;
		indicatorX = std::clamp(indicatorX, kScreenMargin, KamataEngine::WinApp::kWindowWidth - kScreenMargin);
		indicatorY = std::clamp(indicatorY, kScreenMargin, KamataEngine::WinApp::kWindowHeight - kScreenMargin);

		directionIndicatorSprite_->SetPosition({indicatorX, indicatorY});
		directionIndicatorSprite_->SetRotation(angle + KamataEngine::MathUtility::PI / 2.0f);
	}

	bool justAppeared = (isOnScreen_ && !wasOnScreenLastFrame_);

	if (justAppeared) {
		lockOnAnimRotation_ = 3.14159265f * 1.0f;// lockOnAnimRotation_ = KamataEngine::MathUtility::PI * 1.0f; どっちも変わらない
		lockOnAnimScale_ = 2.0f;
	}

	const float kLockOnAnimFriction = 0.90f;
	const float kTargetScale = 1.0f;

	if (lockOnAnimRotation_ > 0.0f) {
		/// 赤い回転ロックfalseの場合のみ、回転アニメーションを実行する
		if (!useGreenLock_) {
			lockOnAnimRotation_ *= kLockOnAnimFriction;
			if (lockOnAnimRotation_ < 0.01f) {
				lockOnAnimRotation_ = 0.0f;
			}
		}
	}

	if (lockOnAnimScale_ != kTargetScale) {
		lockOnAnimScale_ += (kTargetScale - lockOnAnimScale_) * (1.0f - kLockOnAnimFriction);
		if (std::abs(lockOnAnimScale_ - kTargetScale) < 0.01f) {
			lockOnAnimScale_ = kTargetScale;
		}
	}

	if (targetSprite_) {
		/// 赤い回転ロックfalseの場合のみ、回転アニメーションを実行する
		if (!useGreenLock_) {
			targetSprite_->SetRotation(lockOnAnimRotation_);
			const float baseSize = 50.0f;
			targetSprite_->SetSize({baseSize * lockOnAnimScale_, baseSize * lockOnAnimScale_});
		} else {
			targetSprite_->SetRotation(0.0f);
			const float baseSize = 50.0f;
			targetSprite_->SetSize({baseSize, baseSize});
		}
	}

	// 緑ロックの場合、アシストロックスプライトのサイズを設定する
	if (assistLockSprite_) {
		if (useGreenLock_) {
			assistLockSprite_->SetSize({15.0f, 15.0f});
		} else {
			assistLockSprite_->SetSize({20.0f, 20.0f});
		}
		float rollAngle = std::atan2(viewMatrix.m[0][1], viewMatrix.m[1][1]);

		// スプライトに回転を設定
		assistLockSprite_->SetRotation(rollAngle);
	}

	wasOnScreenLastFrame_ = isOnScreen_;
}

void Enemy::SetParent(const KamataEngine::WorldTransform* parent) { worldtransfrom_.parent_ = parent; }