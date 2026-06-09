#include "GaneScene.h"
#include "PlayerState.h"
#include <algorithm>

// タイトル画面の更新本体
void GameScene::UpdateStateBody_Start() {
	titleAnimationTimer_++;
	const int32_t cycleFrames = kTitleRotateFrames + kTitlePauseFrames;
	int32_t timeInCycle = titleAnimationTimer_ % cycleFrames;
	if (timeInCycle < kTitleRotateFrames) {
		float progress = static_cast<float>(timeInCycle) / kTitleRotateFrames;
		float easedProgress = (1.0f - cosf(progress * 3.14159265f)) / 2.0f;
		worldTransformTitleObject_.rotation_.y = easedProgress * (2.0f * 3.14159265f);
	} else {
		worldTransformTitleObject_.rotation_.y = 0.0f;
	}
	worldTransformTitleObject_.UpdateMatrix();
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

	// 飛翔状態をカメラに通知（飛翔中=水平追従、着地後=正面へ復帰）
	railCamera_->SetBallFlying(player_->IsFlying());
	railCamera_->Update();
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

		// ゴルフ: ボールの Z 飛距離をスコア（メートル）として毎フレーム更新
		if (player_) {
			float distZ = player_->GetWorldPosition().z - ballStartZ_;
			int distMeters = static_cast<int>(std::fmax(0.0f, distZ));
			if (distMeters > score_) {
				score_ = distMeters;
				UpdateScoreSprites();
			}
		}

		if (player_ && minimapPlayerSprite_) {
			KamataEngine::Vector3 playerPos = player_->GetWorldPosition();

			// 左上アンカー基準でミニマップ中心を計算（ゴルフ: 左上配置）
			KamataEngine::Vector2 minimapCenterPos = {kMinimapPosition_.x + kMinimapSize_.x * 0.5f, kMinimapPosition_.y + kMinimapSize_.y * 0.5f};
			minimapPlayerSprite_->SetPosition(minimapCenterPos);

			float dx = playerPos.x - lastPlayerPos_.x;
			float dz = playerPos.z - lastPlayerPos_.z;
			const float kMoveThresholdSq = 0.0001f;
			float moveDistSq = dx * dx + dz * dz;
			if (moveDistSq > kMoveThresholdSq) {
				float mx = dx;
				float my = -dz;
				float angle = std::atan2(my, mx);
				const float kPI = 3.14159265f;
				minimapPlayerSprite_->SetRotation(angle + kPI / 2.0f);
				lastPlayerPos_ = playerPos;
			}

			size_t activeEnemyCount = 0;
			for (Enemy* enemy : enemies_) {
				if (enemy && !enemy->IsDead() && activeEnemyCount < kMaxMinimapEnemies_) {
					KamataEngine::Vector3 enemyPos = enemy->GetWorldPosition();
					KamataEngine::Vector2 minimapPos = ConvertWorldToMinimap(enemyPos, playerPos);
					minimapEnemySprites_[activeEnemyCount]->SetPosition(minimapPos);
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
				minimapEnemyBulletSprites_[activeBulletCount]->SetPosition(bmin);
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

// ゲームオーバー演出の更新本体
void GameScene::UpdateStateBody_Over() {
	gameOverTimer_++;

	if (player_) {
		// ゲームオーバー演出は Dead 状態が担当
		if (player_->GetState() != PlayerStateDead::Instance()) {
			player_->ChangeState(PlayerStateDead::Instance());
		}
		player_->SetGameOverAnimationTime(static_cast<float>(gameOverTimer_));
		player_->Update();
	}

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

	LoadEnemyPopData();
	hasSpawnedEnemies_ = false;
}
