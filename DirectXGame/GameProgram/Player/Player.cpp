#include "Player.h"
#include "Enemy.h"
#include "RailCamera.h"
#include <algorithm>
#include <cassert>
#include <limits>
#include <cmath>

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
	modelbullet_ = KamataEngine::Model::CreateFromOBJ("cube", true);
	worldtransfrom_.translation_ = pos;
	input_ = KamataEngine::Input::GetInstance();

	worldtransfrom_.Initialize();

	modelParticle_ = KamataEngine::Model::CreateFromOBJ("cube", true);
	engineExhaust_ = new ParticleEmitter();
	engineExhaust_->Initialize(modelParticle_);
}

void Player::OnCollision() { isDead_ = true; }

void Player::Attack() {
	specialTimer--;
	if (specialTimer < 0) {
		if (input_->TriggerKey(DIK_SPACE)) {
			assert(railCamera_);

			KamataEngine::Vector3 moveBullet = GetWorldPosition();
			const float kBulletSpeed = 20.0f;
			KamataEngine::Vector3 velocity; // Will be calculated based on target or camera forward

			// Find the nearest enemy that is ALSO on screen
			float minDistanceSq = FLT_MAX;
			Enemy* nearestOnScreenEnemy = nullptr; // Changed variable name for clarity
			const float maxHomingDistance = 1000.0f;

			if (enemies_) { // Check if the enemy list pointer is valid
				for (Enemy* enemy : *enemies_) {
					// Skip dead enemies, null pointers
					if (!enemy || enemy->IsDead())
						continue;

					// --- ▼▼▼ Check if the enemy is on screen ▼▼▼ ---
					if (enemy->IsOnScreen()) { // ★ Add this check
						                       // --- ▲▲▲ ---

						KamataEngine::Vector3 enemyPos = enemy->GetWorldPosition();
						KamataEngine::Vector3 toEnemy = enemyPos - moveBullet;
						float distanceSq = toEnemy.x * toEnemy.x + toEnemy.y * toEnemy.y + toEnemy.z * toEnemy.z;

						// Check distance
						if (distanceSq < minDistanceSq && distanceSq < maxHomingDistance * maxHomingDistance) {
							float distance = sqrtf(distanceSq);
							if (distance > 0.001f) {
								minDistanceSq = distanceSq;
								nearestOnScreenEnemy = enemy; // Assign if closest *and* on screen
							}
						}
					} // Closing brace for the IsOnScreen() check
				}
			}

			// --- Determine initial velocity based on whether a target was found ---
			if (nearestOnScreenEnemy) {
				// Target found: Aim initial velocity towards it
				KamataEngine::Vector3 targetPosition = nearestOnScreenEnemy->GetWorldPosition();
				velocity = targetPosition - moveBullet;
			} else {
				// No target on screen: Fire straight using camera forward
				const KamataEngine::Matrix4x4& cameraWorldMatrix = railCamera_->GetWorldTransform().matWorld_;
				KamataEngine::Vector3 cameraPosition = {cameraWorldMatrix.m[3][0], cameraWorldMatrix.m[3][1], cameraWorldMatrix.m[3][2]};
				KamataEngine::Vector3 cameraForward = {cameraWorldMatrix.m[2][0], cameraWorldMatrix.m[2][1], cameraWorldMatrix.m[2][2]};
				cameraForward = KamataEngine::MathUtility::Normalize(cameraForward);
				KamataEngine::Vector3 targetPosition = cameraPosition + cameraForward * 1000.0f; // Far ahead
				velocity = targetPosition - moveBullet;
			}

			// Normalize velocity and apply speed
			velocity = KamataEngine::MathUtility::Normalize(velocity);
			velocity = velocity * kBulletSpeed;

			PlayerBullet* newBullet = new PlayerBullet();
			newBullet->Initialize(modelbullet_, moveBullet, velocity); // Use the calculated velocity

			// Set random homing strength (optional)
			{

				newBullet->SetHomingStrength(0.50f);
			}

			// --- Set homing target ONLY if an on-screen enemy was found ---
			if (nearestOnScreenEnemy) {
				newBullet->SetHomingTarget(nearestOnScreenEnemy);
				newBullet->SetHomingEnabled(true);
			}
			// If nearestOnScreenEnemy is null, homing won't be enabled

			bullets_.push_back(newBullet);
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

void Player::SetParent(const WorldTransform* parent) { worldtransfrom_.parent_ = parent; }

void Player::Update() {
	Attack();

	for (PlayerBullet* bullet : bullets_) {
		bullet->Update();
	}

	bullets_.remove_if([](PlayerBullet* bullet) {
		if (bullet->IsDead()) {
			delete bullet;
			return true;
		}
		return false;
	});

	const float kPlayerMaxX = 4.0f;
	float targetX = 0.0f;
	if (input_->PushKey(DIK_A)) {
		targetX = kPlayerMaxX;
	}
	if (input_->PushKey(DIK_D)) {
		targetX = -kPlayerMaxX;
	}

	float targetY = worldtransfrom_.translation_.y;

	float moveLerpFactor = 0.0f;
	if (input_->PushKey(DIK_A) || input_->PushKey(DIK_D)) {
		moveLerpFactor = 0.015f;
	} else {
		moveLerpFactor = 0.03f;
	}
	worldtransfrom_.translation_.x += (targetX - worldtransfrom_.translation_.x) * moveLerpFactor;

	// Y軸は targetY と translation_.y が同じ値なので、右辺が 0 になり、Y座標は変更されなくなる
	worldtransfrom_.translation_.y += (targetY - worldtransfrom_.translation_.y) * moveLerpFactor;

	if (railCamera_) {
		// 左右
		float yawVelocity = railCamera_->GetRotationVelocity().y;
		const float tiltFactor = -90.0f; // 傾きの速度
		float targetRoll = yawVelocity * tiltFactor;
		const float maxRollAngle = 4.0f; // 傾きの最大値
		targetRoll = std::clamp(targetRoll, -maxRollAngle, maxRollAngle);
		const float lerpFactor = 0.1f;
		worldtransfrom_.rotation_.z += (targetRoll - worldtransfrom_.rotation_.z) * lerpFactor;

		// 上
		float pitchVelocity = railCamera_->GetRotationVelocity().x;
		const float pitchFactor = 70.0f;
		float targetPitch = pitchVelocity * pitchFactor;
		const float maxPitchAngle = 1.5f;
		targetPitch = std::clamp(targetPitch, -maxPitchAngle, maxPitchAngle);
		worldtransfrom_.rotation_.x += (targetPitch - worldtransfrom_.rotation_.x) * lerpFactor;
	}

	// 1. プレイヤーのワールド行列を計算して、位置を確定
	worldtransfrom_.UpdateMatrix();

	// 2. 最新の座標から排気口のワールド座標を計算
	KamataEngine::Vector3 exhaustOffset = {0.0f, -0.2f, -1.0f};
	exhaustOffset = KamataEngine::MathUtility::TransformNormal(exhaustOffset, worldtransfrom_.matWorld_);
	KamataEngine::Vector3 emitterPos = GetWorldPosition() + exhaustOffset;

	KamataEngine::Vector3 playerBackVector = {-worldtransfrom_.matWorld_.m[2][0], -worldtransfrom_.matWorld_.m[2][1], -worldtransfrom_.matWorld_.m[2][2]};
	// ベクトルを正規化して純粋な方向にする
	playerBackVector = KamataEngine::MathUtility::Normalize(playerBackVector);

	// 4. 排気の速度を決定
	const float exhaustSpeed = 1.5f; // この数値を大きくすると排気が速くなる
	KamataEngine::Vector3 exhaustVelocity = playerBackVector * exhaustSpeed;

	// 5. 計算した「位置」と「速度」でパーティクルを発生
	engineExhaust_->Emit(emitterPos, exhaustVelocity);

	// 4. パーティクルシステムを更新
	engineExhaust_->Update();
}

void Player::Draw() {
	model_->Draw(worldtransfrom_, *camera_);
	engineExhaust_->Draw(*camera_);
	for (PlayerBullet* bullet : bullets_) {
		bullet->Draw(*camera_);
	}
}

void Player::SetRailCamera(RailCamera* camera) { railCamera_ = camera; }

void Player::ResetRotation() {
	// Playerのローカルな回転（傾きなど）をすべてゼロに戻す
	worldtransfrom_.rotation_ = {0.0f, 0.0f, 0.0f};
}

void Player::ResetParticles() {
	if (engineExhaust_) {
		engineExhaust_->Clear(); // ParticleEmitter の Clear 関数を呼び出す
	}
}