#include "GaneScene.h"
#include "PlayerState.h"
#include <algorithm>

void GameScene::UpdateStateBody_Start() {}

void GameScene::UpdateStateBody_TransitionToGame() {
	transitionTimer_++;
	float maxScale = sqrtf(powf(WinApp::kWindowWidth, 2) + powf(WinApp::kWindowHeight, 2));
	float progress = std::fmin(transitionTimer_ / kTransitionTime, 1.0f);
	float easedProgress = 1.0f - cosf(progress * 3.14159265f / 2.0f);
	float scale = easedProgress * maxScale;
	transitionSprite_->SetSize({scale, scale});
}

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

void GameScene::UpdateStateBody_GameIntro() {
	// 振り子ゲーム: イントロ省略
	if (player_) {
		player_->ResetStats();
		player_->RefreshWorldMatrix();
	}
	isGameIntroFinished_ = true;
	gameSceneTimer_ = 0;
	if (railCamera_) {
		railCamera_->SetCanMove(true);
		railCamera_->Update();
	}
	if (player_) {
		ballStartZ_ = player_->GetProgressZ();
		score_ = 0;
		UpdateScoreSprites();
		SpawnNextRing(true);
	}
	cameraPositionAnchor_.translation_ = railCamera_->GetWorldTransform().translation_;
	cameraPositionAnchor_.UpdateMatrix();
	camera_.matView = railCamera_->GetViewProjection().matView;
	camera_.matProjection = railCamera_->GetViewProjection().matProjection;
	camera_.TransferMatrix();
}

void GameScene::UpdateStateBody_Game() {
	railCamera_->SetBallFlying(player_->IsFlying());
	railCamera_->Update();
	cameraPositionAnchor_.translation_ = railCamera_->GetWorldTransform().translation_;
	cameraPositionAnchor_.UpdateMatrix();
	camera_.matView = railCamera_->GetViewProjection().matView;
	camera_.matProjection = railCamera_->GetViewProjection().matProjection;
	camera_.TransferMatrix();

	if (explosionEmitter_) {
		explosionEmitter_->Update();
	}

	if (!isGameIntroFinished_) {
		if (player_) {
			player_->RefreshWorldMatrix();
		}
		return;
	}

	player_->Update();

	UpdateRing();

	// 画面下落下ゲームオーバー（いったん無効）
	/*
	if (railCamera_ && player_) {
		const float bottomY = railCamera_->GetFixedFocusY() - railCamera_->GetOrthoHalfHeight();
		if (player_->GetBobPosition().y < bottomY) {
			TransitionToClearScene2();
			return;
		}
	}
	*/

	for (EnemyBullet* bullet : enemyBullets_) {
		delete bullet;
	}
	enemyBullets_.clear();

	// 距離スコア（右方向の最大到達 Z）
	int distance = static_cast<int>((std::max)(0.0f, player_->GetProgressZ() - ballStartZ_));
	if (distance != score_) {
		score_ = distance;
		UpdateScoreSprites();
	}

	if (skydome_) {
		const KamataEngine::Vector3 focus = player_->GetCameraFocusPosition();
		skydome_->SetPositionXZ(focus.x, focus.z);
	}
}

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
						float r = 1.0f, g = 0.5f, b = 0.2f;
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

void GameScene::UpdateStateBody_Over() {
	if (railCamera_) {
		cameraPositionAnchor_.translation_ = railCamera_->GetWorldTransform().translation_;
		cameraPositionAnchor_.UpdateMatrix();
		camera_.matView = railCamera_->GetViewProjection().matView;
		camera_.matProjection = railCamera_->GetViewProjection().matProjection;
		camera_.TransferMatrix();
	}
}

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
	ResetRing();
}
