#include "GaneScene.h"
#include "PlayerState.h"
#include <algorithm>

// タイトル画面の更新本体
void GameScene::UpdateStateBody_Start() {
}

// 画面遷移（拡大）の更新本体
void GameScene::UpdateStateBody_TransitionToGame() {
	transitionTimer_++;
	float maxScale = sqrtf(powf(WinApp::kWindowWidth, 2) + powf(WinApp::kWindowHeight, 2));
	float progress = std::fmin(transitionTimer_ / kTransitionTime, 1.0f);
	float easedProgress = 1.0f - cosf(progress * 3.14159265f / 2.0f);
	float scale = easedProgress * maxScale;
	transitionSprite_->SetSize({scale, scale});
}

// 画面遷移（縮小）の更新本体
void GameScene::UpdateStateBody_TransitionFromGame() {
	transitionTimer_++;
	float maxScale = sqrtf(powf(WinApp::kWindowWidth, 2) + powf(WinApp::kWindowHeight, 2));
	float progress = std::fmin(transitionTimer_ / kTransitionTime, 1.0f);
	float easedProgress = sinf(progress * 3.14159265f / 2.0f);
	float scale = (1.0f - easedProgress) * maxScale;
	transitionSprite_->SetSize({scale, scale});

	if (railCamera_) {
		railCamera_->Update();
		cameraPositionAnchor_.translation_ = railCamera_->GetWorldTransform().translation_;
		cameraPositionAnchor_.UpdateMatrix();
	}
	if (player_) {
		player_->RefreshWorldMatrix();
	}
	camera_.matView = railCamera_->GetViewProjection().matView;
	camera_.matProjection = railCamera_->GetViewProjection().matProjection;
	camera_.TransferMatrix();
}

// ゲーム開始前イントロの更新本体
void GameScene::UpdateStateBody_GameIntro() {
	gameIntroTimer_++;

	float t = gameIntroTimer_ / kGameIntroDuration_;
	t = 1.0f - std::pow(1.0f - t, 3.0f);
	t = std::clamp(t, 0.0f, 1.0f);

	if (player_) {
		player_->SetPosition(Lerp(playerIntroStartPosition_, playerIntroTargetPosition_, t));
		player_->RefreshWorldMatrix();
	}

	UpdateAimAssist();
	if (railCamera_) {
		railCamera_->Update();
	}

	if (explosionEmitter_) {
		explosionEmitter_->Update();
	}

	cameraPositionAnchor_.translation_ = railCamera_->GetWorldTransform().translation_;
	cameraPositionAnchor_.UpdateMatrix();
	camera_.matView = railCamera_->GetViewProjection().matView;
	camera_.matProjection = railCamera_->GetViewProjection().matProjection;
	camera_.TransferMatrix();
}

// 本編ゲームプレイの更新本体
void GameScene::UpdateStateBody_Game() {
	if (isGameIntroFinished_) {
		const float kDeltaSecGame = 1.0f / 60.0f;
		gameSceneTimer_ += kDeltaSecGame;
		const float kAutoGameOverSeconds = 180.0f; // 3分
		if (gameSceneTimer_ >= kAutoGameOverSeconds) {
			TransitionToClearScene2();
			return;
		}
	}

	// 飛翔状態をカメラに通知（飛翔中=水平追従、着地後=ゴール方向へ復帰）
	railCamera_->SetBallFlying(player_->IsFlying());
	railCamera_->Update();
	// カメラの現在ヨー角をプレイヤーに渡す（照準矢印の基準方向に使用）
	player_->SetCameraYaw(railCamera_->GetCurrentYaw());
	// ゴール位置をプレイヤーに渡す（ゴール方向インジケーター矢印に使用）
	player_->SetGoalPosition(railCamera_->GetGoalPosition());
	cameraPositionAnchor_.translation_ = railCamera_->GetWorldTransform().translation_;
	cameraPositionAnchor_.UpdateMatrix();
	camera_.matView = railCamera_->GetViewProjection().matView;
	camera_.matProjection = railCamera_->GetViewProjection().matProjection;
	camera_.TransferMatrix();

	UpdateAimAssist();

	if (explosionEmitter_) {
		explosionEmitter_->Update();
	}

	if (isGameIntroFinished_) {
		// ゴルフモード: 流星・敵弾・ホーミング生成は全て無効化
		// （敵は「ゴルフの穴」として当たり判定のみ使用）

		player_->Update();

		for (Enemy* enemy : enemies_) {
			enemy->Update();
		}

		// 既存の敵弾があれば全て削除（残骸クリーン）
		for (EnemyBullet* bullet : enemyBullets_) {
			delete bullet;
		}
		enemyBullets_.clear();

		CheckAllCollisions();

		// ゴルフ: ゴールまでの残り距離（メートル）を毎フレーム更新
		if (player_ && railCamera_) {
			const KamataEngine::Vector3 ballPos = player_->GetWorldPosition();
			const KamataEngine::Vector3 goalPos = railCamera_->GetGoalPosition();
			const float dx = goalPos.x - ballPos.x;
			const float dz = goalPos.z - ballPos.z;
			int remaining = static_cast<int>(std::sqrtf(dx * dx + dz * dz));
			remaining = (std::max)(0, remaining);
			if (remaining != score_) {
				score_ = remaining;
				UpdateScoreSprites();
			}
		}

		if (player_ && minimapPlayerSprite_) {
			KamataEngine::Vector3 playerPos = player_->GetWorldPosition();

			// 左上アンカー基準でミニマップ中心を計算（ゴルフ: 左上配置）
			KamataEngine::Vector2 minimapCenterPos = {kMinimapPosition_.x + kMinimapSize_.x * 0.5f, kMinimapPosition_.y + kMinimapSize_.y * 0.5f};
			minimapPlayerSprite_->SetPosition(minimapCenterPos);

			// ミニマップ矢印: ボール→ゴールの方向を直接計算
			// ミニマップ座標系: +Z → 上方向, +X → 右方向
			// atan2(-dgz, dgx) がミニマップ上の角度、+π/2 でスプライトのデフォルト上向きに補正
			const float kPI = 3.14159265f;
			KamataEngine::Vector3 goalPos = railCamera_->GetGoalPosition();
			float dgx = goalPos.x - playerPos.x;
			float dgz = goalPos.z - playerPos.z;
			float goalAngle = std::atan2(-dgz, dgx);
			minimapPlayerSprite_->SetRotation(goalAngle + kPI * 0.5f);
			lastPlayerPos_ = playerPos;

			UpdateMinimapPonds(playerPos);

			size_t activeEnemyCount = 0;
			for (Enemy* enemy : enemies_) {
				if (enemy && !enemy->IsDead() && activeEnemyCount < kMaxMinimapEnemies_) {
					KamataEngine::Vector3 enemyPos = enemy->GetWorldPosition();
					KamataEngine::Vector2 minimapPos = ConvertWorldToMinimap(enemyPos, playerPos);
					float goalMarkerSize = 8.0f;
					ClampMinimapSpriteMarker(minimapPos, goalMarkerSize);
					KamataEngine::Sprite* goalIcon = minimapEnemySprites_[activeEnemyCount];
					goalIcon->SetPosition(minimapPos);
					goalIcon->SetSize({goalMarkerSize, goalMarkerSize});
					// greenBox は緑チャンネルのみなので (1,0,0) 乗算では黒になる → 赤テクスチャを使用
					if (minimapEnemyBulletTextureHandle_ != 0) {
						goalIcon->SetTextureHandle(minimapEnemyBulletTextureHandle_);
					}
					goalIcon->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
					activeEnemyCount++;
				}
			}

			size_t activeBulletCount = 0;
			for (EnemyBullet* eb : enemyBullets_) {
				if (!eb || eb->IsDead())
					continue;
				if (activeBulletCount >= kMaxMinimapEnemyBullets_)
					break;
				KamataEngine::Vector3 bpos = eb->GetWorldPosition();
				KamataEngine::Vector2 bmin = ConvertWorldToMinimap(bpos, playerPos);
				float bulletMarkerSize = 6.0f;
				ClampMinimapSpriteMarker(bmin, bulletMarkerSize);
				minimapEnemyBulletSprites_[activeBulletCount]->SetPosition(bmin);
				minimapEnemyBulletSprites_[activeBulletCount]->SetSize({bulletMarkerSize, bulletMarkerSize});
				activeBulletCount++;
			}

			for (size_t i = activeEnemyCount; i < kMaxMinimapEnemies_; ++i) {
				minimapEnemySprites_[i]->SetPosition({-100.0f, -100.0f});
			}
			for (size_t i = activeBulletCount; i < kMaxMinimapEnemyBullets_; ++i) {
				minimapEnemyBulletSprites_[i]->SetPosition({-100.0f, -100.0f});
			}
		}
	} else {
		if (player_) {
			player_->RefreshWorldMatrix();
		}
	}
}

// クリア演出の更新本体
void GameScene::UpdateStateBody_Clear() {
	if (!confettiActive_) {
		confettiActive_ = true;
		confettiSpawnTimer_ = 0;
	}

	if (confettiActive_) {
		confettiSpawnTimer_++;
		if (confettiSpawnTimer_ >= 3) {
			confettiSpawnTimer_ = 0;
			for (int s = 0; s < 6; ++s) {
				for (auto& c : confettiParticles_) {
					if (!c.active && c.sprite) {
						float x = static_cast<float>(std::rand()) / RAND_MAX * (float)WinApp::kWindowWidth;
						float y = -20.0f;
						c.pos = {x, y};
						c.vel = {(static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * 1.5f, 1.5f + static_cast<float>(std::rand()) / RAND_MAX * 2.0f};
						c.rotation = (static_cast<float>(std::rand()) / RAND_MAX) * 6.28f;
						c.rotVel = (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * 0.2f;
						c.life = 120 + (MT::GetRand() % 120);
						c.age = 0;
						c.active = true;

						float r, g, b;
						int pattern = std::rand() % 6;
						float randomValue = static_cast<float>(std::rand()) / RAND_MAX;
						switch (pattern) {
						case 0:
							r = 1.0f;
							g = randomValue;
							b = 0.0f;
							break;
						case 1:
							r = randomValue;
							g = 1.0f;
							b = 0.0f;
							break;
						case 2:
							r = 0.0f;
							g = 1.0f;
							b = randomValue;
							break;
						case 3:
							r = 0.0f;
							g = randomValue;
							b = 1.0f;
							break;
						case 4:
							r = randomValue;
							g = 0.0f;
							b = 1.0f;
							break;
						default:
							r = 1.0f;
							g = 0.0f;
							b = randomValue;
							break;
						}
						c.sprite->SetColor({r, g, b, 1.0f});
						c.sprite->SetPosition(c.pos);
						c.sprite->SetRotation(c.rotation);
						break;
					}
				}
			}
		}
	}

	for (auto& c : confettiParticles_) {
		if (!c.active || !c.sprite)
			continue;
		c.age++;
		c.pos.x += c.vel.x;
		c.pos.y += c.vel.y;
		c.vel.y += 0.02f;
		c.rotation += c.rotVel;
		c.sprite->SetPosition(c.pos);
		c.sprite->SetRotation(c.rotation);
		if (c.age > c.life) {
			c.active = false;
			c.sprite->SetPosition({-100.0f, -100.0f});
		}
	}
}

// ゲームオーバー（演出なし・スプライト表示のみ）
void GameScene::UpdateStateBody_Over() {
	if (railCamera_) {
		cameraPositionAnchor_.translation_ = railCamera_->GetWorldTransform().translation_;
		cameraPositionAnchor_.UpdateMatrix();
		camera_.matView = railCamera_->GetViewProjection().matView;
		camera_.matProjection = railCamera_->GetViewProjection().matProjection;
		camera_.TransferMatrix();
	}
}

// タイトルへ戻るときの共通リセット
void GameScene::ResetToTitle() {
	debug10ElapsedSec_ = 0.0f;
	gameOverTimer_ = 0;
	currentStage_ = 1;
	confettiActive_ = false;

	camera_.Initialize();
	camera_.TransferMatrix();

	if (railCamera_) {
		railCamera_->Reset();
	}

	if (player_) {
		player_->ResetStats();
		player_->ResetRotation();
		player_->SetPosition(playerIntroStartPosition_);
		player_->RefreshWorldMatrix();
		player_->ResetParticles();
		player_->ResetBullets();
	}

	for (Enemy* enemy : enemies_) {
		delete enemy;
	}
	enemies_.clear();
	for (EnemyBullet* bullet : enemyBullets_) {
		delete bullet;
	}
	enemyBullets_.clear();
	for (Meteorite* meteor : meteorites_) {
		delete meteor;
	}
	meteorites_.clear();
	meteoriteSpawnTimer_ = 0;

	for (WaterPond* pond : waterPonds_) {
		delete pond;
	}
	waterPonds_.clear();

	LoadEnemyPopData();
	hasSpawnedEnemies_ = false;
}
