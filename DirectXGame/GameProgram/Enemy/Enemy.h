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

	int GetHp() const { return hp_; }

	// 画面座標の更新
	void UpdateScreenPosition();

	// 発射間隔
	static const int kFireInterval = 20;

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

	// 大航海のような広範囲移動用の変数（X軸とZ軸に散らばる）
	float baseX_ = 0.0f; // 基準X座標
	float baseZ_ = 0.0f; // 基準Z座標
	float currentOffsetX_ = 0.0f; // 現在のXオフセット
	float currentOffsetZ_ = 0.0f; // 現在のZオフセット
	float moveSpeedX_ = 2.0f; // X軸方向の移動速度
	float moveSpeedZ_ = 1.5f; // Z軸方向の移動速度
	float directionX_ = 1.0f; // X軸移動方向（1.0f = 右、-1.0f = 左）
	float directionZ_ = 1.0f; // Z軸移動方向（1.0f = 前、-1.0f = 後）
	float directionChangeTimerX_ = 0.0f; // X軸方向変更タイマー
	float directionChangeTimerZ_ = 0.0f; // Z軸方向変更タイマー
	float directionChangeIntervalX_ = 120.0f; // X軸方向を変更する間隔（フレーム、ランダムに変動）
	float directionChangeIntervalZ_ = 150.0f; // Z軸方向を変更する間隔（フレーム、ランダムに変動）
	const float kMaxOffsetX_ = 15000.0f; // X軸方向の最大オフセット（大航海範囲）
	const float kMaxOffsetZ_ = 10000.0f; // Z軸方向の最大オフセット（大航海範囲）
};
