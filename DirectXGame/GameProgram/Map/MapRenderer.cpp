#include "MapRenderer.h"

#include <algorithm>

namespace {
constexpr float kGroundModelExtent = 8.271102f;
constexpr float kSpikeModelExtent = 6.673206f;
constexpr float kGoalModelExtent = 8.0f;
constexpr float kButtonModelExtent = 1.46492f; // key.obj の最大辺
constexpr float kSpikeHeightScale = 0.82f;
constexpr float kSpikeWidthScale = 1.05f;

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
	const float scaleX = tileW / modelExtent * kSpikeWidthScale;
	const float scaleY = tileH / modelExtent * kSpikeHeightScale;
	return {scaleX, scaleY, 1.0f};
}

KamataEngine::Vector3 MakeButtonScale(float tileW, float tileH, float modelExtent) {
	const float uniform = (std::min)(tileW, tileH) / modelExtent * 0.85f;
	return {uniform, uniform, 1.0f};
}
} // namespace

int MapRenderer::EncodeTileKey(int col, int row) { return col + row * 100000; }

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

void MapRenderer::AddDisappearingWallTile(const TileMap& tileMap, int col, int row) {
	const float tileW = tileMap.GetTileWidth();
	const float tileH = tileMap.GetTileHeight();

	TileDrawEntry entry;
	entry.col = col;
	entry.row = row;
	entry.visible = true;
	entry.transform = std::make_unique<KamataEngine::WorldTransform>();
	entry.transform->Initialize();
	entry.transform->translation_ = tileMap.TileCenterToWorld(col, row);
	entry.transform->rotation_ = {0.0f, 0.0f, 0.0f};
	entry.transform->scale_ = MakeTileScale(tileW, tileH, kGroundModelExtent);
	entry.transform->UpdateMatrix();
	entry.visible = tileMap.IsDisappearingWallActive(col, row);
	disappearingWallEntries_.push_back(std::move(entry));
}

void MapRenderer::AddButtonTile(const TileMap& tileMap, int col, int row) {
	const float tileW = tileMap.GetTileWidth();
	const float tileH = tileMap.GetTileHeight();

	TileDrawEntry entry;
	entry.col = col;
	entry.row = row;
	entry.visible = true;
	entry.transform = std::make_unique<KamataEngine::WorldTransform>();
	entry.transform->Initialize();
	entry.transform->translation_ = tileMap.TileCenterToWorld(col, row);
	entry.transform->rotation_ = {0.0f, 0.0f, 0.0f};
	entry.transform->scale_ = MakeButtonScale(tileW, tileH, kButtonModelExtent);
	entry.transform->UpdateMatrix();
	buttonEntries_.push_back(std::move(entry));
}

void MapRenderer::Initialize(KamataEngine::Model* groundModel, KamataEngine::Model* spikeModel, KamataEngine::Model* goalModel,
    KamataEngine::Model* disappearingWallModel, KamataEngine::Model* buttonModel, const TileMap& tileMap) {
	groundModel_ = groundModel;
	spikeModel_ = spikeModel;
	goalModel_ = goalModel ? goalModel : groundModel;
	disappearingWallModel_ = disappearingWallModel;
	buttonModel_ = buttonModel;

	groundTransforms_.clear();
	spikeTransforms_.clear();
	goalTransforms_.clear();
	disappearingWallEntries_.clear();
	buttonEntries_.clear();
	pressedButtonKeys_.clear();

	for (int row = 0; row < tileMap.GetHeight(); ++row) {
		for (int col = 0; col < tileMap.GetWidth(); ++col) {
			if (tileMap.GetTile(col, row) == 1) {
				AddGroundTile(tileMap, col, row);
			} else if (tileMap.IsSpike(col, row)) {
				AddSpikeTile(tileMap, col, row);
			} else if (tileMap.IsGoal(col, row)) {
				AddGoalTile(tileMap, col, row);
			} else if (tileMap.IsDisappearingWall(col, row)) {
				AddDisappearingWallTile(tileMap, col, row);
			} else if (tileMap.IsButton(col, row)) {
				AddButtonTile(tileMap, col, row);
				if (tileMap.IsButtonPressed(col, row)) {
					const TileDrawEntry& added = buttonEntries_.back();
					SetButtonPressed(added.col, added.row);
				}
			}
		}
	}
}

void MapRenderer::ApplyGimmickVisualsFromTileMap(const TileMap& tileMap) {
	const float tileW = tileMap.GetTileWidth();
	const float tileH = tileMap.GetTileHeight();

	for (TileDrawEntry& entry : disappearingWallEntries_) {
		entry.visible = tileMap.IsDisappearingWallActive(entry.col, entry.row);
	}

	pressedButtonKeys_.clear();
	for (TileDrawEntry& entry : buttonEntries_) {
		if (!entry.transform) {
			continue;
		}
		entry.transform->scale_ = MakeButtonScale(tileW, tileH, kButtonModelExtent);
		entry.transform->UpdateMatrix();
		if (tileMap.IsButtonPressed(entry.col, entry.row)) {
			SetButtonPressed(entry.col, entry.row);
		}
	}
}

void MapRenderer::DeactivateAllDisappearingWalls() {
	for (TileDrawEntry& entry : disappearingWallEntries_) {
		entry.visible = false;
	}
}

void MapRenderer::SetButtonPressed(int col, int row) {
	pressedButtonKeys_.insert(EncodeTileKey(col, row));
	for (TileDrawEntry& entry : buttonEntries_) {
		if (entry.col == col && entry.row == row && entry.transform) {
			entry.transform->scale_ *= 0.75f;
			entry.transform->UpdateMatrix();
			break;
		}
	}
}

void MapRenderer::Draw(KamataEngine::Camera& camera) {
	if (groundModel_) {
		for (const std::unique_ptr<KamataEngine::WorldTransform>& wt : groundTransforms_) {
			groundModel_->Draw(*wt, camera);
		}
	}

	if (disappearingWallModel_) {
		for (const TileDrawEntry& entry : disappearingWallEntries_) {
			if (entry.visible && entry.transform) {
				disappearingWallModel_->Draw(*entry.transform, camera);
			}
		}
	}

	if (spikeModel_) {
		for (const std::unique_ptr<KamataEngine::WorldTransform>& wt : spikeTransforms_) {
			spikeModel_->Draw(*wt, camera);
		}
	}

	if (buttonModel_) {
		for (const TileDrawEntry& entry : buttonEntries_) {
			if (entry.transform) {
				buttonModel_->Draw(*entry.transform, camera);
			}
		}
	}

	if (goalModel_) {
		for (const std::unique_ptr<KamataEngine::WorldTransform>& wt : goalTransforms_) {
			goalModel_->Draw(*wt, camera);
		}
	}
}
