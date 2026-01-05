#include "RailCamera.h"
#include "../../Quaternion.h"
#include <KamataEngine.h>
#include <algorithm>
#include <cmath>

using namespace KamataEngine;

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

	assistAcceleration_ = {0.0f, 0.0f, 0.0f};
}

void RailCamera::Update() {
	KamataEngine::Input* input = KamataEngine::Input::GetInstance();

	// 自動飛行の速度
	const float kCameraSpeed = 5.0f;          // 0.8f; // 1.5
	const float kPitchAcceleration = 0.0019f; // 縦の回転 0.002　　//0.0022f
	const float kRollAcceleration = 0.0016f;  // 横の回転
	const float kYawAcceleration = 0.0008f;   // 左右の旋回0.001
	const float kRotFriction = 0.95f;
	const float kYawFriction = 0.86f;
	const float kXawFriction = 0.90f; // 89

	Vector3 rotAcceleration = assistAcceleration_;
	assistAcceleration_ = {0.0f, 0.0f, 0.0f};

	if (input->PushKey(DIK_W)) {
		rotAcceleration.x = -kPitchAcceleration;
	}
	if (input->PushKey(DIK_S)) {
		rotAcceleration.x = kPitchAcceleration;
	}
	if (input->PushKey(DIK_LEFT)) {
		rotAcceleration.z = kRollAcceleration; // ロール
	}
	if (input->PushKey(DIK_RIGHT)) {
		rotAcceleration.z = -kRollAcceleration; // ロール
	}

	if (input->PushKey(DIK_A)) {
		rotAcceleration.y = -kYawAcceleration; // ヨー
	}
	if (input->PushKey(DIK_D)) {
		rotAcceleration.y = kYawAcceleration; // ヨー
	}

	rotationVelocity_ += rotAcceleration;

	// 自動水平
	if (input->PushKey(DIK_R)) {

		const float kRestoreAcceleration = 0.001f;

		Matrix4x4 currentRotationMatrix = Quaternion::MakeMatrix(rotation_);
		Vector3 localX_Right = {currentRotationMatrix.m[0][0], currentRotationMatrix.m[0][1], currentRotationMatrix.m[0][2]};
		Vector3 localZ_Forward = {currentRotationMatrix.m[2][0], currentRotationMatrix.m[2][1], currentRotationMatrix.m[2][2]};

		rotationVelocity_.z += localX_Right.y * kRestoreAcceleration;

		rotationVelocity_.x -= localZ_Forward.y * kRestoreAcceleration;
	}

	rotationVelocity_.x *= kXawFriction;
	rotationVelocity_.z *= kRotFriction;
	rotationVelocity_.y *= kYawFriction;

	// クォータニオン
	Matrix4x4 currentRotationMatrix = Quaternion::MakeMatrix(rotation_);
	Vector3 localXAxis = {currentRotationMatrix.m[0][0], currentRotationMatrix.m[0][1], currentRotationMatrix.m[0][2]};
	Vector3 localYAxis = {currentRotationMatrix.m[1][0], currentRotationMatrix.m[1][1], currentRotationMatrix.m[1][2]};
	Vector3 localZAxis = {currentRotationMatrix.m[2][0], currentRotationMatrix.m[2][1], currentRotationMatrix.m[2][2]};

	localXAxis = localXAxis.Normalize();
	localYAxis = localYAxis.Normalize();
	localZAxis = localZAxis.Normalize();

	float deltaPitch = rotationVelocity_.x;
	float deltaYaw = rotationVelocity_.y;
	float deltaRoll = rotationVelocity_.z;

	Quaternion deltaQuatYaw = Quaternion::MakeRotateAxisAngle(localYAxis, deltaYaw);
	Quaternion deltaQuatPitch = Quaternion::MakeRotateAxisAngle(localXAxis, deltaPitch);
	Quaternion deltaQuatRoll = Quaternion::MakeRotateAxisAngle(localZAxis, deltaRoll);

	rotation_ = deltaQuatRoll * deltaQuatPitch * deltaQuatYaw * rotation_;
	rotation_ = Quaternion::Normalize(rotation_);

	Quaternion finalRotation = rotation_;

	Matrix4x4 rotationMatrix = Quaternion::MakeMatrix(rotation_);

	// 常に自動で前進
	Vector3 move = {0.0f, 0.0f, 0.0f};

	if (canMove_) {
		move.z += kCameraSpeed;
	}

	if (isDodging_) {

		dodgeTimer_ += 1.0f;
		float t = dodgeTimer_ / kDodgeDuration_;

		if (t >= 1.0f) {
			t = 1.0f;
			isDodging_ = false;
		}

		//　回避の時に横移動する距離
		const float kDodgeMoveSpeed = 3.0f; // 2.0f

		move.x += dodgeDirection_ * kDodgeMoveSpeed * (1.0f - t);
	}

	move = KamataEngine::MathUtility::TransformNormal(move, rotationMatrix);
	Vector3 currentPosition = worldtransfrom_.translation_;
	Vector3 newPosition = currentPosition + move;
	
	// Playerの移動範囲を円状に制限（初期位置からの距離が15000以内）
	Vector3 offsetFromInitial = newPosition - initialPosition_;
	float distanceFromInitial = std::sqrt(offsetFromInitial.x * offsetFromInitial.x + offsetFromInitial.y * offsetFromInitial.y + offsetFromInitial.z * offsetFromInitial.z);
	
	if (distanceFromInitial > kMaxMoveRadius_) {
		// 範囲を超えた場合、初期位置方向にクランプ
		Vector3 directionToInitial = offsetFromInitial;
		if (distanceFromInitial > 0.001f) {
			directionToInitial.x /= distanceFromInitial;
			directionToInitial.y /= distanceFromInitial;
			directionToInitial.z /= distanceFromInitial;
		}
		newPosition = initialPosition_ + directionToInitial * kMaxMoveRadius_;
	}
	
	worldtransfrom_.matWorld_ = rotationMatrix;
	worldtransfrom_.matWorld_.m[3][0] = newPosition.x;
	worldtransfrom_.matWorld_.m[3][1] = newPosition.y;
	worldtransfrom_.matWorld_.m[3][2] = newPosition.z;
	worldtransfrom_.translation_ = newPosition;

	camera_.matView = KamataEngine::MathUtility::Inverse(worldtransfrom_.matWorld_);
	camera_.TransferMatrix();
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

void RailCamera::ApplyAimAssist(float ndcX, float ndcY) {
	// 視点が吸い寄せられる強さ (この値を調整)
	const float kAimAssistStrength = 0.005f; // 0.005fから0.015f

	// (※キー操作設定 (A/D=ヨー, W/S=ピッチ) に合わせて加速度を設定)

	// ターゲットが右(ndcX > 0)なら、右(Dキー)方向の加速度(y+)を加える
	assistAcceleration_.y += ndcX * kAimAssistStrength;

	// ターゲットが上(ndcY > 0)なら、上(Wキー)方向の加速度(x-)を加える
	assistAcceleration_.x -= ndcY * kAimAssistStrength;
}