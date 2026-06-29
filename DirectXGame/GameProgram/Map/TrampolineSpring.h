#pragma once







#include "KamataEngine.h"



#include <memory>







// トランポリン種別（―=上下、I=左右へ跳ねる）



enum class TrampolineSpringType {



	Up = 0,



	Right = 1,



	Left = 2,



	Down = 3,



};







enum class TrampolineBounceResult {



	None = 0,



	EnterUp = 1,



	EnterSide = 2,



	EnterDown = 3,



};







class TrampolineSpring {



public:



	static constexpr float kBaseThickness = 14.0f;



	static constexpr float kThicknessScale = 1.56f;



	static constexpr float kBounceSpeedY = 16.0f;



	static constexpr float kDownBounceSpeedY = 16.0f;



	static constexpr float kSideBounceSpeedX = 10.0f;



	static constexpr float kSideBounceLiftY = 10.0f;



	static constexpr float kModelExtent = 8.0f;







	TrampolineSpring() = default;



	TrampolineSpring(const TrampolineSpring&) = delete;



	TrampolineSpring& operator=(const TrampolineSpring&) = delete;



	TrampolineSpring(TrampolineSpring&&) = default;



	TrampolineSpring& operator=(TrampolineSpring&&) = default;







	static TrampolineSpringType GetPlacementType(int placementIndex);



	static float GetThicknessHalf();



	static float GetUpSpringSpanHalf(float playerHalfW);



	static float GetSideSpringSpanHalf(float playerHalfW);







	void SetType(TrampolineSpringType type);



	TrampolineSpringType GetType() const { return type_; }







	void SetCenter(const KamataEngine::Vector3& center, float playerHalfW, float playerHalfH);



	const KamataEngine::Vector3& GetCenter() const { return center_; }



	void GetHalfSize(float& halfW, float& halfH) const;



	bool IsPlayerCenterInStopZone(float playerX, float playerY) const;



	bool DidEnterOrCrossStopZone(float prevX, float prevY, float curX, float curY) const;



	bool IsPlayerOverlapping(float playerX, float playerY, float playerHalfW, float playerHalfH) const;



	void ResetPlayerContact();

	void Draw(KamataEngine::Model* model, KamataEngine::Camera& camera) const;

	TrampolineBounceResult TryBounce(float prevX, float prevY, float& playerX, float& playerY, float playerHalfW, float playerHalfH) const;







private:



	void EnsureTransform();



	void SyncTransform();



	float GetStopZoneHalfThickness() const;



	bool IsHorizontalType() const;

	bool FindOverlapEntryOnPath(float prevX, float prevY, float curX, float curY, float playerHalfW, float playerHalfH, float& outX, float& outY) const;

	float spanHalf_ = 28.0f;



	float thicknessHalf_ = 9.1f;







	TrampolineSpringType type_ = TrampolineSpringType::Up;



	KamataEngine::Vector3 center_{};



	std::unique_ptr<KamataEngine::WorldTransform> transform_;



	mutable bool isPlayerInside_ = false;



};





