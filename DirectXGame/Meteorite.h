#pragma once
#include "3d/Model.h"
#include "3d/WorldTransform.h"
#include <3d/Camera.h>
#include <KamataEngine.h>

class Meteorite {
public:
	Meteorite() = default;
	~Meteorite() = default;

	void Initialize(KamataEngine::Model* model, const KamataEngine::Vector3& pos, float baseScale, float radius); /// @brief 更新処理
	void Update(const KamataEngine::Vector3& playerPos);

	void Draw(const KamataEngine::Camera& camera);

	void OnCollision();

	float GetRadius() const { return radius_; }

	bool IsDead() const { return isDead_; }

	KamataEngine::Vector3 GetWorldPosition() const;

private:
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::WorldTransform worldtransfrom_;
	KamataEngine::Vector3 velocity_ = {0.0f, 0.0f, 0.0f};
	float radius_ = 1.0f;
	float baseScale_ = 1.0f;
	bool isDead_ = false;
};