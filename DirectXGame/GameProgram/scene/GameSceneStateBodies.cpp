#include "GaneScene.h"
#include "ConfettiColorTable.h"
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
	UpdateTitleCamera();
}

// 画面遷移（拡大）の更新本体
void GameScene::UpdateStateBody_TransitionToGame() {
	transitionTimer_++;
	float maxScale = sqrtf(powf(WinApp::kWindowWidth, 2) + powf(WinApp::kWindowHeight, 2));
	float progress = std::fmin(transitionTimer_ / kTransitionTime, 1.0f);
	float easedProgress = 1.0f - cosf(progress * 3.14159265f / 2.0f);
	float scale = easedProgress * maxScale;
	transitionSprite_->SetSize({scale, scale});
	UpdateTitleCamera();
}

// 画面遷移（縮小）の更新本体
void GameScene::UpdateStateBody_TransitionFromGame() {
	transitionTimer_++;
	float maxScale = sqrtf(powf(WinApp::kWindowWidth, 2) + powf(WinApp::kWindowHeight, 2));
	float progress = std::fmin(transitionTimer_ / kTransitionTime, 1.0f);
	float easedProgress = sinf(progress * 3.14159265f / 2.0f);
	float scale = (1.0f - easedProgress) * maxScale;
	transitionSprite_->SetSize({scale, scale});

	UpdateMapCamera();
	if (player_) {
		player_->RefreshWorldMatrix();
	}
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

	UpdateMapCamera();

	if (explosionEmitter_) {
		explosionEmitter_->Update();
	}
}

// 本編ゲームプレイの更新本体
void GameScene::UpdateStateBody_Game() {
	UpdateCameraControl();
	UpdateMapCamera();
	UpdateTrampolinePlacement();

	if (explosionEmitter_) {
		explosionEmitter_->Update();
	}

	if (isGameIntroFinished_) {
		player_->Update();
		UpdatePlayerScreenTransition();
		UpdateMapCamera();

		if (player_ && minimapPlayerSprite_) {
			KamataEngine::Vector3 playerPos = player_->GetWorldPosition();
			minimapPlayerSprite_->SetPosition(ConvertWorldToMinimapPosition(playerPos));

			float dx = playerPos.x - lastPlayerPos_.x;
			float dy = playerPos.y - lastPlayerPos_.y;
			const float kMoveThresholdSq = 0.0001f;
			float moveDistSq = dx * dx + dy * dy;
			if (moveDistSq > kMoveThresholdSq) {
				float angle = std::atan2(dy, dx);
				const float kPI = 3.14159265f;
				minimapPlayerSprite_->SetRotation(angle + kPI / 2.0f);
				lastPlayerPos_ = playerPos;
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
	UpdateTitleCamera();

	if (!confettiActive_) {
		confettiActive_ = true;
		confettiSpawnTimer_ = 0;
	}

	if (confettiActive_) {
		confettiSpawnTimer_++;
		const int spawnInterval = balanceTable_.GetInt("confettiSpawnInterval", 3);
		const int batchCount = balanceTable_.GetInt("confettiBatchCount", 6);
		if (confettiSpawnTimer_ >= spawnInterval) {
			confettiSpawnTimer_ = 0;
			for (int s = 0; s < batchCount; ++s) {
				for (auto& c : confettiParticles_) {
					if (!c.active && c.sprite) {
						float x = static_cast<float>(std::rand()) / RAND_MAX * (float)WinApp::kWindowWidth;
						float y = -20.0f;
						c.pos = {x, y};
						c.vel = {(static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * 1.5f, 1.5f + static_cast<float>(std::rand()) / RAND_MAX * 2.0f};
						c.rotation = (static_cast<float>(std::rand()) / RAND_MAX) * 6.28f;
						c.rotVel = (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * 0.2f;
						const int lifeMin = balanceTable_.GetInt("confettiLifeMin", 120);
						const int lifeRange = balanceTable_.GetInt("confettiLifeRange", 120);
						c.life = lifeMin + (MT::GetRand() % lifeRange);
						c.age = 0;
						c.active = true;

						const ConfettiColorPattern& pattern = PickConfettiColorPattern(std::rand() % static_cast<int>(kConfettiColorPatterns.size()));
						float randomValue = static_cast<float>(std::rand()) / RAND_MAX;
						KamataEngine::Vector4 color = MakeConfettiColor(pattern, randomValue);
						c.sprite->SetColor(color);
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
	gameOverTimer_ = 0;
	trampolineSprings_.clear();
	hasTrampolinePreview_ = false;
	nextTrampolineTypeIndex_ = 0;

	camera_.Initialize();
	UpdateTitleCamera();

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
