#include "Meteorite.h"
#include <cassert>

void Meteorite::Initialize(KamataEngine::Model* model, const KamataEngine::Vector3& pos, float baseScale, float radius) {
	assert(model);
	model_ = model;
	radius_ = radius;
	baseScale_ = baseScale;

	worldtransfrom_.Initialize();
	worldtransfrom_.translation_ = pos;
	worldtransfrom_.scale_ = {baseScale_, baseScale_, baseScale_};
	worldtransfrom_.UpdateMatrix();
	isDead_ = false;
}

void Meteorite::Update(const KamataEngine::Vector3& playerPos) {

	KamataEngine::Vector3 pos = worldtransfrom_.translation_;
	float dx = pos.x - playerPos.x;
	float dy = pos.y - playerPos.y;
	float dz = pos.z - playerPos.z;
	float distance = std::sqrt(dx * dx + dy * dy + dz * dz);

	// この距離で隕石が完全に小さくなって見えなくなる、この距離から近づいた時に徐々にデカくなる
	const float kMaxDistance = 800.0f;
	// Playerに近づいた時にデカくなる限界
	const float kMaxScaleFactor = 1.06f;
	const float kMinScaleFactor = 0.0f;

	float scaleFactor = 0.0f;

	if (distance < kMaxDistance) {
		float t = distance / kMaxDistance;
		float easedT = 1.0f - t;

		scaleFactor = kMinScaleFactor + (easedT * (kMaxScaleFactor - kMinScaleFactor));

	} else {
		// kMaxDistanceより遠い場合は見えない
		scaleFactor = 0.0f;
		isDead_ = true;
	}

	float finalScale = baseScale_ * scaleFactor;
	worldtransfrom_.scale_ = {finalScale, finalScale, finalScale};

	worldtransfrom_.UpdateMatrix();
}

void Meteorite::Draw(const KamataEngine::Camera& camera) {
	if (model_ && !isDead_) {
		model_->Draw(worldtransfrom_, camera);
	}
}

void Meteorite::OnCollision() {
	isDead_ = true;
}

KamataEngine::Vector3 Meteorite::GetWorldPosition() const {
	// ワールド行列から座標を取得
	KamataEngine::Vector3 worldPos;
	worldPos.x = worldtransfrom_.matWorld_.m[3][0];
	worldPos.y = worldtransfrom_.matWorld_.m[3][1];
	worldPos.z = worldtransfrom_.matWorld_.m[3][2];
	return worldPos;
}