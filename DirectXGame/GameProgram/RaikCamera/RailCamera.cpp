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

	const float kCameraSpeed = 0.5f; // ★ 自動前進する速度
	const float kRotSpeed = 0.02f;   // 回転速度

	// --- 1. 回転量の決定 ---

	float deltaYaw = 0.0f;   // ヨー (Y軸回転、首振り)
	float deltaPitch = 0.0f; // ピッチ (X軸回転、上下)
	float deltaRoll = 0.0f;  // ロール (Z軸回転、傾き)

	// ▼▼▼ "自動前進 + WASD/矢印で回転" の操作系 ▼▼▼

	// ピッチ (上下) -> W/Sキー
	if (input->PushKey(DIK_W)) {
		deltaPitch = -kRotSpeed; // 上昇
	}
	if (input->PushKey(DIK_S)) {
		deltaPitch = kRotSpeed; // 下降
	}

	// ヨー (左右の首振り) -> A/Dキー
	if (input->PushKey(DIK_A)) {
		deltaYaw = -kRotSpeed; // 左旋回
	}
	if (input->PushKey(DIK_D)) {
		deltaYaw = kRotSpeed; // 右旋回
	}

	// ロール (左右の傾き) -> 矢印キー LEFT/RIGHT
	if (input->PushKey(DIK_LEFT)) {
		deltaRoll = -kRotSpeed;
	}
	if (input->PushKey(DIK_RIGHT)) {
		deltaRoll = kRotSpeed;
	}

	// (矢印キー UP/DOWN はピッチと重複するため、ここでは使わない)

	// ▲▲▲ 操作系の変更完了 ▲▲▲

	// Playerが参照できるようにメンバ変数に保存
	lastDeltaYaw_ = deltaYaw;
	lastDeltaPitch_ = deltaPitch;
	// (必要なら lastDeltaRoll_ も .h に追加して保存)

	// --- 3. 差分回転クォータニオンの生成 ---

	// (現在の回転からローカル軸を取得)
	Matrix4x4 currentRotationMatrix = Quaternion::MakeMatrix(rotation_);
	Vector3 localXAxis = {currentRotationMatrix.m[0][0], currentRotationMatrix.m[0][1], currentRotationMatrix.m[0][2]};
	Vector3 localYAxis = {currentRotationMatrix.m[1][0], currentRotationMatrix.m[1][1], currentRotationMatrix.m[1][2]};
	Vector3 localZAxis = {currentRotationMatrix.m[2][0], currentRotationMatrix.m[2][1], currentRotationMatrix.m[2][2]};

	localXAxis = localXAxis.Normalize();
	localYAxis = localYAxis.Normalize();
	localZAxis = localZAxis.Normalize();

	// ヨー回転（左右首振り）: ローカルY軸周り (機体の上方向)
	Quaternion deltaQuatYaw = Quaternion::MakeRotateAxisAngle(localYAxis, deltaYaw);

	// ピッチ回転（上下）: ローカルX軸周り (機体の右方向)
	Quaternion deltaQuatPitch = Quaternion::MakeRotateAxisAngle(localXAxis, deltaPitch);

	// ロール回転（傾き）: ローカルZ軸周り (機体の前方向)
	Quaternion deltaQuatRoll = Quaternion::MakeRotateAxisAngle(localZAxis, deltaRoll);

	// --- 4. 回転の合成 ---
	rotation_ = deltaQuatRoll * deltaQuatPitch * deltaQuatYaw * rotation_;
	rotation_ = Quaternion::Normalize(rotation_);

	// --- 5. 回転行列の生成 ---
	Matrix4x4 rotationMatrix = Quaternion::MakeMatrix(rotation_);

	// --- 6. 移動処理 (★ 常に自動で前進) ---
	Vector3 move = {0.0f, 0.0f, 0.0f};

	// ★ 常に前進 (ローカルZ軸方向)
	move.z += kCameraSpeed;

	// (W/S/A/Dキーでの移動は行わない)

	// 回転行列に基づいて移動ベクトルを変換
	move = KamataEngine::MathUtility::TransformNormal(move, rotationMatrix);

	Vector3 currentPosition = worldtransfrom_.translation_;
	Vector3 newPosition = currentPosition + move;

	// ワールド行列を新しい回転と位置で再構築
	worldtransfrom_.matWorld_ = rotationMatrix;
	worldtransfrom_.matWorld_.m[3][0] = newPosition.x;
	worldtransfrom_.matWorld_.m[3][1] = newPosition.y;
	worldtransfrom_.matWorld_.m[3][2] = newPosition.z;

	worldtransfrom_.translation_ = newPosition;

	// --- 7. ビュー行列の更新 ---
	camera_.matView = KamataEngine::MathUtility::Inverse(worldtransfrom_.matWorld_);
	camera_.TransferMatrix();
}
void RailCamera::Reset() {
	// ★ 回転を初期クォータニオンに戻す
	Quaternion qPitch = Quaternion::MakeRotateAxisAngle({1.0f, 0.0f, 0.0f}, initialRotationEuler_.x);
	Quaternion qYaw = Quaternion::MakeRotateAxisAngle({0.0f, 1.0f, 0.0f}, initialRotationEuler_.y);
	rotation_ = qYaw * qPitch;
	rotation_ = KamataEngine::Quaternion::Normalize(rotation_);

	// 位置を initialPosition_ に戻す
	worldtransfrom_.matWorld_ = MakeIdentityMatrix(); // (MakeIdentityMatrix()は.hに残している前提)
	worldtransfrom_.matWorld_.m[3][0] = initialPosition_.x;
	worldtransfrom_.matWorld_.m[3][1] = initialPosition_.y;
	worldtransfrom_.matWorld_.m[3][2] = initialPosition_.z;
	worldtransfrom_.translation_ = initialPosition_;

	// (ビュー行列の更新も忘れずに)
	// ▼▼▼ 足りないもの(4): Inverse の呼び出し方 ▼▼▼
	// camera_.matView = Inverse(worldtransfrom_.matWorld_); // ★ 修正前 (エラー)
	camera_.matView = KamataEngine::MathUtility::Inverse(worldtransfrom_.matWorld_); // ★ 修正後
	// ▲▲▲ 修正完了 ▲▲▲
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