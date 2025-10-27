#include "RailCamera.h"
#include <KamataEngine.h> // KamataEngine ヘッダをインクルード
#include <algorithm>      // std::clamp のためにインクルード
#include <cmath>          // MathUtility::PI のためにインクルード

void RailCamera::Initialize(const KamataEngine::Vector3& pos, const KamataEngine::Vector3& rad) {
	// 初期位置を設定
	worldtransfrom_.translation_ = pos;
	// WorldTransform を初期化 (スケールと回転は単位行列、位置は pos)
	worldtransfrom_.Initialize();
	// カメラを初期化
	camera_.Initialize();

	// 初期角度を設定 (引数 rad を使う場合)
	totalPitch_ = rad.x; // 必要に応じてコメント解除
	totalYaw_ = rad.y;   // 必要に応じてコメント解除
}

void RailCamera::Update() {
	// キーボード入力インスタンスの取得
	KamataEngine::Input* input = KamataEngine::Input::GetInstance();

	// カメラの移動速度 (正の値)
	const float kCameraSpeed = 0.5f;
	// 回転加速度
	const float kRotAcceleration = 0.0005f;
	// 回転摩擦係数
	const float kRotFriction = 0.95f;

	// --- 1. 回転速度の更新 ---
	// キー入力で回転速度を変化させる
	if (input->PushKey(DIK_A)) {
		rotationVelocity_.y -= kRotAcceleration; // 左回転 (ヨー速度を減らす)
	} else if (input->PushKey(DIK_D)) {
		rotationVelocity_.y += kRotAcceleration; // 右回転 (ヨー速度を増やす)
	}
	if (input->PushKey(DIK_W)) {
		rotationVelocity_.x -= kRotAcceleration / 1.5f; // 上回転 (ピッチ速度を減らす)
	} else if (input->PushKey(DIK_S)) {
		rotationVelocity_.x += kRotAcceleration / 1.5f; // 下回転 (ピッチ速度を増やす)
	}

	// 摩擦による減速
	rotationVelocity_.x *= kRotFriction;
	rotationVelocity_.y *= kRotFriction;

	// --- 2. 累積角度の更新 ---
	totalYaw_ += rotationVelocity_.y;   // ヨー角を更新
	totalPitch_ += rotationVelocity_.x; // ピッチ角を更新

	// --- 3. ピッチ角の制限 ---
	// 真上 (約+89.4度) や真下 (約-89.4度) に行きすぎないように制限
	const float pitchLimit = KamataEngine::MathUtility::PI / 2.0f - 0.01f;
	totalPitch_ = std::clamp(totalPitch_, -pitchLimit, pitchLimit);

	// --- 4. 回転行列の作成 ---
	// 累積したヨー角とピッチ角から回転行列を作成
	// Y軸周りの回転 (ヨー)
	KamataEngine::Matrix4x4 matRotY = KamataEngine::MathUtility::MakeRotateYMatrix(totalYaw_);
	// X軸周りの回転 (ピッチ)
	KamataEngine::Matrix4x4 matRotX = KamataEngine::MathUtility::MakeRotateXMatrix(totalPitch_);
	// 回転行列を合成 (X回転を適用した後にY回転を適用)
	KamataEngine::Matrix4x4 rotationMatrix = matRotX * matRotY;

	// --- 5. 移動方向と移動量の計算 ---
	// 回転行列からカメラのローカル +Z軸 ベクトルを取得 (進行方向)
	KamataEngine::Vector3 lookDirection;
	lookDirection.x = rotationMatrix.m[2][0]; // ★ 符号を元に戻しました
	lookDirection.y = rotationMatrix.m[2][1]; // ★ 符号を元に戻しました
	lookDirection.z = rotationMatrix.m[2][2]; // ★ 符号を元に戻しました
	// lookDirection = KamataEngine::MathUtility::Normalize(lookDirection); // 必要であれば正規化

	// 移動量を計算 (前方ベクトル * 速度)
	KamataEngine::Vector3 move = lookDirection * kCameraSpeed;

	// --- 6. ワールド行列の更新 ---
	// 現在の位置に移動量を加算して新しい位置を計算
	KamataEngine::Vector3 currentPosition = worldtransfrom_.translation_;
	KamataEngine::Vector3 newPosition = currentPosition + move;

	// ワールド行列を新しい回転と位置で再構築 (スケールは{1,1,1}とする)
	worldtransfrom_.matWorld_ = rotationMatrix;        // まず回転行列をコピー
	worldtransfrom_.matWorld_.m[3][0] = newPosition.x; // 平行移動成分を設定
	worldtransfrom_.matWorld_.m[3][1] = newPosition.y;
	worldtransfrom_.matWorld_.m[3][2] = newPosition.z;

	// WorldTransform の translation_ メンバーも更新しておく
	worldtransfrom_.translation_ = newPosition;

	// --- 7. ビュー行列の更新 ---
	// ワールド行列の逆行列をビュー行列として設定
	camera_.matView = Inverse(worldtransfrom_.matWorld_);
	// カメラの行列をGPUに転送 (必要な場合)
	camera_.TransferMatrix();
}

void RailCamera::Reset() {
	// 位置はそのまま、回転の勢いと累積角度をリセット
	KamataEngine::Vector3 currentPosition = worldtransfrom_.translation_; // 位置を保持
	rotationVelocity_ = {0.0f, 0.0f, 0.0f};

	// 累積角度をリセット
	totalYaw_ = 0.0f;
	totalPitch_ = 0.0f;

	// ワールド行列も初期化 (単位行列 + 位置)
	//worldtransfrom_.matWorld_ = MakeIdentityMatrix();
	worldtransfrom_.matWorld_.m[3][0] = currentPosition.x;
	worldtransfrom_.matWorld_.m[3][1] = currentPosition.y;
	worldtransfrom_.matWorld_.m[3][2] = currentPosition.z;
	worldtransfrom_.translation_ = currentPosition; // translation_ も更新

	// ビュー行列も更新しておくのが安全
	camera_.matView = Inverse(worldtransfrom_.matWorld_);
	camera_.TransferMatrix();
}