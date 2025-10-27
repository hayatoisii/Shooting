#include "Player.h"
#include "Enemy.h"
#include <algorithm>
#include "RailCamera.h"
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

	worldtransfrom_.Initialize();

	modelParticle_ = KamataEngine::Model::CreateFromOBJ("cube", true);
	engineExhaust_ = new ParticleEmitter();
	engineExhaust_->Initialize(modelParticle_);
}

void Player::OnCollision() { isDead_ = true; }

void Player::Attack() {
	specialTimer--;
	if (specialTimer < 0) {
		if (input_->TriggerKey(DIK_SPACE)) {
			// assert(enemy_); // (敵がいない時でも弾を撃てるように、これはコメントアウトしても良いかもしれません)

			// ★ railCamera_ が無いと計算できないので、アサート（チェック）を追加
			assert(railCamera_);

			// 1. 弾の発射位置 (プレイヤーのワールド座標)
			KamataEngine::Vector3 moveBullet = GetWorldPosition();

			const float kBulletSpeed = 30.0f;

			// ▼▼▼ ここから速度計算のロジックを差し替え ▼▼▼

			// 2. カメラ（レティクル）の向いている先を計算

			// 2a. カメラのワールド行列を取得
			const KamataEngine::Matrix4x4& cameraWorldMatrix = railCamera_->GetWorldTransform().matWorld_;

			// 2b. カメラのワールド座標を取得
			KamataEngine::Vector3 cameraPosition;
			cameraPosition.x = cameraWorldMatrix.m[3][0];
			cameraPosition.y = cameraWorldMatrix.m[3][1];
			cameraPosition.z = cameraWorldMatrix.m[3][2];

			// 2c. カメラの前方ベクトル(Z軸)を取得
			KamataEngine::Vector3 cameraForward;
			cameraForward.x = cameraWorldMatrix.m[2][0];
			cameraForward.y = cameraWorldMatrix.m[2][1];
			cameraForward.z = cameraWorldMatrix.m[2][2];
			cameraForward = KamataEngine::MathUtility::Normalize(cameraForward); // 念のため正規化

			// 2d. 画面中央の「目標地点」をカメラのはるか前方に設定
			// (この 1000.0f は "十分遠く" であればどんな大きな値でもOKです)
			KamataEngine::Vector3 targetPosition = cameraPosition + cameraForward * 1000.0f;

			// 3. 発射位置(moveBullet) から 目標地点(targetPosition) へ向かうベクトルを計算
			KamataEngine::Vector3 velocity = targetPosition - moveBullet;

			// 4. ベクトルを正規化して、弾の速度を適用
			velocity = KamataEngine::MathUtility::Normalize(velocity);
			velocity = velocity * kBulletSpeed;

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

	const float kPlayerMaxX = 4.0f;
	float targetX = 0.0f;
	if (input_->PushKey(DIK_A)) {
		targetX = kPlayerMaxX;
	}
	if (input_->PushKey(DIK_D)) {
		targetX = -kPlayerMaxX;
	}

	float targetY = worldtransfrom_.translation_.y;

	float moveLerpFactor = 0.0f;
	if (input_->PushKey(DIK_A) || input_->PushKey(DIK_D)) {
		moveLerpFactor = 0.015f;
	} else {
		moveLerpFactor = 0.03f;
	}
	worldtransfrom_.translation_.x += (targetX - worldtransfrom_.translation_.x) * moveLerpFactor;

	// Y軸は targetY と translation_.y が同じ値なので、右辺が 0 になり、Y座標は変更されなくなる
	worldtransfrom_.translation_.y += (targetY - worldtransfrom_.translation_.y) * moveLerpFactor;

	if (railCamera_) {
		// 左右
		float yawVelocity = railCamera_->GetRotationVelocity().y;
		const float tiltFactor = -90.0f; // 傾きの速度
		float targetRoll = yawVelocity * tiltFactor;
		const float maxRollAngle = 4.0f; // 傾きの最大値
		targetRoll = std::clamp(targetRoll, -maxRollAngle, maxRollAngle);
		const float lerpFactor = 0.1f;
		worldtransfrom_.rotation_.z += (targetRoll - worldtransfrom_.rotation_.z) * lerpFactor;

		// 上
		float pitchVelocity = railCamera_->GetRotationVelocity().x;
		const float pitchFactor = 70.0f;
		float targetPitch = pitchVelocity * pitchFactor;
		const float maxPitchAngle = 1.5f;
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

void Player::ResetParticles() {
	if (engineExhaust_) {
		    engineExhaust_->Clear(); // ParticleEmitter の Clear 関数を呼び出す
	}
}