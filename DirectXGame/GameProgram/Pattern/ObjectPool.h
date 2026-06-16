#pragma once

#include <cassert>
#include <vector>

// Object Pool Pattern: 弾・エフェクト等の生成/破棄コストを削減する
template<typename T>
class ObjectPool {
public:
	ObjectPool() = default;

	T* Acquire() {
		if (!freeList_.empty()) {
			T* object = freeList_.back();
			freeList_.pop_back();
			return object;
		}
		return new T();
	}

	void Release(T* object) {
		if (object == nullptr) {
			return;
		}
		freeList_.push_back(object);
	}

	void Clear() {
		for (T* object : freeList_) {
			delete object;
		}
		freeList_.clear();
	}

	size_t GetFreeCount() const { return freeList_.size(); }

private:
	std::vector<T*> freeList_;
};
