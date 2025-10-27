#include "Enemy.h"
#include "2d/Sprite.h"
#include "Player.h"
#include "base/TextureManager.h"
#include "base/WinApp.h"
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
		targetSprite_->SetSize({50.0f, 50.0f}); // 半分のサイズ
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

void Enemy::OnCollision() { isDead_ = true; }

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

	// ▼▼▼ 修正 ▼▼▼
	// 接近
	KamataEngine::Vector3 accessSpeed = {0.1f, 0.1f, 0.1f}; // ★ 1.1f だと速すぎるので 0.1f に
	// 離脱
	KamataEngine::Vector3 eliminationSpeed = {0.3f, 0.3f, 0.3f};

	switch (phase_) {
	case Phase::Approach:
	default: // 移動(ベクトルを加算)
		worldtransfrom_.translation_.z -= accessSpeed.z;
		// 規定の位置に到達したら離脱
		// ★ 0.0f (プレイヤー) より手前 (奥) の 20.0f あたりで止まるように調整
		if (worldtransfrom_.translation_.z < 20.0f) {
			phase_ = Phase::Leave;
		}
		break;
	case Phase::Leave:
		// 移動(ベクトルを加算)
		// ★ 上 (y) に消えるのではなく、奥 (z) に戻るように変更
		worldtransfrom_.translation_.z += accessSpeed.z;
		// ★ ある程度奥 (例: 60.0f) に戻ったら、再度 Approach にする
		if (worldtransfrom_.translation_.z > 60.0f) {
			phase_ = Phase::Approach;
		}
		break;
	}

	worldtransfrom_.UpdateMatrix();

	// 画面内判定とスプライト位置の更新
	if (camera_ && targetSprite_) {
		UpdateScreenPosition();
	}
}

void Enemy::Draw(const KamataEngine::Camera& camera) { model_->Draw(worldtransfrom_, camera); }

void Enemy::DrawSprite() {
	// ▼▼▼ 修正 ▼▼▼
	// デバッグ用の強制描画から、isOnScreen_ をチェックする処理に戻します
	if (isOnScreen_ && targetSprite_) {
		// ▲▲▲ ▲▲▲
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

	// カメラの行列
	const KamataEngine::Matrix4x4& viewMatrix = camera_->matView;
	const KamataEngine::Matrix4x4& projMatrix = camera_->matProjection;

	// --- 1. ワールド -> ビュー変換 (vM形式) ---
	KamataEngine::Vector3 viewPos;
	viewPos.x = worldPos.x * viewMatrix.m[0][0] + worldPos.y * viewMatrix.m[1][0] + worldPos.z * viewMatrix.m[2][0] + viewMatrix.m[3][0];
	viewPos.y = worldPos.x * viewMatrix.m[0][1] + worldPos.y * viewMatrix.m[1][1] + worldPos.z * viewMatrix.m[2][1] + viewMatrix.m[3][1];
	viewPos.z = worldPos.x * viewMatrix.m[0][2] + worldPos.y * viewMatrix.m[1][2] + worldPos.z * viewMatrix.m[2][2] + viewMatrix.m[3][2];

	// (ビュー行列の第4列 (m[...][3]) は通常 (0,0,0,1) なので、w_view は 1.0 になる)
	// float w_view = worldPos.x * viewMatrix.m[0][3] + worldPos.y * viewMatrix.m[1][3] + worldPos.z * viewMatrix.m[2][3] + viewMatrix.m[3][3];

	// カメラの前方にあるかチェック (ビュー座標のZがプラスか)
	if (viewPos.z > 0.0f) {

		// --- 2. ビュー -> クリップ座標変換 (vM形式) ---
		// viewPos (x, y, z, 1.0) * projMatrix
		float clipX = viewPos.x * projMatrix.m[0][0] + viewPos.y * projMatrix.m[1][0] + viewPos.z * projMatrix.m[2][0] + 1.0f * projMatrix.m[3][0];
		float clipY = viewPos.x * projMatrix.m[0][1] + viewPos.y * projMatrix.m[1][1] + viewPos.z * projMatrix.m[2][1] + 1.0f * projMatrix.m[3][1];
		// float clipZ = viewPos.x * projMatrix.m[0][2] + viewPos.y * projMatrix.m[1][2] + viewPos.z * projMatrix.m[2][2] + 1.0f * projMatrix.m[3][2];

		// ★ パースペクティブ除算に使う w (w_clip) は、プロジェクション行列をかけた結果の第4成分
		float w_clip = viewPos.x * projMatrix.m[0][3] + viewPos.y * projMatrix.m[1][3] + viewPos.z * projMatrix.m[2][3] + 1.0f * projMatrix.m[3][3];

		// (w_clip が 0 またはマイナスの場合も描画しない)
		if (w_clip > 0.0f) {

			// --- 3. パースペクティブ除算 (クリップ -> NDC) ---
			float ndcX = clipX / w_clip;
			float ndcY = clipY / w_clip;
			// float ndcZ = clipZ / w_clip;

			// --- 4. NDC -> スクリーン座標 ---
			float screenX = (ndcX + 1.0f) * 0.5f * WinApp::kWindowWidth;
			float screenY = (1.0f - ndcY) * 0.5f * WinApp::kWindowHeight;

			screenPosition_ = {screenX, screenY};

			// 画面内判定 (NDCが -1.0～+1.0 の範囲内か)
			bool isWithinBounds = (ndcX >= -1.0f && ndcX <= 1.0f) && (ndcY >= -1.0f && ndcY <= 1.0f);
			// (ndcZ >= 0.0f && ndcZ <= 1.0f); // Zもチェックするのが望ましい

			if (isWithinBounds) {
				isOnScreen_ = true;
				targetSprite_->SetPosition(screenPosition_);
			} else {
				isOnScreen_ = false; // 画面外
			}
		} else {
			isOnScreen_ = false; // w_clip が0以下 (クリップ平面の手前か後方)
		}
	} else {
		// カメラの後方 (ビュー座標のZがマイナス)
		isOnScreen_ = false;
	}
}