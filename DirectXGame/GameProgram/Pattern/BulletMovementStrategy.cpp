#include "BulletMovementStrategy.h"
#include "GameBalanceAccess.h"

#include "Enemy.h"
#include "EnemyBullet.h"
#include "GameCharacter.h"
#include "PlayerBullet.h"

#include <algorithm>
#include <cmath>

KamataEngine::Vector3 SlerpRotateDirection(const KamataEngine::Vector3& current, const KamataEngine::Vector3& target, float maxAngle) {
	float dot = current.x * target.x + current.y * target.y + current.z * target.z;
	dot = std::clamp(dot, -1.0f, 1.0f);
	float angle = std::acos(dot);
	if (std::abs(angle) < 0.001f || std::abs(angle - 3.14159f) < 0.001f) {
		return target;
	}
	float t = 1.0f;
	if (angle > maxAngle) {
		t = maxAngle / angle;
	}
	float sinTheta = std::sin(angle);
	float ps = std::sin(angle * (1.0f - t)) / sinTheta;
	float pt = std::sin(angle * t) / sinTheta;
	KamataEngine::Vector3 result;
	result.x = current.x * ps + target.x * pt;
	result.y = current.y * ps + target.y * pt;
	result.z = current.z * ps + target.z * pt;
	return result;
}

BulletMovementResult StraightBulletMovementStrategy::Apply(KamataEngine::Vector3& velocity, const KamataEngine::Vector3& position) const {
	(void)velocity;
	(void)position;
	return {};
}

BulletMovementResult HomingToEnemyMovementStrategy::Apply(PlayerBullet& bullet, KamataEngine::Vector3& velocity, const KamataEngine::Vector3& position) const {
	BulletMovementResult result;

	if (!bullet.IsHomingEnabled()) {
		return result;
	}

	Enemy* homingTarget = bullet.GetHomingTarget();
	if (homingTarget == nullptr || homingTarget->IsDead()) {
		bullet.SetHomingEnabled(false);
		bullet.SetHomingTarget(nullptr);
		return result;
	}

	if (!homingTarget->IsOnScreen()) {
		bullet.SetHomingEnabled(false);
		bullet.SetHomingTarget(nullptr);
		return result;
	}

	KamataEngine::Vector3 targetPos = homingTarget->GetWorldPosition();
	KamataEngine::Vector3 toTarget = {targetPos.x - position.x, targetPos.y - position.y, targetPos.z - position.z};
	float distance = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);

	const float kHitRadius = GameBalanceAccess::Get().GetFloat("playerBulletHomingHitRadius", 15.0f);
	if (distance <= kHitRadius) {
		homingTarget->OnCollision();
		result.shouldDestroy = true;
		return result;
	}

	if (distance <= 0.001f) {
		return result;
	}

	float currentSpeed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z);
	KamataEngine::Vector3 currentDir = velocity;
	if (currentSpeed > 0.001f) {
		currentDir.x /= currentSpeed;
		currentDir.y /= currentSpeed;
		currentDir.z /= currentSpeed;
	}

	KamataEngine::Vector3 targetDir = toTarget;
	targetDir.x /= distance;
	targetDir.y /= distance;
	targetDir.z /= distance;

	float dot = currentDir.x * targetDir.x + currentDir.y * targetDir.y + currentDir.z * targetDir.z;
	if (dot < -0.2f) {
		bullet.SetHomingEnabled(false);
		return result;
	}

	float baseTurn = 0.05f * bullet.GetHomingStrength();
	if (distance < 500.0f) {
		float rate = 1.0f - (distance / 500.0f);
		baseTurn += rate * 0.2f * bullet.GetHomingStrength();
	}

	KamataEngine::Vector3 newDir = SlerpRotateDirection(currentDir, targetDir, baseTurn);
	float len = std::sqrt(newDir.x * newDir.x + newDir.y * newDir.y + newDir.z * newDir.z);
	if (len > 0.001f) {
		newDir.x /= len;
		newDir.y /= len;
		newDir.z /= len;
	}
	velocity.x = newDir.x * currentSpeed;
	velocity.y = newDir.y * currentSpeed;
	velocity.z = newDir.z * currentSpeed;

	return result;
}

BulletMovementResult HomingToPlayerMovementStrategy::Apply(EnemyBullet& bullet, KamataEngine::Vector3& velocity, const KamataEngine::Vector3& position) const {
	BulletMovementResult result;

	if (!bullet.IsHoming()) {
		return result;
	}

	GameCharacter* homingTarget = bullet.GetHomingTarget();
	if (homingTarget == nullptr || homingTarget->IsDead()) {
		return result;
	}

	KamataEngine::Vector3 targetPos = homingTarget->GetWorldPosition();
	KamataEngine::Vector3 toTarget = {targetPos.x - position.x, targetPos.y - position.y, targetPos.z - position.z};
	float dist = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);

	const float kHitRange = GameBalanceAccess::Get().GetFloat("enemyBulletHomingHitRadius", 15.0f);
	if (dist <= kHitRange) {
		ApplyCollisionDamage(homingTarget);
		result.shouldDestroy = true;
		return result;
	}

	if (dist <= 0.001f) {
		return result;
	}

	KamataEngine::Vector3 toTargetDir = toTarget;
	toTargetDir.x /= dist;
	toTargetDir.y /= dist;
	toTargetDir.z /= dist;

	KamataEngine::Vector3 currentDir = velocity;
	float currentSpeed = std::sqrt(currentDir.x * currentDir.x + currentDir.y * currentDir.y + currentDir.z * currentDir.z);
	if (currentSpeed < 0.001f) {
		currentSpeed = bullet.GetSpeed();
	}

	currentDir.x /= currentSpeed;
	currentDir.y /= currentSpeed;
	currentDir.z /= currentSpeed;

	float maxTurnAngle = 0.05f;
	if (dist < 400.0f) {
		float rate = 1.0f - (dist / 400.0f);
		maxTurnAngle += rate * 0.3f;
	}

	KamataEngine::Vector3 newDir = SlerpRotateDirection(currentDir, toTargetDir, maxTurnAngle);
	float len = std::sqrt(newDir.x * newDir.x + newDir.y * newDir.y + newDir.z * newDir.z);
	if (len > 0.001f) {
		newDir.x /= len;
		newDir.y /= len;
		newDir.z /= len;
	}

	velocity.x = newDir.x * currentSpeed;
	velocity.y = newDir.y * currentSpeed;
	velocity.z = newDir.z * currentSpeed;

	return result;
}
