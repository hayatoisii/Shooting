#include "PlayerBullet.h"
#include "Enemy.h"
#include "base/TextureManager.h"
#include <algorithm>
#include <cassert>
#include <math.h>

PlayerBullet::~PlayerBullet() { model_ = nullptr; }

void PlayerBullet::Initialize(KamataEngine::Model* model, const KamataEngine::Vector3& position, const KamataEngine::Vector3& velocity) {
	assert(model);
	model_ = model;
	worldtransfrom_.translation_ = position;
	worldtransfrom_.Initialize();
	velocity_ = velocity;

	const float kDesiredRange = 5000.0f;
	float speed = sqrtf(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);
	if (speed > 0.001f) {
		int32_t frames = static_cast<int32_t>(ceilf(kDesiredRange / speed));
		frames += 2;
		deathTimer_ = frames;
	}
}

void PlayerBullet::OnCollision() { isDead_ = true; }

// SlerpRotate関数（PlayerBullet内にも定義、あるいはヘッダーなどで共有しても良い）
KamataEngine::Vector3 PlayerBulletSlerp(const KamataEngine::Vector3& current, const KamataEngine::Vector3& target, float maxAngle) {
	float dot = current.x * target.x + current.y * target.y + current.z * target.z;
	dot = std::clamp(dot, -1.0f, 1.0f);
	float angle = std::acos(dot);
	if (std::abs(angle) < 0.001f || std::abs(angle - 3.14159f) < 0.001f)
		return target;
	float t = 1.0f;
	if (angle > maxAngle)
		t = maxAngle / angle;
	float sinTheta = std::sin(angle);
	float ps = std::sin(angle * (1.0f - t)) / sinTheta;
	float pt = std::sin(angle * t) / sinTheta;
	KamataEngine::Vector3 result;
	result.x = current.x * ps + target.x * pt;
	result.y = current.y * ps + target.y * pt;
	result.z = current.z * ps + target.z * pt;
	return result;
}

void PlayerBullet::Update() {
	if (--deathTimer_ <= 0) {
		isDead_ = true;
		return;
	}

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

	// --- ホーミング本処理 (刷新) ---
	if (isHomingEnabled_ && homingTarget_ && !homingTarget_->IsDead()) {
		if (!homingTarget_->IsOnScreen()) {
			isHomingEnabled_ = false;
			homingTarget_ = nullptr;
		} else {
			KamataEngine::Vector3 targetPos = homingTarget_->GetWorldPosition();
			KamataEngine::Vector3 bulletPos = GetWorldPosition();
			KamataEngine::Vector3 toTarget = {targetPos.x - bulletPos.x, targetPos.y - bulletPos.y, targetPos.z - bulletPos.z};
			float distance = sqrtf(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);

			// ヒット確定距離
			const float kHitRadius = 15.0f;
			if (distance <= kHitRadius) {
				homingTarget_->OnCollision();
				isDead_ = true;
				return;
			}

			if (distance > 0.001f) {
				// 現在の速度（大きさ）
				float currentSpeed = sqrtf(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);

				// 正規化ベクトル
				KamataEngine::Vector3 currentDir = velocity_;
				if (currentSpeed > 0.001f) {
					currentDir.x /= currentSpeed;
					currentDir.y /= currentSpeed;
					currentDir.z /= currentSpeed;
				}

				KamataEngine::Vector3 targetDir = toTarget;
				targetDir.x /= distance;
				targetDir.y /= distance;
				targetDir.z /= distance;

				// 通り過ぎ判定
				float dot = currentDir.x * targetDir.x + currentDir.y * targetDir.y + currentDir.z * targetDir.z;
				if (dot < -0.2f) {
					// 通り過ぎたらホーミング終了
					isHomingEnabled_ = false;
				} else {
					// ■ 回転角度制限
					// homingStrength_ をベースに、1フレームあたりの回転角度を決める
					// 1.0f なら 3度(0.05rad) 程度、近距離なら増やす

					float baseTurn = 0.05f * homingStrength_;

					// 近距離ブースト
					if (distance < 500.0f) {
						float rate = 1.0f - (distance / 500.0f);
						baseTurn += rate * 0.2f * homingStrength_;
					}

					// Slerpで向きを変える
					KamataEngine::Vector3 newDir = PlayerBulletSlerp(currentDir, targetDir, baseTurn);

					// 長さを整えて速度適用
					float len = sqrtf(newDir.x * newDir.x + newDir.y * newDir.y + newDir.z * newDir.z);
					if (len > 0.001f) {
						newDir.x /= len;
						newDir.y /= len;
						newDir.z /= len;
					}
					velocity_.x = newDir.x * currentSpeed;
					velocity_.y = newDir.y * currentSpeed;
					velocity_.z = newDir.z * currentSpeed;
				}
			}
		}
	} else if (isHomingEnabled_ && (!homingTarget_ || homingTarget_->IsDead())) {
		isHomingEnabled_ = false;
		homingTarget_ = nullptr;
	}

	worldtransfrom_.translation_.x += velocity_.x;
	worldtransfrom_.translation_.y += velocity_.y;
	worldtransfrom_.translation_.z += velocity_.z;
	worldtransfrom_.UpdateMatrix();
}

KamataEngine::Vector3 PlayerBullet::GetWorldPosition() {
	worldtransfrom_.UpdateMatrix();
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