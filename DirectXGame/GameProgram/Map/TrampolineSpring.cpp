#include "TrampolineSpring.h"







#include <cmath>







TrampolineSpringType TrampolineSpring::GetPlacementType(int placementIndex) {



	const int typeIndex = placementIndex % 4;



	if (typeIndex == 1) {



		return TrampolineSpringType::Right;



	}



	if (typeIndex == 2) {



		return TrampolineSpringType::Left;



	}



	if (typeIndex == 3) {



		return TrampolineSpringType::Down;



	}



	return TrampolineSpringType::Up;



}







float TrampolineSpring::GetThicknessHalf() { return kBaseThickness * kThicknessScale * 0.5f; }







float TrampolineSpring::GetUpSpringSpanHalf(float playerHalfW) { return playerHalfW * 3.0f; }







float TrampolineSpring::GetSideSpringSpanHalf(float playerHalfW) { return playerHalfW * 3.0f; }







bool TrampolineSpring::IsHorizontalType() const {



	return type_ == TrampolineSpringType::Up || type_ == TrampolineSpringType::Down;



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



	if (IsHorizontalType()) {



		halfW = spanHalf_;



		halfH = thicknessHalf_;



	} else {



		halfW = thicknessHalf_;



		halfH = spanHalf_;



	}



}







float TrampolineSpring::GetStopZoneHalfThickness() const {



	float springHalfW = 0.0f;



	float springHalfH = 0.0f;



	GetHalfSize(springHalfW, springHalfH);







	if (IsHorizontalType()) {



		return springHalfH * 0.35f;



	}



	return springHalfW * 0.35f;



}







bool TrampolineSpring::IsPlayerOverlapping(float playerX, float playerY, float playerHalfW, float playerHalfH) const {



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







	return !(playerMaxX <= springMinX || playerMinX >= springMaxX || playerMaxY <= springMinY || playerMinY >= springMaxY);



}







bool TrampolineSpring::IsPlayerCenterInStopZone(float playerX, float playerY) const {



	const float stopHalf = GetStopZoneHalfThickness();







	if (IsHorizontalType()) {



		return std::abs(playerY - center_.y) <= stopHalf;



	}



	return std::abs(playerX - center_.x) <= stopHalf;



}







bool TrampolineSpring::DidEnterOrCrossStopZone(float prevX, float prevY, float curX, float curY) const {



	if (IsPlayerCenterInStopZone(curX, curY)) {



		return true;



	}







	const float stopHalf = GetStopZoneHalfThickness();







	if (IsHorizontalType()) {



		const float prevOffset = prevY - center_.y;



		const float curOffset = curY - center_.y;



		if (prevOffset > stopHalf && curOffset <= stopHalf) {



			return true;



		}



		if (prevOffset < -stopHalf && curOffset >= -stopHalf) {



			return true;



		}



		return false;



	}







	const float prevOffset = prevX - center_.x;



	const float curOffset = curX - center_.x;



	if (prevOffset > stopHalf && curOffset <= stopHalf) {



		return true;



	}



	if (prevOffset < -stopHalf && curOffset >= -stopHalf) {



		return true;



	}



	return false;



}







void TrampolineSpring::ResetPlayerContact() { isPlayerInside_ = false; }

bool TrampolineSpring::FindOverlapEntryOnPath(float prevX, float prevY, float curX, float curY, float playerHalfW, float playerHalfH, float& outX, float& outY) const {
	if (IsPlayerOverlapping(curX, curY, playerHalfW, playerHalfH)) {
		outX = curX;
		outY = curY;
		return true;
	}

	constexpr int kSteps = 8;
	for (int i = 1; i <= kSteps; ++i) {
		const float t = static_cast<float>(i) / static_cast<float>(kSteps);
		const float sampleX = prevX + (curX - prevX) * t;
		const float sampleY = prevY + (curY - prevY) * t;
		if (IsPlayerOverlapping(sampleX, sampleY, playerHalfW, playerHalfH)) {
			outX = sampleX;
			outY = sampleY;
			return true;
		}
	}

	if (IsPlayerOverlapping(prevX, prevY, playerHalfW, playerHalfH)) {
		outX = prevX;
		outY = prevY;
		return true;
	}

	return false;
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



	(void)playerHalfH;



	center_ = center;



	center_.z = 0.8f;



	if (IsHorizontalType()) {



		spanHalf_ = GetUpSpringSpanHalf(playerHalfW);



	} else {



		spanHalf_ = GetSideSpringSpanHalf(playerHalfW);



	}



	thicknessHalf_ = GetThicknessHalf();



	SyncTransform();



}







void TrampolineSpring::Draw(KamataEngine::Model* model, KamataEngine::Camera& camera) const {



	if (!model || !transform_) {



		return;



	}







	model->Draw(*transform_, camera);



}







TrampolineBounceResult TrampolineSpring::TryBounce(float prevX, float prevY, float& playerX, float& playerY, float playerHalfW, float playerHalfH) const {

	float entryX = playerX;
	float entryY = playerY;
	if (!FindOverlapEntryOnPath(prevX, prevY, playerX, playerY, playerHalfW, playerHalfH, entryX, entryY)) {
		isPlayerInside_ = false;
		return TrampolineBounceResult::None;
	}

	if (isPlayerInside_) {
		return TrampolineBounceResult::None;
	}

	isPlayerInside_ = true;
	playerX = entryX;
	playerY = entryY;

	if (type_ == TrampolineSpringType::Up) {
		return TrampolineBounceResult::EnterUp;
	}

	if (type_ == TrampolineSpringType::Down) {
		return TrampolineBounceResult::EnterDown;
	}

	return TrampolineBounceResult::EnterSide;
}





