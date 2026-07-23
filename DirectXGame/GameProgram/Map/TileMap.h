#pragma once

#include "KamataEngine.h"
#include <unordered_set>
#include <vector>

struct TileMapGimmickSnapshot {
	std::unordered_set<int> deactivatedWalls;
	std::unordered_set<int> pressedButtons;
};

// CSVマップ（0=空白, 1=地面, 2=トゲ, 3=ゴール, 4=消える壁, 5=ボタン, 7=Player初期位置）
class TileMap {
public:
	static constexpr float kTileWidth = 42.0f;
	static constexpr float kTileHeight = 42.0f;
	static constexpr int kScreenTilesW = 36;
	static constexpr int kScreenTilesH = 20;
	// バネサイズ用（Player拡大前の基準半幅）
	static constexpr float kSpringReferenceHalfW = kTileWidth * 0.375f;
	static constexpr float kSpringReferenceHalfH = kTileHeight * 0.375f;
	static constexpr float kSpikeVerticalHitScale = 0.72f;
	static constexpr float kGoalHitScaleX = 1.3f;
	static constexpr float kGoalHitScaleY = 1.4f;
	// portal 見た目：下にはみ出す分を上にずらす（スケール後の高さに対する比率）
	static constexpr float kGoalModelRaiseRatio = 0.05f;
	// パーティクル基準点：モデル中心からわずかに下（上下とも隠す）
	static constexpr float kGoalParticleLowerRatio = 0.05f;
	static constexpr int kSpawnTile = 7; // Player初期位置マーカー（描画・当たり判定なし）
	// 7番チップが無いときのフォールバック（TileMap::FindSpawnPosition）
	static constexpr int kSpawnColumn = 0; // 列（0=左端）。負の値なら下から最初に見つかった地面
	static constexpr int kSpawnPlatformTiers = 2; // 足場の段数（下の段から数える）

	static float GetGoalModelRaiseOffsetY(float tileHeight);
	static float GetGoalParticleBaseOffsetY(float tileHeight);

	bool LoadFromFile(const char* filePath);

	int GetWidth() const { return width_; }
	int GetHeight() const { return height_; }
	int GetTile(int col, int row) const;
	bool IsGround(int col, int row) const;
	bool IsSpawnMarker(int col, int row) const;
	bool IsSpike(int col, int row) const;
	bool IsGoal(int col, int row) const;
	bool IsDisappearingWall(int col, int row) const;
	bool IsButton(int col, int row) const;
	bool IsSolidForCollision(int col, int row) const;
	bool IsDisappearingWallActive(int col, int row) const;
	bool IsButtonPressed(int col, int row) const;

	void ResetGimmickState();
	void DeactivateAllDisappearingWalls();
	bool PressButton(int col, int row);
	void CaptureGimmickSnapshot(TileMapGimmickSnapshot& outSnapshot) const;
	void ApplyGimmickSnapshot(const TileMapGimmickSnapshot& snapshot);

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
	bool OverlapsSpike(float worldX, float worldY, float halfWidth, float halfHeight) const;
	bool OverlapsGoal(float worldX, float worldY, float halfWidth, float halfHeight) const;
	bool FindOverlappingGoalCenter(float worldX, float worldY, float halfWidth, float halfHeight, KamataEngine::Vector3& outCenter) const;
	bool FindOverlappingUnpressedButton(float worldX, float worldY, float halfWidth, float halfHeight, int& outCol, int& outRow) const;

	float GetSpikeRotationZ(int col, int row) const;
	void GetSpikeAnchorOffset(int col, int row, float& offsetX, float& offsetY) const;
	void GetMapWorldBounds(float& minX, float& minY, float& maxX, float& maxY) const;

private:
	std::vector<std::vector<int>> tiles_;
	int width_ = 0;
	int height_ = 0;
	float tileWidth_ = kTileWidth;
	float tileHeight_ = kTileHeight;
	float offsetX_ = 0.0f;
	float offsetY_ = 0.0f;

	std::unordered_set<int> deactivatedWallKeys_;
	std::unordered_set<int> pressedButtonKeys_;

	static int EncodeTileKey(int col, int row);
	bool IsMultiScreenMap() const;
	float GetViewportMarginX() const;
	float GetViewportMarginY() const;
};
