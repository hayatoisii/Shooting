#include "Player.h"
#include "Enemy.h"
#include <algorithm>
#include <cassert>

Player::~Player() {
	delete modelbullet_;
	delete modelParticle_;
	delete engineExhaust_;
	for (PlayerBullet* bullet : bullets_) {
		delete bullet;
	}
}

void Player::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& pos) {
	assert(model);
	model_ = model;
	camera_ = camera;
	modelbullet_ = KamataEngine::Model::CreateFromOBJ("cube", true);
	worldtransfrom_.translation_ = pos;
	input_ = KamataEngine::Input::GetInstance();
	worldtransfrom_.translation_.y = -3.0f;

	// カメラからの距離を調整したい場合は、このZ座標の数値を変更します
	worldtransfrom_.translation_.z = 28.0f;

	worldtransfrom_.Initialize();

	modelParticle_ = KamataEngine::Model::CreateFromOBJ("cube", true);

	// ▼▼▼ ここが最重要修正ポイントです ▼▼▼
	// 'engineExfrom_' になっていたタイプミスを 'engineExhaust_' に修正
	engineExhaust_ = new ParticleEmitter();
	engineExhaust_->Initialize(modelParticle_);
	// ▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲
}

void Player::OnCollision() { isDead_ = true; }

void Player::Attack() {
	specialTimer--;
	if (specialTimer < 0) {
		if (input_->TriggerKey(DIK_SPACE)) {
			assert(enemy_);
			KamataEngine::Vector3 moveBullet = GetWorldPosition();
			const float kBulletSpeed = 9.0f;
			KamataEngine::Vector3 velocity(0, 0, kBulletSpeed);
			velocity = KamataEngine::MathUtility::TransformNormal(velocity, worldtransfrom_.matWorld_);
			PlayerBullet* newBullet = new PlayerBullet();
			newBullet->Initialize(modelbullet_, moveBullet, velocity);
			bullets_.push_back(newBullet);
			isParry_ = false;
		}
	}
}

KamataEngine::Vector3 Player::GetWorldPosition() {
	KamataEngine::Vector3 worldPos;
	worldPos.x = worldtransfrom_.matWorld_.m[3][0];
	worldPos.y = worldtransfrom_.matWorld_.m[3][1];
	worldPos.z = worldtransfrom_.matWorld_.m[3][2];
	return worldPos;
}

AABB Player::GetAABB() {
	KamataEngine::Vector3 worldPos = GetWorldPosition();
	AABB aabb;
	aabb.min = {worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f};
	aabb.max = {worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f};
	return aabb;
}

void Player::SetParent(const WorldTransform* parent) { worldtransfrom_.parent_ = parent; }

void Player::Update() {
	Attack();

	for (PlayerBullet* bullet : bullets_) {
		bullet->Update();
	}

	bullets_.remove_if([](PlayerBullet* bullet) {
		if (bullet->IsDead()) {
			delete bullet;
			return true;
		}
		return false;
	});

	const float kPlayerMaxX = 3.0f;
	float targetX = 0.0f;
	if (input_->PushKey(DIK_A)) {
		targetX = kPlayerMaxX;
	}
	if (input_->PushKey(DIK_D)) {
		targetX = -kPlayerMaxX;
	}

	const float kPlayerDefaultY = -3.0f;
	float targetY = kPlayerDefaultY;

	float moveLerpFactor = 0.0f;
	if (input_->PushKey(DIK_A) || input_->PushKey(DIK_D)) {
		moveLerpFactor = 0.015f;
	} else {
		moveLerpFactor = 0.03f;
	}
	worldtransfrom_.translation_.x += (targetX - worldtransfrom_.translation_.x) * moveLerpFactor;
	worldtransfrom_.translation_.y += (targetY - worldtransfrom_.translation_.y) * moveLerpFactor;

	if (railCamera_) {
		float yawVelocity = railCamera_->GetRotationVelocity().y;
		const float tiltFactor = -50.0f;
		float targetRoll = yawVelocity * tiltFactor;
		const float maxRollAngle = 0.5f;
		targetRoll = std::clamp(targetRoll, -maxRollAngle, maxRollAngle);
		const float lerpFactor = 0.1f;
		worldtransfrom_.rotation_.z += (targetRoll - worldtransfrom_.rotation_.z) * lerpFactor;

		float pitchVelocity = railCamera_->GetRotationVelocity().x;
		const float pitchFactor = 20.0f;
		float targetPitch = pitchVelocity * pitchFactor;
		const float maxPitchAngle = 0.3f;
		targetPitch = std::clamp(targetPitch, -maxPitchAngle, maxPitchAngle);
		worldtransfrom_.rotation_.x += (targetPitch - worldtransfrom_.rotation_.x) * lerpFactor;
	}

	// 1. プレイヤーのワールド行列を計算して、位置を確定
	worldtransfrom_.UpdateMatrix();

	// 2. 最新の座標から排気口のワールド座標を計算
	KamataEngine::Vector3 exhaustOffset = {0.0f, -0.2f, -1.0f};
	exhaustOffset = KamataEngine::MathUtility::TransformNormal(exhaustOffset, worldtransfrom_.matWorld_);
	KamataEngine::Vector3 emitterPos = GetWorldPosition() + exhaustOffset;

	KamataEngine::Vector3 playerBackVector = {-worldtransfrom_.matWorld_.m[2][0], -worldtransfrom_.matWorld_.m[2][1], -worldtransfrom_.matWorld_.m[2][2]};
	// ベクトルを正規化して純粋な方向にする
	playerBackVector = KamataEngine::MathUtility::Normalize(playerBackVector);

	// 4. 排気の速度を決定
	const float exhaustSpeed = 1.5f; // この数値を大きくすると排気が速くなる
	KamataEngine::Vector3 exhaustVelocity = playerBackVector * exhaustSpeed;

	// 5. 計算した「位置」と「速度」でパーティクルを発生
	engineExhaust_->Emit(emitterPos, exhaustVelocity);

	// 4. パーティクルシステムを更新
	engineExhaust_->Update();
}

void Player::Draw() {
	model_->Draw(worldtransfrom_, *camera_);
	engineExhaust_->Draw(*camera_);
	for (PlayerBullet* bullet : bullets_) {
		bullet->Draw(*camera_);
	}
}

void Player::SetRailCamera(RailCamera* camera) { railCamera_ = camera; }

void Player::ResetRotation() {
	// Playerのローカルな回転（傾きなど）をすべてゼロに戻す
	worldtransfrom_.rotation_ = {0.0f, 0.0f, 0.0f};
}