#pragma once
#include <KamataEngine.h>

struct Particle {
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Vector3 velocity_;             
	KamataEngine::Vector4 color_;                
	float lifeTime_;
	float currentTime_;                          
	bool isActive_ = false;                      
};