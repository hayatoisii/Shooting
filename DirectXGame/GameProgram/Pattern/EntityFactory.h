#pragma once

#include "ObjectPool.h"

#include <KamataEngine.h>

class Enemy;
class EnemyBullet;
class GameScene;
class Player;
class PlayerBullet;

// Factory Method Pattern: キャラクター・弾の生成を一元化し Object Pool と連携する
class EntityFactory {
public:
	EntityFactory() = default;
	~EntityFactory();

	PlayerBullet* CreatePlayerBullet(KamataEngine::Model* model, const KamataEngine::Vector3& position, const KamataEngine::Vector3& velocity);
	void ReleasePlayerBullet(PlayerBullet* bullet);

	EnemyBullet* CreateEnemyHomingBullet(KamataEngine::Model* model, const KamataEngine::Vector3& position, const KamataEngine::Vector3& velocity, Player* player, float speed);
	void ReleaseEnemyBullet(EnemyBullet* bullet);

	Enemy* CreateEnemy(KamataEngine::Model* model, const KamataEngine::Vector3& position, Player* player, GameScene* gameScene, const KamataEngine::Camera* camera);

private:
	ObjectPool<PlayerBullet> playerBulletPool_;
	ObjectPool<EnemyBullet> enemyBulletPool_;
};
