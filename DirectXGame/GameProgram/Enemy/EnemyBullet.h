#pragma once
#include "AABB.h"
#include "GameBullet.h"
#include <3d/Camera.h>
#include <3d/Model.h>
#include <3d/WorldTransform.h>

class Player;
class GameCharacter;

// 敵弾（GameBullet を継承）
class EnemyBullet : public GameBullet {
public:
	void Initialize(KamataEngine::Model* model, const KamataEngine::Vector3& position, const KamataEngine::Vector3& velocity);

	void Update();

	void Draw(const KamataEngine::Camera& camera);

	void OnEvaded();

	~EnemyBullet() override;

	// --- GameBullet の仮想関数（override） ---
	KamataEngine::Vector3 GetWorldPosition() const override;
	bool IsDead() const override;
	void OnCollision() override;
	float GetCollisionRadius() const override;
	const char* GetKindName() const override;

	AABB GetAABB();

	// ホーミング（ターゲットは GameCharacter* で保持し、Player を指す）
	void SetHomingTarget(Player* target);
	GameCharacter* GetHomingTarget() const { return homingTarget_; }
	void SetHomingEnabled(bool enabled) { isHoming_ = enabled; }
	void SetSpeed(float s) { speed_ = s; }
	float GetSpeed() const { return speed_; }
	bool IsHoming() const { return isHoming_; }

	// 回避後のタイマーを取得（-1は未回避、0以上は残りフレーム数）
	int32_t GetEvadedDeathTimer() const { return evadedDeathTimer_; }

	void StopHoming() {
		isHoming_ = false;
		deathTimer_ = 60;
	}

	void SetInvulnerableFrames(int frames) { invulnerableFrames_ = frames; }

protected:
	bool UpdateLifetime() override;
	void ApplyMovement() override;
	void UpdateTransform() override;

private:
	KamataEngine::WorldTransform worldtransfrom_;
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Vector3 velocity_;

	static const int32_t kLifeTime = 60 * 10;
	int32_t deathTimer_ = kLifeTime;

	static inline const float kWidth = 1.0f;
	static inline const float kHeight = 1.0f;

	// ホーミング先（Player は GameCharacter の派生クラス）
	GameCharacter* homingTarget_ = nullptr;
	bool isHoming_ = false;
	float speed_ = 1.0f;

	int invulnerableFrames_ = 0;

	// 回避後のタイマー（1秒 = 60フレーム）
	int32_t evadedDeathTimer_ = -1;
};
