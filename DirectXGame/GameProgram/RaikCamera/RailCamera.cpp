#include "RailCamera.h"
#include "../../Quaternion.h" // (★ クォータニオンヘッダのパス)
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
}

void RailCamera::Update() {
	KamataEngine::Input* input = KamataEngine::Input::GetInstance();

	const float kCameraSpeed = 2.0f; // 0.8f; // 1.5
	const float kRotAcceleration = 0.0008f; // 0.0006f;  // 0.001f;
	const float kRotFriction = 0.95f;

	// --- 1. 回転「速度」の更新 (慣性モデル) ---

	Vector3 rotAcceleration = {0.0f, 0.0f, 0.0f};

	// (キー入力による加速度計算)
	if (input->PushKey(DIK_W)) {
		rotAcceleration.x = -kRotAcceleration; // ピッチ
	}
	if (input->PushKey(DIK_S)) {
		rotAcceleration.x = kRotAcceleration; // ピッチ
	}

	// ▼▼▼ A/D と LEFT/RIGHT の役割を入れ替え ▼▼▼

	if (input->PushKey(DIK_A)) {
		// rotAcceleration.y = -kRotAcceleration; // ★ 修正前 (ヨー)
		rotAcceleration.z = kRotAcceleration; // ★ 修正後 (ロール)
	}
	if (input->PushKey(DIK_D)) {
		// rotAcceleration.y = kRotAcceleration; // ★ 修正前 (ヨー)
		rotAcceleration.z = -kRotAcceleration; // ★ 修正後 (ロール)
	}
	if (input->PushKey(DIK_LEFT)) {
		// rotAcceleration.z = -kRotAcceleration; // ★ 修正前 (ロール)
		rotAcceleration.y = -kRotAcceleration; // ★ 修正後 (ヨー)
	}
	if (input->PushKey(DIK_RIGHT)) {
		// rotAcceleration.z = kRotAcceleration; // ★ 修正前 (ロール)
		rotAcceleration.y = kRotAcceleration; // ★ 修正後 (ヨー)
	}
	// ▲▲▲ 修正完了 ▲▲▲

	// 加速度を速度に反映
	rotationVelocity_ += rotAcceleration;

	// --- 2. 自動水平復元 (Rキーが押されている場合) ---
	if (input->PushKey(DIK_R)) {

		const float kRestoreAcceleration = 0.001f; // (ご提示の値)

		Matrix4x4 currentRotationMatrix = Quaternion::MakeMatrix(rotation_);
		Vector3 localX_Right = {currentRotationMatrix.m[0][0], currentRotationMatrix.m[0][1], currentRotationMatrix.m[0][2]};
		Vector3 localZ_Forward = {currentRotationMatrix.m[2][0], currentRotationMatrix.m[2][1], currentRotationMatrix.m[2][2]};

		// ▼▼▼ 暴走しないよう復元ロジックの符号を両方修正 ▼▼▼

		// 2. ロール（傾き）の復元
		// (localX.y が + なら左傾き -> 右ロール(Zプラス)方向に加速)
		// rotationVelocity_.z -= localX_Right.y * kRestoreAcceleration; // ★ 修正前 (暴走)
		rotationVelocity_.z += localX_Right.y * kRestoreAcceleration; // ★ 修正後 (z +=)

		// 3. ピッチ（上下）の復元
		// (localZ.y が + なら機首上げ -> 機首下げ(Xマイナス)方向に加速)
		// rotationVelocity_.x += localZ_Forward.y * kRestoreAcceleration; // ★ 修正前 (暴走)
		rotationVelocity_.x -= localZ_Forward.y * kRestoreAcceleration; // ★ 修正後 (x -=)

		// ▲▲▲ 修正完了 ▲▲▲
	}

	// --- 3. 摩擦を適用 ---
	rotationVelocity_ *= kRotFriction;

	// --- 4. 差分回転クォータニオンの生成 ---
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

	// --- 5. 回転の合成 ---
	rotation_ = deltaQuatRoll * deltaQuatPitch * deltaQuatYaw * rotation_;
	rotation_ = Quaternion::Normalize(rotation_);

	// --- 6. 回転行列の生成 ---
	Matrix4x4 rotationMatrix = Quaternion::MakeMatrix(rotation_);

	// --- 7. 移動処理 (★ 常に自動で前進) ---
	Vector3 move = {0.0f, 0.0f, 0.0f};
	move.z += kCameraSpeed;
	move = KamataEngine::MathUtility::TransformNormal(move, rotationMatrix);
	Vector3 currentPosition = worldtransfrom_.translation_;
	Vector3 newPosition = currentPosition + move;
	worldtransfrom_.matWorld_ = rotationMatrix;
	worldtransfrom_.matWorld_.m[3][0] = newPosition.x;
	worldtransfrom_.matWorld_.m[3][1] = newPosition.y;
	worldtransfrom_.matWorld_.m[3][2] = newPosition.z;
	worldtransfrom_.translation_ = newPosition;

	// --- 8. ビュー行列の更新 ---
	camera_.matView = KamataEngine::MathUtility::Inverse(worldtransfrom_.matWorld_);
	camera_.TransferMatrix();
}

void RailCamera::Reset() {
	// ★ 回転を初期クォータニオンに戻す
	Quaternion qPitch = Quaternion::MakeRotateAxisAngle({1.0f, 0.0f, 0.0f}, initialRotationEuler_.x);
	Quaternion qYaw = Quaternion::MakeRotateAxisAngle({0.0f, 1.0f, 0.0f}, initialRotationEuler_.y);
	rotation_ = qYaw * qPitch;
	rotation_ = KamataEngine::Quaternion::Normalize(rotation_);

	// ▼▼▼ 慣性（速度）もリセット ▼▼▼
	rotationVelocity_ = {0.0f, 0.0f, 0.0f};
	// ▲▲▲ 修正完了 ▲▲▲

	// 位置を initialPosition_ に戻す
	worldtransfrom_.matWorld_ = MakeIdentityMatrix();
	worldtransfrom_.matWorld_.m[3][0] = initialPosition_.x;
	worldtransfrom_.matWorld_.m[3][1] = initialPosition_.y; // (★ y に修正)
	worldtransfrom_.matWorld_.m[3][2] = initialPosition_.z;
	worldtransfrom_.translation_ = initialPosition_;

	// (ビュー行列の更新も忘れずに)
	camera_.matView = KamataEngine::MathUtility::Inverse(worldtransfrom_.matWorld_);
	camera_.TransferMatrix();
}

// RailCamera::MakeRotateAxisAngle は廃止

// MakeIdentityMatrix は Reset で使われているため残す
KamataEngine::Matrix4x4 RailCamera::MakeIdentityMatrix() {
	KamataEngine::Matrix4x4 result = {};
	result.m[0][0] = 1.0f;
	result.m[1][1] = 1.0f;
	result.m[2][2] = 1.0f;
	result.m[3][3] = 1.0f;
	return result;
}