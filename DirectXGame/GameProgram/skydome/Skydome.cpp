#include "Skydome.h"
#include "KamataEngine.h"

void Skydome::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera) {
	worldtransfrom_.Initialize();
	model_ = model;
	camera_ = camera;

	worldtransfrom_.scale_ = {1.5f, 1.5f, 1.5f};
}

void Skydome::Update() {
	// ▼▼▼ スカイドームがカメラに追従するようにする ▼▼▼
	if (camera_) {
		// 1. カメラのビュー行列 (camera_->matView) を取得
		// 2. ビュー行列の逆行列 (Inverse) を計算 = カメラのワールド行列
		KamataEngine::Matrix4x4 cameraWorldMatrix = KamataEngine::MathUtility::Inverse(camera_->matView);

		// 3. カメラのワールド行列から座標 (m[3][0], m[3][1], m[3][2]) を抽出
		KamataEngine::Vector3 cameraPosition = {cameraWorldMatrix.m[3][0], cameraWorldMatrix.m[3][1], cameraWorldMatrix.m[3][2]};

		// 4. スカイドームの座標 (translation_) をカメラの座標と一致させる
		worldtransfrom_.translation_ = cameraPosition;

		// 5. 座標とスケールをワールド行列に反映させる
		worldtransfrom_.UpdateMatrix();
	}
	// ▲▲▲ 修正完了 ▲▲▲
}

void Skydome::Draw() { model_->Draw(worldtransfrom_, *camera_); }