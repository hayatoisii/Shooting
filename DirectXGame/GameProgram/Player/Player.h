#pragma once
#include "AABB.h"
#include "EnemyBullet.h"
#include "KamataEngine.h"
#include "ParticleEmitter.h"
#include "PlayerBullet.h"
#include <list>

class Enemy;
class RailCamera;

class Player {
public:
	Player() = default;
	~Player();

	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& pos);
	void Update();
	void Draw();
	void Attack();
	void OnCollision();

	bool IsDead() const { return isDead_; }
	KamataEngine::WorldTransform& GetWorldTransform() { return worldtransfrom_; }

	void UpdateGameOver(float animationTime);

	KamataEngine::Vector3 GetWorldPosition();
	AABB GetAABB();
	const std::list<PlayerBullet*>& GetBullets() const { return bullets_; }

	void SetParent(const KamataEngine::WorldTransform* parent);
	void SetRailCamera(RailCamera* camera);
	void SetEnemies(std::list<Enemy*>* enemies) { enemies_ = enemies; }

	void ResetRotation();
	void ResetParticles();
	void ResetBullets();

	// 当たり判定用のサイズ
	static inline const float kWidth = 1.0f;
	static inline const float kHeight = 1.0f;

private:
	KamataEngine::WorldTransform worldtransfrom_;
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::Input* input_ = nullptr;
	RailCamera* railCamera_ = nullptr;

	KamataEngine::Model* modelbullet_ = nullptr;
	std::list<PlayerBullet*> bullets_;

	std::list<Enemy*>* enemies_ = nullptr;

	int specialTimer = 0;
	bool isParry_ = false;

	// パーティクル
	KamataEngine::Model* modelParticle_ = nullptr;
	ParticleEmitter* engineExhaust_ = nullptr;

	int hp_ = 3;
	bool isDead_ = false;
	int shotTimer_;
};