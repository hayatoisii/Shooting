#include "RailCamera.h"
#include <KamataEngine.h>

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
	const float kCameraSpeed = -0.5f;      // 常に進む速度
	//	const float kRotSpeed = 0.02f;         // 回転速度
	const float kRotAcceleration = 0.0005f; // 回転加速度
	const float kRotFriction = 0.95f;      // 回転摩擦係数


	// キー入力で回転速度を変化させる
	if (input->PushKey(DIK_A)) {
		rotationVelocity_.y -= kRotAcceleration/1.5f; // 左回転（yaw）
	} else if (input->PushKey(DIK_D)) {
		rotationVelocity_.y += kRotAcceleration/1.5f; // 右回転（yaw）
	}
	if (input->PushKey(DIK_W)) {
		rotationVelocity_.x -= kRotAcceleration/2; // 上方向に回転（pitch）
	} else if (input->PushKey(DIK_S)) {
		rotationVelocity_.x += kRotAcceleration/2; // 下方向に回転（pitch）
	}

	// 摩擦による減速
	rotationVelocity_.x *= kRotFriction;
	rotationVelocity_.y *= kRotFriction;

	// --- ここから新しい回転処理 ---
	// オイラー角を更新する代わりに、ワールド行列を直接操作します。

	// 1. 回転速度から回転行列を生成する
	// ヨー（左右）回転はワールドのY軸周りで行う
	KamataEngine::Matrix4x4 matRotY = KamataEngine::MathUtility::MakeRotateYMatrix(rotationVelocity_.y);

	// ピッチ（上下）回転はカメラのローカルX軸（右方向ベクトル）周りで行う
	// 現在のワールド行列からローカルの右方向ベクトルを取得
	KamataEngine::Vector3 right = {worldtransfrom_.matWorld_.m[0][0], worldtransfrom_.matWorld_.m[0][1], worldtransfrom_.matWorld_.m[0][2]};
	KamataEngine::Matrix4x4 matRotX = MakeRotateAxisAngle(right, rotationVelocity_.x); // `this->` は省略可能

	// 2. 現在のワールド行列にヨー回転とピッチ回転を合成する
	// 注意：計算順序が重要です。 ヨー(ワールド) * 現在の向き * ピッチ(ローカル)
	worldtransfrom_.matWorld_ = matRotY * worldtransfrom_.matWorld_ * matRotX;

	// 3. 更新された行列から前方向ベクトルを計算する
	// 行列のZ軸が前方向を表す
	KamataEngine::Vector3 forward = {worldtransfrom_.matWorld_.m[2][0], worldtransfrom_.matWorld_.m[2][1], worldtransfrom_.matWorld_.m[2][2]};
	// カメラは通常-Z方向を向くため、ベクトルを反転させる
	forward = KamataEngine::MathUtility::Normalize(forward) * -1.0f;

	// 4. 新しい前方向ベクトルに沿ってカメラを移動させる
	KamataEngine::Vector3 move = {forward.x * kCameraSpeed, forward.y * kCameraSpeed, forward.z * kCameraSpeed};

	// 移動量を行列の平行移動成分に加算する
	worldtransfrom_.matWorld_.m[3][0] += move.x;
	worldtransfrom_.matWorld_.m[3][1] += move.y;
	worldtransfrom_.matWorld_.m[3][2] += move.z;

	// 5. worldTransform構造体のtranslation_も更新しておく（今回は使わないが、念のため）
	worldtransfrom_.translation_ = {worldtransfrom_.matWorld_.m[3][0], worldtransfrom_.matWorld_.m[3][1], worldtransfrom_.matWorld_.m[3][2]};

	// 行列を直接操作したので、worldtransfrom_.UpdateMatrix() を呼び出す必要はありません。
	// また、worldtransfrom_.rotation_ も更新しません。

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

	// ▼▼▼▼▼ ここからが修正箇所 ▼▼▼▼▼

	// 1. constで変更できない`axis`を、変更可能なローカル変数`n`にコピーする
	KamataEngine::Vector3 n = axis;

	// 2. コピーした変数`n`をNormalize関数に渡す（これなら変更できる）
	KamataEngine::MathUtility::Normalize(n);
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