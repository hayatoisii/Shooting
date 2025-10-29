#pragma once
#include<3d/WorldTransform.h>
#include <3d/Camera.h>
#include"MT.h"

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

	void Reset(); 

	KamataEngine::Matrix4x4 MakeRotateAxisAngle(const KamataEngine::Vector3& axis, float angle);
	KamataEngine::Matrix4x4 MakeIdentityMatrix();

private:

	float totalYaw_ = 0.0f;   // 水平回転 (左右)
	float totalPitch_ = 0.0f; // 垂直回転 (上下)

	KamataEngine::WorldTransform worldtransfrom_;

	KamataEngine::Vector3 initialPosition_;

	KamataEngine::Camera camera_;

	KamataEngine::Vector3 velocity_ = {1.0f,1.0f,1.0f};
	KamataEngine::Vector3 rotationVelocity_ = {0.0f, 0.0f, 0.0f};

};