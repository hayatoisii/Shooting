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
	// Fire one shot immediately towards the player (used for centralized firing control)
	void ShootOnce();

	bool IsDead() const { return isDead_; }

	void OnCollision();

	KamataEngine::Vector3 GetWorldPosition();

	void SetPlayer(Player* player) { player_ = player; }
	void SetGameScene(GameScene* gameScene) { gameScene_ = gameScene; }
	void SetCamera(const KamataEngine::Camera* camera) { camera_ = camera; }
	// 画面内判定
	bool IsOnScreen() const { return isOnScreen_; }

	int GetHp() const { return hp_; }

	// 画面座標の更新
	void UpdateScreenPosition();

	// 発射間隔
	static const int kFireInterval = 120; // ~2 seconds at 60 FPS

	bool isDead_ = false;

	void SetParent(const KamataEngine::WorldTransform* parent);
	void SetAssistLocked(bool isLocked) { isAssistLocked_ = isLocked; }

private:

	KamataEngine::WorldTransform worldtransfrom_;
	KamataEngine::Model* model_ = nullptr;

	KamataEngine::Model* modelbullet_ = nullptr;

	int hp_ = 1;

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

	bool wasOnScreenLastFrame_ = false; // 1フレーム前の画面内判定
	float lockOnAnimRotation_ = 0.0f;
	float lockOnAnimScale_ = 1.0f;

	KamataEngine::Sprite* directionIndicatorSprite_;
	bool isOffScreen_;
	// Whether to show the off-screen direction indicator (hidden if too far)
	bool showDirectionIndicator_ = true;

	KamataEngine::Vector3 initialRelativePos_;
	KamataEngine::Vector3 initialWorldPos_;
	float circleTimer_ = 0.0f;

	bool isFollowing_ = false;
	bool isFollowingFast_ = false;

	KamataEngine::Sprite* assistLockSprite_ = nullptr;
	uint32_t assistLockTextureHandle_ = 0;
	bool isAssistLocked_ = false; // アシスト円に入っているか

	// New: whether to show green lock (far) instead of red rotating lock
	bool useGreenLock_ = false;

};
