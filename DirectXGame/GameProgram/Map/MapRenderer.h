#pragma once

#include "KamataEngine.h"
#include "TileMap.h"
#include <memory>
#include <vector>

// 地面タイル（cubeモデル）を描画する
class MapRenderer {
public:
	void Initialize(KamataEngine::Model* cubeModel, const TileMap& tileMap);
	void Draw(KamataEngine::Camera& camera);

private:
	KamataEngine::Model* model_ = nullptr;
	std::vector<std::unique_ptr<KamataEngine::WorldTransform>> groundTransforms_;
};
