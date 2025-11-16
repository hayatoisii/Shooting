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

	hp_ = 20;

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
		assistLockSprite_->SetSize({1.0f, 1.0f});            // (サイズは調整してください)
		assistLockSprite_->SetColor({0.0f, 1.0f, 0.0f, 1.0f}); // (例: 緑色)
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

		if (gameScene_) {
			gameScene_->AddEnemyBullet(newBullet);
		}

		spawnTimer = kFireInterval;
	}
}

void Enemy::Update() {

	// Fire();

	assert(player_ && "Enemy::Update() player_ が null です");
	/*
	KamataEngine::Vector3 playerPos = player_->GetWorldPosition();
	KamataEngine::Vector3 myCurrentPos = GetWorldPosition();

	KamataEngine::Vector3 vecToPlayer = playerPos - myCurrentPos;
	float distance = KamataEngine::MathUtility::Length(vecToPlayer);

	// この数値よりPlayerから離れたらEnemyが追尾する
	const float kFarDistance = 2500.0f;
	// これより近づいたら追尾やめる
	const float kNearDistance = 2490.0f;
	// 追尾速度
	const float kFastSpeed = 5.0f;
	// 常に進み続ける速度
	const float kSlowSpeed = 0.1f;

	float currentSpeed = 0.0f;

	if (distance > kFarDistance) {
	    isFollowingFast_ = true;
	    currentSpeed = kFastSpeed;
	} else if (distance < kNearDistance) {
	    isFollowingFast_ = false;
	    currentSpeed = kSlowSpeed;
	} else {
	    currentSpeed = (isFollowingFast_) ? kFastSpeed : kSlowSpeed;
	}

	if (distance < 0.01f) {
	    currentSpeed = 0.0f;
	}

	KamataEngine::Vector3 velocity = {0.0f, 0.0f, 0.0f};
	if (currentSpeed > 0.0f) {
	    KamataEngine::Vector3 dirToPlayer = KamataEngine::MathUtility::Normalize(vecToPlayer);
	    velocity = dirToPlayer * currentSpeed;
	}

	KamataEngine::Vector3 newPosition = myCurrentPos + velocity;
	worldtransfrom_.translation_ = newPosition;
	*/

	worldtransfrom_.UpdateMatrix();

	if (camera_ && targetSprite_) {
		UpdateScreenPosition();
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
		// アシストロックオン中は、(緑の) ロックオンスプライトも描画する
		assistLockSprite_->Draw();
	}
}

void Enemy::UpdateScreenPosition() {

	// isAssistLocked_ = false;

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

	// 緑ロックの場合、アシストロックスプライトのサイズを設定する (固定値)
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