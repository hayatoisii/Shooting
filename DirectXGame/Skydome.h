#pragma once
#include <3d/WorldTransform.h>
#include <3d/Model.h>
#include <3d/Camera.h>

class Skydome {
public:

	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera);
	void Update();
	void Draw();

private:

	KamataEngine::WorldTransform worldtransfrom_;
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;

};
