#include "RailCamera.h"
#include <KamataEngine.h>
#include <algorithm> // Player.cpp と GaneScene.cpp で <algorithm> が使われているため、念のためインクルードします

void RailCamera::Initialize(const KamataEngine::Vector3& pos, const KamataEngine::Vector3& rad) {

	worldtransfrom_.translation_ = pos;
	worldtransfrom_.rotation_ = rad;

	worldtransfrom_.Initialize();
	camera_.Initialize();
}

void RailCamera::Update() {
	// キーボード入力インスタンスの取得
	KamataEngine::Input* input = KamataEngine::Input::GetInstance();

	// カメラの移動速度
	const float kCameraSpeed = -0.5f; // 常に進む速度
	//	const float kRotSpeed = 0.02f;         // 回転速度
	const float kRotAcceleration = 0.0005f; // 回転加速度
	const float kRotFriction = 0.95f;       // 回転摩擦係数

	// キー入力で回転速度を変化させる
	if (input->PushKey(DIK_A)) {
		rotationVelocity_.y -= kRotAcceleration / 1.0f; // 左回転
	} else if (input->PushKey(DIK_D)) {
		rotationVelocity_.y += kRotAcceleration / 1.0f; // 右回転
	}
	if (input->PushKey(DIK_W)) {
		rotationVelocity_.x -= kRotAcceleration / 1.5f; // 上
	} else if (input->PushKey(DIK_S)) {
		rotationVelocity_.x += kRotAcceleration / 1.5f; // 下
	}

	// 摩擦による減速
	rotationVelocity_.x *= kRotFriction;
	rotationVelocity_.y *= kRotFriction;

	// ▼▼▼ 修正点 1：回転の適用順序を変更 ▼▼▼
	// (カメラが横転するのを防ぎます)

	// 1. 左右回転(Y軸回転)を先に適用する
	KamataEngine::Matrix4x4 matRotY = KamataEngine::MathUtility::MakeRotateYMatrix(rotationVelocity_.y);
	worldtransfrom_.matWorld_ = matRotY * worldtransfrom_.matWorld_;

	// 2. 左右回転した後の、*新しい*「右」ベクトルを取得する
	KamataEngine::Vector3 right = {worldtransfrom_.matWorld_.m[0][0], worldtransfrom_.matWorld_.m[0][1], worldtransfrom_.matWorld_.m[0][2]};

	// 3. *新しい*「右」ベクトルを軸にして、上下回転(X軸回転)を適用する
	KamataEngine::Matrix4x4 matRotX = MakeRotateAxisAngle(right, rotationVelocity_.x);
	worldtransfrom_.matWorld_ = worldtransfrom_.matWorld_ * matRotX;

	// (↓ 問題があった古い回転方法)
	// worldtransfrom_.matWorld_ = matRotY * worldtransfrom_.matWorld_ * matRotX;

	// ▲▲▲ 修正点 1 ここまで ▲▲▲

	KamataEngine::Vector3 forward = {worldtransfrom_.matWorld_.m[2][0], worldtransfrom_.matWorld_.m[2][1], worldtransfrom_.matWorld_.m[2][2]};
	forward = KamataEngine::MathUtility::Normalize(forward) * -1.0f;

	KamataEngine::Vector3 move = {forward.x * kCameraSpeed, forward.y * kCameraSpeed, forward.z * kCameraSpeed};

	// 移動量を行列の平行移動成分に加算する
	worldtransfrom_.matWorld_.m[3][0] += move.x;
	worldtransfrom_.matWorld_.m[3][1] += move.y;
	worldtransfrom_.matWorld_.m[3][2] += move.z;

	worldtransfrom_.translation_ = {worldtransfrom_.matWorld_.m[3][0], worldtransfrom_.matWorld_.m[3][1], worldtransfrom_.matWorld_.m[3][2]};

	// ビュー行列を更新
	camera_.matView = Inverse(worldtransfrom_.matWorld_);
	camera_.TransferMatrix();
}

KamataEngine::Matrix4x4 RailCamera::MakeIdentityMatrix() {
	KamataEngine::Matrix4x4 result;
	result.m[0][0] = 1.0f;
	result.m[0][1] = 0.0f;
	result.m[0][2] = 0.0f;
	result.m[0][3] = 0.0f;
	result.m[1][0] = 0.0f;
	result.m[1][1] = 1.0f;
	result.m[1][2] = 0.0f;
	result.m[1][3] = 0.0f;
	result.m[2][0] = 0.0f;
	result.m[2][1] = 0.0f;
	result.m[2][2] = 1.0f;
	result.m[2][3] = 0.0f;
	result.m[3][0] = 0.0f;
	result.m[3][1] = 0.0f;
	result.m[3][2] = 0.0f;
	result.m[3][3] = 1.0f;
	return result;
}

// MakeRotateAxisAngleの実装 (RailCameraのメンバー関数として)
KamataEngine::Matrix4x4 RailCamera::MakeRotateAxisAngle(const KamataEngine::Vector3& axis, float angle) {
	KamataEngine::Matrix4x4 result = this->MakeIdentityMatrix();

	KamataEngine::Vector3 n = axis;

	// ▼▼▼ 修正点 2：Normalizeの戻り値を受け取る ▼▼▼
	// 2. Normalize関数が返した「正規化済みのベクトル」を `n` 自身に代入する
	n = KamataEngine::MathUtility::Normalize(n);
	// ▲▲▲ 修正点 2 ここまで ▲▲▲

	float cosTheta = std::cos(angle);
	float sinTheta = std::sin(angle);
	float oneMinusCos = 1.0f - cosTheta;

	result.m[0][0] = n.x * n.x * oneMinusCos + cosTheta;
	result.m[0][1] = n.x * n.y * oneMinusCos + n.z * sinTheta;
	result.m[0][2] = n.x * n.z * oneMinusCos - n.y * sinTheta;

	result.m[1][0] = n.x * n.y * oneMinusCos - n.z * sinTheta;
	result.m[1][1] = n.y * n.y * oneMinusCos + cosTheta;
	result.m[1][2] = n.y * n.z * oneMinusCos + n.x * sinTheta;

	result.m[2][0] = n.x * n.z * oneMinusCos + n.y * sinTheta;
	result.m[2][1] = n.y * n.z * oneMinusCos - n.x * sinTheta;
	result.m[2][2] = n.z * n.z * oneMinusCos + cosTheta;

	return result;
}

void RailCamera::Reset() {
	// --- 現在はこちらが有効：回転だけをリセットし、位置はそのまま ---

	// 1. 現在の位置を一時的な変数に保存する
	KamataEngine::Vector3 currentPosition = {worldtransfrom_.matWorld_.m[3][0], worldtransfrom_.matWorld_.m[3][1], worldtransfrom_.matWorld_.m[3][2]};
	// 2. 回転の勢いを完全に止める
	rotationVelocity_ = {0.0f, 0.0f, 0.0f};
	// 3. ワールド行列を単位行列にして、回転だけをリセットする
	worldtransfrom_.matWorld_ = MakeIdentityMatrix();
	// 4. 保存しておいた位置情報を、リセットしたワールド行列に書き戻す
	worldtransfrom_.matWorld_.m[3][0] = currentPosition.x;
	worldtransfrom_.matWorld_.m[3][1] = currentPosition.y;
	worldtransfrom_.matWorld_.m[3][2] = currentPosition.z;
	// 5. 整合性を保つため、translation_メンバーも更新しておく
	worldtransfrom_.translation_ = currentPosition;

	// ▼▼▼ 修正点 3：重複していたコードを削除 ▼▼▼
	// (以下の3行は上の処理と重複しているため不要)
	// 1. 回転の勢いを完全に止める
	// rotationVelocity_ = {0.0f, 0.0f, 0.0f};
	// 2. ワールド行列を完全に初期状態（単位行列）に戻す
	// worldtransfrom_.matWorld_ = MakeIdentityMatrix();
	// 3. WorldTransform内のtranslation_も念のため初期値に戻す
	// worldtransfrom_.translation_ = {0.0f, 0.0f, 0.0f};
	// ▲▲▲ 修正点 3 ここまで ▲▲▲
}