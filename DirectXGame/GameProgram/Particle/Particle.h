#pragma once
#include <KamataEngine.h>

struct Particle {
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Vector3 velocity_;             
	KamataEngine::Vector4 color_;                                  
	bool isActive_ = false;
	uint32_t lifeTime_ = 100;
	uint32_t currentTime_ = 0;

	float startScale_ = 1.0f;
	float endScale_ = 0.0f;
	bool isExplosion_ = false;
};