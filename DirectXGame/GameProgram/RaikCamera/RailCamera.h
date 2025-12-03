#pragma once
#include "../../Quaternion.h"
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

	Player* target_ = nullptr;

	void setTarget(Player* target) { target_ = target; }
	const Camera& GetViewProjection() { return camera_; }
	const WorldTransform& GetWorldTransform() { return worldtransfrom_; }

	const KamataEngine::Vector3& GetRotationVelocity() const { return rotationVelocity_; }

	void SetCanMove(bool canMove) { canMove_ = canMove; }

	void Reset();

	void ApplyAimAssist(float ndcX, float ndcY);

	KamataEngine::Matrix4x4 MakeIdentityMatrix();

	void Dodge(float direction);

private:
	KamataEngine::WorldTransform worldtransfrom_;

	KamataEngine::Vector3 initialPosition_;
	KamataEngine::Vector3 initialRotationEuler_;

	KamataEngine::Quaternion rotation_;

	KamataEngine::Vector3 rotationVelocity_{0.0f, 0.0f, 0.0f};

	KamataEngine::Vector3 assistAcceleration_ = {0.0f, 0.0f, 0.0f}; // アシストによる加速度

	Camera camera_;

	bool canMove_;

};