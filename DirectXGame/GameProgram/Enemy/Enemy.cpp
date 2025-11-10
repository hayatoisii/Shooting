#include "Enemy.h"
#include "2d/Sprite.h"
#include "GaneScene.h"
#include "Player.h"
#include "base/TextureManager.h"
#include "base/WinApp.h"
#include <algorithm>
#include <cassert>

// ▼▼▼ 1. include の不足を修正 ▼▼▼
#include "KamataEngine.h" // MathUtility (Normalize, Length) のために追加
#include <cmath>          // std::cos, std::sin のために追加
// ▲▲▲ 修正完了 ▲▲▲

Enemy::~Enemy() {
	delete modelbullet_;
	delete targetSprite_;
}

void Enemy::Initialize(KamataEngine::Model* model, const KamataEngine::Vector3& pos) {
	assert(model);
	model_ = model;
	modelbullet_ = KamataEngine::Model::CreateFromOBJ("cube", true);
	worldtransfrom_.translation_ = pos;

	// ▼▼▼ 2. Initialize の変数名を修正 ▼▼▼
	// initialRelativePos_ = pos; // ★ 修正前
	initialWorldPos_ = pos; // ★ 修正後 (スポーン時の「ワールド座標」を保存)
	// ▲▲▲ 修正完了 ▲▲▲

	circleTimer_ = 0.0f; // タイマーをリセット

	isFollowing_ = false;

	worldtransfrom_.Initialize();
	// worldtransfrom_.UpdateMatrix(); // Initialize内でも呼ばれるので不要

	hp_ = 8;

	// ( ... スプライト初期化 ... )
	uint32_t texHandle = TextureManager::Load("redbox.png");
	targetSprite_ = Sprite::Create(texHandle, {0, 0});
	if (targetSprite_) {
		targetSprite_->SetSize({50.0f, 50.0f});
		targetSprite_->SetColor({1.0f, 0.0f, 0.0f, 1.0f});
		targetSprite_->SetAnchorPoint({0.5f, 0.5f});
	}
	isOnScreen_ = false;
	wasOnScreenLastFrame_ = false;
	lockOnAnimRotation_ = 0.0f;
	lockOnAnimScale_ = 1.0f;
}

KamataEngine::Vector3 Enemy::GetWorldPosition() {
	KamataEngine::Vector3 worldPos;
	worldPos.x = worldtransfrom_.matWorld_.m[3][0];
	worldPos.y = worldtransfrom_.matWorld_.m[3][1];
	worldPos.z = worldtransfrom_.matWorld_.m[3][2];
	return worldPos;
}

void Enemy::OnCollision() {
	hp_--;
	if (hp_ <= 0) {
		isDead_ = true;
	};
}

void Enemy::Fire() {
	assert(player_);
	spawnTimer--;

	if (spawnTimer < 0.0f) {

		KamataEngine::Vector3 moveBullet = GetWorldPosition();
		const float kBulletSpeed = 10.0f;
		KamataEngine::Vector3 velocity(0, 0, 0);

		KamataEngine::Vector3 playerWorldtransform = player_->GetWorldPosition();
		KamataEngine::Vector3 enemyWorldtransform = GetWorldPosition();

		KamataEngine::Vector3 homingBullet = playerWorldtransform - enemyWorldtransform;

		// ▼▼▼ 3. Fire の Normalize 呼び出しを修正 ▼▼▼
		// homingBullet = Normalize(homingBullet); // ★ 修正前 (エラー)
		homingBullet = KamataEngine::MathUtility::Normalize(homingBullet); // ★ 修正後
		// ▲▲▲ 修正完了 ▲▲▲

		velocity.x = kBulletSpeed * homingBullet.x;
		velocity.y = kBulletSpeed * homingBullet.y;
		velocity.z = kBulletSpeed * homingBullet.z;

		EnemyBullet* newBullet = new EnemyBullet();
		newBullet->Initialize(modelbullet_, moveBullet, velocity);

		if (gameScene_) {
			gameScene_->AddEnemyBullet(newBullet);
		}

		spawnTimer = kFireInterval;
	}
}

void Enemy::Update() {

	// Fire();

	// --- 1. 必要な情報を取得 ---
	assert(player_ && "Enemy::Update() player_ が null です");
	KamataEngine::Vector3 playerPos = player_->GetWorldPosition();
	KamataEngine::Vector3 myCenterPos = initialWorldPos_;

	// --- 2. プレイヤーと敵（の中心）との距離を計算 ---
	KamataEngine::Vector3 vecToPlayer = playerPos - myCenterPos;
	float distance = KamataEngine::MathUtility::Length(vecToPlayer);

	// ▼▼▼ ステップ3のロジックを修正 ▼▼▼

	// --- 3. 距離に基づき、円運動の中心 (center) を決定 ---
	const float kEngageDistance = 400.0f; // ★ この距離より近づくと、追従をやめる
	const float kFollowDistance = 500.0f; // ★ この距離より離れると、追従を始める

	KamataEngine::Vector3 center; // このフレームで円運動の中心に使う座標

	if (isFollowing_) {
		// 【現在、追従モードの場合】
		KamataEngine::Vector3 dirToPlayer = KamataEngine::MathUtility::Normalize(vecToPlayer);

		// 1. プレイヤーから 600 離れた位置を「このフレームの中心」として計算
		center = playerPos - (dirToPlayer * kFollowDistance);

		// ▼▼▼ 修正 ▼▼▼
		// ★ 追従モード中は、基準位置(initialWorldPos_)も常に更新する
		initialWorldPos_ = center;
		// ▲▲▲ 修正完了 ▲▲▲

		if (distance < kEngageDistance) {
			// 2. プレイヤーが十分近づいた(500未満)ので、追従をやめる
			isFollowing_ = false;

			// 3. (基準位置は ↑ で更新済み)
			// initialWorldPos_ = center; // ★ この行は不要
		}
		// (else: まだ離れている(500以上)なら、center を使い、基準位置も更新)

	} else {
		// 【現在、戦闘（ワールド固定）モードの場合】

		// 1. 基準位置(initialWorldPos_)をそのまま使う
		center = initialWorldPos_;

		if (distance > kFollowDistance) {
			// 2. プレイヤーが離れすぎた(600超過)ので、追従を開始
			isFollowing_ = true;

			// 3. 追従を開始する瞬間の「中心」も計算
			KamataEngine::Vector3 dirToPlayer = KamataEngine::MathUtility::Normalize(vecToPlayer);
			center = playerPos - (dirToPlayer * kFollowDistance);

			// ★ 追従開始時も、基準位置(initialWorldPos_)を更新する
			initialWorldPos_ = center;
		}
	}

	// --- 4. 円運動の計算 ---
	const float kCircleRadius = 650.0f;
	const float kCircleSpeed = 0.0008f;

	circleTimer_ += kCircleSpeed;

	worldtransfrom_.translation_.x = center.x + (kCircleRadius * std::cos(circleTimer_));
	worldtransfrom_.translation_.y = center.y + (kCircleRadius * std::sin(circleTimer_));
	worldtransfrom_.translation_.z = center.z;

	// --- 5. ワールド行列の更新 ---
	worldtransfrom_.UpdateMatrix();

	// --- 6. 画面内判定とスプライト位置の更新 ---
	if (camera_ && targetSprite_) {
		UpdateScreenPosition();
	}
}

void Enemy::Draw(const KamataEngine::Camera& camera) { model_->Draw(worldtransfrom_, camera); }

void Enemy::DrawSprite() {
	if (isOnScreen_ && targetSprite_) {
		targetSprite_->Draw();
	}
}

// ... (UpdateScreenPosition 関数はご提示のコードのままでOK) ...
void Enemy::UpdateScreenPosition() {
	if (!camera_ || !targetSprite_) {
		isOnScreen_ = false;
		return;
	}

	const KamataEngine::Matrix4x4& viewMatrix = camera_->matView;
	const KamataEngine::Matrix4x4& projMatrix = camera_->matProjection;

	// 1. ワールド -> ビュー座標変換
	KamataEngine::Vector3 worldPos = GetWorldPosition();
	KamataEngine::Vector3 viewPos;
	viewPos.x = worldPos.x * viewMatrix.m[0][0] + worldPos.y * viewMatrix.m[1][0] + worldPos.z * viewMatrix.m[2][0] + 1.0f * viewMatrix.m[3][0];
	viewPos.y = worldPos.x * viewMatrix.m[0][1] + worldPos.y * viewMatrix.m[1][1] + worldPos.z * viewMatrix.m[2][1] + 1.0f * viewMatrix.m[3][1];
	viewPos.z = worldPos.x * viewMatrix.m[0][2] + worldPos.y * viewMatrix.m[1][2] + worldPos.z * viewMatrix.m[2][2] + 1.0f * viewMatrix.m[3][2];

	// 2. ビュー -> クリップ座標変換 & 画面内判定
	if (viewPos.z > 0.0f) { // カメラの前方か
		float clipX = viewPos.x * projMatrix.m[0][0] + viewPos.y * projMatrix.m[1][0] + viewPos.z * projMatrix.m[2][0] + 1.0f * projMatrix.m[3][0];
		float clipY = viewPos.x * projMatrix.m[0][1] + viewPos.y * projMatrix.m[1][1] + viewPos.z * projMatrix.m[2][1] + 1.0f * projMatrix.m[3][1];
		float w_clip = viewPos.x * projMatrix.m[0][3] + viewPos.y * projMatrix.m[1][3] + viewPos.z * projMatrix.m[2][3] + 1.0f * projMatrix.m[3][3];

		if (w_clip > 0.0f) { // w が 0 より大きいか
			float ndcX = clipX / w_clip;
			float ndcY = clipY / w_clip;

			// 画面内 (NDC -1～1) か
			if (ndcX >= -1.0f && ndcX <= 1.0f && ndcY >= -1.0f && ndcY <= 1.0f) {
				isOnScreen_ = true; // ★ 画面内
				// 4. NDC -> スクリーン座標変換
				float screenX = (ndcX + 1.0f) * 0.5f * KamataEngine::WinApp::kWindowWidth;
				float screenY = (1.0f - ndcY) * 0.5f * KamataEngine::WinApp::kWindowHeight;
				targetSprite_->SetPosition({screenX, screenY});
			} else {
				isOnScreen_ = false; // ★ 画面外 (NDC範囲外)
			}
		} else {
			isOnScreen_ = false; // ★ 画面外 (w_clip <= 0)
		}
	} else {
		isOnScreen_ = false; // ★ 画面外 (カメラの後ろ)
	}

	// --- ▼▼▼ ロックオン演出の計算 ▼▼▼ ---

	bool justAppeared = (isOnScreen_ && !wasOnScreenLastFrame_);

	if (justAppeared) {
		lockOnAnimRotation_ = KamataEngine::MathUtility::PI * 1.0f;
		lockOnAnimScale_ = 2.0f;
	}

	const float kLockOnAnimFriction = 0.90f;
	const float kTargetScale = 1.0f;

	if (lockOnAnimRotation_ > 0.0f) {
		lockOnAnimRotation_ *= kLockOnAnimFriction;
		if (lockOnAnimRotation_ < 0.01f) {
			lockOnAnimRotation_ = 0.0f;
		}
	}

	if (lockOnAnimScale_ != kTargetScale) {
		lockOnAnimScale_ += (kTargetScale - lockOnAnimScale_) * (1.0f - kLockOnAnimFriction);
		if (std::abs(lockOnAnimScale_ - kTargetScale) < 0.01f) {
			lockOnAnimScale_ = kTargetScale;
		}
	}

	if (targetSprite_) {
		targetSprite_->SetRotation(lockOnAnimRotation_);
		const float baseSize = 50.0f;
		targetSprite_->SetSize({baseSize * lockOnAnimScale_, baseSize * lockOnAnimScale_});
	}

	wasOnScreenLastFrame_ = isOnScreen_;
}

void Enemy::SetParent(const KamataEngine::WorldTransform* parent) { worldtransfrom_.parent_ = parent; }