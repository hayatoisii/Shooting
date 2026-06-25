#pragma once

#include "KamataEngine.h"
#include <memory>

// トランポリン種別（―=上、I=左右へ跳ねる）
enum class TrampolineSpringType {
	Up = 0,
	Right = 1,
	Left = 2,
};

class TrampolineSpring {
public:
	static constexpr float kBaseThickness = 14.0f;
	static constexpr float kThicknessScale = 1.3f;
	static constexpr float kBounceSpeedY = 16.0f;
	static constexpr float kSideBounceSpeedX = 10.0f;
	static constexpr float kSideBounceLiftY = 14.0f;
	static constexpr float kModelExtent = 8.0f;

	TrampolineSpring() = default;
	TrampolineSpring(const TrampolineSpring&) = delete;
	TrampolineSpring& operator=(const TrampolineSpring&) = delete;
	TrampolineSpring(TrampolineSpring&&) = default;
	TrampolineSpring& operator=(TrampolineSpring&&) = default;

	static TrampolineSpringType GetPlacementType(int placementIndex);
	static float GetThicknessHalf();
	static float GetPlayerSpanHalf(float playerHalfW, float playerHalfH);

	void SetType(TrampolineSpringType type);
	TrampolineSpringType GetType() const { return type_; }

	void SetCenter(const KamataEngine::Vector3& center, float playerHalfW, float playerHalfH);
	const KamataEngine::Vector3& GetCenter() const { return center_; }
	void GetHalfSize(float& halfW, float& halfH) const;

	void Draw(KamataEngine::Model* model, KamataEngine::Camera& camera) const;
	bool TryBounce(float playerX, float playerY, float playerHalfW, float playerHalfH, float& velocityX, float& velocityY, bool& onGround) const;

private:
	void EnsureTransform();
	void SyncTransform();

	float spanHalf_ = 28.0f;
	float thicknessHalf_ = 9.1f;

	TrampolineSpringType type_ = TrampolineSpringType::Up;
	KamataEngine::Vector3 center_{};
	std::unique_ptr<KamataEngine::WorldTransform> transform_;
	mutable bool isPlayerInside_ = false;
};
