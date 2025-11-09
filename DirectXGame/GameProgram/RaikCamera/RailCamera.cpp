#include "RailCamera.h"
#include "../../Quaternion.h" // (★ クォータニオンヘッダのパス)
#include <KamataEngine.h>
#include <algorithm>
#include <cmath>

// ▼▼▼ 足りないもの(1): 名前空間の指定 ▼▼▼
// (これがないと Quaternion や Vector3 などが見つからずエラーになります)
using namespace KamataEngine;
// ▲▲▲ 修正完了 ▲▲▲

void RailCamera::Initialize(const KamataEngine::Vector3& pos, const KamataEngine::Vector3& rad) {
	initialPosition_ = pos;
	initialRotationEuler_ = rad; // ★ 初期回転を保存

	worldtransfrom_.translation_ = pos;
	worldtransfrom_.Initialize();
	camera_.Initialize();

	// ★ 初期オイラー角からクォータニオンを生成
	Quaternion qPitch = Quaternion::MakeRotateAxisAngle({1.0f, 0.0f, 0.0f}, initialRotationEuler_.x);
	Quaternion qYaw = Quaternion::MakeRotateAxisAngle({0.0f, 1.0f, 0.0f}, initialRotationEuler_.y);
	// (ヨー -> ピッチ の順で合成)
	rotation_ = qYaw * qPitch;
	rotation_ = Quaternion::Normalize(rotation_);
}

void RailCamera::Update() {
	KamataEngine::Input* input = KamataEngine::Input::GetInstance();

	const float kCameraSpeed = 0.8f;

	// ▼▼▼ 慣性のために定数を復活 ▼▼▼
	const float kRotAcceleration = 0.0006f; // 回転加速度 const float kRotAcceleration = 0.0005f;
	const float kRotFriction = 0.95f;       // 回転摩擦 (1に近いほど慣性が残る)
	// ▲▲▲ 復活完了 ▲▲▲

	// --- 1. 回転「速度」の更新 (慣性モデル) ---

	Vector3 rotAcceleration = {0.0f, 0.0f, 0.0f};

	// (キー入力による加速度計算)
	if (input->PushKey(DIK_W)) {
		rotAcceleration.x = -kRotAcceleration;
	}
	if (input->PushKey(DIK_S)) {
		rotAcceleration.x = kRotAcceleration;
	}
	if (input->PushKey(DIK_A)) {
		rotAcceleration.y = -kRotAcceleration;
	}
	if (input->PushKey(DIK_D)) {
		rotAcceleration.y = kRotAcceleration;
	}
	if (input->PushKey(DIK_LEFT)) {
		rotAcceleration.z = -kRotAcceleration;
	}
	if (input->PushKey(DIK_RIGHT)) {
		rotAcceleration.z = kRotAcceleration;
	}

	// 加速度を速度に反映
	rotationVelocity_ += rotAcceleration;

	// キー入力があったかどうかをチェック
	//const float kInputThreshold = 0.0001f; // 入力検知のしきい値
	//bool isInputActive = (std::abs(rotAcceleration.x) > kInputThreshold || std::abs(rotAcceleration.y) > kInputThreshold || std::abs(rotAcceleration.z) > kInputThreshold);

	// ▼▼▼ 自動水平復元処理を追加 ▼▼▼

	// --- 2. 自動水平復元 (キー入力がない場合) ---
	if (input->PushKey(DIK_R)) {

		// ★ 水平に戻すための「復元力」を加える
		const float kRestoreAcceleration = 0.001f; // 水平に戻ろうとする加速度 (kRotAccelerationより少し弱め)

		// 1. 現在の回転行列からローカル軸(右/前)を取得
		Matrix4x4 currentRotationMatrix = Quaternion::MakeMatrix(rotation_);
		Vector3 localX_Right = {currentRotationMatrix.m[0][0], currentRotationMatrix.m[0][1], currentRotationMatrix.m[0][2]};
		Vector3 localZ_Forward = {currentRotationMatrix.m[2][0], currentRotationMatrix.m[2][1], currentRotationMatrix.m[2][2]};

		// 2. ロール（傾き）の復元
		// localX_Right.y は、機体の右(X)がワールドの上(Y)をどれだけ向いているかを示す
		// (これが 0 になるようにロール速度(Z)を加える)
		// (localX.y が + なら右が上 -> 左ロール(Zマイナス)方向に加速)
		rotationVelocity_.z -= localX_Right.y * kRestoreAcceleration;
		rotationVelocity_.x += localZ_Forward.y * kRestoreAcceleration;
	}
	// ▲▲▲ 修正完了 ▲▲▲

	// --- 3. 摩擦を適用 ---
	// (摩擦は復元力とキー入力の両方に適用される)
	rotationVelocity_ *= kRotFriction;

	// --- 4. 差分回転クォータニオンの生成 ---

	// (現在の回転からローカル軸を取得)
	// (★ 復元処理で Matrix を計算している可能性があるので、ここで改めて計算する)
	Matrix4x4 currentRotationMatrix = Quaternion::MakeMatrix(rotation_);
	Vector3 localXAxis = {currentRotationMatrix.m[0][0], currentRotationMatrix.m[0][1], currentRotationMatrix.m[0][2]};
	Vector3 localYAxis = {currentRotationMatrix.m[1][0], currentRotationMatrix.m[1][1], currentRotationMatrix.m[1][2]};
	Vector3 localZAxis = {currentRotationMatrix.m[2][0], currentRotationMatrix.m[2][1], currentRotationMatrix.m[2][2]};

	localXAxis = localXAxis.Normalize();
	localYAxis = localYAxis.Normalize();
	localZAxis = localZAxis.Normalize();

	// (慣性を持つ rotationVelocity_ から回転量を決定)
	float deltaPitch = rotationVelocity_.x;
	float deltaYaw = rotationVelocity_.y;
	float deltaRoll = rotationVelocity_.z;

	// (ヨー、ピッチ、ロールのクォータニオン生成)
	Quaternion deltaQuatYaw = Quaternion::MakeRotateAxisAngle(localYAxis, deltaYaw);
	Quaternion deltaQuatPitch = Quaternion::MakeRotateAxisAngle(localXAxis, deltaPitch);
	Quaternion deltaQuatRoll = Quaternion::MakeRotateAxisAngle(localZAxis, deltaRoll);

	// --- 5. 回転の合成 ---
	rotation_ = deltaQuatRoll * deltaQuatPitch * deltaQuatYaw * rotation_;
	rotation_ = Quaternion::Normalize(rotation_);

	// --- 6. 回転行列の生成 ---
	// (★ 5 で rotation_ が更新されたので、行列も更新する)
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
	// ... (中略) ...
	worldtransfrom_.translation_ = initialPosition_;

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