#pragma once

#include "KamataEngine.h"
#include "Player.h"
#include "TileMap.h"
#include "TrampolineSpring.h"
#include <vector>

struct TrampolineSpringSnapshot {
	TrampolineSpringType type = TrampolineSpringType::Up;
	KamataEngine::Vector3 center{};
};

struct GameplaySnapshot {
	PlayerSnapshot player;
	TileMapGimmickSnapshot gimmick;
	std::vector<TrampolineSpringSnapshot> trampolines;
	int currentScreenX = 0;
	int currentScreenY = 0;
	bool isFreeCamera = false;
	float freeCameraCenterX = 0.0f;
	float freeCameraCenterY = 0.0f;
	float cameraZoomOut = 0.0f;
	int score = 0;
	int hitCount = 0;
	int hitCount2 = 0;
	int nextTrampolineTypeIndex = 0;
};

class GameplayRewindBuffer {
public:
	static constexpr int kSnapshotIntervalFrames = 2;
	static constexpr int kMaxSnapshots = 300;
	static constexpr int kScrubStepsPerApply = 1;
	static constexpr int kScrubFramesPerStep = 3;
	static constexpr float kHistorySeconds = 10.0f;

	void Clear();
	void ForceRecord(const GameplaySnapshot& snapshot);
	void Record(const GameplaySnapshot& snapshot);
	bool CanUndo() const;
	bool CanRedo() const;
	bool Undo(GameplaySnapshot& outSnapshot);
	bool Redo(GameplaySnapshot& outSnapshot);
	int GetTimelineIndex() const { return timelineIndex_; }
	int GetSnapshotCount() const { return static_cast<int>(snapshots_.size()); }

private:
	std::vector<GameplaySnapshot> snapshots_;
	int timelineIndex_ = -1;
	int recordFrameAccumulator_ = 0;
};
