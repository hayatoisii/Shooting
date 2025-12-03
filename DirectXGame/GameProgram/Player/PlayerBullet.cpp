#include "PlayerBullet.h"
#include "Enemy.h"
#include "base/TextureManager.h"
#include <cassert>
#include <math.h>

PlayerBullet::~PlayerBullet() { model_ = nullptr; }

void PlayerBullet::Initialize(KamataEngine::Model* model, const KamataEngine::Vector3& position, const KamataEngine::Vector3& velocity) {
	assert(model);
	model_ = model;
	worldtransfrom_.translation_ = position;
	worldtransfrom_.Initialize();
	velocity_ = velocity;

	// 5000まで弾が飛ぶようにする
	const float kDesiredRange = 5000.0f;
	float speed = sqrtf(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);
	if (speed > 0.001f) {
		int32_t frames = static_cast<int32_t>(ceilf(kDesiredRange / speed));
		// Add a small margin
		frames += 2;
		deathTimer_ = frames;
	} else {
		// fallback to default lifetime
		// ... keep existing deathTimer_
	}
}

void PlayerBullet::OnCollision() { isDead_ = true; }

void PlayerBullet::Update() {

    if (--deathTimer_ <= 0) {
        isDead_ = true;
    }

    /*
    // 追尾処理
    if (isHomingEnabled_ && homingTarget_ && !homingTarget_->IsDead()) {
        KamataEngine::Vector3 targetPos = homingTarget_->GetWorldPosition();
        KamataEngine::Vector3 bulletPos = GetWorldPosition();
        
        KamataEngine::Vector3 toTarget = targetPos - bulletPos;
        float distance = sqrtf(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);
        
        const float stopHomingDistance = 1.0f; // この距離より近づいたらホーミング停止

        const float maxHomingDistance = 500.0f; 
        if (distance > maxHomingDistance) {
            isHomingEnabled_ = false; 
            homingTarget_ = nullptr;
 
        } else if (distance < stopHomingDistance) { // 停止距離より近ければ
            isHomingEnabled_ = false;             // ホーミングを無効化
            homingTarget_ = nullptr;
        } else if (distance > 0.001f) {
            toTarget.x /= distance;
            toTarget.y /= distance;
            toTarget.z /= distance;

            if (homingCheckDelayTimer_ > 0) {
                homingCheckDelayTimer_--;
            }

            bool passedTarget = false;
            if (homingCheckDelayTimer_ <= 0) { 
                float dotVT = velocity_.x * toTarget.x + velocity_.y * toTarget.y + velocity_.z * toTarget.z;
                if (dotVT < 0.0f) {
                    passedTarget = true; // 通り過ぎた
                }
            }

            // 通り過ぎていなければホーミング処理を行う
            if (!passedTarget) { 
                float currentSpeed = sqrtf(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);
                
                velocity_.x += toTarget.x * currentSpeed * homingStrength_;
                velocity_.y += toTarget.y * currentSpeed * homingStrength_;
                velocity_.z += toTarget.z * currentSpeed * homingStrength_;
                
                float newSpeed = sqrtf(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);
                if (newSpeed > 0.001f) {
                    velocity_.x = (velocity_.x / newSpeed) * currentSpeed;
                    velocity_.y = (velocity_.y / newSpeed) * currentSpeed;
                    velocity_.z = (velocity_.z / newSpeed) * currentSpeed;
                }
            } else {
                // 通り過ぎたらホーミングを無効化
                isHomingEnabled_ = false;
                homingTarget_ = nullptr;
            }
        }
    }
    */

    worldtransfrom_.translation_.x += velocity_.x;
    worldtransfrom_.translation_.y += velocity_.y;
    worldtransfrom_.translation_.z += velocity_.z;

    worldtransfrom_.UpdateMatrix();
}

KamataEngine::Vector3 PlayerBullet::GetWorldPosition() {
	KamataEngine::Vector3 worldPos;
	worldPos.x = worldtransfrom_.matWorld_.m[3][0];
	worldPos.y = worldtransfrom_.matWorld_.m[3][1];
	worldPos.z = worldtransfrom_.matWorld_.m[3][2];
	return worldPos;
}

void PlayerBullet::Draw(const KamataEngine::Camera& camera) {
	if (!isDead_) {
		model_->Draw(worldtransfrom_, camera);
	}
}