#include "GaneScene.h"
#include "3d/AxisIndicator.h"
#include <algorithm> // std::clamp のため
#include <cassert>
#include <cmath> // std::lerp, std::abs, std::pow, std::sqrtのため
#include <fstream>

// Vector3 線形補間ヘルパー関数
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
	// ... (デストラクタは変更なし)
	delete modelPlayer_;
	delete modelEnemy_;
	delete modelSkydome_;
	delete modelTitleObject_;
	delete player_;
	delete skydome_;
	delete railCamera_;
	delete transitionSprite_;
	for (EnemyBullet* bullet : enemyBullets_) {
		delete bullet;
	}
	for (Enemy* enemy : enemies_) {
		delete enemy;
	}
}

void GameScene::Initialize() {
	// ... (DirectX, Input, Audioなどの初期化)
	dxCommon_ = DirectXCommon::GetInstance();
	input_ = Input::GetInstance();
	audio_ = Audio::GetInstance();

	player_ = new Player();
	skydome_ = new Skydome();
	// ... (モデルの読み込み)
	modelPlayer_ = KamataEngine::Model::CreateFromOBJ("fly", true);
	modelEnemy_ = KamataEngine::Model::CreateFromOBJ("cube", true);
	modelSkydome_ = Model::CreateFromOBJ("skydome", true);
	modelTitleObject_ = Model::CreateFromOBJ("title", true);
	// ... (遷移スプライトの初期化)
	transitionTextureHandle_ = KamataEngine::TextureManager::Load("black.png");
	transitionSprite_ = Sprite::Create(transitionTextureHandle_, {0, 0});
	Vector2 screenCenter = {WinApp::kWindowWidth / 2.0f, WinApp::kWindowHeight / 2.0f};
	transitionSprite_->SetPosition(screenCenter);
	transitionSprite_->SetAnchorPoint({0.5f, 0.5f});
	transitionSprite_->SetSize({0.0f, 0.0f});

	camera_.Initialize();

	// 自機とカメラの距離
	playerIntroTargetPosition_ = {0.0f, -3.0f, 20.0f}; // Player::Initialize内のZ座標と合わせる

	// イントロアニメーションの開始座標を定義（目標より奥）
	playerIntroStartPosition_ = playerIntroTargetPosition_;
	playerIntroStartPosition_.z += -50.0f; // 例: 50ユニット奥から開始

	// プレイヤーをまず「イントロ開始座標」で初期化する
	player_->Initialize(modelPlayer_, &camera_, playerIntroStartPosition_);

	skydome_->Initialize(modelSkydome_, &camera_);
	worldTransformTitleObject_.Initialize();
	worldTransformTitleObject_.translation_ = {0.0f, 0.0f, -43.0f};
	worldTransformTitleObject_.UpdateMatrix();

	KamataEngine::AxisIndicator::GetInstance()->SetVisible(true);

	railCamera_ = new RailCamera();
	railCamera_->Initialize(railcameraPos, railcameraRad);
	player_->SetParent(&railCamera_->GetWorldTransform());
	player_->SetRailCamera(railCamera_);

	LoadEnemyPopData();
	hitSoundHandle_ = audio_->LoadWave("./sound/parry.wav");
}

void GameScene::Update() {

	// Rキーリセットは常に受け付ける
	if (input_->TriggerKey(DIK_R)) {
		if (railCamera_) {
			railCamera_->Reset();
		}
		if (player_) {
			player_->ResetRotation();
		}
	}

	switch (sceneState) {
	case SceneState::Start: {
		// ... (Startシーンのロジックは変更なし)
		if (input_->TriggerKey(DIK_SPACE)) {
			sceneState = SceneState::TransitionToGame;
			transitionTimer_ = 0.0f;
			gameSceneTimer_ = 0;
		}
		// ... (タイトルアニメーション)
		titleAnimationTimer_++;
		const int32_t cycleFrames = kTitleRotateFrames + kTitlePauseFrames;
		int32_t timeInCycle = titleAnimationTimer_ % cycleFrames;
		if (timeInCycle < kTitleRotateFrames) {
			// 回転中の処理
			float progress = static_cast<float>(timeInCycle) / kTitleRotateFrames;
			// イージング（ここでは EaseInOutSine）
			float easedProgress = (1.0f - cosf(progress * 3.14159265f)) / 2.0f;
			// Y軸周りに回転させる (0 -> 2π)
			worldTransformTitleObject_.rotation_.y = easedProgress * (2.0f * 3.14159265f);
		} else {
			// ポーズ中の処理（もし何かあれば）
			// 例: 回転をキープ
			worldTransformTitleObject_.rotation_.y = 0.0f; // 2πは0と同じなのでリセット
		}
		worldTransformTitleObject_.UpdateMatrix();
		break;
	}
	case SceneState::TransitionToGame: {
		// ... (遷移アニメーション（拡大）は変更なし)
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
		// ... (遷移アニメーション（縮小）)
		transitionTimer_++;
		float maxScale = sqrtf(powf(WinApp::kWindowWidth, 2) + powf(WinApp::kWindowHeight, 2));
		float progress = std::fmin(transitionTimer_ / kTransitionTime, 1.0f);
		float easedProgress = sinf(progress * 3.14159265f / 2.0f);
		float scale = (1.0f - easedProgress) * maxScale;
		transitionSprite_->SetSize({scale, scale});

		// ★★★ 遷移が終わったら GameIntro へ ★★★
		if (transitionTimer_ >= kTransitionTime) {
			sceneState = SceneState::GameIntro;
			gameIntroTimer_ = 0.0f; // イントロタイマーリセット
			// プレイヤーを開始位置にセット
			player_->GetWorldTransform().translation_ = playerIntroStartPosition_;
			player_->GetWorldTransform().UpdateMatrix(); // 行列を即時更新
			isGameIntroFinished_ = false;                // イントロ完了フラグをリセット
		}

		// 遷移中もカメラとプレイヤーの「行列更新」だけは動かす
		if (railCamera_)
			railCamera_->Update();
		if (player_)
			player_->GetWorldTransform().UpdateMatrix(); // 座標だけ更新
		camera_.matView = railCamera_->GetViewProjection().matView;
		camera_.matProjection = railCamera_->GetViewProjection().matProjection;
		camera_.TransferMatrix();
		break;
	}
	case SceneState::GameIntro: { // ★★★ 新しい状態の処理 ★★★
		// 1. イントロタイマーを進める
		gameIntroTimer_++;

		// 2. 補間係数 t を計算 (0.0 -> 1.0)
		float t = gameIntroTimer_ / kGameIntroDuration_;
		// イージングを適用して動きを滑らかに（例: EaseOutCubic）
		t = 1.0f - std::pow(1.0f - t, 3.0f);
		t = std::clamp(t, 0.0f, 1.0f); // 念のため範囲内に収める

		// 3. プレイヤーの位置を開始座標から目標座標へ線形補間
		player_->GetWorldTransform().translation_ = Lerp(playerIntroStartPosition_, playerIntroTargetPosition_, t);
		player_->GetWorldTransform().UpdateMatrix(); // プレイヤーの行列を更新

		// 4. レールカメラも更新し続ける
		railCamera_->Update();
		camera_.matView = railCamera_->GetViewProjection().matView;
		camera_.matProjection = railCamera_->GetViewProjection().matProjection;
		camera_.TransferMatrix();

		// 5. プレイヤーのパーティクルだけ更新 (もしPlayerクラスにUpdateParticlesOnlyがあれば)
		// player_->UpdateParticlesOnly();

		// 6. アニメーション終了判定
		const float kArrivalThreshold = 0.1f; // 到着したとみなす距離
		if (gameIntroTimer_ >= kGameIntroDuration_ || Distance(player_->GetWorldTransform().translation_, playerIntroTargetPosition_) < kArrivalThreshold) {
			// 最終座標に確定させて Game 状態へ移行
			player_->GetWorldTransform().translation_ = playerIntroTargetPosition_;
			player_->GetWorldTransform().UpdateMatrix();
			sceneState = SceneState::Game;
			isGameIntroFinished_ = true; // イントロ完了フラグを立てる
		}
		// この状態では敵や弾の更新は行わない
		break;
	}
	case SceneState::Game: {
		// デモ用自動復帰タイマー
		//gameSceneTimer_++;
		if (gameSceneTimer_ > kGameTimeLimit_) {
			sceneState = SceneState::Start;
			camera_.Initialize(); // 通常カメラをリセット
			camera_.TransferMatrix();
			if (railCamera_) {
				railCamera_->Reset();
			} // レールカメラもリセット
			if (player_) {
				player_->ResetRotation(); // プレイヤーの回転リセット
				// 次回のためにプレイヤー位置をイントロ開始位置に戻す
				player_->GetWorldTransform().translation_ = playerIntroStartPosition_;
				player_->GetWorldTransform().UpdateMatrix();
				player_->ResetParticles();
			}
			// 敵・弾のクリアと敵データの再読み込み
			for (Enemy* enemy : enemies_) {
				delete enemy;
			}
			enemies_.clear();
			for (EnemyBullet* bullet : enemyBullets_) {
				delete bullet;
			}
			enemyBullets_.clear();
			LoadEnemyPopData();
			break;
		}

		// ★★★ イントロが終わってからゲームの更新処理を開始 ★★★
		if (isGameIntroFinished_) {
			UpdateEnemyPopCommands();       // 敵出現処理
			for (Enemy* enemy : enemies_) { // 敵の更新
				enemy->Update();
			}
			for (EnemyBullet* bullet : enemyBullets_) { // 敵弾の更新
				bullet->Update();
			}
			enemyBullets_.remove_if([](EnemyBullet* bullet) { // 敵弾の削除
				if (bullet && bullet->IsDead()) {
					delete bullet;
					return true;
				}
				return false;
			});
			CheckAllCollisions(); // 当たり判定
			player_->Update();    // プレイヤーの完全な更新（移動、攻撃、パーティクル等）
		} else {
			// イントロが終わるまではプレイヤーの行列更新とパーティクル更新のみ
			if (player_) {
				// player_->UpdateParticlesOnly(); // パーティクルのみ更新する場合
				player_->GetWorldTransform().UpdateMatrix(); // 行列更新のみでも良いかも
			}
		}

		// レールカメラと通常カメラは常に更新
		railCamera_->Update();
		camera_.matView = railCamera_->GetViewProjection().matView;
		camera_.matProjection = railCamera_->GetViewProjection().matProjection;
		camera_.TransferMatrix();
		break;
	}
	case SceneState::Clear: // Clear と over は変更なし
	case SceneState::over:
		if (input_->TriggerKey(DIK_SPACE)) {
			sceneState = SceneState::Start;
			// タイトルに戻るときも状態をリセット
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
		}
		break;
	}
}

void GameScene::Draw() {
	ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

	Sprite::PreDraw(commandList); /* 背景スプライト描画前処理 */
	Sprite::PostDraw();
	dxCommon_->ClearDepthBuffer();

	KamataEngine::Model::PreDraw(commandList);

	// シーン状態に応じて描画内容を切り替え
	if (sceneState == SceneState::Start || sceneState == SceneState::TransitionToGame) {
		modelTitleObject_->Draw(worldTransformTitleObject_, camera_);
	} else if (
	    sceneState == SceneState::GameIntro || // ★★★ イントロ中も描画 ★★★
	    sceneState == SceneState::Game || sceneState == SceneState::TransitionFromGame || sceneState == SceneState::over) {
		// イントロ、ゲーム中共通で描画するもの
		player_->Draw();
		skydome_->Draw();

		// ★★★ ゲームが始まってから（イントロ完了後）描画するもの ★★★
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
		// クリア画面の描画
	}

	KamataEngine::Model::PostDraw();

	Sprite::PreDraw(commandList);
	// 遷移スプライトは該当する状態のときだけ描画
	if (sceneState == SceneState::TransitionToGame || sceneState == SceneState::TransitionFromGame) {
		transitionSprite_->Draw();
	}
	Sprite::PostDraw();
}

// ... (以降の関数 AddEnemyBullet, EnemySpawn, LoadEnemyPopData, UpdateEnemyPopCommands, CheckAllCollisions, Transitions はほぼ変更なし) ...

void GameScene::AddEnemyBullet(EnemyBullet* bullet) {
	if (bullet)
		enemyBullets_.push_back(bullet);
}

void GameScene::EnemySpawn(const Vector3& position) {
	Enemy* newEnemy = new Enemy();
	newEnemy->Initialize(modelEnemy_, position);
	newEnemy->SetPlayer(player_);
	if (player_)
		player_->SetEnemy(newEnemy); // Playerが存在するか確認
	newEnemy->SetGameScene(this);
	enemies_.push_back(newEnemy);
}

void GameScene::LoadEnemyPopData() {
	// 読み込む前にクリアする
	enemyPopCommands.str("");
	enemyPopCommands.clear(); // エラーフラグもクリア

	std::ifstream file;
	file.open("Resources/enemyPop.csv");
	assert(file.is_open());
	enemyPopCommands << file.rdbuf();
	file.close();

	// タイマーフラグもリセット
	timer = 0;
	timerflag = false;
}

void GameScene::UpdateEnemyPopCommands() {
	if (timerflag) {
		timer--;
		if (timer <= 0) {
			timerflag = false;
		}
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
			getline(line_stream, word, ',');
			int32_t waitTime = atoi(word.c_str());
			timerflag = true;
			timer = waitTime;
			break;
		}
	}
}

void GameScene::CheckAllCollisions() {
	if (!player_)
		return; // プレイヤーがいない場合は何もしない

	KamataEngine::Vector3 posA[3]{}, posB[3]{};
	float radiusA[3] = {0.8f, 2.0f, 0.8f};
	float radiusB[3] = {0.8f, 2.0f, 10.8f}; // 半径の値は見直しが必要かも
	const std::list<PlayerBullet*>& playerBullets = player_->GetBullets();

	// 自キャラ vs 敵弾
	posA[0] = player_->GetWorldPosition();
	for (EnemyBullet* bullet : enemyBullets_) {
		if (!bullet)
			continue; // nullptrチェック
		posB[0] = bullet->GetWorldPosition();
		float distanceSquared = DistanceSquared(posA[0], posB[0]);
		float combinedRadiusSquared = (radiusA[0] + radiusB[0]) * (radiusA[0] + radiusB[0]);
		if (distanceSquared <= combinedRadiusSquared) {
			hitCount2++;
			if (hitCount2 >= 3) {
				TransitionToClearScene2();
				return; // 状態が変わったら抜ける
			}
		}
	}

	// 自弾 vs 敵キャラ
	// 敵リストをコピーしてイテレート（安全な削除のため）
	auto enemiesCopy = enemies_;
	for (Enemy* enemy : enemiesCopy) {
		if (!enemy || enemy->IsDead())
			continue; // nullptrチェックと死亡チェック
		posA[1] = enemy->GetWorldPosition();
		for (PlayerBullet* bullet : playerBullets) {
			if (!bullet || bullet->IsDead())
				continue; // nullptrチェックと死亡チェック
			posB[1] = bullet->GetWorldPosition();
			float distanceSquared = DistanceSquared(posA[1], posB[1]);
			float combinedRadiusSquared = (radiusA[2] + radiusB[2]) * (radiusA[2] + radiusB[2]); // インデックス注意: radiusA[1] かも？
			if (distanceSquared <= combinedRadiusSquared) {
				enemy->OnCollision();
				bullet->OnCollision();
				hitCount++;
				if (audio_)
					audio_->playAudio(hitSound_, hitSoundHandle_, false, 0.7f); // audio_ nullptrチェック
				if (hitCount >= 5) {
					TransitionToClearScene();
					return; // 状態が変わったら抜ける
				}
			}
		}
	}
	// 死んだ敵を元のリストから削除
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
	// playerPos = {0, 0, 30}; // この変数は直接使っていない
	hitCount = 0;
}

void GameScene::TransitionToClearScene2() {
	sceneState = SceneState::over;
	hitCount2 = 0;
}