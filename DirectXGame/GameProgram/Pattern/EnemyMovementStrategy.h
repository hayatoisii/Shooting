#pragma once

#include <KamataEngine.h>

// Strategy Pattern: 敵AIの移動行動を切り替える
struct EnemyMovementState {
	float baseX = 0.0f;
	float baseZ = 0.0f;
	float currentOffsetX = 0.0f;
	float currentOffsetZ = 0.0f;
	float prevRenderedX = 0.0f;
	float prevRenderedZ = 0.0f;

	KamataEngine::Vector3 smoothedForward = {0.0f, 0.0f, 1.0f};
	KamataEngine::Vector3 smoothedVelocity = {0.0f, 0.0f, 0.0f};

	float wanderAngle = 0.0f;
	float wanderJitter = 0.6f;
	float wanderRadius = 800.0f;
	float wanderDistance = 600.0f;
	float desiredSpeed = 2.0f;

	float posSmoothFactor = 0.18f;
	float facingSmoothFactor = 0.08f;
	float turnSmoothFactor = 0.06f;
};

struct EnemyMovementResult {
	float renderX = 0.0f;
	float renderZ = 0.0f;
	float yaw = 0.0f;
};

class IEnemyMovementStrategy {
public:
	virtual ~IEnemyMovementStrategy() = default;
	virtual EnemyMovementResult Update(EnemyMovementState& state) = 0;
};

class WanderEnemyMovementStrategy : public IEnemyMovementStrategy {
public:
	EnemyMovementResult Update(EnemyMovementState& state) override;
};
