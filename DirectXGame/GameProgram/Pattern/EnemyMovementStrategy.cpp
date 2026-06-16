#include "EnemyMovementStrategy.h"

#include <cmath>
#include <cstdlib>

EnemyMovementResult WanderEnemyMovementStrategy::Update(EnemyMovementState& state) {
	EnemyMovementResult result;

	float targetX = state.baseX + state.currentOffsetX;
	float targetZ = state.baseZ + state.currentOffsetZ;

	result.renderX = state.prevRenderedX + (targetX - state.prevRenderedX) * state.posSmoothFactor;
	result.renderZ = state.prevRenderedZ + (targetZ - state.prevRenderedZ) * state.posSmoothFactor;

	KamataEngine::Vector3 moveDir = {result.renderX - state.prevRenderedX, 0.0f, result.renderZ - state.prevRenderedZ};
	float lenMove = std::sqrt(moveDir.x * moveDir.x + moveDir.z * moveDir.z);
	if (lenMove > 0.0001f) {
		moveDir.x /= lenMove;
		moveDir.z /= lenMove;

		state.smoothedForward.x += (moveDir.x - state.smoothedForward.x) * state.facingSmoothFactor;
		state.smoothedForward.z += (moveDir.z - state.smoothedForward.z) * state.facingSmoothFactor;

		result.yaw = std::atan2(state.smoothedForward.x, state.smoothedForward.z);
	}

	state.prevRenderedX = result.renderX;
	state.prevRenderedZ = result.renderZ;

	KamataEngine::Vector3 forward = {state.smoothedForward.x, 0.0f, state.smoothedForward.z};
	KamataEngine::Vector3 wanderCenter = forward;
	{
		float lv = wanderCenter.x * wanderCenter.x + wanderCenter.z * wanderCenter.z;
		if (lv > 0.0001f) {
			float inv = 1.0f / std::sqrt(lv);
			wanderCenter.x *= inv;
			wanderCenter.z *= inv;
		}
	}
	wanderCenter.x *= state.wanderDistance;
	wanderCenter.z *= state.wanderDistance;
	state.wanderAngle += ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f) * state.wanderJitter;

	KamataEngine::Vector3 wanderPoint = {std::sin(state.wanderAngle) * state.wanderRadius, 0.0f, std::cos(state.wanderAngle) * state.wanderRadius};

	KamataEngine::Vector3 targetVelocity = {wanderCenter.x + wanderPoint.x, 0.0f, wanderCenter.z + wanderPoint.z};
	{
		float lv = targetVelocity.x * targetVelocity.x + targetVelocity.z * targetVelocity.z;
		if (lv > 0.0001f) {
			float inv = 1.0f / std::sqrt(lv);
			targetVelocity.x *= inv * state.desiredSpeed;
			targetVelocity.z *= inv * state.desiredSpeed;
		}
	}

	state.smoothedVelocity.x += (targetVelocity.x - state.smoothedVelocity.x) * state.turnSmoothFactor;
	state.smoothedVelocity.z += (targetVelocity.z - state.smoothedVelocity.z) * state.turnSmoothFactor;

	{
		float lv = state.smoothedVelocity.x * state.smoothedVelocity.x + state.smoothedVelocity.z * state.smoothedVelocity.z;
		if (lv > 0.0001f) {
			float inv = 1.0f / std::sqrt(lv);
			float vx = state.smoothedVelocity.x * inv;
			float vz = state.smoothedVelocity.z * inv;
			state.currentOffsetX += vx * state.desiredSpeed;
			state.currentOffsetZ += vz * state.desiredSpeed;
			state.smoothedForward.x += (vx - state.smoothedForward.x) * state.facingSmoothFactor;
			state.smoothedForward.z += (vz - state.smoothedForward.z) * state.facingSmoothFactor;
			result.yaw = std::atan2(state.smoothedForward.x, state.smoothedForward.z);
		}
	}

	const float kMaxOffsetRadius = 4000.0f;
	float distSq = state.currentOffsetX * state.currentOffsetX + state.currentOffsetZ * state.currentOffsetZ;
	if (distSq > kMaxOffsetRadius * kMaxOffsetRadius) {
		float r = std::sqrt(distSq);
		if (r > 0.0001f) {
			state.currentOffsetX = (state.currentOffsetX / r) * kMaxOffsetRadius;
			state.currentOffsetZ = (state.currentOffsetZ / r) * kMaxOffsetRadius;
		}
	}

	return result;
}
