#pragma once
#include <math/Vector3.h>

struct AABB {
	KamataEngine::Vector3 min;
	KamataEngine::Vector3 max;
};

bool IsCollisionAABB(const AABB& a, const AABB& b);