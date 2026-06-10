#pragma once
#include <3d/Camera.h>
#include <3d/Model.h>
#include <3d/ObjectColor.h>
#include <3d/WorldTransform.h>
#include <memory>
#include <vector>

// 地面に置く池（複数の円を組み合わせた形状）。ボールが入るとゲームオーバー
class WaterPond {
public:
	// 1つの円構成要素（グループ中心からの XZ オフセット + 半径）
	struct PondPart {
		float offsetX = 0.0f;
		float offsetZ = 0.0f;
		float radius  = 5.0f;
	};

	void Initialize(KamataEngine::Model* model, const KamataEngine::Vector3& center, const std::vector<PondPart>& parts, int drawLayerIndex = 0);
	void Draw(const KamataEngine::Camera& camera);
	bool CheckBallFallIn(const KamataEngine::Vector3& ballPos, float ballRadius) const;
	KamataEngine::Vector3 GetCenter() const;
	const std::vector<PondPart>& GetParts() const { return parts_; }
	float GetBoundingRadius() const { return boundingRadius_; }
	float GetSurfaceY() const { return surfaceY_; }

	// ランダムな複合形状を生成（seed で再現可能、sizeMultiplier で大きさを変える）
	static std::vector<PondPart> GenerateRandomShape(int seed, float sizeMultiplier = 1.0f);

private:
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Vector3 center_ = {0.0f, 0.0f, 0.0f};
	std::vector<PondPart> parts_;
	std::unique_ptr<KamataEngine::WorldTransform> drawTransform_;
	KamataEngine::ObjectColor objectColor_;
	float surfaceY_ = 0.0f;
	float boundingRadius_ = 0.0f;
	float drawLayerOffsetY_ = 0.0f;
};
