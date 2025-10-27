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

private:
	void CreateParticle(const KamataEngine::Vector3& position, const KamataEngine::Vector3& velocity);

	KamataEngine::Model* model_ = nullptr;
	std::list<Particle> particles_;
	// ヘッダ内で初期化
	int32_t frequency_ = 1;
	int32_t frequencyTimer_ = 0;
};