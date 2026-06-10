#pragma once
#include <3d/WorldTransform.h>
#include <3d/Model.h>
#include <3d/Camera.h>

class Skydome {
public:

	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera);
	void Update();
	void Draw();
	// ボールに追従: X と Z だけ動かす（Y は固定して地面が浮かないようにする）
	void SetPositionXZ(float x, float z) {
		worldtransfrom_.translation_.x = x;
		worldtransfrom_.translation_.z = z;
	}

private:

	KamataEngine::WorldTransform worldtransfrom_;
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;

};
