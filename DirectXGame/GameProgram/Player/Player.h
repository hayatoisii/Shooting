#pragma once
#include "AABB.h"
#include "EnemyBullet.h"
#include "KamataEngine.h"
#include "ParticleEmitter.h"
#include "PlayerBullet.h"
#include <list>

class Enemy;      // 前方宣言
class RailCamera; // 前方宣言

class Player {
public:
	Player() = default;
	~Player();

	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& pos);
	void Update();
	void Draw();
	void Attack();
	void OnCollision(); // 衝突時に呼ばれる関数

	// ▼▼▼ 追加 ▼▼▼
	// 死亡フラグを返す関数
	bool IsDead() const { return isDead_; }
	// WorldTransform の参照を返す関数
	KamataEngine::WorldTransform& GetWorldTransform() { return worldtransfrom_; }
	// ▲▲▲ 追加完了 ▲▲▲

	// ゲームオーバー演出用の更新関数
	void UpdateGameOver(float animationTime);

	KamataEngine::Vector3 GetWorldPosition();
	AABB GetAABB();
	const std::list<PlayerBullet*>& GetBullets() const { return bullets_; }

	void SetParent(const KamataEngine::WorldTransform* parent);
	void SetRailCamera(RailCamera* camera);
	void SetEnemies(std::list<Enemy*>* enemies) { enemies_ = enemies; }
	//void SetEnemy(Enemy* enemy) { /* (未使用？) */ }

	void ResetRotation();
	void ResetParticles();

	// 当たり判定用のサイズ (AABB で使用)
	static inline const float kWidth = 1.0f;
	static inline const float kHeight = 1.0f;

private:
	KamataEngine::WorldTransform worldtransfrom_;
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::Input* input_ = nullptr;
	RailCamera* railCamera_ = nullptr;

	// 弾
	KamataEngine::Model* modelbullet_ = nullptr;
	std::list<PlayerBullet*> bullets_;
	// 敵リスト
	std::list<Enemy*>* enemies_ = nullptr;

	int specialTimer = 180;
	bool isParry_ = false;

	// パーティクル
	KamataEngine::Model* modelParticle_ = nullptr;
	ParticleEmitter* engineExhaust_ = nullptr;

	// HP と死亡フラグ
	int hp_ = 3;          // ★ HP (初期値 3)
	bool isDead_ = false; // 死亡フラグ
};