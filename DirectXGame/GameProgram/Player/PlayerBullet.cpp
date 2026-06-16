#include "PlayerBullet.h"
#include "BulletMovementStrategy.h"
#include "Enemy.h"
#include "base/TextureManager.h"
#include <algorithm>
#include <cassert>
#include <math.h>

namespace {
StraightBulletMovementStrategy kStraightMovement;
HomingToEnemyMovementStrategy kHomingToEnemyMovement;
} // namespace

PlayerBullet::~PlayerBullet() { model_ = nullptr; }

void PlayerBullet::Initialize(KamataEngine::Model* model, const KamataEngine::Vector3& position, const KamataEngine::Vector3& velocity) {
	assert(model);
	model_ = model;
	worldtransfrom_.translation_ = position;
	worldtransfrom_.Initialize();
	velocity_ = velocity;
	isDead_ = false;

	homingTarget_ = nullptr;
	isHomingEnabled_ = false;
	isAimAssistHoming_ = false;
	assistLockId_ = 0;
	pendingHomingTarget_ = nullptr;
	pendingLockDistance_ = 0.0f;
	deathTimer_ = kLifeTime;

	const float kDesiredRange = 5000.0f;
	float speed = sqrtf(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);
	if (speed > 0.001f) {
		int32_t frames = static_cast<int32_t>(ceilf(kDesiredRange / speed));
		frames += 2;
		deathTimer_ = frames;
	}
}

// ポリモーフィズム: 自機弾の消滅処理（GameBullet::OnCollision の override）
void PlayerBullet::OnCollision() { isDead_ = true; }

bool PlayerBullet::IsDead() const { return isDead_; }

float PlayerBullet::GetCollisionRadius() const { return 0.8f; }

const char* PlayerBullet::GetKindName() const { return "PlayerBullet"; }

bool PlayerBullet::UpdateLifetime() {
	if (--deathTimer_ <= 0) {
		isDead_ = true;
		return false;
	}
	return true;
}

void PlayerBullet::UpdatePreMovement() {
	// Pending Homing
	if (pendingHomingTarget_ && !isHomingEnabled_) {
		if (pendingHomingTarget_->GetAssistLockId() != assistLockId_) {
			pendingHomingTarget_ = nullptr;
			pendingLockDistance_ = 0.0f;
		} else if (!pendingHomingTarget_->IsDead()) {
			KamataEngine::Vector3 targetPos = pendingHomingTarget_->GetWorldPosition();
			KamataEngine::Vector3 bulletPos = GetWorldPosition();
			float dx = targetPos.x - bulletPos.x;
			float dy = targetPos.y - bulletPos.y;
			float dz = targetPos.z - bulletPos.z;
			float distSq = dx * dx + dy * dy + dz * dz;
			if (distSq <= pendingLockDistance_ * pendingLockDistance_) {
				homingTarget_ = pendingHomingTarget_;
				isHomingEnabled_ = true;
				isAimAssistHoming_ = true;
				pendingHomingTarget_ = nullptr;
				pendingLockDistance_ = 0.0f;
			}
		} else {
			pendingHomingTarget_ = nullptr;
			pendingLockDistance_ = 0.0f;
		}
	}
}

void PlayerBullet::ApplyMovement() {
	KamataEngine::Vector3 position = GetWorldPosition();
	BulletMovementResult result;
	if (isHomingEnabled_) {
		result = kHomingToEnemyMovement.Apply(*this, velocity_, position);
	} else {
		result = kStraightMovement.Apply(velocity_, position);
	}
	if (result.shouldDestroy) {
		isDead_ = true;
	}
}

void PlayerBullet::UpdateTransform() {
	worldtransfrom_.translation_.x += velocity_.x;
	worldtransfrom_.translation_.y += velocity_.y;
	worldtransfrom_.translation_.z += velocity_.z;
	worldtransfrom_.UpdateMatrix();
}

void PlayerBullet::Update() {
	UpdateFrame();
}

KamataEngine::Vector3 PlayerBullet::GetWorldPosition() const {
	KamataEngine::Vector3 worldPos;
	worldPos.x = worldtransfrom_.matWorld_.m[3][0];
	worldPos.y = worldtransfrom_.matWorld_.m[3][1];
	worldPos.z = worldtransfrom_.matWorld_.m[3][2];
	return worldPos;
}

void PlayerBullet::Draw(const KamataEngine::Camera& camera) {
	if (!isDead_) {
		model_->Draw(worldtransfrom_, camera);
	}
}
