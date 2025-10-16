#include "Skydome.h"

void Skydome::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera) {
	worldtransfrom_.Initialize();
	model_ = model;
	camera_ = camera;
}

void Skydome::Update() {}

void Skydome::Draw() { model_->Draw(worldtransfrom_, *camera_); }