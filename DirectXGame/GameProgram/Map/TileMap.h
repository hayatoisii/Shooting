#pragma once

#include "KamataEngine.h"
#include <vector>

// CSVマップ（0=空白, 1=地面）。1画面=30×17タイル、複数画面を1枚のCSVに連結可能
class TileMap {
public:
	static constexpr float kTileWidth = 42.0f;
	static constexpr float kTileHeight = 42.0f;
	static constexpr int kScreenTilesW = 30;
	static constexpr int kScreenTilesH = 17;

	bool LoadFromFile(const char* filePath);

	int GetWidth() const { return width_; }
	int GetHeight() const { return height_; }
	int GetTile(int col, int row) const;
	bool IsGround(int col, int row) const;

	int GetScreenCountX() const;
	int GetScreenCountY() const;
	float GetScreenWorldWidth() const { return static_cast<float>(kScreenTilesW) * tileWidth_; }
	float GetScreenWorldHeight() const { return static_cast<float>(kScreenTilesH) * tileHeight_; }

	float GetTileWidth() const { return tileWidth_; }
	float GetTileHeight() const { return tileHeight_; }
	float GetOffsetX() const { return offsetX_; }
	float GetOffsetY() const { return offsetY_; }
	float GetMapPixelWidth() const { return static_cast<float>(width_) * tileWidth_; }
	float GetMapPixelHeight() const { return static_cast<float>(height_) * tileHeight_; }

	void GetTileWorldRect(int col, int row, float& minX, float& minY, float& maxX, float& maxY) const;
	KamataEngine::Vector3 TileCenterToWorld(int col, int row) const;
	void WorldToTile(float worldX, float worldY, int& col, int& row) const;

	void GetScreenFromWorld(float worldX, float worldY, int& screenX, int& screenY) const;
	void GetScreenViewportBounds(int screenX, int screenY, float& left, float& bottom, float& right, float& top) const;

	KamataEngine::Vector3 FindSpawnPosition(float halfWidth, float halfHeight) const;

	void ResolveCollisionX(float& x, float y, float halfWidth, float halfHeight) const;
	void ResolveCollisionY(float& y, float x, float halfWidth, float halfHeight, float velocityY, bool& onGround) const;
	void ClampPositionToMapBounds(float& x, float& y, float halfWidth, float halfHeight) const;
	void GetMapWorldBounds(float& minX, float& minY, float& maxX, float& maxY) const;

private:
	std::vector<std::vector<int>> tiles_;
	int width_ = 0;
	int height_ = 0;
	float tileWidth_ = kTileWidth;
	float tileHeight_ = kTileHeight;
	float offsetX_ = 0.0f;
	float offsetY_ = 0.0f;

	bool IsMultiScreenMap() const;
	float GetViewportMarginX() const;
	float GetViewportMarginY() const;
};
