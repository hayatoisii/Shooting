#pragma once
#include "3d/Camera.h"
#include "3d/Model.h"
#include "Particle.h"
#include <cstddef>
#include <list>

class ParticleEmitter {
public:
	void Initialize(KamataEngine::Model* model, size_t maxParticles = 100, float trailDrawAlpha = 1.0f);
	void Update();
	void UpdateTrailScale();
	void Draw(const KamataEngine::Camera& camera);
	void Emit(const KamataEngine::Vector3& position, const KamataEngine::Vector3& velocity);
	void Clear();
	void EmitBurst(const KamataEngine::Vector3& position, int numParticles, float speed, float lifeTime, float startScale, float endScale);
	// 飛翔軌跡用: 移動方向に伸ばした1セグメント
	void EmitTrailSegment(const KamataEngine::Vector3& center, const KamataEngine::Vector3& direction, float stretchLength, float lifeTime, float crossScale);

private:
	void CreateParticle(const KamataEngine::Vector3& position, const KamataEngine::Vector3& velocity);
	void CreateExplosionParticle(const KamataEngine::Vector3& position, const KamataEngine::Vector3& velocity, float lifeTime, float startScale, float endScale);
	void CreateTrailSegment(const KamataEngine::Vector3& center, const KamataEngine::Vector3& direction, float stretchLength, float lifeTime, float crossScale);

	KamataEngine::Model* model_ = nullptr;
	KamataEngine::ObjectColor trailDrawColor_;
	float trailDrawAlpha_ = 1.0f;
	std::list<Particle> particles_;
	int32_t frequency_ = 1;
	int32_t frequencyTimer_ = 0;
};