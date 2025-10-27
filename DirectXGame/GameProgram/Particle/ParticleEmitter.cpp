#include "ParticleEmitter.h"
#include "MT.h"

void ParticleEmitter::Initialize(KamataEngine::Model* model) {
	model_ = model;
	particles_.resize(100);
	frequency_ = 1; // 少し発生頻度を上げてみましょう
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

			// 速度が0なので、この行は実質何もしないが、構造として残しておく
			particle.worldTransform_.translation_ += particle.velocity_;

			// 時間経過で小さくなって消える処理
			//float lifeRatio = particle.currentTime_ / particle.lifeTime_;
			float scale = 0.2f;
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

		// 一度に生成したいパーティクルの数 (この数値を増やすと、より煙が濃くなります)
		const int particlesToEmit = 10;

		// 指定した数だけ、CreateParticleを繰り返し呼び出す
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

			// ▼▼▼ 受け取った速度をパーティクルに設定 ▼▼▼
			// 少しだけランダムなばらつきを加える
			KamataEngine::Vector3 randomVelocity = {(MT::GetRand() / (float)RAND_MAX - 0.5f) * 0.1f, (MT::GetRand() / (float)RAND_MAX - 0.5f) * 0.1f, (MT::GetRand() / (float)RAND_MAX - 0.5f) * 0.1f};
			particle.velocity_ = velocity + randomVelocity;

			particle.lifeTime_ = static_cast<float>(5 + MT::GetRand() % 5); // 寿命を少し調整
			particle.currentTime_ = 0;
			return;
		}
	}
}

void ParticleEmitter::Clear() {
	// 現在アクティブな全てのパーティクルを非アクティブにする
	for (Particle& particle : particles_) {
		particle.isActive_ = false;
	}
	// frequencyTimer_もリセットしておくと、シーン切り替え直後に意図せず生成されるのを防げる
	frequencyTimer_ = 0;
}