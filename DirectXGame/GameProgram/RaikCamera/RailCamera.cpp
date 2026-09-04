#include "RailCamera.h"
#include "Player.h"
#include "Quaternion.h"
#include <KamataEngine.h>
#include <cmath>

using namespace KamataEngine;

static Matrix4x4 MakeLookAtWorldMatrix(const Vector3& eye, const Vector3& target) {
	Vector3 zAxis = {target.x - eye.x, target.y - eye.y, target.z - eye.z};
	float zLen = std::sqrt(zAxis.x * zAxis.x + zAxis.y * zAxis.y + zAxis.z * zAxis.z);
	if (zLen > 0.001f) {
		zAxis.x /= zLen;
		zAxis.y /= zLen;
		zAxis.z /= zLen;
	} else {
		zAxis = {0.0f, 0.0f, 1.0f};
	}

	Vector3 worldUp = {0.0f, 1.0f, 0.0f};
	Vector3 xAxis = {
	    worldUp.y * zAxis.z - worldUp.z * zAxis.y,
	    worldUp.z * zAxis.x - worldUp.x * zAxis.z,
	    worldUp.x * zAxis.y - worldUp.y * zAxis.x};
	float xLen = std::sqrt(xAxis.x * xAxis.x + xAxis.y * xAxis.y + xAxis.z * xAxis.z);
	if (xLen > 0.001f) {
		xAxis.x /= xLen;
		xAxis.y /= xLen;
		xAxis.z /= xLen;
	} else {
		xAxis = {1.0f, 0.0f, 0.0f};
	}

	Vector3 yAxis = {
	    zAxis.y * xAxis.z - zAxis.z * xAxis.y,
	    zAxis.z * xAxis.x - zAxis.x * xAxis.z,
	    zAxis.x * xAxis.y - zAxis.y * xAxis.x};

	Matrix4x4 lookMat = {};
	lookMat.m[0][0] = xAxis.x;
	lookMat.m[0][1] = xAxis.y;
	lookMat.m[0][2] = xAxis.z;
	lookMat.m[1][0] = yAxis.x;
	lookMat.m[1][1] = yAxis.y;
	lookMat.m[1][2] = yAxis.z;
	lookMat.m[2][0] = zAxis.x;
	lookMat.m[2][1] = zAxis.y;
	lookMat.m[2][2] = zAxis.z;
	lookMat.m[3][0] = eye.x;
	lookMat.m[3][1] = eye.y;
	lookMat.m[3][2] = eye.z;
	lookMat.m[3][3] = 1.0f;
	return lookMat;
}

void RailCamera::Initialize(const KamataEngine::Vector3& pos, const KamataEngine::Vector3& rad) {
	initialPosition_ = pos;
	initialRotationEuler_ = rad;

	worldtransfrom_.translation_ = pos;
	worldtransfrom_.Initialize();
	camera_.Initialize();

	Quaternion qPitch = Quaternion::MakeRotateAxisAngle({1.0f, 0.0f, 0.0f}, initialRotationEuler_.x);
	Quaternion qYaw = Quaternion::MakeRotateAxisAngle({0.0f, 1.0f, 0.0f}, initialRotationEuler_.y);
	rotation_ = qYaw * qPitch;
	rotation_ = Quaternion::Normalize(rotation_);

	canMove_ = false;
	currentYaw_ = 0.0f;
	orbitZoom_ = 1.0f;
	assistAcceleration_ = {0.0f, 0.0f, 0.0f};
	focusInitialized_ = false;
}

void RailCamera::Update() {
	if (golfChaseMode_ && target_) {
		Vector3 focus = target_->GetCameraFocusPosition();

		if (!focusInitialized_) {
			smoothedFocus_ = {focus.x, kFixedFocusY_, focus.z};
			focusInitialized_ = true;
		} else {
			Vector3 targetFocus = {focus.x, kFixedFocusY_, focus.z};
			Vector3 delta = {
			    targetFocus.x - smoothedFocus_.x,
			    0.0f,
			    targetFocus.z - smoothedFocus_.z};
			float dist = std::sqrt(delta.x * delta.x + delta.z * delta.z);
			if (dist > 0.0001f) {
				const bool isFar = dist > kFocusFarDist_;
				const float smooth = isFar ? kFocusFarSmooth_ : kFocusSmooth_;
				const float maxStep = isFar ? kFocusFarMaxStep_ : kFocusMaxStep_;
				float step = dist * smooth;
				if (step > maxStep) {
					step = maxStep;
				}
				float t = step / dist;
				smoothedFocus_.x += delta.x * t;
				smoothedFocus_.z += delta.z * t;
			}
			smoothedFocus_.y = kFixedFocusY_;
		}

		worldtransfrom_.translation_ = {
		    smoothedFocus_.x + kSideCamDistX_,
		    smoothedFocus_.y + kSideCamYBias_,
		    smoothedFocus_.z};

		worldtransfrom_.matWorld_ = MakeLookAtWorldMatrix(worldtransfrom_.translation_, smoothedFocus_);
		camera_.matView = KamataEngine::MathUtility::Inverse(worldtransfrom_.matWorld_);

		const float halfH = kOrthoHalfHeight_;
		const float halfW = halfH * camera_.aspectRatio;
		camera_.matProjection = KamataEngine::MathUtility::MakeOrthographicMatrix(
		    -halfW, halfH, halfW, -halfH, camera_.nearZ, camera_.farZ);
		camera_.TransferMatrix();
		return;
	}

	if (fixedMode_) {
		Matrix4x4 rotationMatrix = Quaternion::MakeMatrix(rotation_);
		worldtransfrom_.matWorld_ = rotationMatrix;
		worldtransfrom_.matWorld_.m[3][0] = initialPosition_.x;
		worldtransfrom_.matWorld_.m[3][1] = initialPosition_.y;
		worldtransfrom_.matWorld_.m[3][2] = initialPosition_.z;
		worldtransfrom_.translation_ = initialPosition_;
		camera_.matView = KamataEngine::MathUtility::Inverse(worldtransfrom_.matWorld_);
		camera_.TransferMatrix();
		return;
	}

	(void)isBallFlying_;
	(void)canMove_;
}

void RailCamera::Reset() {
	Quaternion qPitch = Quaternion::MakeRotateAxisAngle({1.0f, 0.0f, 0.0f}, initialRotationEuler_.x);
	Quaternion qYaw = Quaternion::MakeRotateAxisAngle({0.0f, 1.0f, 0.0f}, initialRotationEuler_.y);
	rotation_ = qYaw * qPitch;
	rotation_ = KamataEngine::Quaternion::Normalize(rotation_);

	rotationVelocity_ = {0.0f, 0.0f, 0.0f};
	assistAcceleration_ = {0.0f, 0.0f, 0.0f};

	worldtransfrom_.matWorld_ = MakeIdentityMatrix();
	worldtransfrom_.matWorld_.m[3][0] = initialPosition_.x;
	worldtransfrom_.matWorld_.m[3][1] = initialPosition_.y;
	worldtransfrom_.matWorld_.m[3][2] = initialPosition_.z;
	worldtransfrom_.translation_ = initialPosition_;

	camera_.matView = KamataEngine::MathUtility::Inverse(worldtransfrom_.matWorld_);
	camera_.TransferMatrix();

	canMove_ = false;
	orbitZoom_ = 1.0f;
	currentYaw_ = 0.0f;
	focusInitialized_ = false;
}

KamataEngine::Matrix4x4 RailCamera::MakeIdentityMatrix() {
	KamataEngine::Matrix4x4 result = {};
	result.m[0][0] = 1.0f;
	result.m[1][1] = 1.0f;
	result.m[2][2] = 1.0f;
	result.m[3][3] = 1.0f;
	return result;
}

void RailCamera::Dodge(float direction) {
	if (isDodging_) {
		return;
	}
	isDodging_ = true;
	dodgeTimer_ = 0.0f;
	dodgeDirection_ = direction;
}

void RailCamera::ApplyAimAssist(float, float) {}
