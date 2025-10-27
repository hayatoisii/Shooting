#include "Enemy.h"
#include "Player.h"
#include "base/WinApp.h"
#include "base/TextureManager.h"
#include "2d/Sprite.h"
#include <algorithm>

Enemy::~Enemy() {

	delete modelbullet_;
	delete targetSprite_;
}

void Enemy::Initialize(KamataEngine::Model* model, const KamataEngine::Vector3& pos) {
	assert(model);
	model_ = model;
	modelbullet_ = KamataEngine::Model::CreateFromOBJ("cube", true);
	worldtransfrom_.translation_ = pos;
	worldtransfrom_.Initialize();
	worldtransfrom_.UpdateMatrix(); // 初期化後に行列を更新
	UpdateScreenPosition();
	
	// 追尾スプライトの初期化
	uint32_t texHandle = TextureManager::Load("redbox.png");
	targetSprite_ = Sprite::Create(texHandle, {0, 0});
	if (targetSprite_) {
		targetSprite_->SetSize({0.1f, 0.1f}); // 半分のサイズ
		targetSprite_->SetColor({1.0f, 0.0f, 0.0f, 1.0f});
		targetSprite_->SetAnchorPoint({0.5f, 0.5f});
	}
	isOnScreen_ = false;
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
	
	// 画面内判定とスプライト位置の更新
	if (camera_ && targetSprite_) {
		UpdateScreenPosition();
	}
}

void Enemy::Draw(const KamataEngine::Camera& camera) {

	model_->Draw(worldtransfrom_, camera);
}

void Enemy::DrawSprite() {
	// 画面内のときだけ追尾スプライトを描画
	if (isOnScreen_ && targetSprite_) {
		targetSprite_->Draw();
	}
}

void Enemy::UpdateScreenPosition() {
	if (!camera_ || !targetSprite_) {
		isOnScreen_ = false;
		return;
	}
	
	// ワールド座標を取得
	KamataEngine::Vector3 worldPos = GetWorldPosition();
	
	// カメラの行列を使用してビュー座標に変換
	const KamataEngine::Matrix4x4& viewMatrix = camera_->matView;
	const KamataEngine::Matrix4x4& projMatrix = camera_->matProjection;
	
	// ワールド座標をビュー座標に変換
	KamataEngine::Vector3 viewPos;
	viewPos.x = worldPos.x * viewMatrix.m[0][0] + worldPos.y * viewMatrix.m[1][0] + worldPos.z * viewMatrix.m[2][0] + viewMatrix.m[3][0];
	viewPos.y = worldPos.x * viewMatrix.m[0][1] + worldPos.y * viewMatrix.m[1][1] + worldPos.z * viewMatrix.m[2][1] + viewMatrix.m[3][1];
	viewPos.z = worldPos.x * viewMatrix.m[0][2] + worldPos.y * viewMatrix.m[1][2] + worldPos.z * viewMatrix.m[2][2] + viewMatrix.m[3][2];
	
	// w値を計算
	float w = worldPos.x * viewMatrix.m[0][3] + worldPos.y * viewMatrix.m[1][3] + worldPos.z * viewMatrix.m[2][3] + viewMatrix.m[3][3];
	
	// カメラの前方にあるかチェック
	if (w > 0.0f && viewPos.z > 0.0f) {
		// 射影変換
		float clipX = (viewPos.x * projMatrix.m[0][0] + viewPos.y * projMatrix.m[1][0] + viewPos.z * projMatrix.m[2][0] + projMatrix.m[3][0]) / w;
		float clipY = (viewPos.x * projMatrix.m[0][1] + viewPos.y * projMatrix.m[1][1] + viewPos.z * projMatrix.m[2][1] + projMatrix.m[3][1]) / w;
		
		// 正規化デバイス座標に変換
		float ndcX = clipX;
		float ndcY = clipY;
		
		// スクリーン座標に変換
		float screenX = (ndcX + 1.0f) * 0.5f * WinApp::kWindowWidth;
		float screenY = (1.0f - ndcY) * 0.5f * WinApp::kWindowHeight;
		
		screenPosition_ = {screenX, screenY};
		
		// 画面内かどうかを判定（画面端に大きな余裕を持たせる）
		//const float margin = 200.0f;
		//bool isWithinBounds = screenX >= -margin && screenX <= WinApp::kWindowWidth + margin &&
		  //                    screenY >= -margin && screenY <= WinApp::kWindowHeight + margin;
		
		// デバッグ用：常に描画
		isOnScreen_ = true;
		targetSprite_->SetPosition(screenPosition_);
	} else {
		isOnScreen_ = false;
	}
}
