#pragma once
#include <3d/Model.h>
#include <3d/WorldTransform.h>
#include "GameCharacter.h"
#include "KamataEngine.h"
#include <3d/Camera.h>
#include "EnemyBullet.h"
#include <cassert>
#include "MT.h"
#include "GaneScene.h"
#include "GameEvent.h"
#include "EnemyMovementStrategy.h"
#include <array>

// 前方宣言
namespace KamataEngine { class Sprite; }
class Vector2;

class Player;
class GameScene;

enum class Phase {
	Approach, // 接近する
	Leave,    // 離脱する
};

// 敵キャラクター（GameCharacter を継承。ポリモーフィズムの派生クラス）
class Enemy : public GameCharacter {
public:

	void Initialize(KamataEngine::Model* model, const KamataEngine::Vector3& pos);
	void Update();
	void Draw(const KamataEngine::Camera& camera);
	void DrawSprite(); // スプライトを描画
	~Enemy() override;
	void Fire();

	// --- GameCharacter の仮想関数（override） ---
	KamataEngine::Vector3 GetWorldPosition() const override;
	bool IsDead() const override;
	void OnCollision() override;
	int GetHp() const override;
	int GetMaxHp() const override;
	float GetCollisionRadius() const override;
	const char* GetKindName() const override;

	void SetPlayer(Player* player) { player_ = player; }
	void SetGameScene(GameScene* gameScene) { gameScene_ = gameScene; }
	void SetEventSubject(GameEventSubject* subject) { eventSubject_ = subject; }
	void SetCamera(const KamataEngine::Camera* camera) { camera_ = camera; }
	// 画面内判定
	bool IsOnScreen() const { return isOnScreen_; }

	// 画面座標の更新
	void UpdateScreenPosition();

	// 発射間隔
	static const int kFireInterval = 20;

	void SetParent(const KamataEngine::WorldTransform* parent);
	void SetAssistLocked(bool isLocked) { isAssistLocked_ = isLocked; }
	bool IsAssistLocked() const { return isAssistLocked_; }
	void SetAssistLockId(int id) { assistLockId_ = id; }
	int GetAssistLockId() const { return assistLockId_; }

private:

	KamataEngine::WorldTransform worldtransfrom_;
	KamataEngine::Model* model_ = nullptr;

	KamataEngine::Model* modelbullet_ = nullptr;

	static const int kMaxHp_ = 5;
	int hp_ = kMaxHp_;
	bool isDead_ = false;

	// 発射タイマー
	int32_t spawnTimer = 0;

	Player* player_ = nullptr;
	GameScene* gameScene_ = nullptr;
	GameEventSubject* eventSubject_ = nullptr;
	const KamataEngine::Camera* camera_ = nullptr;

	// Strategy Pattern: 敵の移動行動
	EnemyMovementState movementState_;
	WanderEnemyMovementStrategy movementStrategy_;

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

	// データドリブン: X/Z 軸で同じ処理・異なるパラメータをテーブル化
	struct DirectionAxisState {
		float* timer = nullptr;
		float* interval = nullptr;
		float* direction = nullptr;
		const char* intervalMinKey = nullptr;
		const char* intervalMaxKey = nullptr;
	};

	std::array<DirectionAxisState, 2> directionAxes_;
	float directionChangeTimerX_ = 0.0f;
	float directionChangeTimerZ_ = 0.0f;
	float directionChangeIntervalX_ = 120.0f;
	float directionChangeIntervalZ_ = 150.0f;
	const float kMaxOffsetX_ = 4500.0f; // X軸方向の最大オフセット（大航海範囲）
	const float kMaxOffsetZ_ = 4500.0f; // Z軸方向の最大オフセット（大航海範囲）

	// スムーズな向き制御用
	KamataEngine::Vector3 smoothedForward_ = {0.0f, 0.0f, 1.0f};
	float prevRenderedX_ = 0.0f;
	float prevRenderedZ_ = 0.0f;
};
