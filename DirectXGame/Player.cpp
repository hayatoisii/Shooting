#include "Player.h"
#include "Enemy.h"
#include <cassert>
#include <algorithm>

Player::~Player() {
	//model_ = nullptr;
	//camera_ = nullptr;
	delete modelbullet_;
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

}

void Player::OnCollision() {

	isDead_ = true;

}

void Player::Attack() {

	specialTimer--;
	if (specialTimer < 0) {

		if (input_->TriggerKey(DIK_SPACE)) {

			assert(enemy_);

			KamataEngine::Vector3 moveBullet = GetWorldPosition();

			// 弾の速度
			const float kBulletSpeed = 9.0f;

			// KamataEngine::Vector3 velocity(0, 0, 0);
			KamataEngine::Vector3 velocity(0, 0, kBulletSpeed);

			// 速度ベクトルを自機の向きに合わせて回転させる
			velocity = KamataEngine::MathUtility::TransformNormal(velocity, worldtransfrom_.matWorld_);

			// 弾を生成し、初期化
			PlayerBullet* newBullet = new PlayerBullet();
			newBullet->Initialize(modelbullet_, moveBullet, velocity);

			// 弾を登録する
			bullets_.push_back(newBullet);

			isParry_ = false;
		}
	}
}

// ワールド座標を取得
KamataEngine::Vector3 Player::GetWorldPosition() { 

	// ワールド座標を入れる変数
	KamataEngine::Vector3 worldPos;
	// ワールド行列の平行移動成分を取得（ワールド座標）
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

void Player::SetParent(const WorldTransform* parent) {

	worldtransfrom_.parent_ = parent;

}

void Player::Update() {

	Attack();

	// 弾更新
	for (PlayerBullet* bullet : bullets_) {
		bullet->Update();
	}

	// デスフラグの立った弾を削除
	bullets_.remove_if([](PlayerBullet* bullet) {
		if (bullet->IsDead()) {
			delete bullet;
			return true;
		}
		return false;
	});

	// キャラクターの移動ベクトル
	KamataEngine::Vector3 move = {0, 0, 0};
	

	worldtransfrom_.translation_.x += move.x;
	worldtransfrom_.translation_.y += move.y;

	if (railCamera_) {
		// カメラのY軸（左右）の回転速度を取得
		float yawVelocity = railCamera_->GetRotationVelocity().y;

		// 目標となる機体の傾き（Z軸回転）を計算
		// yawVelocityがマイナスなら右に、プラスなら左に傾くように調整
		const float tiltFactor = -30.0f; // 傾きの感度（この値を大きくするとより大きく傾く）
		float targetRoll = yawVelocity * tiltFactor;

		// 傾きすぎないように最大角度を制限
		const float maxRollAngle = 0.5f; // 最大傾き角度（ラジアン単位。約28度）
		targetRoll = std::clamp(targetRoll, -maxRollAngle, maxRollAngle);

		// 現在の傾きから目標の傾きへ滑らかに変化させる (Lerp: 線形補間)
		// 最後の数値(0.1f)が小さいほど、ゆっくり滑らかに傾き、戻ります
		const float lerpFactor = 0.1f;
		worldtransfrom_.rotation_.z += (targetRoll - worldtransfrom_.rotation_.z) * lerpFactor;
	
		//上下
		float pitchVelocity = railCamera_->GetRotationVelocity().x;
		// 目標となる機首の角度（X軸回転）を計算
		const float pitchFactor = 20.0f; // 上下の感度
		float targetPitch = pitchVelocity * pitchFactor;
		// 傾きすぎないように最大角度を制限
		const float maxPitchAngle = 0.3f; // 最大上下角度
		targetPitch = std::clamp(targetPitch, -maxPitchAngle, maxPitchAngle);
		// 現在の傾きから目標の傾きへ滑らかに変化させる
		worldtransfrom_.rotation_.x += (targetPitch - worldtransfrom_.rotation_.x) * lerpFactor;		
	}

	worldtransfrom_.UpdateMatrix();

}

void Player::Draw() { 

	model_->Draw(worldtransfrom_, *camera_);

	// 弾描画
	for (PlayerBullet* bullet : bullets_) {
	bullet->Draw(*camera_);
	}
}

void Player::SetRailCamera(RailCamera* camera) { railCamera_ = camera; }