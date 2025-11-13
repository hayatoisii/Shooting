#include "ParticleEmitter.h"
#include "MT.h"
#include <algorithm>

void ParticleEmitter::Initialize(KamataEngine::Model* model) {
	model_ = model;
	particles_.resize(100);
	frequency_ = 1; // 発生頻度
	frequencyTimer_ = 0;
}

void ParticleEmitter::Update() {
	//particles_.remove_if([](Particle& particle) { return !particle.isActive_; });

	for (Particle& particle : particles_) {
		if (particle.isActive_) {
			particle.currentTime_++;
			if (particle.currentTime_ >= particle.lifeTime_) {
				particle.isActive_ = false;
				continue;
			}

			particle.worldTransform_.translation_ += particle.velocity_;

			if (particle.isExplosion_) {
				// 1. (爆発用) だんだん小さくなる処理
				float t = (float)particle.currentTime_ / (float)particle.lifeTime_;
				t = std::clamp(t, 0.0f, 1.0f); // t を 0.0f～1.0f に制限
				float scale = particle.startScale_ + (particle.endScale_ - particle.startScale_) * t;
				particle.worldTransform_.scale_ = {scale, scale, scale};

			} else {
				// 2. (排気用) 既存のロジック (一切変更しない)
				float scale = 0.3f;
				particle.worldTransform_.scale_ = {scale, scale, scale};
			}

			// ▲▲▲ 分岐終了 ▲▲▲

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

			particle.lifeTime_ = 3 + MT::GetRand() % 3;
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

void ParticleEmitter::EmitBurst(const KamataEngine::Vector3& position, int numParticles, float speed, float lifeTime, float startScale, float endScale) {
	for (int i = 0; i < numParticles; ++i) {

		// (※ MT::GetRand() がなければ std::rand() に変更)
		KamataEngine::Vector3 velocity = {
		    (MT::GetRand() / (float)RAND_MAX * 2.0f - 1.0f), // -1.0f ～ 1.0f
		    (MT::GetRand() / (float)RAND_MAX * 2.0f - 1.0f), (MT::GetRand() / (float)RAND_MAX * 2.0f - 1.0f)};
		velocity = KamataEngine::MathUtility::Normalize(velocity);
		velocity = velocity * speed;

		CreateExplosionParticle(position, velocity, lifeTime, startScale, endScale);
	}
}


void ParticleEmitter::CreateExplosionParticle(const KamataEngine::Vector3& position, const KamataEngine::Vector3& velocity, float lifeTime, float startScale, float endScale) {

	// リスト内の非アクティブなパーティクルを探して再利用する
	for (Particle& particle : particles_) {
		if (!particle.isActive_) {
			particle.isActive_ = true;
			particle.worldTransform_.translation_ = position;
			particle.worldTransform_.Initialize();

			particle.velocity_ = velocity;
			particle.currentTime_ = 0;

			// (排気用とは違うロジック)
			particle.lifeTime_ = static_cast<uint32_t>(lifeTime);
			particle.startScale_ = startScale;
			particle.endScale_ = endScale;
			particle.isExplosion_ = true; // ★ 爆発フラグを立てる

			return; // 1つ生成したら終了
		}
	}
}