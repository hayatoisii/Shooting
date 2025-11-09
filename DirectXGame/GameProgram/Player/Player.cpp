#include "Player.h"
#include "Enemy.h"      // Enemy クラスの定義が必要
#include "RailCamera.h" // RailCamera クラスの定義が必要
#include <algorithm>    // std::clamp
#include <cassert>
#include <cmath>  // sqrt, sqrtf, std::rand, RAND_MAX
#include <limits> // FLT_MAX

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
	modelbullet_ = KamataEngine::Model::CreateFromOBJ("Bullet", true); // "Bullet" モデルを使用
	worldtransfrom_.translation_ = pos;
	input_ = KamataEngine::Input::GetInstance();

	worldtransfrom_.Initialize();

	modelParticle_ = KamataEngine::Model::CreateFromOBJ("flare", true); // "flare" モデルを使用
	engineExhaust_ = new ParticleEmitter();
	engineExhaust_->Initialize(modelParticle_);

	hp_ = 3;         // ★ HP初期化
	isDead_ = false; // 死亡フラグ初期化
}

void Player::OnCollision() {
	// isDead_ = true; // 即死ではなくHPを減らす
	hp_--;           // HPを減らす
	if (hp_ <= 0) {  // HPが0以下になったら
		isDead_ = true; // 死亡フラグを立てる
	}
}

void Player::Attack() {
	specialTimer--;
	if (specialTimer < 0) {
		if (input_->TriggerKey(DIK_SPACE)) {
			assert(railCamera_);

			KamataEngine::Vector3 moveBullet = GetWorldPosition();
			moveBullet.y -= 0.5f; // 弾の発射位置を調整
			const float kBulletSpeed = 20.0f; // 弾速
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

			if (nearestOnScreenEnemy) {
				KamataEngine::Vector3 targetPosition = nearestOnScreenEnemy->GetWorldPosition();
				velocity = targetPosition - moveBullet;
			} else {
				const KamataEngine::Matrix4x4& cameraWorldMatrix = railCamera_->GetWorldTransform().matWorld_;
				KamataEngine::Vector3 cameraPosition = {cameraWorldMatrix.m[3][0], cameraWorldMatrix.m[3][1], cameraWorldMatrix.m[3][2]};
				KamataEngine::Vector3 cameraForward = {cameraWorldMatrix.m[2][0], cameraWorldMatrix.m[2][1], cameraWorldMatrix.m[2][2]};
				cameraForward = KamataEngine::MathUtility::Normalize(cameraForward);
				KamataEngine::Vector3 targetPosition = cameraPosition + cameraForward * 1000.0f;
				velocity = targetPosition - moveBullet;
			}

			velocity = KamataEngine::MathUtility::Normalize(velocity);
			velocity = velocity * kBulletSpeed;

			PlayerBullet* newBullet = new PlayerBullet();
			newBullet->Initialize(modelbullet_, moveBullet, velocity);

			// ▼▼▼ ホーミング強度を修正 ▼▼▼
			// 0.10f だと緩すぎるため、1.0f (100%補正) に変更
			newBullet->SetHomingStrength(1.0f);
			// ▲▲▲ 修正完了 ▲▲▲

			if (nearestOnScreenEnemy) {
				newBullet->SetHomingTarget(nearestOnScreenEnemy);
				newBullet->SetHomingEnabled(true);
			}

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

void Player::SetParent(const KamataEngine::WorldTransform* parent) { worldtransfrom_.parent_ = parent; }

void Player::Update() {
	Attack(); // 攻撃処理

	// 弾の更新と削除
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

	// --- 移動処理 ---
	const float kPlayerMaxX = 2.5f; // 横移動の限界値
	float targetX = 0.0f;
	if (input_->PushKey(DIK_A)) {
		targetX = kPlayerMaxX;
	}
	if (input_->PushKey(DIK_D)) {
		targetX = -kPlayerMaxX;
	}
	float targetY = worldtransfrom_.translation_.y;
	float moveLerpFactor = (input_->PushKey(DIK_A) || input_->PushKey(DIK_D)) ? 0.015f : 0.03f;
	worldtransfrom_.translation_.x += (targetX - worldtransfrom_.translation_.x) * moveLerpFactor;
	worldtransfrom_.translation_.y += (targetY - worldtransfrom_.translation_.y) * moveLerpFactor;

	// --- 回転処理 (カメラの動きに連動) ---
	if (railCamera_) {
		const float lerpFactor = 0.1f;

		// float yawVelocity = railCamera_->GetRotationVelocity().y; // ★ 修正前
		float yawDelta = railCamera_->GetLastDeltaYaw(); // ★ 修正後
		const float tiltFactor = -75.0f;                 // 傾き係数
		// float targetRoll = yawVelocity * tiltFactor;               // ★ 修正前
		float targetRoll = yawDelta * tiltFactor; // ★ 修正後
		const float maxRollAngle = 4.0f;          // 最大傾き
		targetRoll = std::clamp(targetRoll, -maxRollAngle, maxRollAngle);
		worldtransfrom_.rotation_.z += (targetRoll - worldtransfrom_.rotation_.z) * lerpFactor;

		// float pitchVelocity = railCamera_->GetRotationVelocity().x; // ★ 修正前
		float pitchDelta = railCamera_->GetLastDeltaPitch(); // ★ 修正後
		const float pitchFactor = 35.0f;                     // 傾き係数
		// float targetPitch = pitchVelocity * pitchFactor;             // ★ 修正前
		float targetPitch = pitchDelta * pitchFactor; // ★ 修正後
		const float maxPitchAngle = 1.5f;             // 最大傾き
		targetPitch = std::clamp(targetPitch, -maxPitchAngle, maxPitchAngle);
		worldtransfrom_.rotation_.x += (targetPitch - worldtransfrom_.rotation_.x) * lerpFactor;
	}

	// --- ワールド行列の更新 ---
	worldtransfrom_.UpdateMatrix();

	// --- 排気パーティクルの処理 ---
	if (engineExhaust_) {
		KamataEngine::Vector3 exhaustOffset = {0.0f, -0.5f, -2.0f};
		KamataEngine::Vector3 emitterPos = KamataEngine::MathUtility::Transform(exhaustOffset, worldtransfrom_.matWorld_); // TransformNormal ではなく Transform

		KamataEngine::Vector3 playerBackVector = {-worldtransfrom_.matWorld_.m[2][0], -worldtransfrom_.matWorld_.m[2][1], -worldtransfrom_.matWorld_.m[2][2]};
		playerBackVector = KamataEngine::MathUtility::Normalize(playerBackVector);

		const float exhaustSpeed = 0.5f; // 排気速度
		KamataEngine::Vector3 exhaustVelocity = playerBackVector * exhaustSpeed;

		engineExhaust_->Emit(emitterPos, exhaustVelocity);
		engineExhaust_->Update(); // ★ UpdateGameOver との重複注意
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
	// --- 1. 姿勢制御 (斜め下向き) ---
	const float pitchDownAngle = KamataEngine::MathUtility::PI / 4.0f;
	worldtransfrom_.rotation_.x = pitchDownAngle;

	// --- 2. 回転 (徐々に加速) ---
	const float baseSpinSpeed = 0.01f;
	const float spinAcceleration = 0.0005f;
	float currentSpinSpeed = baseSpinSpeed + spinAcceleration * animationTime;
	worldtransfrom_.rotation_.y += currentSpinSpeed; // Y軸周りに回転

	// --- 3. 移動 (徐々に加速して落下) ---
	const float baseFallSpeed = 0.02f;
	const float fallAcceleration = 0.001f;
	float currentFallSpeed = baseFallSpeed + fallAcceleration * animationTime;
	worldtransfrom_.translation_.y -= currentFallSpeed; // Y座標を減らす

	// --- 4. ワールド行列を更新 ---
	worldtransfrom_.UpdateMatrix(); // ここまでの変更を反映

	// --- 5. パーティクル放出 (斜め右上へ) ---
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

	// --- 6. パーティクルシステム自体の更新 ---
	if (engineExhaust_) {
		engineExhaust_->Update(); // ★ ゲームオーバー中はここで更新
	}
}