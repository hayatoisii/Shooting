#pragma once
#include "GameBullet.h"
#include <3d/Camera.h>
#include <3d/Model.h>
#include <3d/WorldTransform.h>
#include <vector>

// 前方宣言
namespace KamataEngine {
struct Vector3;
}

class Enemy;

// 自機弾（GameBullet を継承）
class PlayerBullet : public GameBullet {
public:
	void Initialize(KamataEngine::Model* model, const KamataEngine::Vector3& position, const KamataEngine::Vector3& velocity);

	void Update();

	void Draw(const KamataEngine::Camera& camera);

	~PlayerBullet() override;

	// --- GameBullet の仮想関数（override） ---
	KamataEngine::Vector3 GetWorldPosition() const override;
	bool IsDead() const override;
	void OnCollision() override;
	float GetCollisionRadius() const override;
	const char* GetKindName() const override;

	// 追尾設定
	void SetHomingTarget(Enemy* target) { homingTarget_ = target; }
	Enemy* GetHomingTarget() const { return homingTarget_; }
	void SetHomingEnabled(bool enabled) { isHomingEnabled_ = enabled; }
	void SetHomingStrength(float strength) { homingStrength_ = strength; }
	bool IsHomingEnabled() const { return isHomingEnabled_; }
	float GetHomingStrength() const { return homingStrength_; }

	// UpdateAimAssistで設定されたホーミングかどうかを区別するフラグ
	void SetAimAssistHoming(bool isAimAssist) { isAimAssistHoming_ = isAimAssist; }
	bool IsAimAssistHoming() const { return isAimAssistHoming_; }

	// 敵のアシストロックIDを弾が保持する
	void SetAssistLockId(int id) { assistLockId_ = id; }
	int GetAssistLockId() const { return assistLockId_; }

	// ロックオン済みの敵に対して、"ロックオン距離" に入ったらホーミングを開始するための保留設定
	void SetPendingHomingTarget(Enemy* target, float lockDistance) { pendingHomingTarget_ = target; pendingLockDistance_ = lockDistance; }

protected:
	bool UpdateLifetime() override;
	void UpdatePreMovement() override;
	void ApplyMovement() override;
	void UpdateTransform() override;

private:
	KamataEngine::WorldTransform worldtransfrom_;

	KamataEngine::Model* model_ = nullptr;

	KamataEngine::Vector3 velocity_;

	// 追尾関連
	Enemy* homingTarget_ = nullptr;
	bool isHomingEnabled_ = false;
	float homingStrength_ = 0.1f; // 追尾の強さ
	bool isAimAssistHoming_ = false; // UpdateAimAssistで設定されたホーミングかどうか
	int assistLockId_ = 0; // 0 = none

	// Pending homing (start when within distance)
	Enemy* pendingHomingTarget_ = nullptr;
	float pendingLockDistance_ = 0.0f;

	// 寿命<frm>
	static const int32_t kLifeTime = 60 * 2;
	int32_t deathTimer_ = kLifeTime;

	// ホーミング開始後のオーバーシュートチェック遅延タイマー
	int homingCheckDelayTimer_ = 10;

	int32_t homingDelayTimer_ = 0;
};
