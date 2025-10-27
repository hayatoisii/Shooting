#include "PlayerBullet.h"
#include <cassert>
#include "base/TextureManager.h"
#include "Enemy.h"
#include <math.h>

PlayerBullet::~PlayerBullet() { 
	model_ = nullptr; }

void PlayerBullet::Initialize(KamataEngine::Model* model, const KamataEngine::Vector3& position, const KamataEngine::Vector3& velocity) {

	assert(model);
	model_ = model;
	worldtransfrom_.translation_ = position;
	worldtransfrom_.Initialize();
	velocity_ = velocity;

}

void PlayerBullet::OnCollision() { isDead_ = true; }


void PlayerBullet::Update() {

	if (--deathTimer_ <= 0) {
		isDead_ = true;
	}

	// 追尾処理（軽度な追尾：50%程度の命中率）
	if (isHomingEnabled_ && homingTarget_ && !homingTarget_->IsDead()) {
		KamataEngine::Vector3 targetPos = homingTarget_->GetWorldPosition();
		KamataEngine::Vector3 bulletPos = GetWorldPosition();
		
		// ターゲットへの方向ベクトル
		KamataEngine::Vector3 toTarget = targetPos - bulletPos;
		float distance = sqrtf(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);
		
		// 一定距離以上離れたら追尾を無効化
		const float maxHomingDistance = 500.0f;
		if (distance > maxHomingDistance) {
			isHomingEnabled_ = false; // 外れたら追尾をやめる
			homingTarget_ = nullptr;
		} else if (distance > 0.001f) { // ゼロ除算を避ける
			// 正規化
			toTarget.x /= distance;
			toTarget.y /= distance;
			toTarget.z /= distance;
			
			// すでにターゲットを通過していたら追尾終了（速度とtoTargetの内積が負）
			float dotVT = velocity_.x * toTarget.x + velocity_.y * toTarget.y + velocity_.z * toTarget.z;
			if (dotVT < 0.0f) {
				isHomingEnabled_ = false;
				homingTarget_ = nullptr;
			} else {
				// 現在の速度を取得
				float currentSpeed = sqrtf(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);
				
				// 軽度な追尾：ターゲット方向に少しだけ寄せる
				velocity_.x += toTarget.x * currentSpeed * homingStrength_;
				velocity_.y += toTarget.y * currentSpeed * homingStrength_;
				velocity_.z += toTarget.z * currentSpeed * homingStrength_;
				
				// 正規化して元の速度に戻す
				float newSpeed = sqrtf(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);
				if (newSpeed > 0.001f) {
					velocity_.x = (velocity_.x / newSpeed) * currentSpeed;
					velocity_.y = (velocity_.y / newSpeed) * currentSpeed;
					velocity_.z = (velocity_.z / newSpeed) * currentSpeed;
				}
			}
		}
	}

	worldtransfrom_.translation_.x += velocity_.x;
	worldtransfrom_.translation_.y += velocity_.y;
	worldtransfrom_.translation_.z += velocity_.z;

	worldtransfrom_.UpdateMatrix();

}

// ワールド座標を取得
KamataEngine::Vector3 PlayerBullet::GetWorldPosition() {

	// ワールド座標を入れる変数
	KamataEngine::Vector3 worldPos;
	// ワールド行列の平行移動成分を取得（ワールド座標）
	worldPos.x = worldtransfrom_.matWorld_.m[3][0];
	worldPos.y = worldtransfrom_.matWorld_.m[3][1];
	worldPos.z = worldtransfrom_.matWorld_.m[3][2];

	return worldPos;
}

void PlayerBullet::Draw(const KamataEngine::Camera& camera) {

	// モデルの描画
	model_->Draw(worldtransfrom_, camera);

}
