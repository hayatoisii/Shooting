#include "Skydome.h"
#include "KamataEngine.h"

#include <3d/Mesh.h>

#include <algorithm>
#include <cfloat>

namespace {
// マップ(z=0)より奥。値を大きくするほど奥になる
constexpr float kWallDepthZ = 60.0f;
constexpr float kWallScaleX = 4.0f;
constexpr float kWallScaleY = 4.0f;
constexpr float kWallScaleZ = 1.0f;
} // namespace

void Skydome::Initialize(KamataEngine::Camera* camera) {
	wallWorldTransform_.Initialize();
	camera_ = camera;
	wallTextureHandle_ = KamataEngine::TextureManager::Load("wall2/backrock.png");
}

void Skydome::SetWallBackdrop(KamataEngine::Model* wallModel) {
	wallModel_ = wallModel;
	RecomputeWallModelBounds();
}

void Skydome::RecomputeWallModelBounds() {
	wallModelCenterX_ = 0.0f;
	wallModelCenterY_ = 0.0f;
	wallModelCenterZ_ = 0.0f;

	if (!wallModel_) {
		return;
	}

	float minX = FLT_MAX;
	float maxX = -FLT_MAX;
	float minY = FLT_MAX;
	float maxY = -FLT_MAX;
	float minZ = FLT_MAX;
	float maxZ = -FLT_MAX;
	bool hasVertex = false;

	for (const std::unique_ptr<KamataEngine::Mesh>& mesh : wallModel_->GetMeshes()) {
		for (const KamataEngine::Mesh::VertexPosNormalUv& vertex : mesh->GetVertices()) {
			hasVertex = true;
			minX = (std::min)(minX, vertex.pos.x);
			maxX = (std::max)(maxX, vertex.pos.x);
			minY = (std::min)(minY, vertex.pos.y);
			maxY = (std::max)(maxY, vertex.pos.y);
			minZ = (std::min)(minZ, vertex.pos.z);
			maxZ = (std::max)(maxZ, vertex.pos.z);
		}
	}

	if (!hasVertex) {
		return;
	}

	wallModelCenterX_ = (minX + maxX) * 0.5f;
	wallModelCenterY_ = (minY + maxY) * 0.5f;
	wallModelCenterZ_ = (minZ + maxZ) * 0.5f;
}

void Skydome::DrawAt(float centerX, float centerY, float viewScale) {
	if (!wallModel_) {
		return;
	}

	const float scaleX = kWallScaleX * viewScale;
	const float scaleY = kWallScaleY * viewScale;

	wallWorldTransform_.rotation_ = {0.0f, 0.0f, 0.0f};
	// Zを負にして表をカメラ側へ向ける（裏面カリング対策）
	wallWorldTransform_.scale_ = {scaleX, scaleY, -kWallScaleZ};
	wallWorldTransform_.translation_ = {
	    centerX - wallModelCenterX_ * scaleX,
	    centerY - wallModelCenterY_ * scaleY,
	    kWallDepthZ + wallModelCenterZ_ * kWallScaleZ,
	};
	wallWorldTransform_.UpdateMatrix();
}

void Skydome::Draw() {
	if (!wallModel_ || !camera_) {
		return;
	}

	if (wallTextureHandle_ != 0) {
		wallModel_->Draw(wallWorldTransform_, *camera_, wallTextureHandle_);
	} else {
		wallModel_->Draw(wallWorldTransform_, *camera_);
	}
}
