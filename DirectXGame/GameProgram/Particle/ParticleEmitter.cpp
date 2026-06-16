#include "ParticleEmitter.h"
#include "MT.h"
#include <algorithm>
#include <cstdint>
#include <KamataEngine.h>

void ParticleEmitter::Initialize(KamataEngine::Model* model, size_t maxParticles, float trailDrawAlpha) {
	model_ = model;
	trailDrawAlpha_ = trailDrawAlpha;
	particles_.clear();
	particles_.resize(maxParticles);
	trailDrawColor_.Initialize();
	trailDrawColor_.SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	frequency_ = 1;
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
				if (!particle.isTrail_) {
					float t = (float)particle.currentTime_ / (float)particle.lifeTime_;
					t = std::clamp(t, 0.0f, 1.0f);
					float scale = particle.startScale_ + (particle.endScale_ - particle.startScale_) * t;
					particle.worldTransform_.scale_ = {scale, scale, scale};
				}

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

void ParticleEmitter::UpdateTrailScale() {
	uint32_t minAge = UINT32_MAX;
	uint32_t maxAge = 0;
	int trailCount = 0;

	for (Particle& particle : particles_) {
		if (!particle.isActive_ || !particle.isTrail_) {
			continue;
		}
		++trailCount;
		if (particle.currentTime_ < minAge) {
			minAge = particle.currentTime_;
		}
		if (particle.currentTime_ > maxAge) {
			maxAge = particle.currentTime_;
		}
	}

	if (trailCount <= 0) {
		return;
	}

	const float ageSpan = static_cast<float>(maxAge - minAge);

	for (Particle& particle : particles_) {
		if (!particle.isActive_ || !particle.isTrail_) {
			continue;
		}

		const float baseCross = particle.startScale_;
		const float baseStretch = particle.endScale_;
		float scaleMul = 1.0f;

		if (trailCount > 1 && ageSpan > 0.5f) {
			float tailT = (static_cast<float>(particle.currentTime_) - static_cast<float>(minAge)) / ageSpan;
			tailT = std::clamp(tailT, 0.0f, 1.0f);
			if (tailT > 0.2f) {
				const float t = (tailT - 0.2f) / 0.8f;
				scaleMul = 1.0f - t * t * 0.48f;
			}
		}

		const float cross = baseCross * scaleMul;
		const float stretch = baseStretch * scaleMul;
		particle.worldTransform_.scale_ = {cross, cross, stretch};
		particle.worldTransform_.UpdateMatrix();
	}
}

void ParticleEmitter::Draw(const KamataEngine::Camera& camera) {
	// Guard: model_ must be valid and there must be particles
	if (!model_) {
		return;
	}

	// Ensure a valid command list is active (Model::PreDraw must have been called)
	auto dx = KamataEngine::DirectXCommon::GetInstance();
	if (!dx) return;
	ID3D12GraphicsCommandList* cmdList = dx->GetCommandList();
	if (!cmdList) {
		// No active command list -> cannot draw models now
		return;
	}

	for (Particle& particle : particles_) {
		if (particle.isActive_) {
			if (particle.isTrail_ && trailDrawAlpha_ < 0.99f) {
				trailDrawColor_.SetColor({1.0f, 1.0f, 1.0f, trailDrawAlpha_});
				model_->SetAlpha(trailDrawAlpha_);
				model_->Draw(particle.worldTransform_, camera, &trailDrawColor_);
			} else {
				model_->Draw(particle.worldTransform_, camera);
			}
		}
	}
	if (trailDrawAlpha_ < 0.99f) {
		model_->SetAlpha(1.0f);
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

			// Reuse safety: ensure this particle is treated as exhaust (not explosion)
			particle.isExplosion_ = false;
			particle.isTrail_ = false;
			particle.startScale_ = 1.0f;
			particle.endScale_ = 0.0f;

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

void ParticleEmitter::EmitTrailSegment(const KamataEngine::Vector3& center, const KamataEngine::Vector3& direction, float stretchLength, float lifeTime, float crossScale) {
	CreateTrailSegment(center, direction, stretchLength, lifeTime, crossScale);
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
			particle.lifeTime_ = static_cast<uint32_t>(std::fmax(1.0f, lifeTime));
			particle.startScale_ = startScale;
			particle.endScale_ = endScale;
			particle.isExplosion_ = true;
			particle.isTrail_ = false;

			return;
		}
	}
}

void ParticleEmitter::CreateTrailSegment(const KamataEngine::Vector3& center, const KamataEngine::Vector3& direction, float stretchLength, float lifeTime, float crossScale) {
	Particle* target = nullptr;
	for (Particle& particle : particles_) {
		if (!particle.isActive_) {
			target = &particle;
			break;
		}
	}
	if (!target) {
		uint32_t maxAge = 0;
		for (Particle& particle : particles_) {
			if (particle.isActive_ && particle.isTrail_ && particle.currentTime_ >= maxAge) {
				maxAge = particle.currentTime_;
				target = &particle;
			}
		}
	}
	if (!target) {
		return;
	}

	const float dirLen = std::sqrtf(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
	if (dirLen < 0.0001f) {
		return;
	}
	const float invDirLen = 1.0f / dirLen;
	const float dirX = direction.x * invDirLen;
	const float dirY = direction.y * invDirLen;
	const float dirZ = direction.z * invDirLen;

	Particle& particle = *target;
	particle.isActive_ = true;
	particle.worldTransform_.Initialize();
	particle.worldTransform_.translation_ = center;

	const float horiz = std::sqrtf(dirX * dirX + dirZ * dirZ);
	const float yaw = std::atan2f(dirX, dirZ);
	const float pitch = std::atan2f(dirY, horiz);
	particle.worldTransform_.rotation_ = {-pitch, yaw, 0.0f};

	const float stretch = std::fmax(crossScale * 0.75f, stretchLength);
	particle.worldTransform_.scale_ = {crossScale, crossScale, stretch};
	particle.worldTransform_.UpdateMatrix();

	particle.velocity_ = {0.0f, 0.0f, 0.0f};
	particle.currentTime_ = 0;
	particle.lifeTime_ = static_cast<uint32_t>(std::fmax(1.0f, lifeTime));
	particle.startScale_ = crossScale;
	particle.endScale_ = stretch;
	particle.isExplosion_ = true;
	particle.isTrail_ = true;
	particle.startAlpha_ = 1.0f;
	particle.endAlpha_ = 0.0f;
	particle.trailDepth_ = 0.0f;
	particle.color_ = {1.0f, 1.0f, 1.0f, 1.0f};
}