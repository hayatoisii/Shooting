#include "GameplayRewind.h"

void GameplayRewindBuffer::Clear() {
	snapshots_.clear();
	timelineIndex_ = -1;
	recordFrameAccumulator_ = 0;
}

void GameplayRewindBuffer::ForceRecord(const GameplaySnapshot& snapshot) {
	recordFrameAccumulator_ = 0;

	if (timelineIndex_ >= 0 && timelineIndex_ < static_cast<int>(snapshots_.size()) - 1) {
		snapshots_.erase(snapshots_.begin() + timelineIndex_ + 1, snapshots_.end());
	}

	snapshots_.push_back(snapshot);
	if (static_cast<int>(snapshots_.size()) > kMaxSnapshots) {
		snapshots_.erase(snapshots_.begin());
	}

	timelineIndex_ = static_cast<int>(snapshots_.size()) - 1;
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
	if (static_cast<int>(snapshots_.size()) > kMaxSnapshots) {
		snapshots_.erase(snapshots_.begin());
	}

	timelineIndex_ = static_cast<int>(snapshots_.size()) - 1;
}

bool GameplayRewindBuffer::CanUndo() const { return timelineIndex_ > 0; }

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
