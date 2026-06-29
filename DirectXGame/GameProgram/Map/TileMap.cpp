#include "TileMap.h"

#include "base/WinApp.h"

#include <algorithm>
#include <fstream>
#include <sstream>

bool TileMap::IsMultiScreenMap() const {
	return width_ > kScreenTilesW || height_ > kScreenTilesH;
}

float TileMap::GetViewportMarginX() const {
	return (static_cast<float>(KamataEngine::WinApp::kWindowWidth) - GetScreenWorldWidth()) * 0.5f;
}

float TileMap::GetViewportMarginY() const {
	return (static_cast<float>(KamataEngine::WinApp::kWindowHeight) - GetScreenWorldHeight()) * 0.5f;
}

bool TileMap::LoadFromFile(const char* filePath) {
	tiles_.clear();
	width_ = 0;
	height_ = 0;

	std::ifstream file(filePath);
	if (!file.is_open()) {
		return false;
	}

	std::string line;
	while (std::getline(file, line)) {
		if (line.empty()) {
			continue;
		}

		std::vector<int> row;
		std::istringstream stream(line);
		std::string cell;
		while (std::getline(stream, cell, ',')) {
			row.push_back(std::atoi(cell.c_str()));
		}

		if (!row.empty()) {
			if (width_ == 0) {
				width_ = static_cast<int>(row.size());
			}
			tiles_.push_back(row);
		}
	}

	height_ = static_cast<int>(tiles_.size());
	if (width_ <= 0 || height_ <= 0) {
		return false;
	}

	tileWidth_ = kTileWidth;
	tileHeight_ = kTileHeight;

	if (IsMultiScreenMap()) {
		offsetX_ = 0.0f;
		offsetY_ = 0.0f;
	} else {
		const float mapPixelW = GetMapPixelWidth();
		const float mapPixelH = GetMapPixelHeight();
		offsetX_ = (static_cast<float>(KamataEngine::WinApp::kWindowWidth) - mapPixelW) * 0.5f;
		offsetY_ = (static_cast<float>(KamataEngine::WinApp::kWindowHeight) - mapPixelH) * 0.5f;
	}
	return true;
}

int TileMap::GetScreenCountX() const {
	return (width_ + kScreenTilesW - 1) / kScreenTilesW;
}

int TileMap::GetScreenCountY() const {
	return (height_ + kScreenTilesH - 1) / kScreenTilesH;
}

int TileMap::GetTile(int col, int row) const {
	if (col < 0 || col >= width_ || row < 0 || row >= height_) {
		return 0;
	}
	return tiles_[row][col];
}

bool TileMap::IsGround(int col, int row) const { return GetTile(col, row) == 1; }

bool TileMap::IsSpike(int col, int row) const { return GetTile(col, row) == 2; }

bool TileMap::IsGoal(int col, int row) const { return GetTile(col, row) == 3; }

void TileMap::GetTileWorldRect(int col, int row, float& minX, float& minY, float& maxX, float& maxY) const {
	minX = offsetX_ + static_cast<float>(col) * tileWidth_;
	maxX = minX + tileWidth_;
	maxY = offsetY_ + static_cast<float>(height_ - row) * tileHeight_;
	minY = maxY - tileHeight_;
}

KamataEngine::Vector3 TileMap::TileCenterToWorld(int col, int row) const {
	float minX = 0.0f;
	float minY = 0.0f;
	float maxX = 0.0f;
	float maxY = 0.0f;
	GetTileWorldRect(col, row, minX, minY, maxX, maxY);

	KamataEngine::Vector3 pos;
	pos.x = (minX + maxX) * 0.5f;
	pos.y = (minY + maxY) * 0.5f;
	pos.z = 0.0f;
	return pos;
}

void TileMap::WorldToTile(float worldX, float worldY, int& col, int& row) const {
	const float localX = worldX - offsetX_;
	const float localY = worldY - offsetY_;
	col = static_cast<int>(localX / tileWidth_);
	row = height_ - 1 - static_cast<int>(localY / tileHeight_);
}

void TileMap::GetScreenFromWorld(float worldX, float worldY, int& screenX, int& screenY) const {
	const float localX = worldX - offsetX_ + GetViewportMarginX();
	const float localY = worldY - offsetY_ + GetViewportMarginY();
	const float screenW = GetScreenWorldWidth();
	const float screenH = GetScreenWorldHeight();

	screenX = static_cast<int>(localX / screenW);
	screenY = GetScreenCountY() - 1 - static_cast<int>(localY / screenH);

	screenX = std::clamp(screenX, 0, GetScreenCountX() - 1);
	screenY = std::clamp(screenY, 0, GetScreenCountY() - 1);
}

void TileMap::GetScreenViewportBounds(int screenX, int screenY, float& left, float& bottom, float& right, float& top) const {
	const float screenW = GetScreenWorldWidth();
	const float screenH = GetScreenWorldHeight();
	const float marginX = GetViewportMarginX();
	const float marginY = GetViewportMarginY();

	const float baseX = offsetX_ + static_cast<float>(screenX) * screenW;
	const float baseY = offsetY_ + static_cast<float>(GetScreenCountY() - 1 - screenY) * screenH;

	left = baseX - marginX;
	bottom = baseY - marginY;
	right = left + static_cast<float>(KamataEngine::WinApp::kWindowWidth);
	top = bottom + static_cast<float>(KamataEngine::WinApp::kWindowHeight);
}

KamataEngine::Vector3 TileMap::FindSpawnPosition(float halfWidth, float halfHeight) const {
	(void)halfWidth;

	const int startRow = 0;
	const int endRow = IsMultiScreenMap() ? kScreenTilesH - 1 : height_ - 1;
	const int startCol = 0;
	const int endCol = IsMultiScreenMap() ? kScreenTilesW - 1 : width_ - 1;

	for (int r = endRow; r >= startRow; --r) {
		for (int c = startCol; c <= endCol; ++c) {
			if (!IsGround(c, r)) {
				continue;
			}

			float minX = 0.0f;
			float minY = 0.0f;
			float maxX = 0.0f;
			float maxY = 0.0f;
			GetTileWorldRect(c, r, minX, minY, maxX, maxY);

			KamataEngine::Vector3 pos;
			pos.x = (minX + maxX) * 0.5f;
			pos.y = maxY + halfHeight;
			pos.z = 1.0f;
			return pos;
		}
	}

	for (int r = height_ - 1; r >= 0; --r) {
		for (int c = 0; c < width_; ++c) {
			if (!IsGround(c, r)) {
				continue;
			}

			float minX = 0.0f;
			float minY = 0.0f;
			float maxX = 0.0f;
			float maxY = 0.0f;
			GetTileWorldRect(c, r, minX, minY, maxX, maxY);

			KamataEngine::Vector3 pos;
			pos.x = (minX + maxX) * 0.5f;
			pos.y = maxY + halfHeight;
			pos.z = 1.0f;
			return pos;
		}
	}

	return {offsetX_ + tileWidth_ * 0.5f, offsetY_ + tileHeight_ + halfHeight, 1.0f};
}

void TileMap::GetMapWorldBounds(float& minX, float& minY, float& maxX, float& maxY) const {
	minX = offsetX_;
	minY = offsetY_;
	maxX = offsetX_ + GetMapPixelWidth();
	maxY = offsetY_ + GetMapPixelHeight();
}

void TileMap::ClampPositionToMapBounds(float& x, float& y, float halfWidth, float halfHeight) const {
	float minX = 0.0f;
	float minY = 0.0f;
	float maxX = 0.0f;
	float maxY = 0.0f;
	GetMapWorldBounds(minX, minY, maxX, maxY);

	x = std::clamp(x, minX + halfWidth, maxX - halfWidth);
	y = std::clamp(y, minY + halfHeight, maxY - halfHeight);
}

bool TileMap::OverlapsSpike(float worldX, float worldY, float halfWidth, float halfHeight) const {
	(void)halfWidth;
	(void)halfHeight;

	int col = 0;
	int row = 0;
	WorldToTile(worldX, worldY, col, row);
	return IsSpike(col, row);
}

bool TileMap::OverlapsGoal(float worldX, float worldY, float halfWidth, float halfHeight) const {
	const float playerMinX = worldX - halfWidth;
	const float playerMaxX = worldX + halfWidth;
	const float playerMinY = worldY - halfHeight;
	const float playerMaxY = worldY + halfHeight;

	for (int row = 0; row < height_; ++row) {
		for (int col = 0; col < width_; ++col) {
			if (!IsGoal(col, row)) {
				continue;
			}

			float tMinX = 0.0f;
			float tMinY = 0.0f;
			float tMaxX = 0.0f;
			float tMaxY = 0.0f;
			GetTileWorldRect(col, row, tMinX, tMinY, tMaxX, tMaxY);

			if (playerMaxX > tMinX && playerMinX < tMaxX && playerMaxY > tMinY && playerMinY < tMaxY) {
				return true;
			}
		}
	}

	return false;
}

void TileMap::ResolveCollisionX(float& x, float y, float halfWidth, float halfHeight) const {
	const float playerMinY = y - halfHeight;
	const float playerMaxY = y + halfHeight;

	for (int row = 0; row < height_; ++row) {
		for (int col = 0; col < width_; ++col) {
			if (!IsGround(col, row)) {
				continue;
			}

			float tMinX = 0.0f;
			float tMinY = 0.0f;
			float tMaxX = 0.0f;
			float tMaxY = 0.0f;
			GetTileWorldRect(col, row, tMinX, tMinY, tMaxX, tMaxY);

			const float playerMinX = x - halfWidth;
			const float playerMaxX = x + halfWidth;
			if (playerMaxX <= tMinX || playerMinX >= tMaxX || playerMaxY <= tMinY || playerMinY >= tMaxY) {
				continue;
			}

			const float overlapLeft = playerMaxX - tMinX;
			const float overlapRight = tMaxX - playerMinX;
			if (overlapLeft < overlapRight) {
				x -= overlapLeft;
			} else {
				x += overlapRight;
			}
		}
	}
}

void TileMap::ResolveCollisionY(float& y, float x, float halfWidth, float halfHeight, float velocityY, bool& onGround) const {
	onGround = false;
	const float playerMinX = x - halfWidth;
	const float playerMaxX = x + halfWidth;

	for (int row = 0; row < height_; ++row) {
		for (int col = 0; col < width_; ++col) {
			if (!IsGround(col, row)) {
				continue;
			}

			float tMinX = 0.0f;
			float tMinY = 0.0f;
			float tMaxX = 0.0f;
			float tMaxY = 0.0f;
			GetTileWorldRect(col, row, tMinX, tMinY, tMaxX, tMaxY);

			const float playerMinY = y - halfHeight;
			const float playerMaxY = y + halfHeight;
			if (playerMaxX <= tMinX || playerMinX >= tMaxX || playerMaxY <= tMinY || playerMinY >= tMaxY) {
				continue;
			}

			const float overlapBottom = playerMaxY - tMinY;
			const float overlapTop = tMaxY - playerMinY;

			if (velocityY > 0.0f) {
				// 上昇中は足元の地面では解決しない（ジャンプ直後に押し戻されるのを防ぐ）
				if (playerMinY >= tMaxY - 1.0f) {
					continue;
				}
				// 頭上の天井だけ解決
				if (overlapBottom < overlapTop) {
					y = tMinY - halfHeight;
				}
				continue;
			}

			if (overlapTop <= overlapBottom) {
				y = tMaxY + halfHeight;
				onGround = true;
			} else {
				y = tMaxY + halfHeight;
				onGround = true;
			}
		}
	}
}
