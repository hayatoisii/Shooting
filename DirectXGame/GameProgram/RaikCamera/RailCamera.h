#pragma once
#include "../../Quaternion.h" // (クォータニオンのヘッダパス)
#include "MT.h"
#include <3d/Camera.h>
#include <3d/WorldTransform.h>

class Player;

class RailCamera {

public:
	struct Rect {
		float left = 0.0f;
		float right = 1.0f;
		float bottom = 0.0f;
		float top = 1.0f;
	};

	void Initialize(const KamataEngine::Vector3& pos, const KamataEngine::Vector3& rad);
	void Update();

	Player* target_ = nullptr;

	void setTarget(Player* target) { target_ = target; }
	const Camera& GetViewProjection() { return camera_; }
	const WorldTransform& GetWorldTransform() { return worldtransfrom_; }

	// ▼▼▼ 慣性のために復活 ▼▼▼
	const KamataEngine::Vector3& GetRotationVelocity() const { return rotationVelocity_; }
	// ▲▲▲ 復活完了 ▲▲▲

	void Reset();

	// KamataEngine::Matrix4x4 MakeRotateAxisAngle(const KamataEngine::Vector3& axis, float angle); // ★ 廃止
	KamataEngine::Matrix4x4 MakeIdentityMatrix();

private:
	KamataEngine::WorldTransform worldtransfrom_;

	KamataEngine::Vector3 initialPosition_;
	KamataEngine::Vector3 initialRotationEuler_; // ★ リセット用に初期回転（オイラー角）を保持

	KamataEngine::Quaternion rotation_; // ★ 現在の回転をクォータニオンで保持

	// ▼▼▼ 慣性のために復活 ▼▼▼
	KamataEngine::Vector3 rotationVelocity_{0.0f, 0.0f, 0.0f}; // X:Pitch, Y:Yaw, Z:Roll の速度
	// ▲▲▲ 復活完了 ▲▲▲

	Camera camera_; // (元ファイルに無かったですが、Updateで使っているので追加)
};