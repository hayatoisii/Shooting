#include "TrampolineSpring.h"

TrampolineSpringType TrampolineSpring::GetPlacementType(int placementIndex) {
	const int typeIndex = placementIndex % 3;
	if (typeIndex == 1) {
		return TrampolineSpringType::Right;
	}
	if (typeIndex == 2) {
		return TrampolineSpringType::Left;
	}
	return TrampolineSpringType::Up;
}

float TrampolineSpring::GetThicknessHalf() { return kBaseThickness * kThicknessScale * 0.5f; }

float TrampolineSpring::GetPlayerSpanHalf(float playerHalfW, float playerHalfH) {
	(void)playerHalfH;
	return playerHalfW * 2.0f;
}

void TrampolineSpring::SetType(TrampolineSpringType type) {
	type_ = type;
	if (transform_) {
		SyncTransform();
	}
}

void TrampolineSpring::EnsureTransform() {
	if (transform_) {
		return;
	}

	transform_ = std::make_unique<KamataEngine::WorldTransform>();
	transform_->Initialize();
}

void TrampolineSpring::GetHalfSize(float& halfW, float& halfH) const {
	if (type_ == TrampolineSpringType::Up) {
		halfW = spanHalf_;
		halfH = thicknessHalf_;
	} else {
		halfW = thicknessHalf_;
		halfH = spanHalf_;
	}
}

void TrampolineSpring::SyncTransform() {
	EnsureTransform();

	float halfW = 0.0f;
	float halfH = 0.0f;
	GetHalfSize(halfW, halfH);

	transform_->translation_ = center_;
	transform_->rotation_ = {0.0f, 0.0f, 0.0f};
	transform_->scale_ = {(halfW * 2.0f) / kModelExtent, (halfH * 2.0f) / kModelExtent, 1.0f};
	transform_->parent_ = nullptr;
	transform_->UpdateMatrix();
}

void TrampolineSpring::SetCenter(const KamataEngine::Vector3& center, float playerHalfW, float playerHalfH) {
	center_ = center;
	center_.z = 0.8f;
	spanHalf_ = GetPlayerSpanHalf(playerHalfW, playerHalfH);
	thicknessHalf_ = GetThicknessHalf();
	SyncTransform();
}

void TrampolineSpring::Draw(KamataEngine::Model* model, KamataEngine::Camera& camera) const {
	if (!model || !transform_) {
		return;
	}

	model->Draw(*transform_, camera);
}

bool TrampolineSpring::TryBounce(float playerX, float playerY, float playerHalfW, float playerHalfH, float& velocityX, float& velocityY, bool& onGround) const {
	float springHalfW = 0.0f;
	float springHalfH = 0.0f;
	GetHalfSize(springHalfW, springHalfH);

	const float playerMinX = playerX - playerHalfW;
	const float playerMaxX = playerX + playerHalfW;
	const float playerMinY = playerY - playerHalfH;
	const float playerMaxY = playerY + playerHalfH;

	const float springMinX = center_.x - springHalfW;
	const float springMaxX = center_.x + springHalfW;
	const float springMinY = center_.y - springHalfH;
	const float springMaxY = center_.y + springHalfH;

	const bool isInside = !(playerMaxX <= springMinX || playerMinX >= springMaxX || playerMaxY <= springMinY || playerMinY >= springMaxY);

	if (!isInside) {
		isPlayerInside_ = false;
		return false;
	}

	if (isPlayerInside_) {
		return false;
	}

	isPlayerInside_ = true;

	switch (type_) {
	case TrampolineSpringType::Up:
		velocityY = kBounceSpeedY;
		break;
	case TrampolineSpringType::Right:
		velocityX = kSideBounceSpeedX;
		velocityY = kSideBounceLiftY;
		break;
	case TrampolineSpringType::Left:
		velocityX = -kSideBounceSpeedX;
		velocityY = kSideBounceLiftY;
		break;
	}

	onGround = false;
	return true;
}
