#pragma once
#include <3d/Model.h>
#include <3d/WorldTransform.h>
#include "KamataEngine.h"
#include <3d/Camera.h>
#include "EnemyBullet.h"
#include <cassert>
#include "MT.h"
#include "GaneScene.h"

// 前方宣言
namespace KamataEngine { class Sprite; }
class Vector2;

class Player;
class GameScene;

enum class Phase {
	Approach, // 接近する
	Leave,    // 離脱する
};

class Enemy {
public:

	void Initialize(KamataEngine::Model* model, const KamataEngine::Vector3& pos);
	void Update();
	void Draw(const KamataEngine::Camera& camera);
	void DrawSprite(); // スプライトを描画
	~Enemy();
	void Fire();

	bool IsDead() const { return isDead_; }

	void OnCollision();

	KamataEngine::Vector3 GetWorldPosition();

	void SetPlayer(Player* player) { player_ = player; }
	void SetGameScene(GameScene* gameScene) { gameScene_ = gameScene; }
	void SetCamera(const KamataEngine::Camera* camera) { camera_ = camera; }
	// 画面内判定
	bool IsOnScreen() const { return isOnScreen_; }

	// 画面座標の更新
	void UpdateScreenPosition();

	// 発射間隔
	static const int kFireInterval = 180;

	bool isDead_ = false;


private:

	KamataEngine::WorldTransform worldtransfrom_;
	KamataEngine::Model* model_ = nullptr;

	KamataEngine::Model* modelbullet_ = nullptr;

	// 発射タイマー
	int32_t spawnTimer = 0;

	Player* player_ = nullptr;
	GameScene* gameScene_ = nullptr;
	const KamataEngine::Camera* camera_ = nullptr;

	Phase phase_ = Phase::Approach;

	Phase Bulletphase_ = Phase::Approach;

	// 追尾スプライト
	class Sprite* targetSprite_ = nullptr;
	bool isOnScreen_ = false;
	KamataEngine::Vector2 screenPosition_;
};
