#include "EnemyBullet.h"
#include "BulletMovementStrategy.h"
#include "GameCharacter.h"
#include "Player.h"
#include <algorithm>
#include <cassert>
#include <cmath>



namespace {

HomingToPlayerMovementStrategy kHomingToPlayerMovement;

} // namespace



EnemyBullet::~EnemyBullet() { model_ = nullptr; }



void EnemyBullet::SetHomingTarget(Player* target) {

	// Player は GameCharacter の派生クラス

	homingTarget_ = target;

}



void EnemyBullet::Initialize(KamataEngine::Model* model, const KamataEngine::Vector3& position, const KamataEngine::Vector3& velocity) {

	assert(model);

	model_ = model;

	worldtransfrom_.translation_ = position;

	worldtransfrom_.Initialize();

	velocity_ = velocity;

	isDead_ = false;

	evadedDeathTimer_ = -1;

	deathTimer_ = kLifeTime;



	// 速度（スカラ）を保持

	float sp = std::sqrt(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);

	if (sp > 0.001f) {

		speed_ = sp;

	} else {

		speed_ = 1.0f; // 安全策

	}



	invulnerableFrames_ = 8;

}



void EnemyBullet::OnEvaded() {

	isHoming_ = false;

	evadedDeathTimer_ = 60;

}



bool EnemyBullet::IsDead() const { return isDead_; }



float EnemyBullet::GetCollisionRadius() const { return 0.8f; }



const char* EnemyBullet::GetKindName() const { return "EnemyBullet"; }



KamataEngine::Vector3 EnemyBullet::GetWorldPosition() const {

	KamataEngine::Vector3 worldPos;

	worldPos.x = worldtransfrom_.matWorld_.m[3][0];

	worldPos.y = worldtransfrom_.matWorld_.m[3][1];

	worldPos.z = worldtransfrom_.matWorld_.m[3][2];

	return worldPos;

}



bool EnemyBullet::UpdateLifetime() {

	if (evadedDeathTimer_ > 0) {

		evadedDeathTimer_--;

		if (evadedDeathTimer_ <= 0) {

			isDead_ = true;

			return false;

		}

	}



	if (--deathTimer_ <= 0) {

		isDead_ = true;

		return false;

	}



	if (invulnerableFrames_ > 0) {

		invulnerableFrames_--;

	}



	return true;

}



void EnemyBullet::ApplyMovement() {

	KamataEngine::Vector3 position = GetWorldPosition();

	BulletMovementResult result = kHomingToPlayerMovement.Apply(*this, velocity_, position);

	if (result.shouldDestroy) {

		isDead_ = true;

	}

}



void EnemyBullet::UpdateTransform() {

	worldtransfrom_.translation_.x += velocity_.x;

	worldtransfrom_.translation_.y += velocity_.y;

	worldtransfrom_.translation_.z += velocity_.z;



	worldtransfrom_.UpdateMatrix();

}



void EnemyBullet::Update() {

	UpdateFrame();

}



// ポリモーフィズム: 敵弾の消滅処理（GameBullet::OnCollision の override）

void EnemyBullet::OnCollision() { isDead_ = true; }



void EnemyBullet::Draw(const KamataEngine::Camera& camera) {

	if (!isDead_) {

		model_->Draw(worldtransfrom_, camera);

	}

}