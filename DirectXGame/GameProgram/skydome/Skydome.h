#pragma once
#include <3d/WorldTransform.h>
#include <3d/Model.h>
#include <3d/Camera.h>
#include <cstdint>

// 正射影2D用の背景板（blocks キューブの +Z 面を全面表示）
class Skydome {
public:
	void Initialize(KamataEngine::Model* backdropModel, KamataEngine::Camera* camera);
	void Update();
	void SetViewBounds(float centerX, float centerY, float viewW, float viewH);
	void Draw();

private:
	void LoadSkyTexture();

	KamataEngine::WorldTransform worldtransfrom_;
	KamataEngine::Model* backdropModel_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;
	uint32_t skyTextureHandle_ = 0;
	bool hasSkyTexture_ = false;
};
