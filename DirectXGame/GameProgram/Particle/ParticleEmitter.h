#pragma once
#include "3d/Camera.h"
#include "3d/Model.h"
#include "Particle.h"
#include <list>

class ParticleEmitter {
public:
	void Initialize(KamataEngine::Model* model);
	void Update();
	void Draw(const KamataEngine::Camera& camera);
	void Emit(const KamataEngine::Vector3& position, const KamataEngine::Vector3& velocity);
	void Clear();
	void EmitBurst(const KamataEngine::Vector3& position, int numParticles, float speed, float lifeTime, float startScale, float endScale);

private:
	void CreateParticle(const KamataEngine::Vector3& position, const KamataEngine::Vector3& velocity);
	void CreateExplosionParticle(const KamataEngine::Vector3& position, const KamataEngine::Vector3& velocity, float lifeTime, float startScale, float endScale);

	KamataEngine::Model* model_ = nullptr;
	std::list<Particle> particles_;
	// ヘッダ内で初期化
	int32_t frequency_ = 1;
	int32_t frequencyTimer_ = 0;
};