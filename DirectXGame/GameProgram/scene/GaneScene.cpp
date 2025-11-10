#include "GaneScene.h"
#include "3d/AxisIndicator.h"
#include <algorithm> // std::clamp のため
#include <cassert>
#include <cmath> // std::lerp, std::abs, std::pow, std::sqrtのため
#include <fstream>

KamataEngine::Vector3 Lerp(const KamataEngine::Vector3& start, const KamataEngine::Vector3& end, float t) {
	t = std::clamp(t, 0.0f, 1.0f); // tを0から1の範囲に収める
	return start + (end - start) * t;
}

// Vector3 間の距離を計算するヘルパー関数
float Distance(const KamataEngine::Vector3& v1, const KamataEngine::Vector3& v2) {
	float dx = v1.x - v2.x;
	float dy = v1.y - v2.y;
	float dz = v1.z - v2.z;
	return std::sqrt(dx * dx + dy * dy + dz * dz);
}
// 距離の2乗を計算するヘルパー関数（平方根計算を省略して高速）
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
	delete player_;
	delete skydome_;
	delete railCamera_;
	delete reticleSprite_;
	delete transitionSprite_;
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

	modelPlayer_ = KamataEngine::Model::CreateFromOBJ("fly", true);
	modelEnemy_ = KamataEngine::Model::CreateFromOBJ("cube", true);
	modelSkydome_ = Model::CreateFromOBJ("skydome", true);
	modelTitleObject_ = Model::CreateFromOBJ("title", true);

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

	camera_.Initialize();

	playerIntroTargetPosition_ = {0.0f, -3.0f, 20.0f};
	playerIntroStartPosition_ = playerIntroTargetPosition_;
	playerIntroStartPosition_.z += -50.0f;

	player_->Initialize(modelPlayer_, &camera_, playerIntroStartPosition_);

	skydome_->Initialize(modelSkydome_, &camera_);
	worldTransformTitleObject_.Initialize();
	worldTransformTitleObject_.translation_ = {0.0f, 0.0f, -43.0f};
	worldTransformTitleObject_.UpdateMatrix();

	KamataEngine::AxisIndicator::GetInstance()->SetVisible(true);

	railCamera_ = new RailCamera();
	railCamera_->Initialize(railcameraPos, railcameraRad);
	cameraPositionAnchor_.Initialize(); // (★ ご提示のコードで適用済み)
	player_->SetParent(&railCamera_->GetWorldTransform());
	player_->SetRailCamera(railCamera_);
	player_->SetEnemies(&enemies_);

	LoadEnemyPopData();
	hitSoundHandle_ = audio_->LoadWave("./sound/parry.wav");
}

void GameScene::Update() {

	skydome_->Update();

	switch (sceneState) {
	case SceneState::Start: {
		// ( ... Start 処理 ... )
		if (input_->TriggerKey(DIK_SPACE)) {
			sceneState = SceneState::TransitionToGame;
			transitionTimer_ = 0.0f;
			gameSceneTimer_ = 0; // ゲームタイマーリセット
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
		// ( ... TransitionToGame 処理 ... )
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
		// ( ... TransitionFromGame 処理 ... )
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
		}

		if (railCamera_) {
			railCamera_->Update();
			// ▼▼▼ アンカー更新処理を追加 ▼▼▼
			cameraPositionAnchor_.translation_ = railCamera_->GetWorldTransform().translation_;
			cameraPositionAnchor_.UpdateMatrix();
			// ▲▲▲ 追加完了 ▲▲▲
		}
		if (player_)
			player_->GetWorldTransform().UpdateMatrix();
		camera_.matView = railCamera_->GetViewProjection().matView;
		camera_.matProjection = railCamera_->GetViewProjection().matProjection;
		camera_.TransferMatrix();
		break;
	}
	case SceneState::GameIntro: {
		// ( ... GameIntro 処理 ... )
		gameIntroTimer_++;

		float t = gameIntroTimer_ / kGameIntroDuration_;
		t = 1.0f - std::pow(1.0f - t, 3.0f);
		t = std::clamp(t, 0.0f, 1.0f);

		player_->GetWorldTransform().translation_ = Lerp(playerIntroStartPosition_, playerIntroTargetPosition_, t);
		player_->GetWorldTransform().UpdateMatrix();

		railCamera_->Update();
		// ▼▼▼ アンカー更新処理を追加 ▼▼▼
		cameraPositionAnchor_.translation_ = railCamera_->GetWorldTransform().translation_;
		cameraPositionAnchor_.UpdateMatrix();
		// ▲▲▲ 追加完了 ▲▲▲
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
		}
		break;
	}
	case SceneState::Game: {
		// ( ... Game 処理 ... )

		// デモ用自動復帰タイマー (タイムオーバーでタイトルへ)
		if (gameSceneTimer_ > kGameTimeLimit_) {
			sceneState = SceneState::Start;
			// (リセット処理)
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
			}
			for (Enemy* enemy : enemies_) {
				delete enemy;
			}
			enemies_.clear();
			for (EnemyBullet* bullet : enemyBullets_) {
				delete bullet;
			}
			enemyBullets_.clear();
			LoadEnemyPopData();
			hasSpawnedEnemies_ = false;
			break;
		}

		// --- 通常のゲーム処理 ---
		railCamera_->Update();
		// ▼▼▼ アンカー更新処理を追加 ▼▼▼
		cameraPositionAnchor_.translation_ = railCamera_->GetWorldTransform().translation_;
		cameraPositionAnchor_.UpdateMatrix();
		// ▲▲▲ 追加完了 ▲▲▲
		camera_.matView = railCamera_->GetViewProjection().matView;
		camera_.matProjection = railCamera_->GetViewProjection().matProjection;
		camera_.TransferMatrix();

		if (isGameIntroFinished_) {
			UpdateEnemyPopCommands();
			for (Enemy* enemy : enemies_) {
				enemy->Update();
			}
			for (EnemyBullet* bullet : enemyBullets_) {
				bullet->Update();
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

		} else { // イントロ中
			if (player_) {
				player_->GetWorldTransform().UpdateMatrix();
			}
		}

		break;
	}
	case SceneState::Clear:
		// ( ... Clear 処理 ... )
		if (input_->TriggerKey(DIK_SPACE)) {
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
			}
			for (Enemy* enemy : enemies_) {
				delete enemy;
			}
			enemies_.clear();
			for (EnemyBullet* bullet : enemyBullets_) {
				delete bullet;
			}
			enemyBullets_.clear();
			LoadEnemyPopData();
			hasSpawnedEnemies_ = false;
		}
		break;

	case SceneState::over:
		// ( ... over 処理 ... )
		gameOverTimer_++;

		if (player_) {
			player_->UpdateGameOver(gameOverTimer_);
		}

		if (railCamera_) {
			// (アンカーの更新もここで行う)
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
			}

			for (Enemy* enemy : enemies_) {
				delete enemy;
			}
			enemies_.clear();
			for (EnemyBullet* bullet : enemyBullets_) {
				delete bullet;
			}
			enemyBullets_.clear();

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

		if ((sceneState == SceneState::Game && isGameIntroFinished_) || sceneState == SceneState::over) {
			for (Enemy* enemy : enemies_) {
				if (enemy)
					enemy->Draw(camera_);
			}
			for (EnemyBullet* bullet : enemyBullets_) {
				if (bullet)
					bullet->Draw(camera_);
			}
		}
	} else if (sceneState == SceneState::Clear) {
		// ( ... Clear 描画 ... )
	}

	KamataEngine::Model::PostDraw();

	KamataEngine::Sprite::PreDraw(commandList);

	if (sceneState == SceneState::TransitionToGame || sceneState == SceneState::TransitionFromGame) {
		transitionSprite_->Draw();
	}

	if (sceneState == SceneState::GameIntro || sceneState == SceneState::Game) {
		if (reticleSprite_) {
			reticleSprite_->Draw();
		}

		if (sceneState == SceneState::Game && isGameIntroFinished_) {
			for (Enemy* enemy : enemies_) {
				if (enemy) {
					enemy->DrawSprite();
				}
			}
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
	newEnemy->Initialize(modelEnemy_, position);
	newEnemy->SetPlayer(player_);
	newEnemy->SetGameScene(this);
	newEnemy->SetCamera(&camera_);

	// ▼▼▼ 親をアンカーに変更 ▼▼▼
	// newEnemy->SetParent(&railCamera_->GetWorldTransform()); // ★ 修正前
	//newEnemy->SetParent(&cameraPositionAnchor_); // ★ 修正後
	// ▲▲▲ 修正完了 ▲▲▲

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

			player_->OnCollision();
			bullet->OnCollision();
			if (player_->IsDead()) {
				TransitionToClearScene2();
				return;
			}
		}
	}

	// --- 自弾 vs 敵キャラ ---
	auto enemiesCopy = enemies_;
	for (Enemy* enemy : enemiesCopy) {
		if (!enemy || enemy->IsDead())
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

				if (hitCount >= 5) {
					TransitionToClearScene();
					return;
				}
			}
		}
	}

	// 死んだ敵をリストから削除
	enemies_.remove_if([](Enemy* enemy) {
		if (enemy && enemy->IsDead()) {
			delete enemy;
			return true;
		}
		return false;
	});
}

void GameScene::TransitionToClearScene() {
	sceneState = SceneState::Clear;
	enemyPopCommands.str("");
	enemyPopCommands.clear();
	hitCount = 0; // 撃破数リセット
}

void GameScene::TransitionToClearScene2() {
	sceneState = SceneState::over;
	gameOverTimer_ = 0;
	hitCount2 = 0;
}