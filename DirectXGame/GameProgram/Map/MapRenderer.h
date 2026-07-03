#pragma once

#include "KamataEngine.h"
#include "TileMap.h"
#include <memory>
#include <vector>

// 地面(blocks)・トゲ(needle)・ゴール(cube)タイルを描画する
class MapRenderer {
public:
	void Initialize(KamataEngine::Model* groundModel, KamataEngine::Model* spikeModel, KamataEngine::Model* goalModel, const TileMap& tileMap);
	void Draw(KamataEngine::Camera& camera);

private:
	void AddGroundTile(const TileMap& tileMap, int col, int row);
	void AddSpikeTile(const TileMap& tileMap, int col, int row);
	void AddGoalTile(const TileMap& tileMap, int col, int row);

	KamataEngine::Model* groundModel_ = nullptr;
	KamataEngine::Model* spikeModel_ = nullptr;
	KamataEngine::Model* goalModel_ = nullptr;
	std::vector<std::unique_ptr<KamataEngine::WorldTransform>> groundTransforms_;
	std::vector<std::unique_ptr<KamataEngine::WorldTransform>> spikeTransforms_;
	std::vector<std::unique_ptr<KamataEngine::WorldTransform>> goalTransforms_;
};
