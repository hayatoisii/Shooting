#include "RailCamera.h"

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
	const float kRotAcceleration = 0.001f; // 回転加速度
	const float kRotFriction = 0.95f;      // 回転摩擦係数

	// キー入力で回転速度を変化させる
	if (input->PushKey(DIK_A)) {
		rotationVelocity_.y -= kRotAcceleration; // 左回転（yaw）
	} else if (input->PushKey(DIK_D)) {
		rotationVelocity_.y += kRotAcceleration; // 右回転（yaw）
	}
	if (input->PushKey(DIK_W)) {
		rotationVelocity_.x -= kRotAcceleration; // 上方向に回転（pitch）
	} else if (input->PushKey(DIK_S)) {
		rotationVelocity_.x += kRotAcceleration; // 下方向に回転（pitch）
	}

	// 摩擦による減速
	rotationVelocity_.x *= kRotFriction;
	rotationVelocity_.y *= kRotFriction;

	// 回転速度を加算
	worldtransfrom_.rotation_.x += rotationVelocity_.x;
	worldtransfrom_.rotation_.y += rotationVelocity_.y;

	// カメラの前方向を計算（z軸の負方向が前）
	KamataEngine::Vector3 forward = {
	    -sinf(worldtransfrom_.rotation_.y) * cosf(worldtransfrom_.rotation_.x), sinf(worldtransfrom_.rotation_.x), -cosf(worldtransfrom_.rotation_.y) * cosf(worldtransfrom_.rotation_.x)};

	// 前方向に速度を掛けて移動ベクトルを設定
	KamataEngine::Vector3 move = {forward.x * kCameraSpeed, forward.y * kCameraSpeed, forward.z * kCameraSpeed};

	// カメラの現在位置に移動ベクトルを加算
	worldtransfrom_.translation_.x += move.x;
	worldtransfrom_.translation_.y += move.y;
	worldtransfrom_.translation_.z += move.z;

	// ワールド行列を更新
	worldtransfrom_.UpdateMatrix();

	// ビュー行列を更新
	camera_.matView = Inverse(worldtransfrom_.matWorld_);
	camera_.TransferMatrix();
}
