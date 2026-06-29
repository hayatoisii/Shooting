#include "MapRenderer.h"



void MapRenderer::AddTile(const TileMap& tileMap, int col, int row, std::vector<std::unique_ptr<KamataEngine::WorldTransform>>& transforms) {

	const float tileW = tileMap.GetTileWidth();

	const float tileH = tileMap.GetTileHeight();

	const float kModelExtent = 8.0f;



	auto wt = std::make_unique<KamataEngine::WorldTransform>();

	wt->Initialize();

	wt->translation_ = tileMap.TileCenterToWorld(col, row);

	wt->scale_ = {tileW / kModelExtent, tileH / kModelExtent, 1.0f};

	wt->UpdateMatrix();

	transforms.push_back(std::move(wt));

}



void MapRenderer::Initialize(KamataEngine::Model* groundModel, KamataEngine::Model* spikeModel, KamataEngine::Model* goalModel, const TileMap& tileMap) {

	groundModel_ = groundModel;

	spikeModel_ = spikeModel;

	goalModel_ = goalModel ? goalModel : groundModel;

	groundTransforms_.clear();

	spikeTransforms_.clear();

	goalTransforms_.clear();



	for (int row = 0; row < tileMap.GetHeight(); ++row) {

		for (int col = 0; col < tileMap.GetWidth(); ++col) {

			if (tileMap.IsGround(col, row)) {

				AddTile(tileMap, col, row, groundTransforms_);

			} else if (tileMap.IsSpike(col, row)) {

				AddTile(tileMap, col, row, spikeTransforms_);

			} else if (tileMap.IsGoal(col, row)) {

				AddTile(tileMap, col, row, goalTransforms_);

			}

		}

	}

}



void MapRenderer::Draw(KamataEngine::Camera& camera) {

	if (groundModel_) {

		for (const std::unique_ptr<KamataEngine::WorldTransform>& wt : groundTransforms_) {

			groundModel_->Draw(*wt, camera);

		}

	}



	if (spikeModel_) {

		for (const std::unique_ptr<KamataEngine::WorldTransform>& wt : spikeTransforms_) {

			spikeModel_->Draw(*wt, camera);

		}

	}



	if (goalModel_) {

		for (const std::unique_ptr<KamataEngine::WorldTransform>& wt : goalTransforms_) {

			goalModel_->Draw(*wt, camera);

		}

	}

}
