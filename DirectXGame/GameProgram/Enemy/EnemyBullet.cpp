#include "EnemyBullet.h"
#include <cassert>
#include "Player.h"
#include <cmath>

EnemyBullet::~EnemyBullet() { model_ = nullptr; }

void EnemyBullet::Initialize(KamataEngine::Model* model, const KamataEngine::Vector3& position, const KamataEngine::Vector3& velocity) {
    assert(model);
    model_ = model;
    worldtransfrom_.translation_ = position;
    worldtransfrom_.Initialize();
    velocity_ = velocity;

    // If explicit speed_ not set, derive from velocity magnitude
    float sp = std::sqrt(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);
    if (sp > 0.001f) {
        speed_ = sp;
    }
}

// ワールド座標を取得
KamataEngine::Vector3 EnemyBullet::GetWorldPosition() {

    // ワールド座標を入れる変数
    KamataEngine::Vector3 worldPos;
    // ワールド行列の平行移動成分を取得（ワールド座標）
    worldPos.x = worldtransfrom_.matWorld_.m[3][0];
    worldPos.y = worldtransfrom_.matWorld_.m[3][1];
    worldPos.z = worldtransfrom_.matWorld_.m[3][2];

    return worldPos;
}

AABB EnemyBullet::GetAABB() {
    KamataEngine::Vector3 worldPos = GetWorldPosition();

    AABB aabb;

    aabb.min = {worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f};
    aabb.max = {worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f};

    return aabb;
}

void EnemyBullet::Update() {

    if (--deathTimer_ <= 0) {
        isDead_ = true;
    }

    // Homing behavior: adjust velocity towards homingTarget_ while maintaining speed_
    if (isHoming_ && homingTarget_ && !homingTarget_->IsDead()) {
        KamataEngine::Vector3 targetPos = homingTarget_->GetWorldPosition();
        KamataEngine::Vector3 bulletPos = GetWorldPosition();
        KamataEngine::Vector3 toTarget = {targetPos.x - bulletPos.x, targetPos.y - bulletPos.y, targetPos.z - bulletPos.z};
        float dist = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);
        if (dist > 0.001f) {
            toTarget.x /= dist;
            toTarget.y /= dist;
            toTarget.z /= dist;
            velocity_.x = toTarget.x * speed_;
            velocity_.y = toTarget.y * speed_;
            velocity_.z = toTarget.z * speed_;
        }
    }

    worldtransfrom_.translation_.x += velocity_.x;
    worldtransfrom_.translation_.y += velocity_.y;
    worldtransfrom_.translation_.z += velocity_.z;

    worldtransfrom_.UpdateMatrix();

}

void EnemyBullet::OnCollision() { isDead_ = true; }


void EnemyBullet::Draw(const KamataEngine::Camera& camera) {

    if (!isDead_) {
        // モデルの描画
        model_->Draw(worldtransfrom_, camera);
    }
}
