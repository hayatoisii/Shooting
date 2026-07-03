#include "TrampolineSpring.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kPi = 3.14159265f;
constexpr float kHalfPi = kPi * 0.5f;

float Lerpf(float a, float b, float t) { return a + (b - a) * t; }

// OBJごとの板の向き（Blender Y-up / -Z forward）
// light(Right): 薄いZ、広いXY → +Z面がカメラ向き
// left/Up/down: 薄いY、広いXZ → +Y面にテクスチャ（矢印）
KamataEngine::Vector3 GetSpringModelRotation(TrampolineSpringType type) {
	switch (type) {
	case TrampolineSpringType::Up:
		return {-kHalfPi, 0.0f, 0.0f};
	case TrampolineSpringType::Down:
		return {kHalfPi, 0.0f, 0.0f};
	case TrampolineSpringType::Right:
		return {0.0f, 0.0f, 0.0f};
	case TrampolineSpringType::Left:
		return {-kHalfPi, 0.0f, kPi};
	}
	return {0.0f, 0.0f, 0.0f};
}

float GetVisualScaleW(TrampolineSpringType type) {
	switch (type) {
	case TrampolineSpringType::Up:
		return TrampolineSpring::kVisualScaleUp;
	case TrampolineSpringType::Down:
		return TrampolineSpring::kVisualScaleDown;
	case TrampolineSpringType::Right:
		return TrampolineSpring::kVisualScaleRight;
	case TrampolineSpringType::Left:
		return TrampolineSpring::kVisualScaleLeft;
	}
	return 1.0f;
}

float GetVisualScaleH(TrampolineSpringType type) { return GetVisualScaleW(type); }

KamataEngine::Vector3 GetSpringVisualScale(TrampolineSpringType type, float halfW, float halfH) {
	const float sw = (halfW * 2.0f) / TrampolineSpring::kModelExtent * GetVisualScaleW(type);
	const float sh = (halfH * 2.0f) / TrampolineSpring::kModelExtent * GetVisualScaleH(type);

	switch (type) {
	case TrampolineSpringType::Right:
		return {sw, sh, 1.0f};
	case TrampolineSpringType::Left:
	case TrampolineSpringType::Up:
	case TrampolineSpringType::Down:
		return {sw, 1.0f, sh};
	}
	return {1.0f, 1.0f, 1.0f};
}
} // namespace

namespace {
KamataEngine::Vector3 GetArrowRotation(TrampolineSpringType type) {
	// yazirusi デフォルトは+Y向き。Z回転でバネの跳ね方向へ向ける
	switch (type) {
	case TrampolineSpringType::Up:
		return {0.0f, 0.0f, 0.0f};
	case TrampolineSpringType::Down:
		return {0.0f, 0.0f, kPi};
	case TrampolineSpringType::Right:
		return {0.0f, 0.0f, -kHalfPi};
	case TrampolineSpringType::Left:
		return {0.0f, 0.0f, kHalfPi};
	}
	return {0.0f, 0.0f, 0.0f};
}

void GetArrowMarkerPosition(TrampolineSpringType type, const KamataEngine::Vector3& center, float halfW, float halfH, float animPhase, int arrowIndex, KamataEngine::Vector3& outPosition) {
	const float end = TrampolineSpring::kArrowEndRatio;
	const float buried = TrampolineSpring::kArrowBuriedRatio;
	const float outward = TrampolineSpring::kArrowOutwardOffset;
	const float extra = TrampolineSpring::kArrowTravelExtra;
	const float travel = TrampolineSpring::kArrowTravelScale;
	const float depthZ = TrampolineSpring::kArrowDepthZ;

	float localPhase = animPhase;
	if (arrowIndex == 1 && TrampolineSpring::kArrowStaggerSecondMarker) {
		localPhase += TrampolineSpring::kArrowSecondMarkerPhaseOffset;
		if (localPhase >= 1.0f) {
			localPhase -= 1.0f;
		}
	}

	float t = localPhase;
	t = std::clamp(t, 0.0f, 1.0f);
	// easeOutQuint
	t = 1.0f - std::pow(1.0f - t, 5.0f);

	switch (type) {
	case TrampolineSpringType::Up: {
		const float y0 = center.y - halfH * buried * travel;
		const float y1 = center.y + halfH + (outward + extra) * travel;
		const float y = Lerpf(y0, y1, t);
		float x = center.x;
		if constexpr (TrampolineSpring::kArrowMarkerCount >= 2) {
			x = (arrowIndex == 0) ? center.x - halfW * end : center.x + halfW * end;
		}
		outPosition = {x, y, depthZ};
		break;
	}
	case TrampolineSpringType::Down: {
		const float y0 = center.y + halfH * buried * travel;
		const float y1 = center.y - halfH - (outward + extra) * travel;
		const float y = Lerpf(y0, y1, t);
		float x = center.x;
		if constexpr (TrampolineSpring::kArrowMarkerCount >= 2) {
			x = (arrowIndex == 0) ? center.x - halfW * end : center.x + halfW * end;
		}
		outPosition = {x, y, depthZ};
		break;
	}
	case TrampolineSpringType::Left: {
		const float x0 = center.x - halfW * buried * travel;
		const float x1 = center.x - halfW - (outward + extra) * travel;
		const float x = Lerpf(x0, x1, t);
		float y = center.y;
		if constexpr (TrampolineSpring::kArrowMarkerCount >= 2) {
			y = (arrowIndex == 0) ? center.y - halfH * end : center.y + halfH * end;
		}
		outPosition = {x, y, depthZ};
		break;
	}
	case TrampolineSpringType::Right: {
		const float x0 = center.x + halfW * buried * travel;
		const float x1 = center.x + halfW + (outward + extra) * travel;
		const float x = Lerpf(x0, x1, t);
		float y = center.y;
		if constexpr (TrampolineSpring::kArrowMarkerCount >= 2) {
			y = (arrowIndex == 0) ? center.y - halfH * end : center.y + halfH * end;
		}
		outPosition = {x, y, depthZ};
		break;
	}
	default:
		outPosition = center;
		break;
	}
}
} // namespace

TrampolineSpringType TrampolineSpring::GetPlacementType(int placementIndex) {
	const int typeIndex = placementIndex % 4;
	if (typeIndex == 1) {
		return TrampolineSpringType::Down;
	}
	if (typeIndex == 2) {
		return TrampolineSpringType::Right;
	}
	if (typeIndex == 3) {
		return TrampolineSpringType::Left;
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
	if (type_ == type) {
		return;
	}
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

void TrampolineSpring::EnsureArrowTransforms() const {
	if (arrowTransform0_) {
		return;
	}
	arrowTransform0_ = std::make_unique<KamataEngine::WorldTransform>();
	arrowTransform0_->Initialize();
	if constexpr (kArrowMarkerCount >= 2) {
		arrowTransform1_ = std::make_unique<KamataEngine::WorldTransform>();
		arrowTransform1_->Initialize();
	}
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
	transform_->rotation_ = GetSpringModelRotation(type_);
	transform_->scale_ = GetSpringVisualScale(type_, halfW, halfH);
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

	if (!arrowAnimOffsetSet_) {
		const int phaseSeed = static_cast<int>(center_.x * 0.17f + center_.y * 0.31f);
		arrowAnimOffset_ = static_cast<float>(phaseSeed % 100) / 100.0f;
		arrowAnimOffsetSet_ = true;
	}

	SyncTransform();
}

float TrampolineSpring::GetArrowAnimPhase(float globalAnimTime) const {
	float phase = globalAnimTime / kArrowAnimSeconds + arrowAnimOffset_;
	phase = phase - std::floor(phase);
	return phase;
}

void TrampolineSpring::Draw(KamataEngine::Model* model, KamataEngine::Camera& camera) const {
	if (!model || !transform_) {
		return;
	}
	model->Draw(*transform_, camera);
}

void TrampolineSpring::DrawArrowMarkers(KamataEngine::Model* arrowModel, float globalAnimTime, KamataEngine::Camera& camera) const {
	if (!arrowModel) {
		return;
	}

	EnsureArrowTransforms();

	float halfW = 0.0f;
	float halfH = 0.0f;
	GetHalfSize(halfW, halfH);

	const float animPhase = GetArrowAnimPhase(globalAnimTime);
	const float scale = kArrowWorldScale;
	const KamataEngine::Vector3 rotation = GetArrowRotation(type_);

	for (int i = 0; i < kArrowMarkerCount; ++i) {
		KamataEngine::WorldTransform* transform = (i == 0) ? arrowTransform0_.get() : arrowTransform1_.get();
		if (!transform) {
			continue;
		}

		KamataEngine::Vector3 position = {};
		GetArrowMarkerPosition(type_, center_, halfW, halfH, animPhase, i, position);
		transform->translation_ = position;
		transform->rotation_ = rotation;
		transform->scale_ = {scale, scale, scale};
		transform->parent_ = nullptr;
		transform->UpdateMatrix();
		arrowModel->Draw(*transform, camera);
	}
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
