#include "Skydome.h"
#include "KamataEngine.h"

namespace {
// 正のZ＝奥（マップ z=0 より後ろ）
constexpr float kSkydomeDepthZ = 90.0f;
// blocks.obj など 8 単位キューブの一辺
constexpr float kBackdropModelExtent = 8.0f;
// 1.0 = 画面ぴったり / 10.0 = 約10倍（テクスチャが拡大表示される）
constexpr float kSkydomeSizeScale = 1.0f;
} // namespace

void Skydome::Initialize(KamataEngine::Model* backdropModel, KamataEngine::Camera* camera) {
	worldtransfrom_.Initialize();
	backdropModel_ = backdropModel;
	camera_ = camera;
	LoadSkyTexture();
}

void Skydome::LoadSkyTexture() {
	hasSkyTexture_ = false;
	skyTextureHandle_ = KamataEngine::TextureManager::Load("skydome/sky_sphere.png");
	hasSkyTexture_ = true;
}

void Skydome::Update() {
	worldtransfrom_.UpdateMatrix();
}

void Skydome::SetViewBounds(float centerX, float centerY, float viewW, float viewH) {
	const float coverW = viewW * 1.05f * kSkydomeSizeScale;
	const float coverH = viewH * 1.05f * kSkydomeSizeScale;

	// 球体ではなく +Z 面の板としてビュー全体を覆う（正射影向け）
	worldtransfrom_.translation_ = {centerX, centerY, kSkydomeDepthZ};
	worldtransfrom_.rotation_ = {0.0f, 0.0f, 0.0f};
	worldtransfrom_.scale_ = {coverW / kBackdropModelExtent, coverH / kBackdropModelExtent, 0.02f};
	worldtransfrom_.UpdateMatrix();
}

void Skydome::Draw() {
	if (!backdropModel_ || !camera_) {
		return;
	}

	if (hasSkyTexture_) {
		backdropModel_->Draw(worldtransfrom_, *camera_, skyTextureHandle_);
		return;
	}

	backdropModel_->Draw(worldtransfrom_, *camera_);
}
