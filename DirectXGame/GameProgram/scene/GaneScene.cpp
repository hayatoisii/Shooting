#include "GaneScene.h"
#include "3d/AxisIndicator.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <fstream>

KamataEngine::Vector3 Lerp(const KamataEngine::Vector3& start, const KamataEngine:: Vector3& end, float t) {
	t = std::clamp(t, 0.0f, 1.0f);
	return start + (end - start) * t;
}
float Distance(const KamataEngine::Vector3& v1, const KamataEngine::Vector3& v2) {
	float dx = v1.x - v2.x;
	float dy = v1.y - v2.y;
	float dz = v1.z - v2.z;
	return std::sqrt(dx * dx + dy * dy + dz * dz);
}
float GameScene::DistanceSquared(const Vector3& v1, const Vector3& v2) {
	float dx = v1.x - v2.x;
	float dy = v1.y - v2.y;
	float dz = v1.z - v2.z;
	return dx * dx + dy * dy + dz * dz;
}

GameScene::GameScene() {}

GameScene::~GameScene() {
	delete modelPlayer_;
	delete modelEnemy_;
	delete modelSkydome_;
	delete modelTitleObject_;
	delete modelMeteorite_;
	for (Meteorite* meteor : meteorites_) {
		delete meteor;
	}
	delete player_;
	delete skydome_;
	delete railCamera_;
	delete reticleSprite_;
	delete transitionSprite_;
	delete taitoruSprite_;
	delete aimAssistCircleSprite_;
	delete explosionEmitter_;
	delete modelParticle_;
	delete minimapSprite_;
	delete minimapPlayerSprite_;
	// clear scene
	delete clearEmitter_;
	delete clearSprite_;
	for (KamataEngine::Sprite* sprite : minimapEnemySprites_) {
		delete sprite;
	}
	minimapEnemySprites_.clear();
	for (KamataEngine::Sprite* sprite : minimapEnemyBulletSprites_) {
		delete sprite;
	}
	minimapEnemyBulletSprites_.clear();
	for (EnemyBullet* bullet : enemyBullets_) {
		delete bullet;
	}
	for (Enemy* enemy : enemies_) {
		delete enemy;
	}
}

void GameScene::Initialize() {
	dxCommon_ = DirectXCommon::GetInstance();
	input_ = Input::GetInstance();
	audio_ = Audio::GetInstance();

	player_ = new Player();
	skydome_ = new Skydome();

	modelPlayer_ = KamataEngine::Model::CreateFromOBJ("fly2", true);
	modelEnemy_ = KamataEngine::Model::CreateFromOBJ("cube", true);
	modelSkydome_ = Model::CreateFromOBJ("skydome", true);
	modelTitleObject_ = Model::CreateFromOBJ("title", true);

	modelMeteorite_ = KamataEngine::Model::CreateFromOBJ("meteorite", true);
	meteoriteSpawnTimer_ = 0;

	transitionTextureHandle_ = KamataEngine::TextureManager::Load("black.png");
	transitionSprite_ = KamataEngine::Sprite::Create(transitionTextureHandle_, {0, 0});
	KamataEngine::Vector2 screenCenter = {WinApp::kWindowWidth / 2.0f, WinApp::kWindowHeight / 2.0f};
	transitionSprite_->SetPosition(screenCenter);
	transitionSprite_->SetAnchorPoint({0.5f, 0.5f});
	transitionSprite_->SetSize({0.0f, 0.0f});

	reticleTextureHandle_ = KamataEngine::TextureManager::Load("reticle.png");
	reticleSprite_ = KamataEngine::Sprite::Create(reticleTextureHandle_, {0, 0});
	reticleSprite_->SetPosition(screenCenter);
	reticleSprite_->SetAnchorPoint({0.5f, 0.5f});

	taitoruTextureHandle_ = KamataEngine::TextureManager::Load("sousa.png");
	taitoruSprite_ = KamataEngine::Sprite::Create(taitoruTextureHandle_, {0, 0});

	aimAssistCircleTextureHandle_ = KamataEngine::TextureManager::Load("aimCircle.png");
	aimAssistCircleSprite_ = KamataEngine::Sprite::Create(aimAssistCircleTextureHandle_, {0, 0});
	aimAssistCircleSprite_->SetSize({0.0f, 0.0f});

	modelParticle_ = KamataEngine::Model::CreateFromOBJ("flare", true);
	explosionEmitter_ = new ParticleEmitter();
	if (explosionEmitter_) {
		explosionEmitter_->Initialize(modelParticle_);
	}

	// Clear scene assets
	clearTextureHandle_ = KamataEngine::TextureManager::Load("kuria.png");
	clearSprite_ = KamataEngine::Sprite::Create(clearTextureHandle_, {0, 0});
	if (clearSprite_) {
		// Make kuria.png cover the whole screen
		clearSprite_->SetAnchorPoint({0.0f, 0.0f});
		clearSprite_->SetPosition({0.0f, 0.0f});
		clearSprite_->SetSize({(float)WinApp::kWindowWidth, (float)WinApp::kWindowHeight});
	}

	clearEmitter_ = new ParticleEmitter();
	if (clearEmitter_) {
		clearEmitter_->Initialize(modelParticle_);
	}

	// Confetti sprite texture
	confettiTextureHandle_ = KamataEngine::TextureManager::Load("confetti.png");
	confettiParticles_.resize(kMaxConfetti_);
	for (size_t i = 0; i < kMaxConfetti_; ++i) {
		confettiParticles_[i].sprite = KamataEngine::Sprite::Create(confettiTextureHandle_, {0, 0});
		if (confettiParticles_[i].sprite) {
			confettiParticles_[i].sprite->SetSize({8.0f, 8.0f});
			confettiParticles_[i].sprite->SetAnchorPoint({0.5f, 0.5f});
			confettiParticles_[i].active = false;
			// default color white
			confettiParticles_[i].sprite->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
		}
	}

	if (aimAssistCircleSprite_) {
		aimAssistCircleSprite_->SetPosition(screenCenter);    // 画面中央
		aimAssistCircleSprite_->SetAnchorPoint({0.5f, 0.5f}); // 中央基点

		// (例: スプライトを少し半透明にする)
		aimAssistCircleSprite_->SetColor({1.0f, 1.0f, 1.0f, 0.5f});
	}

	minimapTextureHandle_ = KamataEngine::TextureManager::Load("minimap.png");
	greenBoxTextureHandle_ = KamataEngine::TextureManager::Load("greenBox.png");
	minimapPlayerTextureHandle_ = KamataEngine::TextureManager::Load("player.png");
	minimapEnemyBulletTextureHandle_ = KamataEngine::TextureManager::Load("missileRedBox.png");

	// 1. ミニマップ背景
	minimapSprite_ = KamataEngine::Sprite::Create(minimapTextureHandle_, {0, 0});
	minimapSprite_->SetPosition(kMinimapPosition_);
	minimapSprite_->SetAnchorPoint({0.0f, 1.0f}); // 左下をアンカーに
	minimapSprite_->SetSize(kMinimapSize_);

	// 2. ミニマップ上の自機
	minimapPlayerSprite_ = KamataEngine::Sprite::Create(minimapPlayerTextureHandle_, {0, 0});
	minimapPlayerSprite_->SetAnchorPoint({0.5f, 0.5f}); // 中央をアンカーに
	minimapPlayerSprite_->SetSize({10.0f, 10.0f});      // 仮サイズ

	// 3. ミニマップ上の敵 (あらかじめ最大数作成し、非表示にしておく)
	minimapEnemySprites_.resize(kMaxMinimapEnemies_);
	for (size_t i = 0; i < kMaxMinimapEnemies_; ++i) {
		minimapEnemySprites_[i] = KamataEngine::Sprite::Create(greenBoxTextureHandle_, {0, 0});
		minimapEnemySprites_[i]->SetAnchorPoint({0.5f, 0.5f});
		minimapEnemySprites_[i]->SetSize({8.0f, 8.0f});           // 敵は少し小さく
		minimapEnemySprites_[i]->SetPosition({-100.0f, -100.0f}); // 初期位置は画面外
	}

	// 4. ミニマップ上の敵弾 (あらかじめ最大数作成し、非表示にしておく)
	minimapEnemyBulletSprites_.resize(kMaxMinimapEnemyBullets_);
	for (size_t i = 0; i < kMaxMinimapEnemyBullets_; ++i) {
		minimapEnemyBulletSprites_[i] = KamataEngine::Sprite::Create(minimapEnemyBulletTextureHandle_, {0, 0});
		minimapEnemyBulletSprites_[i]->SetAnchorPoint({0.5f, 0.5f});
		minimapEnemyBulletSprites_[i]->SetSize({6.0f, 6.0f});
		minimapEnemyBulletSprites_[i]->SetPosition({-100.0f, -100.0f});
	}

	taitoruSprite_->SetPosition(screenCenter); // 画面中央に配置
	taitoruSprite_->SetAnchorPoint({0.5f, 0.5f});

	camera_.Initialize();

	playerIntroTargetPosition_ = {0.0f, -3.0f, 20.0f};
	playerIntroStartPosition_ = playerIntroTargetPosition_;
	playerIntroStartPosition_.z += -50.0f;

	player_->Initialize(modelPlayer_, &camera_, playerIntroStartPosition_);
	// Initialize last player position for minimap rotation tracking
	lastPlayerPos_ = player_->GetWorldPosition();

	skydome_->Initialize(modelSkydome_, &camera_);
	worldTransformTitleObject_.Initialize();
	worldTransformTitleObject_.translation_ = {0.0f, 0.0f, -43.0f};
	worldTransformTitleObject_.UpdateMatrix();

	KamataEngine::AxisIndicator::GetInstance()->SetVisible(true);

	railCamera_ = new RailCamera();
	railCamera_->Initialize(railcameraPos, railcameraRad);
	cameraPositionAnchor_.Initialize();
	player_->SetParent(&railCamera_->GetWorldTransform());
	player_->SetRailCamera(railCamera_);
	player_->SetEnemies(&enemies_);

	LoadEnemyPopData();
	hitSoundHandle_ = audio_->LoadWave("./sound/parry.wav");

	// init homing timer
	homingSpawnTimer_ = kHomingIntervalFrames_; // start timer so first shot occurs after interval
}

void GameScene::Update() {

	skydome_->Update();

	switch (sceneState) {
	case SceneState::Start: {
		if (input_->TriggerKey(DIK_SPACE)) {
			sceneState = SceneState::TransitionToGame;
			transitionTimer_ = 0.0f;
			gameSceneTimer_ = 0;
		}
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
		break;
	}
	case SceneState::TransitionToGame: {
		transitionTimer_++;
		float maxScale = sqrtf(powf(WinApp::kWindowWidth, 2) + powf(WinApp::kWindowHeight, 2));
		float progress = std::fmin(transitionTimer_ / kTransitionTime, 1.0f);
		float easedProgress = 1.0f - cosf(progress * 3.14159265f / 2.0f);
		float scale = easedProgress * maxScale;
		transitionSprite_->SetSize({scale, scale});
		if (transitionTimer_ >= kTransitionTime) {
			sceneState = SceneState::TransitionFromGame;
			transitionTimer_ = 0.0f;
		}
		break;
	}
	case SceneState::TransitionFromGame: {
		transitionTimer_++;
		float maxScale = sqrtf(powf(WinApp::kWindowWidth, 2) + powf(WinApp::kWindowHeight, 2));
		float progress = std::fmin(transitionTimer_ / kTransitionTime, 1.0f);
		float easedProgress = sinf(progress * 3.14159265f / 2.0f);
		float scale = (1.0f - easedProgress) * maxScale;
		transitionSprite_->SetSize({scale, scale});

		if (transitionTimer_ >= kTransitionTime) {
			sceneState = SceneState::GameIntro;
			gameIntroTimer_ = 0.0f;
			player_->GetWorldTransform().translation_ = playerIntroStartPosition_;
			player_->GetWorldTransform().UpdateMatrix();
			isGameIntroFinished_ = false;
			UpdateEnemyPopCommands();
		}

		if (railCamera_) {
			railCamera_->Update();
			// アンカー更新
			cameraPositionAnchor_.translation_ = railCamera_->GetWorldTransform().translation_;
			cameraPositionAnchor_.UpdateMatrix();
		}
		if (player_)
			player_->GetWorldTransform().UpdateMatrix();
		camera_.matView = railCamera_->GetViewProjection().matView;
		camera_.matProjection = railCamera_->GetViewProjection().matProjection;
		camera_.TransferMatrix();
		break;
	}
	case SceneState::GameIntro: {
		gameIntroTimer_++;

		float t = gameIntroTimer_ / kGameIntroDuration_;
		t = 1.0f - std::pow(1.0f - t, 3.0f);
		t = std::clamp(t, 0.0f, 1.0f);

		player_->GetWorldTransform().translation_ = Lerp(playerIntroStartPosition_, playerIntroTargetPosition_, t);
		player_->GetWorldTransform().UpdateMatrix();

		UpdateAimAssist();
		railCamera_->Update();

		if (explosionEmitter_) {
			explosionEmitter_->Update();
		}

		// アンカー更新
		cameraPositionAnchor_.translation_ = railCamera_->GetWorldTransform().translation_;
		cameraPositionAnchor_.UpdateMatrix();
		camera_.matView = railCamera_->GetViewProjection().matView;
		camera_.matProjection = railCamera_->GetViewProjection().matProjection;
		camera_.TransferMatrix();

		const float kArrivalThreshold = 0.1f;
		if (gameIntroTimer_ >= kGameIntroDuration_ || Distance(player_->GetWorldTransform().translation_, playerIntroTargetPosition_) < kArrivalThreshold) {
			player_->GetWorldTransform().translation_ = playerIntroTargetPosition_;
			player_->GetWorldTransform().UpdateMatrix();
			sceneState = SceneState::Game;
			isGameIntroFinished_ = true;
			gameSceneTimer_ = 0;

			// Gameが始まってから移動するようにする
			if (railCamera_) {
				railCamera_->SetCanMove(true);
			}
		}
		break;
	}
	case SceneState::Game: {


		// デバッグ
		//gameSceneTimer_++;

		// --- デバッグ用: 1秒でクリア (60フレーム) ---
		const int DEBUG_CLEAR_TIME = 60; // 1秒
		if (gameSceneTimer_ > DEBUG_CLEAR_TIME) {
			TransitionToClearScene(); // クリア処理を呼び出す
			break;                    // このフレームの残りのGame処理をスキップ
		}


		// デモ用自動復帰タイマー
		if (gameSceneTimer_ > kGameTimeLimit_) {
			sceneState = SceneState::Start;
			camera_.Initialize();
			camera_.TransferMatrix();
			if (railCamera_) {
				railCamera_->Reset();
			}
			if (player_) {
				player_->ResetRotation();
				player_->GetWorldTransform().translation_ = playerIntroStartPosition_;
				player_->GetWorldTransform().UpdateMatrix();
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
			break;
		}

		// --- 通常のゲーム処理 ---
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
			const int kSpawnsPerFrame = 0;
			meteoriteSpawnTimer_--;
			if (meteoriteSpawnTimer_ <= 0) {
				for (int i = 0; i < kSpawnsPerFrame; ++i) {
					// SpawnMeteorite();
				}
				// 隕石の数
				meteoriteSpawnTimer_ = 0;
			}

			player_->EvadeBullets(enemyBullets_);

			for (Enemy* enemy : enemies_) {
				enemy->Update();
			}
			for (EnemyBullet* bullet : enemyBullets_) {
				bullet->Update();
			}

			// HOMING spawn logic
			if (homingSpawnTimer_ > 0) {
				homingSpawnTimer_--;
			} else {
				// Find one enemy within max distance but not too close to player
				Enemy* shooter = nullptr;
				KamataEngine::Vector3 playerPosForHoming = player_->GetWorldPosition();
				float maxDistSq = kHomingMaxDistance_ * kHomingMaxDistance_;
				// この距離にPlayerが近づくとEnemyが弾を撃たなくなります
				const float kMinHomingDistance = 1000.0f;
				float minDistSq = kMinHomingDistance * kMinHomingDistance;
				for (Enemy* enemy : enemies_) {
					if (!enemy || enemy->IsDead())
						continue;
					KamataEngine::Vector3 epos = enemy->GetWorldPosition();
					float dx = epos.x - playerPosForHoming.x;
					float dy = epos.y - playerPosForHoming.y;
					float dz = epos.z - playerPosForHoming.z;
					float distSq = dx * dx + dy * dy + dz * dz;
					// only choose enemy if within max distance and further than min distance
					if (distSq <= maxDistSq && distSq > minDistSq) {
						shooter = enemy;
						break; // use first found
					}
				}

				if (shooter) {
					// create homing bullet
					KamataEngine::Vector3 moveBullet = shooter->GetWorldPosition();
					KamataEngine::Vector3 playerPos = player_->GetWorldPosition();
					KamataEngine::Vector3 toPlayer = playerPos - moveBullet;
					float len = std::sqrt(toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y + toPlayer.z * toPlayer.z);
					if (len > 0.001f) {
						toPlayer.x /= len;
						toPlayer.y /= len;
						toPlayer.z /= len;
					}
					KamataEngine::Vector3 vel = {toPlayer.x * kHomingBulletSpeed_, toPlayer.y * kHomingBulletSpeed_, toPlayer.z * kHomingBulletSpeed_};

					EnemyBullet* newBullet = new EnemyBullet();
					newBullet->Initialize(modelEnemy_, moveBullet, vel);
					newBullet->SetHomingEnabled(true);
					newBullet->SetHomingTarget(player_);
					newBullet->SetSpeed(kHomingBulletSpeed_);
					AddEnemyBullet(newBullet);

					// reset timer
					homingSpawnTimer_ = kHomingIntervalFrames_;
				}
			}

			enemyBullets_.remove_if([](EnemyBullet* bullet) {
				if (bullet && bullet->IsDead()) {
					delete bullet;
					return true;
				}
				return false;
			});
			CheckAllCollisions();
			player_->Update();

			if (player_ && minimapPlayerSprite_) { // player_ が null でないか確認
				KamataEngine::Vector3 playerPos = player_->GetWorldPosition();

				// 1. 自機アイコンをミニマップ中央に設定
				KamataEngine::Vector2 minimapCenterPos = {kMinimapPosition_.x + kMinimapSize_.x * 0.5f, kMinimapPosition_.y - kMinimapSize_.y * 0.5f};
				minimapPlayerSprite_->SetPosition(minimapCenterPos);

				// Rotate the player minimap sprite to match movement direction on XZ plane.
				// Convert player movement (world X,Z) to minimap axes: mx = dx, my = -dz (minimap Y is -world Z).
				float dx = playerPos.x - lastPlayerPos_.x;
				float dz = playerPos.z - lastPlayerPos_.z;
				const float kMoveThresholdSq = 0.0001f; // squared threshold to ignore tiny jitter
				float moveDistSq = dx * dx + dz * dz;
				if (moveDistSq > kMoveThresholdSq) {
					float mx = dx;
					float my = -dz;
					float angle = std::atan2(my, mx);
					// Sprite's default up direction -> adjust by +90 degrees (pi/2)
					const float kPI = 3.14159265f;
					minimapPlayerSprite_->SetRotation(angle + kPI / 2.0f);
					lastPlayerPos_ = playerPos;
				}

				// この処理はミニマップにＥｎｅｍｙを移すために絶対に必要だから消しちゃダメ
				//  2. 敵アイコンの位置を更新
				size_t activeEnemyCount = 0;
				// 敵リスト (enemies_) を走査
				for (Enemy* enemy : enemies_) {
					// 生きていて、スプライトの最大数を超えていない場合
					if (enemy && !enemy->IsDead() && activeEnemyCount < kMaxMinimapEnemies_) {
						KamataEngine::Vector3 enemyPos = enemy->GetWorldPosition(); //
						KamataEngine::Vector2 minimapPos = ConvertWorldToMinimap(enemyPos, playerPos);
						minimapEnemySprites_[activeEnemyCount]->SetPosition(minimapPos);
						activeEnemyCount++;
					}
				}

				// 3. 敵弾アイコンの更新
				size_t activeBulletCount = 0;
				for (EnemyBullet* eb : enemyBullets_) {
					if (!eb || eb->IsDead()) continue;
					if (activeBulletCount >= kMaxMinimapEnemyBullets_) break;
					KamataEngine::Vector3 bpos = eb->GetWorldPosition();
					KamataEngine::Vector2 bmin = ConvertWorldToMinimap(bpos, playerPos);
					minimapEnemyBulletSprites_[activeBulletCount]->SetPosition(bmin);
					activeBulletCount++;
				}

				// 4. 残りのスプライトを非表示（画面外へ）
				for (size_t i = activeEnemyCount; i < kMaxMinimapEnemies_; ++i) {
					minimapEnemySprites_[i]->SetPosition({-100.0f, -100.0f});
				}
				for (size_t i = activeBulletCount; i < kMaxMinimapEnemyBullets_; ++i) {
					minimapEnemyBulletSprites_[i]->SetPosition({-100.0f, -100.0f});
				}
			}

		} else { // イントロ中
			if (player_) {
				player_->GetWorldTransform().UpdateMatrix();
			}
		}

		break;
	}
	case SceneState::Clear:
		// ensure skydome drawn etc handled in Draw
		// Start confetti when entering Clear
		if (!confettiActive_) {
			confettiActive_ = true;
			confettiSpawnTimer_ = 0;

		}

		// spawn sprite confetti from top of screen
		if (confettiActive_) {
			confettiSpawnTimer_++;
			if (confettiSpawnTimer_ >= 3) {
				confettiSpawnTimer_ = 0;
				// spawn a few confetti
				for (int s = 0; s < 6; ++s) {
					for (auto& c : confettiParticles_) {
						if (!c.active && c.sprite) {
							// place at very top across full screen width
							float x = static_cast<float>(std::rand()) / RAND_MAX * (float)WinApp::kWindowWidth;
							float y = -20.0f; // slightly above the top
							c.pos = {x, y};
							c.vel = {(static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * 1.5f, 1.5f + static_cast<float>(std::rand()) / RAND_MAX * 2.0f};
							c.rotation = (static_cast<float>(std::rand()) / RAND_MAX) * 6.28f;
							c.rotVel = (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * 0.2f;
							c.life = 120 + (MT::GetRand() % 120);
							c.age = 0;
							c.active = true;
							// random bright color
							float r, g, b;
							int pattern = std::rand() % 6;                                  // 6パターン
							float randomValue = static_cast<float>(std::rand()) / RAND_MAX; // 0.0f ～ 1.0f

							switch (pattern) {
							case 0:
								r = 1.0f;
								g = randomValue;
								b = 0.0f;
								break; // 赤～黄
							case 1:
								r = randomValue;
								g = 1.0f;
								b = 0.0f;
								break; // 緑～黄
							case 2:
								r = 0.0f;
								g = 1.0f;
								b = randomValue;
								break; // 緑～シアン
							case 3:
								r = 0.0f;
								g = randomValue;
								b = 1.0f;
								break; // 青～シアン
							case 4:
								r = randomValue;
								g = 0.0f;
								b = 1.0f;
								break; // 青～マゼンタ
							default:
								r = 1.0f;
								g = 0.0f;
								b = randomValue;
								break; // 赤～マゼンタ
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

		// update confetti particles
		for (auto& c : confettiParticles_) {
			if (!c.active || !c.sprite)
				continue;
			c.age++;
			c.pos.x += c.vel.x;
			c.pos.y += c.vel.y;
			c.vel.y += 0.02f; // gravity
			c.rotation += c.rotVel;
			c.sprite->SetPosition(c.pos);
			c.sprite->SetRotation(c.rotation);
			// fade out near end
			if (c.age > c.life) {
				c.active = false;
				c.sprite->SetPosition({-100.0f, -100.0f});
			}
		}

		// allow return to title
		if (input_->TriggerKey(DIK_SPACE)) {
			confettiActive_ = false;
			sceneState = SceneState::Start;
			// reset as before...
			// ...existing reset code omitted for brevity...
		}
		break;

	case SceneState::over:
		gameOverTimer_++;

		if (player_) {
			player_->UpdateGameOver(gameOverTimer_);
		}

		if (railCamera_) {
			cameraPositionAnchor_.translation_ = railCamera_->GetWorldTransform().translation_;
			cameraPositionAnchor_.UpdateMatrix();

			camera_.matView = railCamera_->GetViewProjection().matView;
			camera_.matProjection = railCamera_->GetViewProjection().matProjection;
			camera_.TransferMatrix();
		}

		// --- リセット処理 ---
		if (input_->TriggerKey(DIK_SPACE) || gameOverTimer_ >= 90) {
			sceneState = SceneState::Start;
			gameOverTimer_ = 0;

			camera_.Initialize();
			camera_.TransferMatrix();

			if (railCamera_) {
				railCamera_->Reset();
			}

			if (player_) {
				player_->ResetRotation();
				player_->GetWorldTransform().translation_ = playerIntroStartPosition_;
				player_->GetWorldTransform().UpdateMatrix();
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
		break;
	}
}

void GameScene::Draw() {
	ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

	dxCommon_->ClearDepthBuffer();

	KamataEngine::Model::PreDraw(commandList);

	if (sceneState == SceneState::Start || sceneState == SceneState::TransitionToGame) {
		modelTitleObject_->Draw(worldTransformTitleObject_, camera_);
	} else if (sceneState == SceneState::GameIntro || sceneState == SceneState::Game || sceneState == SceneState::TransitionFromGame || sceneState == SceneState::over) {

		player_->Draw();
		skydome_->Draw();

		if (explosionEmitter_) {
			explosionEmitter_->Draw(camera_);
		}

		if ((sceneState == SceneState::Game && isGameIntroFinished_) || sceneState == SceneState::over) {
			for (Enemy* enemy : enemies_) {
				if (enemy)
					enemy->Draw(camera_);
			}
			for (EnemyBullet* bullet : enemyBullets_) {
				if (bullet)
					bullet->Draw(camera_);
			}

			for (Meteorite* meteor : meteorites_) {
				if (meteor) {
					meteor->Draw(camera_);
				}
			}
		}
	} else if (sceneState == SceneState::Clear) {
		// draw skydome so background exists
		skydome_->Draw();
		// draw any particles for clear
		if (clearEmitter_) {
			clearEmitter_->Draw(camera_);
		}
	}

	KamataEngine::Model::PostDraw();

	KamataEngine::Sprite::PreDraw(commandList);

	if (sceneState == SceneState::Start || sceneState == SceneState::TransitionToGame) {
		if (taitoruSprite_) {
			taitoruSprite_->Draw();
		}
	}

	if (sceneState == SceneState::TransitionToGame || sceneState == SceneState::TransitionFromGame) {
		transitionSprite_->Draw();
	}

	if (sceneState == SceneState::GameIntro || sceneState == SceneState::Game) {
		if (reticleSprite_) {
			reticleSprite_->Draw();
		}
		if (aimAssistCircleSprite_) {
			aimAssistCircleSprite_->Draw();
		}

		if (sceneState == SceneState::Game && isGameIntroFinished_) {
			for (Enemy* enemy : enemies_) {
				if (enemy) {
					enemy->DrawSprite();
				}
			}
		}
	}

	if (minimapSprite_) {
		minimapSprite_->Draw(); // 背景
	}
	// 敵アイコン (背景より手前、自機より奥)
	for (KamataEngine::Sprite* sprite : minimapEnemySprites_) {
		if (sprite) {
			sprite->Draw();
		}
	}
	// 敵弾アイコン (背景より手前、自機より奥)
	for (KamataEngine::Sprite* sprite : minimapEnemyBulletSprites_) {
		if (sprite) {
			sprite->Draw();
		}
	}
	// 自機アイコン (最前面)
	if (minimapPlayerSprite_) {
		minimapPlayerSprite_->Draw();
	}

	if (sceneState == SceneState::Clear) {
		if (clearSprite_)
			clearSprite_->Draw();
		// draw sprite confetti on top of clear sprite
		for (auto& c : confettiParticles_) {
			if (c.active && c.sprite)
				c.sprite->Draw();
		}
	}

	KamataEngine::Sprite::PostDraw();
}

void GameScene::AddEnemyBullet(EnemyBullet* bullet) {
	if (bullet)
		enemyBullets_.push_back(bullet);
}

void GameScene::EnemySpawn(const Vector3& position) {
	Enemy* newEnemy = new Enemy();

	assert(railCamera_ && "EnemySpawn: railCamera_ が null です");
	KamataEngine::Vector3 playerPos = railCamera_->GetWorldTransform().translation_;

	KamataEngine::Vector3 spawnPosWorld;
	spawnPosWorld.x = playerPos.x + position.x;
	spawnPosWorld.y = playerPos.y + position.y;
	spawnPosWorld.z = playerPos.z + position.z;

	newEnemy->SetPlayer(player_);
	newEnemy->SetGameScene(this);
	newEnemy->SetCamera(&camera_);

	newEnemy->Initialize(modelEnemy_, spawnPosWorld);

	enemies_.push_back(newEnemy);
}

void GameScene::LoadEnemyPopData() {
	enemyPopCommands.str("");
	enemyPopCommands.clear();

	std::ifstream file;
	file.open("Resources/enemyPop.csv");
	assert(file.is_open());
	enemyPopCommands << file.rdbuf();
	file.close();

	hasSpawnedEnemies_ = false;
}

void GameScene::UpdateEnemyPopCommands() {
	if (hasSpawnedEnemies_) {
		return;
	}

	std::string line;
	while (getline(enemyPopCommands, line)) {
		std::istringstream line_stream(line);
		std::string word;
		getline(line_stream, word, ',');

		if (word.find("//") == 0) {
			continue;
		}

		if (word.find("POP") == 0) {
			getline(line_stream, word, ',');
			float x = (float)std::atof(word.c_str());
			getline(line_stream, word, ',');
			float y = (float)std::atof(word.c_str());
			getline(line_stream, word, ',');
			float z = (float)std::atof(word.c_str());
			EnemySpawn(Vector3(x, y, z));
		} else if (word.find("WAIT") == 0) {
			continue;
		}
	}

	hasSpawnedEnemies_ = true;
	enemyPopCommands.str("");
	enemyPopCommands.clear();
}

void GameScene::CheckAllCollisions() {
	if (!player_)
		return;

	if (sceneState == SceneState::over || sceneState == SceneState::GameIntro) {
		return;
	}

	KamataEngine::Vector3 posA[3]{}, posB[3]{};
	float radiusA[3] = {0.8f, 2.0f, 0.8f};
	float radiusB[3] = {0.8f, 2.0f, 10.8f};
	const std::list<PlayerBullet*>& playerBullets = player_->GetBullets();

	// --- 自キャラ vs 敵弾 (HP制に) ---
	posA[0] = player_->GetWorldPosition();
	for (EnemyBullet* bullet : enemyBullets_) {
		if (!bullet || bullet->IsDead())
			continue;
		posB[0] = bullet->GetWorldPosition();
		float distanceSquared = DistanceSquared(posA[0], posB[0]);
		float combinedRadiusSquared = (radiusA[0] + radiusB[0]) * (radiusA[0] + radiusB[0]);
		if (distanceSquared <= combinedRadiusSquared) {

			// Immediately transition to GameOver scene for easier confirmation
			player_->OnCollision();
			bullet->OnCollision();
			TransitionToClearScene2();
			return;
		}
	}

	/*
	// 自キャラ vs 隕石 の判定
	posA[0] = player_->GetWorldPosition(); // プレイヤー位置
	float playerRadius = radiusA[0];       // プレイヤー半径

	for (Meteorite* meteor : meteorites_) {
	    if (!meteor || meteor->IsDead())
	        continue;

	    posB[0] = meteor->GetWorldPosition();
	    float meteoriteRadius = meteor->GetRadius();
	    float distanceSquared = DistanceSquared(posA[0], posB[0]);
	    float combinedRadiusSquared = (playerRadius + meteoriteRadius) * (playerRadius + meteoriteRadius);

	    if (distanceSquared <= combinedRadiusSquared) {
	        player_->OnCollision();
	        meteor->OnCollision();

	        if (player_->IsDead()) {
	            TransitionToClearScene2();
	            return;
	        }
	    }
	}
	*/

	// 自弾 vs 敵キャラ
	// 変更: 画面外の敵は多くの処理で不要なのでスキップして負荷を下げる
	for (Enemy* enemy : enemies_) {
		if (!enemy || enemy->IsDead())
			continue;
		// 画面外の敵は衝突判定やエイムアシスト用の行列演算を行わない
		if (!enemy->IsOnScreen())
			continue;
		posA[1] = enemy->GetWorldPosition();
		for (PlayerBullet* bullet : playerBullets) {
			if (!bullet || bullet->IsDead())
				continue;
			posB[1] = bullet->GetWorldPosition();
			float distanceSquared = DistanceSquared(posA[1], posB[1]);
			float combinedRadiusSquared = (radiusA[2] + radiusB[2]) * (radiusA[2] + radiusB[2]);
			if (distanceSquared <= combinedRadiusSquared) {
				enemy->OnCollision();
				bullet->OnCollision();

				if (enemy->IsDead()) {
					hitCount++;
				}

				if (audio_)
					audio_->playAudio(hitSound_, hitSoundHandle_, false, 0.7f);

				// デバッグ
				if (hitCount >= 1) {
					//TransitionToClearScene();
					return;
				}
			}
		}
	}

	enemies_.remove_if([](Enemy* enemy) {
		if (enemy && enemy->IsDead()) {
			delete enemy;
			return true;
		}
		return false;
	});
}

void GameScene::TransitionToClearScene() {
	// sceneState = SceneState::Clear; // Clear には行かず Start にする
	// Change: go to Clear scene so player sees clear screen instead of immediately returning to title
	sceneState = SceneState::Clear;

	gameOverTimer_ = 0; // (念のためタイマー系もリセット)
	hitCount = 0;       // 撃破数リセット
	hitCount2 = 0;

	camera_.Initialize();
	camera_.TransferMatrix();

	if (railCamera_) {
		railCamera_->Reset();
	}

	if (player_) {
		player_->ResetRotation();
		player_->GetWorldTransform().translation_ = playerIntroStartPosition_;
		player_->GetWorldTransform().UpdateMatrix();
		player_->ResetParticles();
		player_->ResetBullets(); // (弾のリセットも行う)
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

void GameScene::TransitionToClearScene2() {
	sceneState = SceneState::over;
	gameOverTimer_ = 0;
	hitCount2 = 0;
}

void GameScene::SpawnMeteorite() {
	assert(railCamera_);
	assert(modelMeteorite_);

	KamataEngine::Vector3 cameraPos = railCamera_->GetWorldTransform().translation_;

	float randomYaw = (static_cast<float>(std::rand()) / RAND_MAX) * (KamataEngine::MathUtility::PI * 2.0f);

	float randomPitchFactor = (static_cast<float>(std::rand()) / RAND_MAX) * 2.0f - 1.0f; // -1.0f ～ 1.0f
	float randomPitch = std::acos(randomPitchFactor) - (KamataEngine::MathUtility::PI / 2.0f);

	KamataEngine::Vector3 randomDir;
	randomDir.x = std::cos(randomPitch) * std::sin(randomYaw);
	randomDir.y = std::sin(randomPitch);
	randomDir.z = std::cos(randomPitch) * std::cos(randomYaw);
	randomDir = KamataEngine::MathUtility::Normalize(randomDir);

	// この距離に隕石が発生する
	const float kSpawnDistance = 0.0f;

	KamataEngine::Vector3 offset = randomDir * kSpawnDistance;
	KamataEngine::Vector3 spawnPos = cameraPos + offset;

	// スケールと半径をランダム
	const float kBaseRadius = 0.0f;
	const float kMinScale = 0.0f;
	const float kMaxScale = 0.0f;

	float randFactor = static_cast<float>(std::rand()) / RAND_MAX;
	float randomBaseScale = kMinScale + (randFactor * (kMaxScale - kMinScale));
	float randomRadius = kBaseRadius * randomBaseScale;
	Meteorite* newMeteor = new Meteorite();
	newMeteor->Initialize(modelMeteorite_, spawnPos, randomBaseScale, randomRadius);
	meteorites_.push_back(newMeteor);
}

void GameScene::UpdateMeteorites() {
	// 　この数値より離れたら隕石を消去
	const float kDespawnDistanceSq = 0.0f * 0.0f;
	KamataEngine::Vector3 playerPos = railCamera_->GetWorldTransform().translation_;

	for (Meteorite* meteor : meteorites_) {

		meteor->Update(playerPos);
		float distSq = DistanceSquared(playerPos, meteor->GetWorldPosition());

		if (distSq > kDespawnDistanceSq) {
			meteor->OnCollision();
		}
	}

	meteorites_.remove_if([](Meteorite* meteor) {
		if (meteor && meteor->IsDead()) {
			delete meteor;
			return true;
		}
		return false;
	});
}

KamataEngine::Vector3 GameScene::ProjectToNDC(const KamataEngine::Vector3& worldPos) {
	if (!railCamera_) {
		return {0.0f, 0.0f, -1.0f};
	}

	const KamataEngine::Matrix4x4& viewMatrix = railCamera_->GetViewProjection().matView;
	const KamataEngine::Matrix4x4& projMatrix = railCamera_->GetViewProjection().matProjection;

	KamataEngine::Vector3 viewPos;
	viewPos.x = worldPos.x * viewMatrix.m[0][0] + worldPos.y * viewMatrix.m[1][0] + worldPos.z * viewMatrix.m[2][0] + 1.0f * viewMatrix.m[3][0];
	viewPos.y = worldPos.x * viewMatrix.m[0][1] + worldPos.y * viewMatrix.m[1][1] + worldPos.z * viewMatrix.m[2][1] + 1.0f * viewMatrix.m[3][1];
	viewPos.z = worldPos.x * viewMatrix.m[0][2] + worldPos.y * viewMatrix.m[1][2] + worldPos.z * viewMatrix.m[2][2] + 1.0f * viewMatrix.m[3][2];

	if (viewPos.z < 0.0f) {
		return {0.0f, 0.0f, -1.0f};
	}

	float clipX = viewPos.x * projMatrix.m[0][0] + viewPos.y * projMatrix.m[1][0] + viewPos.z * projMatrix.m[2][0] + 1.0f * projMatrix.m[3][0];
	float clipY = viewPos.x * projMatrix.m[0][1] + viewPos.y * projMatrix.m[1][1] + viewPos.z * projMatrix.m[2][1] + 1.0f * projMatrix.m[3][1];
	float clipZ = viewPos.x * projMatrix.m[0][2] + viewPos.y * projMatrix.m[1][2] + viewPos.z * projMatrix.m[2][2] + 1.0f * projMatrix.m[3][2];

	float w_clip = viewPos.x * projMatrix.m[0][3] + viewPos.y * projMatrix.m[1][3] + viewPos.z * projMatrix.m[2][3] + 1.0f * projMatrix.m[3][3];

	if (std::abs(w_clip) < 0.001f || w_clip < 0.0f) {
		return {0.0f, 0.0f, -1.0f};
	}

	float ndcX = clipX / w_clip;
	float ndcY = clipY / w_clip;
	float ndcZ = clipZ / w_clip;

	return {ndcX, ndcY, ndcZ};
}

void GameScene::UpdateAimAssist() {
	if (!railCamera_)
		return;

	// (リセット処理: user_104.txt で追加済み)
	for (Enemy* enemy : enemies_) {
		if (enemy) {
			enemy->SetAssistLocked(false);
		}
	}

	// 1. スプライトの「見た目」の円の半径 (画面高さに対する比率)
	const float kVisualRadius = 0.08f;
	// 2. アシストが反応する「判定」の円の半径 (画面高さに対する比率)
	const float kDetectionRadius = 0.1f; // 0.1f

	// 4. アスペクト比（縦横比）を取得
	const float kAspect = (float)KamataEngine::WinApp::kWindowWidth / (float)KamataEngine::WinApp::kWindowHeight;

	// 5. スプライトのサイズを「真円」に設定 (kVisualRadius を使用)
	if (aimAssistCircleSprite_) {
		float pixelDiameterY = KamataEngine::WinApp::kWindowHeight * kVisualRadius * 2.0f;
		float pixelDiameterX = pixelDiameterY; // ピクセルで真円
		aimAssistCircleSprite_->SetSize({pixelDiameterX, pixelDiameterY});
	}

	// 6. 敵の検索
	// NDC空間での半径を計算する (ndc は画面幅方向がアスペクトで伸びているため補正が必要)
	// kVisualRadius は画面高さに対する比率なので、NDCでの半径は (2 * kVisualRadius)
	const float ndcVisualRadiusY = kVisualRadius * 2.0f;
	// X方向のNDC半径はアスペクト比で割る（幅が大きいと NDC 単位での幅は小さくなる）
	const float ndcVisualRadiusX = ndcVisualRadiusY / kAspect;

	const float ndcDetectionRadiusY = kDetectionRadius * 2.0f;
	const float ndcDetectionRadiusX = ndcDetectionRadiusY / kAspect;

	// 正規化して比較するための初期閾値 (1.0 = 半径内)
	float minNormalizedDistSq = 1.0f; // (normalized distance squared)
	Enemy* bestTarget = nullptr;
	KamataEngine::Vector3 bestTargetNdc = {0, 0, 0};

	// Camera position for distance check
	KamataEngine::Vector3 cameraPos = railCamera_->GetWorldTransform().translation_;

	const float kMaxAssistDistance = 3000.0f; // アシストが働く最大距離
	const float kMaxAssistDistanceSq = kMaxAssistDistance * kMaxAssistDistance;

	for (Enemy* enemy : enemies_) {
		if (!enemy || enemy->IsDead()) {
			continue;
		}

		// 距離でフィルタ（遠い敵はアシスト対象外）
		KamataEngine::Vector3 enemyPos = enemy->GetWorldPosition();
		float dx = enemyPos.x - cameraPos.x;
		float dy = enemyPos.y - cameraPos.y;
		float dz = enemyPos.z - cameraPos.z;
		float distSq = dx * dx + dy * dy + dz * dz;
		if (distSq > kMaxAssistDistanceSq) {
			continue;
		}

		// 画面外の敵は早期除外
		if (!enemy->IsOnScreen()) {
			continue;
		}

		KamataEngine::Vector3 ndc = ProjectToNDC(enemy->GetWorldPosition());

		if (ndc.z < 0.0f) {
			continue;
		}

		// 正規化した距離を計算 (各軸で半径で割る)
		float normX = ndc.x / ndcDetectionRadiusX;
		float normY = ndc.y / ndcDetectionRadiusY;
		float normalizedDistSq = (normX * normX) + (normY * normY);

		// 判定円の中で、最も中心に近い敵を探す (正規化距離で比較)
		if (normalizedDistSq < minNormalizedDistSq) {
			minNormalizedDistSq = normalizedDistSq; // 最終的に bestTarget の正規化距離(2乗) が入る
			bestTarget = enemy;
			bestTargetNdc = ndc;
		}
	}

	// 9. ターゲットが見つかったらアシスト適用
	if (bestTarget) {

		// アシスト自体は「判定」円で見つかったら実行
		railCamera_->ApplyAimAssist(bestTargetNdc.x, bestTargetNdc.y);

		float visualNormX = bestTargetNdc.x / ndcVisualRadiusX;
		float visualNormY = bestTargetNdc.y / ndcVisualRadiusY;
		float visualNormDistSq = (visualNormX * visualNormX) + (visualNormY * visualNormY);

		if (visualNormDistSq <= 1.0f) {
			bestTarget->SetAssistLocked(true);
		}
	}
}

void GameScene::RequestExplosion(const KamataEngine::Vector3& position) {
	if (!explosionEmitter_) {
		return;
	}

	// ★ ParticleEmitter に追加した「EmitBurst」関数を呼ぶ
	explosionEmitter_->EmitBurst(
	    position, // 発生座標
	    10,       // 粒の数
	    4.0f,     // 速度
	    40.0f,    // 寿命 (30フレーム)
	    10.0f,    // 開始スケール
	    0.0f      // 終了スケール
	);
}

KamataEngine::Vector2 GameScene::ConvertWorldToMinimap(const KamataEngine::Vector3& worldPos, const KamataEngine::Vector3& playerPos) {

	// 1. 自機からの相対座標 (XZ平面のみ)
	float relativeX = worldPos.x - playerPos.x;
	float relativeZ = worldPos.z - playerPos.z;

	// 2. ミニマップのスケールを適用 (ワールドのZ+ を ミニマップのY+ (上) に)
	float minimapOffsetX = relativeX * kMinimapScale_;
	float minimapOffsetY = relativeZ * kMinimapScale_ * -1.0f; // Y軸反転

	// 3. ミニマップの中心座標を計算
	KamataEngine::Vector2 minimapCenterPos = {
	    kMinimapPosition_.x + kMinimapSize_.x * 0.5f,
	    kMinimapPosition_.y - kMinimapSize_.y * 0.5f // 左下アンカー基準
	};

	// 4. 中心の座標にオフセットを加える
	KamataEngine::Vector2 finalPos = {minimapCenterPos.x + minimapOffsetX, minimapCenterPos.y + minimapOffsetY};

	// 5. ミニマップの範囲内に座標をクランプ (はみ出さないように)
	float minX = kMinimapPosition_.x;
	float maxX = kMinimapPosition_.x + kMinimapSize_.x;
	float minY = kMinimapPosition_.y - kMinimapSize_.y; // Yは上(小)・下(大)
	float maxY = kMinimapPosition_.y;

	finalPos.x = std::clamp(finalPos.x, minX, maxX);
	finalPos.y = std::clamp(finalPos.y, minY, maxY);

	return finalPos;
}