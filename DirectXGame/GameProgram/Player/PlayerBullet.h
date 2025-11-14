#pragma once
#include <3d/Camera.h>
#include <3d/Model.h>
#include <3d/WorldTransform.h>
#include <vector>

// 前方宣言
namespace KamataEngine {
struct Vector3;
}

class Enemy;

class PlayerBullet {
public:
	void Initialize(KamataEngine::Model* model, const KamataEngine::Vector3& position, const KamataEngine::Vector3& velocity);

	void Update();

	KamataEngine::Vector3 GetWorldPosition();

	void Draw(const KamataEngine::Camera& camera);

	~PlayerBullet();

	bool IsDead() const { return isDead_; }

	// 衝突を検出したら呼び出されるコールバック関数
	void OnCollision();

	// 追尾設定
	void SetHomingTarget(Enemy* target) { homingTarget_ = target; }
	void SetHomingEnabled(bool enabled) { isHomingEnabled_ = enabled; }
	void SetHomingStrength(float strength) { homingStrength_ = strength; }

private:
	KamataEngine::WorldTransform worldtransfrom_;

	KamataEngine::Model* model_ = nullptr;

	// uint32_t textureHandle_ = 0;

	KamataEngine::Vector3 velocity_;

	// 追尾関連
	Enemy* homingTarget_ = nullptr;
	bool isHomingEnabled_ = false;
	float homingStrength_ = 0.1f; // 追尾の強さ

	// 寿命<frm>
	static const int32_t kLifeTime = 60 * 3;
	// デスタイマー
	int32_t deathTimer_ = kLifeTime;
	// デスフラグ
	bool isDead_ = false;

	// ▼▼▼ 追加 ▼▼▼
	// ホーミング開始後のオーバーシュートチェック遅延タイマー
	int homingCheckDelayTimer_ = 10; // 例: 10フレーム遅延
	
	int32_t homingDelayTimer_ = 0;
};