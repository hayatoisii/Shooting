#pragma once
#include <3d/WorldTransform.h>
#include <3d/Model.h>
#include <3d/Camera.h>

class Skydome {
public:
	void Initialize(KamataEngine::Camera* camera);
	void SetWallBackdrop(KamataEngine::Model* wallModel);
	void DrawAt(float centerX, float centerY, float viewScale = 1.0f);
	void Draw();

private:
	void RecomputeWallModelBounds();

	KamataEngine::WorldTransform wallWorldTransform_;
	KamataEngine::Model* wallModel_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;
	uint32_t wallTextureHandle_ = 0;
	float wallModelCenterX_ = 0.0f;
	float wallModelCenterY_ = 0.0f;
	float wallModelCenterZ_ = 0.0f;
};
