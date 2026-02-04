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
	bool IsAssistLocked() const { return isAssistLocked_; }
	void SetAssistLockId(int id) { assistLockId_ = id; }
	int GetAssistLockId() const { return assistLockId_; }

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
	// 画面外方向インジケーターを表示するか（遠すぎると非表示）
	bool showDirectionIndicator_ = true;

	KamataEngine::Vector3 initialRelativePos_;
	KamataEngine::Vector3 initialWorldPos_;
	float circleTimer_ = 0.0f;

	bool isFollowing_ = false;
	bool isFollowingFast_ = false;

	KamataEngine::Sprite* assistLockSprite_ = nullptr;
	uint32_t assistLockTextureHandle_ = 0;
	bool isAssistLocked_ = false; // アシスト円に入っているか
	int assistLockId_ = 0; // 現在のアシストロックID

	// 新: 遠距離では赤い回転ロックの代わりに緑のロックを表示するか
	bool useGreenLock_ = false;

	// 大航海のような広範囲移動用の変数（X軸とZ軸に散らばる）
	float baseX_ = 0.0f; // 基準X座標
	float baseZ_ = 0.0f; // 基準Z座標
	float currentOffsetX_ = 0.0f; // 現在のXオフセット
	float currentOffsetZ_ = 0.0f; // 現在のZオフセット
	float moveSpeedX_ = 100.0f; // X軸方向の移動速度
	float moveSpeedZ_ = 1.5f; // Z軸方向の移動速度
	float directionX_ = 1.0f; // X軸移動方向（1.0f = 右、-1.0f = 左）
	float directionZ_ = 1.0f; // Z軸移動方向（1.0f = 前、-1.0f = 後）
	float directionChangeTimerX_ = 0.0f; // X軸方向変更タイマー
	float directionChangeTimerZ_ = 0.0f; // Z軸方向変更タイマー
	float directionChangeIntervalX_ = 120.0f; // X軸方向を変更する間隔（フレーム、ランダムに変動）
	float directionChangeIntervalZ_ = 150.0f; // Z軸方向を変更する間隔（フレーム、ランダムに変動）
	const float kMaxOffsetX_ = 4500.0f; // X軸方向の最大オフセット（大航海範囲）
	const float kMaxOffsetZ_ = 4500.0f; // Z軸方向の最大オフセット（大航海範囲）

	// スムーズな向き制御用
	KamataEngine::Vector3 smoothedForward_ = {0.0f, 0.0f, 1.0f};
	float prevRenderedX_ = 0.0f;
	float prevRenderedZ_ = 0.0f;
	float facingSmoothFactor_ = 0.08f; // 回転補間係数（小さいほど滑らか）
	float posSmoothFactor_ = 0.18f;    // 位置補間係数

	// カーブ移動用（目標速度に滑らかに追従する）
	KamataEngine::Vector3 smoothedVelocity_ = {0.0f, 0.0f, 0.0f};
	float turnSmoothFactor_ = 0.06f; // 方向変化の滑らかさ（小さいほどゆっくり曲がる）

	// 大きな滑らかなカーブのためのワンダーパラメータ
	float wanderAngle_ = 0.0f; // current wander angle on circle
	float wanderJitter_ = 0.6f; // jitter per frame (radians)
	float wanderRadius_ = 800.0f; // radius of the wander circle
	float wanderDistance_ = 600.0f; // distance ahead of the agent
	float desiredSpeed_ = 2.0f; // typical forward speed for wander
};
