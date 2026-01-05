#include "EnemyBullet.h"
#include "Player.h"
#include <algorithm>
#include <cassert>
#include <cmath>

EnemyBullet::~EnemyBullet() { model_ = nullptr; }

void EnemyBullet::Initialize(KamataEngine::Model* model, const KamataEngine::Vector3& position, const KamataEngine::Vector3& velocity) {
	assert(model);
	model_ = model;
	worldtransfrom_.translation_ = position;
	worldtransfrom_.Initialize();
	velocity_ = velocity;

	// 速度（スカラ）を保持
	float sp = std::sqrt(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);
	if (sp > 0.001f) {
		speed_ = sp;
	} else {
		speed_ = 1.0f; // 安全策
	}

	invulnerableFrames_ = 8;
}

void EnemyBullet::OnEvaded() {
	isHoming_ = false;
	evadedDeathTimer_ = 60;
}

KamataEngine::Vector3 EnemyBullet::GetWorldPosition() {
	worldtransfrom_.UpdateMatrix();
	KamataEngine::Vector3 worldPos;
	worldPos.x = worldtransfrom_.matWorld_.m[3][0];
	worldPos.y = worldtransfrom_.matWorld_.m[3][1];
	worldPos.z = worldtransfrom_.matWorld_.m[3][2];
	return worldPos;
}

// 球面線形補間（Slerp）の簡易実装関数
// current: 現在の向き(正規化済み), target: 目標の向き(正規化済み), maxAngle: 最大回転角度(ラジアン)
KamataEngine::Vector3 SlerpRotate(const KamataEngine::Vector3& current, const KamataEngine::Vector3& target, float maxAngle) {
	// 内積を計算
	float dot = current.x * target.x + current.y * target.y + current.z * target.z;
	// 誤差対策でクランプ
	dot = std::clamp(dot, -1.0f, 1.0f);

	// 現在の角度差を計算
	float angle = std::acos(dot);

	// ほとんど向きが同じ、または真後ろの場合はそのまま（計算不能回避）
	if (std::abs(angle) < 0.001f || std::abs(angle - 3.14159f) < 0.001f) {
		return target;
	}

	// 回転量が最大角度を超えていたら制限する
	float t = 1.0f;
	if (angle > maxAngle) {
		t = maxAngle / angle;
	}

	// 球面線形補間の公式
	float sinTheta = std::sin(angle);
	float ps = std::sin(angle * (1.0f - t)) / sinTheta;
	float pt = std::sin(angle * t) / sinTheta;

	KamataEngine::Vector3 result;
	result.x = current.x * ps + target.x * pt;
	result.y = current.y * ps + target.y * pt;
	result.z = current.z * ps + target.z * pt;
	return result;
}

void EnemyBullet::Update() {

	if (evadedDeathTimer_ > 0) {
		evadedDeathTimer_--;
		if (evadedDeathTimer_ <= 0) {
			isDead_ = true;
			return;
		}
	}

	if (--deathTimer_ <= 0) {
		isDead_ = true;
		return;
	}

	if (invulnerableFrames_ > 0) {
		invulnerableFrames_--;
	}

	// --- 新しいホーミング処理 ---
	if (isHoming_ && homingTarget_ && !homingTarget_->IsDead()) {
		KamataEngine::Vector3 targetPos = homingTarget_->GetWorldPosition();
		KamataEngine::Vector3 bulletPos = GetWorldPosition();
		KamataEngine::Vector3 toTarget = {targetPos.x - bulletPos.x, targetPos.y - bulletPos.y, targetPos.z - bulletPos.z};
		float dist = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);

		// 必中ヒット距離
		const float kHitRange = 15.0f;
		if (dist <= kHitRange) {
			homingTarget_->OnCollision();
			isDead_ = true;
			return;
		}

		if (dist > 0.001f) {
			// 正規化
			KamataEngine::Vector3 toTargetDir = toTarget;
			toTargetDir.x /= dist;
			toTargetDir.y /= dist;
			toTargetDir.z /= dist;

			KamataEngine::Vector3 currentDir = velocity_;
			float currentSpeed = std::sqrt(currentDir.x * currentDir.x + currentDir.y * currentDir.y + currentDir.z * currentDir.z);
			if (currentSpeed < 0.001f)
				currentSpeed = speed_; // 速度がゼロなら保存していた速度を使う

			currentDir.x /= currentSpeed;
			currentDir.y /= currentSpeed;
			currentDir.z /= currentSpeed;

			// 内積（情報のみ）
			//float dot = currentDir.x * toTargetDir.x + currentDir.y * toTargetDir.y + currentDir.z * toTargetDir.z;

			// 回転角度の制限（自動でホーミング解除しない：回避はプレイヤーの回避アクションでのみ行う）
			float maxTurnAngle = 0.05f; // デフォルト旋回性能

			// 近づくと旋回性能アップ（シフト回避必須にするため強くする）
			if (dist < 400.0f) {
				float rate = 1.0f - (dist / 400.0f);
				maxTurnAngle += rate * 0.3f; // 最大で +0.3rad (約20度/フレーム) 加算
			}

			// Slerpを使って「最大角度分だけ」ターゲットに向ける
			KamataEngine::Vector3 newDir = SlerpRotate(currentDir, toTargetDir, maxTurnAngle);

			// 長さを整える（正規化）
			float len = std::sqrt(newDir.x * newDir.x + newDir.y * newDir.y + newDir.z * newDir.z);
			if (len > 0.001f) {
				newDir.x /= len;
				newDir.y /= len;
				newDir.z /= len;
			}

			// 新しい向きに速度を設定
			velocity_.x = newDir.x * currentSpeed;
			velocity_.y = newDir.y * currentSpeed;
			velocity_.z = newDir.z * currentSpeed;
		}
	}

	worldtransfrom_.translation_.x += velocity_.x;
	worldtransfrom_.translation_.y += velocity_.y;
	worldtransfrom_.translation_.z += velocity_.z;

	worldtransfrom_.UpdateMatrix();
}

void EnemyBullet::OnCollision() { isDead_ = true; }

void EnemyBullet::Draw(const KamataEngine::Camera& camera) {
	if (!isDead_) {
		model_->Draw(worldtransfrom_, camera);
	}
}