#pragma once
#include "PlayerBullet.h"
#include <list>
#include <3d/Model.h>
#include <3d/Camera.h>
#include "AABB.h"
#include "MT.h"
#include "ParticleEmitter.h"
#include <KamataEngine.h>

namespace KamataEngine { class Input; };

class Enemy;
class RailCamera; 

class Player {
public:
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& pos);
	void Update();
	void Draw();
	~Player();
	void Attack();
	// 衝突を検出したら呼び出されるコールバック関数
	void OnCollision();

	KamataEngine::Vector3 GetWorldPosition(); 

	const std::list<PlayerBullet*>& GetBullets() const { return bullets_; }

	// AABBを取得
	AABB GetAABB();

	static inline const float kWidth = 1.0f;
	static inline const float kHeight = 1.0f;

	void SetEnemy(Enemy* enemy) { enemy_ = enemy; }
	void SetParent(const WorldTransform* parent);
	void SetRailCamera(RailCamera* camera);

	void ResetRotation();

private:
	
	KamataEngine::WorldTransform worldtransfrom_;

	KamataEngine::Model* model_ = nullptr;

	KamataEngine::Input* input_ = nullptr;
	
	KamataEngine::Camera* camera_ = nullptr;

	RailCamera* railCamera_ = nullptr;

	KamataEngine::Model* modelbullet_ = nullptr;

	// 弾
	std::list<PlayerBullet*> bullets_;

	bool isDead_ = false;
	bool isParry_ = false;

	Enemy* enemy_ = nullptr;

	// 発射タイマー
	int32_t spawnTimer = 0;

	// 最初の発射までのtimer
	float specialTimer = 10.0f;

	KamataEngine::Model* modelParticle_ = nullptr; // パーティクル用のモデル
	ParticleEmitter* engineExhaust_ = nullptr;     // 排気ガス用のパーティクルエミッタ
};