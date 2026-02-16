#include "Skydome.h"
#include "KamataEngine.h"

void Skydome::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera) {
	worldtransform_.Initialize();
	model_ = model;
	camera_ = camera;

	worldtransform_.scale_ = {8.0f, 8.0f, 8.0f};
}

void Skydome::Update() {
	// スカイドームがカメラに追従するようにする
	/*/
	if (camera_) {
		KamataEngine::Matrix4x4 cameraWorldMatrix = KamataEngine::MathUtility::Inverse(camera_->matView);
		KamataEngine::Vector3 cameraPosition = {cameraWorldMatrix.m[3][0], cameraWorldMatrix.m[3][1], cameraWorldMatrix.m[3][2]};

		// スカイドームの座標 (translation_) をカメラの座標と一致させる
		worldtransform_.translation_ = cameraPosition;
		worldtransform_.UpdateMatrix();
	}
	/*/
	worldtransform_.UpdateMatrix();
}

void Skydome::Draw() { model_->Draw(worldtransform_, *camera_); }