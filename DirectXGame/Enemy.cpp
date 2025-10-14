#include "Enemy.h"
#include "Player.h"

Enemy::~Enemy() {

	delete modelbullet_;
}

void Enemy::Initialize(KamataEngine::Model* model, const KamataEngine::Vector3& pos) {
	assert(model);
	model_ = model;
	modelbullet_ = KamataEngine::Model::CreateFromOBJ("cube", true);
	worldtransfrom_.translation_ = pos;
	worldtransfrom_.Initialize();
	worldtransfrom_.UpdateMatrix(); // 初期化後に行列を更新
}

KamataEngine::Vector3 Enemy::GetWorldPosition() {

	// ワールド座標を入れる変数
	KamataEngine::Vector3 worldPos;
	// ワールド行列の平行移動成分を取得（ワールド座標）
	worldPos.x = worldtransfrom_.matWorld_.m[3][0];
	worldPos.y = worldtransfrom_.matWorld_.m[3][1];
	worldPos.z = worldtransfrom_.matWorld_.m[3][2];

	return worldPos;
}

void Enemy::OnCollision() { 
	isDead_ = true;
}

void Enemy::Fire() {

		assert(player_);

		spawnTimer--;

		if (spawnTimer < -0.0f) {

			KamataEngine::Vector3 moveBullet = worldtransfrom_.translation_;

			// 弾の速度
			const float kBulletSpeed = -0.0f;

			KamataEngine::Vector3 velocity(0, 0, 0);

			KamataEngine::Vector3 playerWorldtransform = player_->GetWorldPosition();
			KamataEngine::Vector3 enemyWorldtransform = GetWorldPosition();
			KamataEngine::Vector3 homingBullet = enemyWorldtransform - playerWorldtransform;
			homingBullet = Normalize(homingBullet);
			velocity.x += kBulletSpeed * homingBullet.x;
			velocity.y += kBulletSpeed * homingBullet.y;
			velocity.z += kBulletSpeed * homingBullet.z;

			// 弾を生成し、初期化
			EnemyBullet* newBullet = new EnemyBullet();
			newBullet->Initialize(modelbullet_, moveBullet, velocity);

			// 弾を登録する
			gameScene_->AddEnemyBullet(newBullet);

			spawnTimer = kFireInterval;
		}
}

void Enemy::Update() {

	Fire();

	// キャラクターの移動ベクトル
	KamataEngine::Vector3 move = {0, 0, 0};
	// 接近
	KamataEngine::Vector3 accessSpeed = {0.1f,0.1f,0.1f};
	// 離脱
	KamataEngine::Vector3 eliminationSpeed = {0.3f, 0.3f, 0.3f};

	/*/
	switch (phase_) { 
	case Phase::Approach:
	default:
		// 移動(ベクトルを加算)
		worldtransfrom_.translation_.z -= accessSpeed.z;
		// 規定の位置に到達したら離脱
		if (worldtransfrom_.translation_.z < 0.0f) {
			phase_ = Phase::Leave;
		}
		break;
	case Phase::Leave:
		// 移動(ベクトルを加算)
		worldtransfrom_.translation_.y += eliminationSpeed.y;
		break;

	}
	/*/

	worldtransfrom_.UpdateMatrix();
}

void Enemy::Draw(const KamataEngine::Camera& camera) {

	model_->Draw(worldtransfrom_, camera);
}
