#pragma once
#include <3d/Camera.h>
#include <3d/Model.h>
#include <3d/WorldTransform.h>
#include "AABB.h"
class Player; // forward
class EnemyBullet {
public:
    void Initialize(KamataEngine::Model* model, const KamataEngine::Vector3& position, const KamataEngine::Vector3& velocity);

    void Update();

    void Draw(const KamataEngine::Camera& camera);

    void OnEvaded();

    ~EnemyBullet();

    bool IsDead() const { return isDead_; }

    void OnCollision();

    KamataEngine::Vector3 GetWorldPosition();

    AABB GetAABB();

    // Homing support
    void SetHomingTarget(Player* target) { homingTarget_ = target; }
    void SetHomingEnabled(bool enabled) { isHoming_ = enabled; }
    void SetSpeed(float s) { speed_ = s; }
    bool IsHoming() const { return isHoming_; }
    
    // 回避後のタイマーを取得（-1は未回避、0以上は残りフレーム数）
    int32_t GetEvadedDeathTimer() const { return evadedDeathTimer_; }

    void StopHoming() {
		isHoming_ = false;
		deathTimer_ = 60;
	}

    void SetInvulnerableFrames(int frames) { invulnerableFrames_ = frames; }

private:

    KamataEngine::WorldTransform worldtransfrom_;
    KamataEngine::Model* model_ = nullptr;
    KamataEngine::Vector3 velocity_;

    // 寿命　Enemyミサイル
    static const int32_t kLifeTime = 60 * 10; // default 10 seconds
    // デスタイマー
    int32_t deathTimer_ = kLifeTime;
    // デスフラグ
    bool isDead_ = false;

    static inline const float kWidth = 1.0f;
    static inline const float kHeight = 1.0f;

    // Homing members
    Player* homingTarget_ = nullptr;
    bool isHoming_ = false;
    float speed_ = 1.0f; // units per frame

    int invulnerableFrames_ = 0;

    // 回避後のタイマー（1秒 = 60フレーム）
    int32_t evadedDeathTimer_ = -1; // -1は未回避状態
};