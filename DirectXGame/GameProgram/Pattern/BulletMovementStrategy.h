#pragma once

#include <KamataEngine.h>

class Enemy;
class GameCharacter;
class PlayerBullet;
class EnemyBullet;

// Strategy Pattern: 弾の移動（直進・ホーミング）を切り替える
struct BulletMovementResult {
	bool shouldDestroy = false;
	bool hitTarget = false;
};

class IBulletMovementStrategy {
public:
	virtual ~IBulletMovementStrategy() = default;
};

class StraightBulletMovementStrategy : public IBulletMovementStrategy {
public:
	BulletMovementResult Apply(KamataEngine::Vector3& velocity, const KamataEngine::Vector3& position) const;
};

class HomingToEnemyMovementStrategy : public IBulletMovementStrategy {
public:
	BulletMovementResult Apply(PlayerBullet& bullet, KamataEngine::Vector3& velocity, const KamataEngine::Vector3& position) const;
};

class HomingToPlayerMovementStrategy : public IBulletMovementStrategy {
public:
	BulletMovementResult Apply(EnemyBullet& bullet, KamataEngine::Vector3& velocity, const KamataEngine::Vector3& position) const;
};

KamataEngine::Vector3 SlerpRotateDirection(const KamataEngine::Vector3& current, const KamataEngine::Vector3& target, float maxAngle);
