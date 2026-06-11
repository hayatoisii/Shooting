#include "WaterPond.h"
#include <cmath>
#include <cstdlib>

namespace {
// ike.obj の XZ 直径（単位スケール時）。cube は 1.0 だったためスケール補正が必要
constexpr float kPondModelDiameter = 2.85285f;
// 地面との Z ファイト防止（描画のみ少し浮かせる）
constexpr float kPondDrawFloatY = 1.2f;

float Rand01(int& seed) {
	seed = seed * 1103515245 + 12345;
	return static_cast<float>((seed / 65536) % 32768) / 32768.0f;
}

void AddCircle(std::vector<WaterPond::PondPart>& parts, float ox, float oz, float r) {
	parts.push_back({ox, oz, r});
}

// attachIdx の円周上に、外接（端と端が触れる）で新しい円を追加
void AddTangentCircle(std::vector<WaterPond::PondPart>& parts, size_t attachIdx, float attachAngle, float newRadius) {
	const WaterPond::PondPart& parent = parts[attachIdx];
	const float dist = (parent.radius + newRadius) * 0.98f;
	const float ox = parent.offsetX + std::cosf(attachAngle) * dist;
	const float oz = parent.offsetZ + std::sinf(attachAngle) * dist;
	AddCircle(parts, ox, oz, newRadius);
}

// チェーン状: 円を次々外接でつなぐ
void BuildTangentChain(std::vector<WaterPond::PondPart>& parts, int& s, float baseR, int count) {
	const float r0 = baseR * (0.85f + Rand01(s) * 0.3f);
	AddCircle(parts, 0.0f, 0.0f, r0);
	float heading = Rand01(s) * 6.2831853f;
	for (int i = 1; i < count; ++i) {
		const float ri = baseR * (0.55f + Rand01(s) * 0.55f);
		heading += (Rand01(s) - 0.5f) * 1.2f;
		AddTangentCircle(parts, parts.size() - 1, heading, ri);
	}
}

// 幹から枝分かれ（各枝は幹または既存円に外接）
void BuildBranchCluster(std::vector<WaterPond::PondPart>& parts, int& s, float baseR) {
	const float rCore = baseR * (0.9f + Rand01(s) * 0.35f);
	AddCircle(parts, 0.0f, 0.0f, rCore);
	const int branchCount = 2 + static_cast<int>(Rand01(s) * 4.0f) % 4;
	for (int i = 0; i < branchCount; ++i) {
		const size_t attach = static_cast<size_t>(Rand01(s) * static_cast<float>(parts.size())) % parts.size();
		const float ang = Rand01(s) * 6.2831853f;
		const float rr = baseR * (0.35f + Rand01(s) * 0.55f);
		AddTangentCircle(parts, attach, ang, rr);
	}
}

// ランダム外接成長（有機的な blob）
void BuildOrganicBlob(std::vector<WaterPond::PondPart>& parts, int& s, float baseR) {
	const float r0 = baseR * (0.75f + Rand01(s) * 0.45f);
	AddCircle(parts, 0.0f, 0.0f, r0);
	const int total = 3 + static_cast<int>(Rand01(s) * 5.0f) % 6;
	for (int i = 1; i < total; ++i) {
		const size_t attach = static_cast<size_t>(Rand01(s) * static_cast<float>(parts.size())) % parts.size();
		const float ang = Rand01(s) * 6.2831853f;
		const float rr = baseR * (0.30f + Rand01(s) * 0.65f);
		AddTangentCircle(parts, attach, ang, rr);
	}
}
} // namespace

std::vector<WaterPond::PondPart> WaterPond::GenerateRandomShape(int seed, float sizeMultiplier) {
	std::vector<PondPart> parts;
	int s = seed;
	if (sizeMultiplier < 0.2f) {
		sizeMultiplier = 0.2f;
	}

	const float baseR = (6.0f + Rand01(s) * 5.0f) * sizeMultiplier;
	const int shapeType = static_cast<int>(Rand01(s) * 8.0f) % 8;

	switch (shapeType) {
	case 0:
		AddCircle(parts, 0.0f, 0.0f, baseR * (0.85f + Rand01(s) * 0.3f));
		break;
	case 1: {
		// 2円: 端と端が接する（横並び）
		const float rA = baseR * (0.65f + Rand01(s) * 0.35f);
		const float rB = baseR * (0.55f + Rand01(s) * 0.45f);
		const float sep = rA + rB;
		AddCircle(parts, -sep * 0.5f, 0.0f, rA);
		AddCircle(parts, sep * 0.5f, 0.0f, rB);
		break;
	}
	case 2: {
		// 3円: 大きい1つ + 上に2つ外接（ミッキー風）
		const float rBig = baseR;
		const float rEar = baseR * (0.38f + Rand01(s) * 0.22f);
		AddCircle(parts, 0.0f, 0.0f, rBig);
		AddTangentCircle(parts, 0, 1.5707963f + (Rand01(s) - 0.5f) * 0.5f, rEar);
		AddTangentCircle(parts, 0, 1.5707963f + 0.85f + (Rand01(s) - 0.5f) * 0.4f, rEar * (0.85f + Rand01(s) * 0.3f));
		break;
	}
	case 3: {
		// 3円チェーン（曲がった川）
		BuildTangentChain(parts, s, baseR, 3);
		break;
	}
	case 4: {
		// 5円チェーン（長い湾曲）
		BuildTangentChain(parts, s, baseR, 5);
		break;
	}
	case 5:
		BuildBranchCluster(parts, s, baseR);
		break;
	case 6:
		BuildOrganicBlob(parts, s, baseR);
		break;
	default: {
		// 8の字風: 2大円が接し、中央に小円を挟む
		const float rL = baseR * (0.70f + Rand01(s) * 0.25f);
		const float rR = baseR * (0.65f + Rand01(s) * 0.30f);
		const float rMid = baseR * (0.28f + Rand01(s) * 0.18f);
		const float sep = rL + rR;
		AddCircle(parts, -sep * 0.5f, 0.0f, rL);
		AddCircle(parts, sep * 0.5f, 0.0f, rR);
		AddTangentCircle(parts, 0, 0.0f, rMid);
		AddTangentCircle(parts, 1, 3.14159265f, rMid * (0.75f + Rand01(s) * 0.35f));
		break;
	}
	}

	// 全体をランダム回転して向きをバラす
	const float rot = Rand01(s) * 6.2831853f;
	const float cosR = std::cosf(rot);
	const float sinR = std::sinf(rot);
	for (PondPart& part : parts) {
		const float ox = part.offsetX;
		const float oz = part.offsetZ;
		part.offsetX = ox * cosR - oz * sinR;
		part.offsetZ = ox * sinR + oz * cosR;
	}

	return parts;
}

void WaterPond::Initialize(KamataEngine::Model* model, const KamataEngine::Vector3& center, const std::vector<PondPart>& parts, int drawLayerIndex) {
	model_  = model;
	center_ = center;
	parts_  = parts;

	const float kThickness = 0.15f;
	surfaceY_ = center.y + kThickness * 0.5f;
	drawLayerOffsetY_ = static_cast<float>(drawLayerIndex) * 0.04f;
	boundingRadius_ = 0.0f;

	objectColor_.Initialize();
	objectColor_.SetColor({1.0f, 1.0f, 1.0f, 1.0f});

	for (const PondPart& part : parts_) {
		const float extent = std::sqrtf(part.offsetX * part.offsetX + part.offsetZ * part.offsetZ) + part.radius;
		if (extent > boundingRadius_) {
			boundingRadius_ = extent;
		}
	}

	if (!drawTransform_) {
		drawTransform_ = std::make_unique<KamataEngine::WorldTransform>();
	}
	drawTransform_->Initialize();
}

void WaterPond::Draw(const KamataEngine::Camera& camera) {
	if (!model_ || !drawTransform_ || parts_.empty() || boundingRadius_ <= 0.0f) {
		return;
	}
	const float kThickness = 0.15f;
	const float diameter = boundingRadius_ * 2.0f;
	const float modelScaleXZ = diameter / kPondModelDiameter;
	const float modelScaleY  = kThickness / kPondModelDiameter;
	// 複数パーツも外接円1枚で描画（重なりによる半透明のギザギザを防ぐ）
	drawTransform_->translation_ = {
	    center_.x,
	    center_.y + drawLayerOffsetY_ + kPondDrawFloatY,
	    center_.z
	};
	drawTransform_->scale_ = {modelScaleXZ, modelScaleY, modelScaleXZ};
	drawTransform_->UpdateMatrix();
	model_->Draw(*drawTransform_, camera, &objectColor_);
}

bool WaterPond::CheckBallFallIn(const KamataEngine::Vector3& ballPos, float ballRadius) const {
	// 水面より十分高い位置を通過中は除外
	const float kMaxHeightAboveSurface = ballRadius + 3.0f;
	if (ballPos.y > surfaceY_ + kMaxHeightAboveSurface) {
		return false;
	}

	const float dx = ballPos.x - center_.x;
	const float dz = ballPos.z - center_.z;

	// 見た目（外接円1枚）と一致: ボール半径分だけ広げて判定
	const float hitRadius = boundingRadius_ + ballRadius;
	return (dx * dx + dz * dz) <= (hitRadius * hitRadius);
}

KamataEngine::Vector3 WaterPond::GetCenter() const {
	return center_;
}
