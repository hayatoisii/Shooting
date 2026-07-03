#include "MapRenderer.h"

#include <algorithm>

namespace {
// Blender出力に依存せず、タイル1マス(42px)に収まるようプログラム側で固定
constexpr float kGroundModelExtent = 8.271102f; // blocks: 4.135551 * 2
constexpr float kSpikeModelExtent = 6.673206f;  // needle: 3.336603 * 2（根元〜先端）
constexpr float kGoalModelExtent = 8.0f;

constexpr float kSpikeHeightScale = 0.5f;

KamataEngine::Vector3 MakeGoalScale(float tileW, float tileH, float modelExtent) {
	const float scaleX = tileW / modelExtent * TileMap::kGoalHitScaleX;
	const float scaleY = tileH / modelExtent * TileMap::kGoalHitScaleY;
	return {scaleX, scaleY, 1.0f};
}

KamataEngine::Vector3 MakeTileScale(float tileW, float tileH, float modelExtent) {
	const float uniform = (std::min)(tileW, tileH) / modelExtent;
	return {uniform, uniform, 1.0f};
}

KamataEngine::Vector3 MakeSpikeScale(float tileW, float tileH, float modelExtent) {
	const float scaleX = tileW / modelExtent;
	const float scaleY = tileH / modelExtent * kSpikeHeightScale;
	return {scaleX, scaleY, 1.0f};
}
} // namespace

void MapRenderer::AddGroundTile(const TileMap& tileMap, int col, int row) {
	const float tileW = tileMap.GetTileWidth();
	const float tileH = tileMap.GetTileHeight();

	auto wt = std::make_unique<KamataEngine::WorldTransform>();
	wt->Initialize();
	wt->translation_ = tileMap.TileCenterToWorld(col, row);
	wt->rotation_ = {0.0f, 0.0f, 0.0f};
	wt->scale_ = MakeTileScale(tileW, tileH, kGroundModelExtent);
	wt->UpdateMatrix();
	groundTransforms_.push_back(std::move(wt));
}

void MapRenderer::AddSpikeTile(const TileMap& tileMap, int col, int row) {
	const float tileW = tileMap.GetTileWidth();
	const float tileH = tileMap.GetTileHeight();

	auto wt = std::make_unique<KamataEngine::WorldTransform>();
	wt->Initialize();
	KamataEngine::Vector3 pos = tileMap.TileCenterToWorld(col, row);
	float anchorOffsetX = 0.0f;
	float anchorOffsetY = 0.0f;
	tileMap.GetSpikeAnchorOffset(col, row, anchorOffsetX, anchorOffsetY);
	pos.x += anchorOffsetX;
	pos.y += anchorOffsetY;
	wt->translation_ = pos;
	wt->rotation_ = {0.0f, 0.0f, tileMap.GetSpikeRotationZ(col, row)};
	wt->scale_ = MakeSpikeScale(tileW, tileH, kSpikeModelExtent);
	wt->UpdateMatrix();
	spikeTransforms_.push_back(std::move(wt));
}

void MapRenderer::AddGoalTile(const TileMap& tileMap, int col, int row) {
	const float tileW = tileMap.GetTileWidth();
	const float tileH = tileMap.GetTileHeight();

	auto wt = std::make_unique<KamataEngine::WorldTransform>();
	wt->Initialize();
	KamataEngine::Vector3 pos = tileMap.TileCenterToWorld(col, row);
	pos.y += TileMap::GetGoalModelRaiseOffsetY(tileH);
	wt->translation_ = pos;
	wt->rotation_ = {0.0f, 0.0f, 0.0f};
	wt->scale_ = MakeGoalScale(tileW, tileH, kGoalModelExtent);
	wt->UpdateMatrix();
	goalTransforms_.push_back(std::move(wt));
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
				AddGroundTile(tileMap, col, row);
			} else if (tileMap.IsSpike(col, row)) {
				AddSpikeTile(tileMap, col, row);
			} else if (tileMap.IsGoal(col, row)) {
				AddGoalTile(tileMap, col, row);
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
