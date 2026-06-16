#include "EntityFactory.h"

#include "Enemy.h"
#include "EnemyBullet.h"
#include "GaneScene.h"
#include "Player.h"
#include "PlayerBullet.h"

EntityFactory::~EntityFactory() {
	playerBulletPool_.Clear();
	enemyBulletPool_.Clear();
}

PlayerBullet* EntityFactory::CreatePlayerBullet(KamataEngine::Model* model, const KamataEngine::Vector3& position, const KamataEngine::Vector3& velocity) {
	PlayerBullet* bullet = playerBulletPool_.Acquire();
	bullet->Initialize(model, position, velocity);
	return bullet;
}

void EntityFactory::ReleasePlayerBullet(PlayerBullet* bullet) {
	if (bullet == nullptr) {
		return;
	}
	playerBulletPool_.Release(bullet);
}

EnemyBullet* EntityFactory::CreateEnemyHomingBullet(KamataEngine::Model* model, const KamataEngine::Vector3& position, const KamataEngine::Vector3& velocity, Player* player, float speed) {
	EnemyBullet* bullet = enemyBulletPool_.Acquire();
	bullet->Initialize(model, position, velocity);
	bullet->SetHomingEnabled(true);
	bullet->SetHomingTarget(player);
	bullet->SetSpeed(speed);
	return bullet;
}

void EntityFactory::ReleaseEnemyBullet(EnemyBullet* bullet) {
	if (bullet == nullptr) {
		return;
	}
	enemyBulletPool_.Release(bullet);
}

Enemy* EntityFactory::CreateEnemy(KamataEngine::Model* model, const KamataEngine::Vector3& position, Player* player, GameScene* gameScene, const KamataEngine::Camera* camera) {
	Enemy* enemy = new Enemy();
	enemy->SetPlayer(player);
	enemy->SetGameScene(gameScene);
	enemy->SetCamera(camera);
	enemy->Initialize(model, position);
	return enemy;}
