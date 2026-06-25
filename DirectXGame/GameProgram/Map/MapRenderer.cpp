#include "MapRenderer.h"

void MapRenderer::Initialize(KamataEngine::Model* cubeModel, const TileMap& tileMap) {
	model_ = cubeModel;
	groundTransforms_.clear();

	const float tileW = tileMap.GetTileWidth();
	const float tileH = tileMap.GetTileHeight();
	const float kModelExtent = 8.0f;

	for (int row = 0; row < tileMap.GetHeight(); ++row) {
		for (int col = 0; col < tileMap.GetWidth(); ++col) {
			if (!tileMap.IsGround(col, row)) {
				continue;
			}

			auto wt = std::make_unique<KamataEngine::WorldTransform>();
			wt->Initialize();
			wt->translation_ = tileMap.TileCenterToWorld(col, row);
			wt->scale_ = {tileW / kModelExtent, tileH / kModelExtent, 1.0f};
			wt->UpdateMatrix();
			groundTransforms_.push_back(std::move(wt));
		}
	}
}

void MapRenderer::Draw(KamataEngine::Camera& camera) {
	if (!model_) {
		return;
	}

	for (const std::unique_ptr<KamataEngine::WorldTransform>& wt : groundTransforms_) {
		model_->Draw(*wt, camera);
	}
}
