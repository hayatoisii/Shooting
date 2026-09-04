#pragma once
#include "Quaternion.h"
#include "MT.h"
#include <3d/Camera.h>
#include <3d/WorldTransform.h>

class Player;

class RailCamera {

public:
	struct Rect {
		float left = 0.0f;
		float right = 1.0f;
		float bottom = 0.0f;
		float top = 1.0f;
	};

	void Initialize(const KamataEngine::Vector3& pos, const KamataEngine::Vector3& rad);
	void Update();

	void SetTarget(Player* target) { target_ = target; }
	const Camera& GetViewProjection() { return camera_; }
	const WorldTransform& GetWorldTransform() { return worldtransfrom_; }

	const KamataEngine::Vector3& GetRotationVelocity() const { return rotationVelocity_; }

	void SetCanMove(bool canMove) { canMove_ = canMove; }

	void SetFixedMode(bool fixed) { fixedMode_ = fixed; }
	bool IsFixedMode() const { return fixedMode_; }

	void SetGolfChaseMode(bool enabled) { golfChaseMode_ = enabled; }
	bool IsGolfChaseMode() const { return golfChaseMode_; }

	void SetBallFlying(bool flying) { isBallFlying_ = flying; }
	void SetGoalPosition(const KamataEngine::Vector3& pos) { goalPosition_ = pos; }
	const KamataEngine::Vector3& GetGoalPosition() const { return goalPosition_; }
	float GetCurrentYaw() const { return currentYaw_; }
	float GetOrbitZoom() const { return orbitZoom_; }
	float GetOrthoHalfHeight() const { return kOrthoHalfHeight_; }
	float GetFixedFocusY() const { return kFixedFocusY_; }

	void Reset();
	void ApplyAimAssist(float ndcX, float ndcY);
	KamataEngine::Matrix4x4 MakeIdentityMatrix();
	void Dodge(float direction);

private:
	Player* target_ = nullptr;
	KamataEngine::WorldTransform worldtransfrom_;
	KamataEngine::Vector3 initialPosition_;
	KamataEngine::Vector3 initialRotationEuler_;
	KamataEngine::Quaternion rotation_;
	KamataEngine::Vector3 rotationVelocity_{0.0f, 0.0f, 0.0f};
	KamataEngine::Vector3 assistAcceleration_ = {0.0f, 0.0f, 0.0f};
	Camera camera_;
	bool canMove_ = false;
	bool fixedMode_ = false;
	bool golfChaseMode_ = false;

	// 2D サイドビュー（固定ズーム・ホイールなし）
	const float kSideCamDistX_ = 55.0f;
	const float kSideCamYBias_ = 4.0f;
	const float kFixedFocusY_ = 18.0f; // Yは追従しない
	// しゅっと追従（大きく離れたら加速して追いつく）
	const float kFocusSmooth_ = 0.14f;
	const float kFocusMaxStep_ = 14.0f;   // 通常時の1フレーム最大移動
	const float kFocusFarDist_ = 40.0f;   // これ以上離れたら遠距離追従
	const float kFocusFarSmooth_ = 0.35f;
	const float kFocusFarMaxStep_ = 55.0f;
	// 以前の約3倍ズームアウト
	const float kOrthoHalfHeight_ = 78.0f;

	KamataEngine::Vector3 goalPosition_ = {0.0f, 10.0f, 1200.0f};
	bool isBallFlying_ = false;
	float orbitZoom_ = 1.0f;
	float currentYaw_ = 0.0f;
	KamataEngine::Vector3 smoothedFocus_ = {0.0f, 18.0f, 0.0f};
	bool focusInitialized_ = false;

	bool isDodging_ = false;
	float dodgeTimer_ = 0.0f;
	float dodgeDirection_ = 0.0f;
	const float kDodgeDuration_ = 30.0f;
};
