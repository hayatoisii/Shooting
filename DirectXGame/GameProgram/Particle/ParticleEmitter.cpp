#include "ParticleEmitter.h"
#include "MT.h"
#include <algorithm>
#include <cmath>
#include <KamataEngine.h>

namespace {
constexpr float kPi = 3.14159265f;
constexpr float kTwoPi = kPi * 2.0f;
constexpr float kPortalEffectScale = 1.2f;

float Rand01() { return MT::GetRand() / static_cast<float>(RAND_MAX); }
} // namespace

void ParticleEmitter::Initialize(KamataEngine::Model* model) {
	model_ = model;
	for (Particle& particle : particles_) {
		particle.worldTransform_.Initialize();
		particle.isActive_ = false;
		particle.isPortal_ = false;
	}
	frequency_ = 1;
	frequencyTimer_ = 0;
}

void ParticleEmitter::Update() {
	for (Particle& particle : particles_) {
		if (!particle.isActive_) {
			continue;
		}

		particle.currentTime_++;
		if (particle.currentTime_ >= particle.lifeTime_) {
			particle.isActive_ = false;
			particle.isPortal_ = false;
			continue;
		}

		particle.worldTransform_.translation_ += particle.velocity_;

		if (particle.isPortal_) {
			particle.worldTransform_.rotation_.z += (0.035f + Rand01() * 0.015f) * kPortalEffectScale;

			float t = static_cast<float>(particle.currentTime_) / static_cast<float>(particle.lifeTime_);
			t = std::clamp(t, 0.0f, 1.0f);
			// ふわっと大きくなってから縮む
			float scale = 0.0f;
			if (t < 0.35f) {
				const float growT = t / 0.35f;
				scale = particle.startScale_ + (particle.endScale_ - particle.startScale_) * growT;
			} else {
				const float shrinkT = (t - 0.35f) / 0.65f;
				scale = particle.endScale_ * (1.0f - shrinkT) + particle.startScale_ * 0.15f * shrinkT;
			}
			particle.worldTransform_.scale_ = {scale, scale, scale};
		} else if (particle.isExplosion_) {
			float t = static_cast<float>(particle.currentTime_) / static_cast<float>(particle.lifeTime_);
			t = std::clamp(t, 0.0f, 1.0f);
			const float scale = particle.startScale_ + (particle.endScale_ - particle.startScale_) * t;
			particle.worldTransform_.scale_ = {scale, scale, scale};
		} else {
			const float scale = 0.3f;
			particle.worldTransform_.scale_ = {scale, scale, scale};
		}

		particle.worldTransform_.UpdateMatrix();
	}
}

void ParticleEmitter::Draw(const KamataEngine::Camera& camera) {
	if (!model_) {
		return;
	}

	auto dx = KamataEngine::DirectXCommon::GetInstance();
	if (!dx) {
		return;
	}
	ID3D12GraphicsCommandList* cmdList = dx->GetCommandList();
	if (!cmdList) {
		return;
	}

	for (Particle& particle : particles_) {
		if (particle.isActive_) {
			model_->Draw(particle.worldTransform_, camera);
		}
	}
}

void ParticleEmitter::Emit(const KamataEngine::Vector3& position, const KamataEngine::Vector3& velocity) {
	frequencyTimer_++;
	if (frequencyTimer_ >= frequency_) {
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

			KamataEngine::Vector3 randomVelocity = {
			    (Rand01() - 0.8f) * 0.1f,
			    (Rand01() - 0.5f) * 0.1f,
			    (Rand01() - 0.5f) * 0.1f,
			};
			particle.velocity_ = velocity + randomVelocity;

			particle.lifeTime_ = 3 + MT::GetRand() % 3;
			particle.currentTime_ = 0;
			particle.isExplosion_ = false;
			particle.isPortal_ = false;
			particle.startScale_ = 1.0f;
			particle.endScale_ = 0.0f;
			particle.worldTransform_.rotation_ = {0.0f, 0.0f, 0.0f};
			particle.worldTransform_.scale_ = {0.3f, 0.3f, 0.3f};
			particle.worldTransform_.UpdateMatrix();
			return;
		}
	}
}

void ParticleEmitter::Clear() {
	for (Particle& particle : particles_) {
		particle.isActive_ = false;
		particle.isPortal_ = false;
	}
	frequencyTimer_ = 0;
}

void ParticleEmitter::EmitPortal(const KamataEngine::Vector3& position) {
	const int count = 4 + MT::GetRand() % 3;

	for (int i = 0; i < count; ++i) {
		const float angle = Rand01() * kTwoPi;
		const float speed = (0.18f + Rand01() * 0.55f) * kPortalEffectScale;
		const float radialOffset = Rand01() * 8.0f * kPortalEffectScale;
		const float downwardBias = 1.2f * kPortalEffectScale;

		KamataEngine::Vector3 velocity = {
		    std::cos(angle) * speed,
		    std::sin(angle) * speed + (0.10f + Rand01() * 0.26f) * kPortalEffectScale,
		    0.0f,
		};

		KamataEngine::Vector3 emitPos = {
		    position.x + std::cos(angle) * radialOffset * 0.45f,
		    position.y + std::sin(angle) * radialOffset * 0.42f - downwardBias,
		    1.8f,
		};

		const float startScale = (0.9f + Rand01() * 1.75f) * kPortalEffectScale;
		const float endScale = (1.75f + Rand01() * 2.25f) * kPortalEffectScale;
		const float lifeTime = (22.0f + Rand01() * 24.0f) * kPortalEffectScale;
		const float rotationZ = Rand01() * kTwoPi;

		CreatePortalParticle(emitPos, velocity, lifeTime, startScale, endScale, rotationZ);
	}
}

void ParticleEmitter::EmitBurst(const KamataEngine::Vector3& position, int numParticles, float speed, float lifeTime, float startScale, float endScale) {
	for (int i = 0; i < numParticles; ++i) {
		KamataEngine::Vector3 velocity = {
		    Rand01() * 2.0f - 1.0f,
		    Rand01() * 2.0f - 1.0f,
		    Rand01() * 2.0f - 1.0f,
		};
		velocity = KamataEngine::MathUtility::Normalize(velocity);
		velocity = velocity * speed;

		CreateExplosionParticle(position, velocity, lifeTime, startScale, endScale);
	}
}

void ParticleEmitter::CreateExplosionParticle(const KamataEngine::Vector3& position, const KamataEngine::Vector3& velocity, float lifeTime, float startScale, float endScale) {
	for (Particle& particle : particles_) {
		if (!particle.isActive_) {
			particle.isActive_ = true;
			particle.worldTransform_.translation_ = position;
			particle.velocity_ = velocity;
			particle.currentTime_ = 0;
			particle.lifeTime_ = static_cast<uint32_t>(std::fmax(1.0f, lifeTime));
			particle.startScale_ = startScale;
			particle.endScale_ = endScale;
			particle.isExplosion_ = true;
			particle.isPortal_ = false;
			particle.worldTransform_.rotation_ = {0.0f, 0.0f, 0.0f};
			particle.worldTransform_.scale_ = {startScale, startScale, startScale};
			particle.worldTransform_.UpdateMatrix();
			return;
		}
	}
}

void ParticleEmitter::CreatePortalParticle(const KamataEngine::Vector3& position, const KamataEngine::Vector3& velocity, float lifeTime, float startScale, float endScale, float rotationZ) {
	for (Particle& particle : particles_) {
		if (!particle.isActive_) {
			particle.isActive_ = true;
			particle.worldTransform_.translation_ = position;
			particle.velocity_ = velocity;
			particle.currentTime_ = 0;
			particle.lifeTime_ = static_cast<uint32_t>(std::fmax(1.0f, lifeTime));
			particle.startScale_ = startScale;
			particle.endScale_ = endScale;
			particle.isExplosion_ = true;
			particle.isPortal_ = true;
			particle.worldTransform_.rotation_ = {0.0f, 0.0f, rotationZ};
			particle.worldTransform_.scale_ = {startScale, startScale, startScale};
			particle.worldTransform_.UpdateMatrix();
			return;
		}
	}
}
