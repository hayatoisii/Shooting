#include "GameplayRewind.h"

#include <algorithm>

void GameplayRewindBuffer::Clear() {
	snapshots_.clear();
	timelineIndex_ = -1;
	recordFrameAccumulator_ = 0;
}

void GameplayRewindBuffer::SetMaxRewindSeconds(float seconds) { maxRewindSeconds_ = seconds; }

void GameplayRewindBuffer::TrimOldSnapshots() {
	if (maxRewindSeconds_ < 0.0f) {
		return;
	}

	const int maxStored = static_cast<int>(maxRewindSeconds_ / kSecondsPerSnapshot) + 1;
	if (static_cast<int>(snapshots_.size()) <= maxStored) {
		return;
	}

	const int removeCount = static_cast<int>(snapshots_.size()) - maxStored;
	snapshots_.erase(snapshots_.begin(), snapshots_.begin() + removeCount);
	timelineIndex_ -= removeCount;
	if (timelineIndex_ < 0) {
		timelineIndex_ = 0;
	}
}

void GameplayRewindBuffer::ForceRecord(const GameplaySnapshot& snapshot) {
	recordFrameAccumulator_ = 0;

	if (timelineIndex_ >= 0 && timelineIndex_ < static_cast<int>(snapshots_.size()) - 1) {
		snapshots_.erase(snapshots_.begin() + timelineIndex_ + 1, snapshots_.end());
	}

	snapshots_.push_back(snapshot);

	timelineIndex_ = static_cast<int>(snapshots_.size()) - 1;
	TrimOldSnapshots();
}

void GameplayRewindBuffer::Record(const GameplaySnapshot& snapshot) {
	recordFrameAccumulator_++;
	if (recordFrameAccumulator_ < kSnapshotIntervalFrames) {
		return;
	}
	recordFrameAccumulator_ = 0;

	if (timelineIndex_ >= 0 && timelineIndex_ < static_cast<int>(snapshots_.size()) - 1) {
		snapshots_.erase(snapshots_.begin() + timelineIndex_ + 1, snapshots_.end());
	}

	snapshots_.push_back(snapshot);

	timelineIndex_ = static_cast<int>(snapshots_.size()) - 1;
	TrimOldSnapshots();
}

bool GameplayRewindBuffer::CanUndo() const { return timelineIndex_ > GetMinUndoTimelineIndex(); }

int GameplayRewindBuffer::GetMinUndoTimelineIndex() const {
	if (snapshots_.empty()) {
		return 0;
	}

	if (maxRewindSeconds_ < 0.0f) {
		return 0;
	}

	const int presentIndex = static_cast<int>(snapshots_.size()) - 1;
	const int maxUndoSteps = static_cast<int>(maxRewindSeconds_ / kSecondsPerSnapshot);
	return (std::max)(0, presentIndex - maxUndoSteps);
}

bool GameplayRewindBuffer::CanRedo() const {
	return timelineIndex_ >= 0 && timelineIndex_ < static_cast<int>(snapshots_.size()) - 1;
}

bool GameplayRewindBuffer::Undo(GameplaySnapshot& outSnapshot) {
	if (!CanUndo()) {
		return false;
	}
	timelineIndex_--;
	outSnapshot = snapshots_[static_cast<size_t>(timelineIndex_)];
	return true;
}

bool GameplayRewindBuffer::Redo(GameplaySnapshot& outSnapshot) {
	if (!CanRedo()) {
		return false;
	}
	timelineIndex_++;
	outSnapshot = snapshots_[static_cast<size_t>(timelineIndex_)];
	return true;
}

bool GameplayRewindBuffer::GetSnapshotAt(int index, GameplaySnapshot& outSnapshot) const {
	if (index < 0 || index >= static_cast<int>(snapshots_.size())) {
		return false;
	}
	outSnapshot = snapshots_[static_cast<size_t>(index)];
	return true;
}
