#include "Skydome.h"
#include "KamataEngine.h"

void Skydome::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera) {
	worldtransfrom_.Initialize();
	model_ = model;
	camera_ = camera;

	worldtransfrom_.scale_ = {4.0f, 4.0f, 4.0f};
}

void Skydome::Update() {
	// スカイドームがカメラに追従するようにする
	if (camera_) {
		KamataEngine::Matrix4x4 cameraWorldMatrix = KamataEngine::MathUtility::Inverse(camera_->matView);
		KamataEngine::Vector3 cameraPosition = {cameraWorldMatrix.m[3][0], cameraWorldMatrix.m[3][1], cameraWorldMatrix.m[3][2]};

		// スカイドームの座標 (translation_) をカメラの座標と一致させる
		worldtransfrom_.translation_ = cameraPosition;
		worldtransfrom_.UpdateMatrix();
	}
}

void Skydome::Draw() { model_->Draw(worldtransfrom_, *camera_); }