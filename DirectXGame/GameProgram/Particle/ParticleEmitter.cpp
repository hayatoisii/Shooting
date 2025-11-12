#include "ParticleEmitter.h"
#include "MT.h"

void ParticleEmitter::Initialize(KamataEngine::Model* model) {
	model_ = model;
	particles_.resize(100);
	frequency_ = 1; // 発生頻度
	frequencyTimer_ = 0;
}

void ParticleEmitter::Update() {
	particles_.remove_if([](Particle& particle) { return !particle.isActive_; });

	for (Particle& particle : particles_) {
		if (particle.isActive_) {
			particle.currentTime_++;
			if (particle.currentTime_ >= particle.lifeTime_) {
				particle.isActive_ = false;
				continue;
			}

			particle.worldTransform_.translation_ += particle.velocity_;

			// 時間経過で小さくなって消える処理
			float scale = 0.3f;
			particle.worldTransform_.scale_ = {scale, scale, scale};

			particle.worldTransform_.UpdateMatrix();
		}
	}
}

void ParticleEmitter::Draw(const KamataEngine::Camera& camera) {
	for (Particle& particle : particles_) {
		if (particle.isActive_) {
			model_->Draw(particle.worldTransform_, camera);
		}
	}
}

void ParticleEmitter::Emit(const KamataEngine::Vector3& position, const KamataEngine::Vector3& velocity) {
	frequencyTimer_++;
	if (frequencyTimer_ >= frequency_) {

		// 一回の発生のパーティクル数
		const int particlesToEmit = 4;

		for (int i = 0; i < particlesToEmit; ++i) {
			CreateParticle(position, velocity);
		}

		frequencyTimer_ = 0;
	}
}
void ParticleEmitter::CreateParticle(const KamataEngine::Vector3& position, const KamataEngine::Vector3& velocity) {
	for (Particle& particle : particles_) {
		if (!particle.isActive_) {
			particle.isActive_ = true;
			particle.worldTransform_.translation_ = position;
			particle.worldTransform_.Initialize();

			// 少しだけランダムなばらつきを加える
			KamataEngine::Vector3 randomVelocity = {(MT::GetRand() / (float)RAND_MAX - 0.8f) * 0.1f, (MT::GetRand() / (float)RAND_MAX - 0.5f) * 0.1f, (MT::GetRand() / (float)RAND_MAX - 0.5f) * 0.1f};
			particle.velocity_ = velocity + randomVelocity;

			particle.lifeTime_ = static_cast<float>(3 + MT::GetRand() % 3);
			particle.currentTime_ = 0;
			return;
		}
	}
}

void ParticleEmitter::Clear() {
	for (Particle& particle : particles_) {
		particle.isActive_ = false;
	}
	frequencyTimer_ = 0;
}