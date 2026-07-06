#pragma once

#include "KamataEngine.h"
#include "TileMap.h"
#include <memory>
#include <unordered_set>
#include <vector>

// 地面(blocks)・トゲ(needle)・ゴール(portal)・消える壁(deleteblocks)・ボタン(bulletEnemy)を描画する
class MapRenderer {
public:
	void Initialize(KamataEngine::Model* groundModel, KamataEngine::Model* spikeModel, KamataEngine::Model* goalModel,
	    KamataEngine::Model* disappearingWallModel, KamataEngine::Model* buttonModel, const TileMap& tileMap);
	void Draw(KamataEngine::Camera& camera);

	void DeactivateAllDisappearingWalls();
	void SetButtonPressed(int col, int row);
	void ApplyGimmickVisualsFromTileMap(const TileMap& tileMap);

private:
	struct TileDrawEntry {
		int col = 0;
		int row = 0;
		bool visible = true;
		std::unique_ptr<KamataEngine::WorldTransform> transform;
	};

	void AddGroundTile(const TileMap& tileMap, int col, int row);
	void AddSpikeTile(const TileMap& tileMap, int col, int row);
	void AddGoalTile(const TileMap& tileMap, int col, int row);
	void AddDisappearingWallTile(const TileMap& tileMap, int col, int row);
	void AddButtonTile(const TileMap& tileMap, int col, int row);
	static int EncodeTileKey(int col, int row);

	KamataEngine::Model* groundModel_ = nullptr;
	KamataEngine::Model* spikeModel_ = nullptr;
	KamataEngine::Model* goalModel_ = nullptr;
	KamataEngine::Model* disappearingWallModel_ = nullptr;
	KamataEngine::Model* buttonModel_ = nullptr;
	std::vector<std::unique_ptr<KamataEngine::WorldTransform>> groundTransforms_;
	std::vector<std::unique_ptr<KamataEngine::WorldTransform>> spikeTransforms_;
	std::vector<std::unique_ptr<KamataEngine::WorldTransform>> goalTransforms_;
	std::vector<TileDrawEntry> disappearingWallEntries_;
	std::vector<TileDrawEntry> buttonEntries_;
	std::unordered_set<int> pressedButtonKeys_;
};
