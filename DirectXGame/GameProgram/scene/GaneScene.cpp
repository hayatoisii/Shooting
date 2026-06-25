#include "GaneScene.h"
#include "GameBalanceAccess.h"
#include "GameBullet.h"
#include "GameCharacter.h"
#include "MT.h"
#include "SpawnCommandTable.h"
#include "3d/AxisIndicator.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <unordered_map>

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

GameScene::GameScene() {
	sceneState_ = SceneStateStart::Instance();
	RegisterDefaultSpawnCommands(spawnCommandTable_);
}

void GameScene::ChangeSceneState(SceneStateBase* newState) {
	if (newState != nullptr && newState != sceneState_) {
		sceneState_ = newState;
	}
}

SceneStateKind GameScene::GetSceneStateKind() const {
	if (sceneState_ == nullptr) {
		return SceneStateKind::Start;
	}
	return sceneState_->GetKind();
}

GameScene::~GameScene() {
	delete modelCube_;
	modelPlayer_ = nullptr;
	delete modelEnemy_;
	delete modelSkydome_;
	delete modelTitleObject_;
	delete modelMeteorite_;
	delete modelEnemyBullet_; // 敵弾モデルを解放（追加）
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
	// 追加: 右/左キー表示用スプライトを解放
	delete lightSprite_;
	delete leftSprite_;
	delete shiftSprite_; // Shiftスプライトを解放
	delete explosionEmitter_;
	delete modelParticle_;
	delete minimapSprite_;
	delete minimapPlayerSprite_;
	// シーンのクリア
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
	for (KamataEngine::Sprite* sprite : minimapGroundSprites_) {
		delete sprite;
	}
	minimapGroundSprites_.clear();
	for (EnemyBullet* bullet : enemyBullets_) {
		entityFactory_.ReleaseEnemyBullet(bullet);
	}
	enemyBullets_.clear();
	for (Enemy* enemy : enemies_) {
		delete enemy;
	}

	// delete score digit sprites
	for (KamataEngine::Sprite* s : scoreDigitSprites_) {
		delete s;
	}
}

void GameScene::Initialize() {
	dxCommon_ = DirectXCommon::GetInstance();
	input_ = Input::GetInstance();
	audio_ = Audio::GetInstance();

	player_ = new Player();
	gameEventSubject_.Subscribe(this);

	balanceTable_.LoadFromFile("Resources/gameBalance.csv");
	GameBalanceAccess::SetTable(&balanceTable_);

	tileMap_.LoadFromFile("Resources/map.csv");
	if (tileMap_.GetWidth() <= 0) {
		tileMap_.LoadFromFile("DirectXGame/Resources/map.csv");
	}

	skydome_ = new Skydome();

	modelCube_ = KamataEngine::Model::CreateFromOBJ("cube", true);
	modelPlayer_ = modelCube_;
	modelEnemy_ = KamataEngine::Model::CreateFromOBJ("boat", true);
	modelSkydome_ = Model::CreateFromOBJ("skydome", true);
	modelTitleObject_ = Model::CreateFromOBJ("title", true);

	// 敵弾用のOBJモデルを読み込む（ファイル名: Resources/bulletEnemy.obj を想定）
	modelEnemyBullet_ = KamataEngine::Model::CreateFromOBJ("bulletEnemy", true);

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

	// シーンクリア用アセット
	clearTextureHandle_ = KamataEngine::TextureManager::Load("kuria.png");
	clearSprite_ = KamataEngine::Sprite::Create(clearTextureHandle_, {0, 0});
	if (clearSprite_) {
		// kuria.png を画面全体にかぶせる
		clearSprite_->SetAnchorPoint({0.0f, 0.0f});
		clearSprite_->SetPosition({0.0f, 0.0f});
		clearSprite_->SetSize({(float)WinApp::kWindowWidth, (float)WinApp::kWindowHeight});
	}

	clearEmitter_ = new ParticleEmitter();
	if (clearEmitter_) {
		clearEmitter_->Initialize(modelParticle_);
	}

	// コンフェッティ用スプライトテクスチャ
	confettiTextureHandle_ = KamataEngine::TextureManager::Load("confetti.png");
	confettiParticles_.resize(kMaxConfetti_);
	for (size_t i = 0; i < kMaxConfetti_; ++i) {
		confettiParticles_[i].sprite = KamataEngine::Sprite::Create(confettiTextureHandle_, {0, 0});
		if (confettiParticles_[i].sprite) {
			confettiParticles_[i].sprite->SetSize({8.0f, 8.0f});
			confettiParticles_[i].sprite->SetAnchorPoint({0.5f, 0.5f});
			confettiParticles_[i].active = false;
			// デフォルト色: 白
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
	// ミニマップ上の敵弾アイコンは元の赤いテクスチャを使用（変更を取り消し）
	minimapEnemyBulletTextureHandle_ = KamataEngine::TextureManager::Load("missileRedBox.png");

	// 1. ミニマップ背景
	minimapSprite_ = KamataEngine::Sprite::Create(minimapTextureHandle_, {0, 0});
	minimapSprite_->SetPosition(kMinimapPosition_);
	minimapSprite_->SetAnchorPoint({0.0f, 0.0f}); // 左上をアンカーに
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

	// --- ビットマップフォントの初期化 ---
    digitTextureHandles_.resize(10);
    // Load textures for digits 1..9 by their names (as user stated), and try 0 if present
    for (int i = 1; i <= 9; ++i) {
        uint32_t h = 0;
        std::string base = std::to_string(i);
        // Try with common extensions first to avoid showing error dialogs from Load when called with bare name
        h = KamataEngine::TextureManager::Load((base + ".png").c_str());
        if (h == 0) h = KamataEngine::TextureManager::Load((base + ".PNG").c_str());
        if (h == 0) h = KamataEngine::TextureManager::Load(base.c_str());
        digitTextureHandles_[i] = h;
    }
    // Try to load '0' if available; otherwise leave 0 handle
    {
        uint32_t h0 = 0;
        h0 = KamataEngine::TextureManager::Load("0.png");
        if (h0 == 0) h0 = KamataEngine::TextureManager::Load("0.PNG");
        if (h0 == 0) h0 = KamataEngine::TextureManager::Load("0");
        digitTextureHandles_[0] = h0;
    }

	// Create 4 digit sprites (thousands, hundreds, tens, ones)
	scoreDigitSprites_.resize(4);
	for (int i = 0; i < 4; ++i) {
		// create placeholder sprite with no texture (handle 0) initially
		scoreDigitSprites_[i] = KamataEngine::Sprite::Create(0, {0, 0});
		if (scoreDigitSprites_[i]) {
			scoreDigitSprites_[i]->SetAnchorPoint({0.0f, 0.0f});
			scoreDigitSprites_[i]->SetSize({80.0f, 64.0f}); // 横に伸ばす
			// 数字の幅が80.0fなので、間隔を90.0fに設定して重ならないようにする
			// 画面右端から余白20.0fを引いた位置から左に配置
			scoreDigitSprites_[i]->SetPosition({(float)WinApp::kWindowWidth - (4 - i) * 90.0f - 20.0f, 20.0f});
			// hidden initially
			scoreDigitSprites_[i]->SetPosition({-100.0f, -100.0f});
		}
	}

	// Ensure initial score display is updated (show 0000 if 0 texture exists)
	UpdateScoreSprites();

	camera_.Initialize();

	const float playerHalfW = TileMap::kTileWidth * 0.375f;
	const float playerHalfH = TileMap::kTileHeight * 0.375f;
	KamataEngine::Vector3 spawnPos = tileMap_.FindSpawnPosition(playerHalfW, playerHalfH);
	playerIntroTargetPosition_ = spawnPos;
	playerIntroStartPosition_ = spawnPos;

	player_->Initialize(modelPlayer_, &camera_, spawnPos);
	player_->SetParent(nullptr);
	player_->SetTileMap(&tileMap_);
	player_->SetTrampolineSprings(&trampolineSprings_);
	mapRenderer_.Initialize(modelCube_, tileMap_);
	RebuildMinimapTiles();
	currentScreenX_ = 0;
	currentScreenY_ = 0;
	UpdateTitleCamera();
	// Initialize last player position for minimap rotation tracking
	lastPlayerPos_ = player_->GetWorldPosition();

	skydome_->Initialize(modelSkydome_, &camera_);
	worldTransformTitleObject_.Initialize();
	worldTransformTitleObject_.translation_ = {0.0f, 0.0f, -43.0f};
	worldTransformTitleObject_.UpdateMatrix();

	KamataEngine::AxisIndicator::GetInstance()->SetVisible(false);

	railCamera_ = new RailCamera();
	railCamera_->Initialize(railcameraPos, railcameraRad);
	cameraPositionAnchor_.Initialize();
	player_->SetRailCamera(railCamera_);
	player_->SetEnemies(&enemies_);
	player_->SetEntityFactory(&entityFactory_);

	LoadEnemyPopData();
	hitSoundHandle_ = audio_->LoadWave("./sound/parry.wav");

	// ホーミング弾生成タイマー初期化
	homingSpawnTimer_ = balanceTable_.GetInt("homingIntervalFrames", kHomingIntervalFrames_);
}

void GameScene::Update() {
	skydome_->Update();

	// State Pattern: 現在のシーン状態に更新を委譲（ポリモーフィズム）
	if (sceneState_) {
		sceneState_->Update(*this);
	}

	// Deferred scene clear: perform transition at a safe point after game update
	if (requestSceneClear_ && GetSceneStateKind() == SceneStateKind::Game) {
		requestSceneClear_ = false;
		TransitionToClearScene();
	}
}

void GameScene::Draw() {
	ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

	dxCommon_->ClearDepthBuffer();

	KamataEngine::Model::PreDraw(commandList);

	if (GetSceneStateKind() == SceneStateKind::Start || GetSceneStateKind() == SceneStateKind::TransitionToGame) {
		modelTitleObject_->Draw(worldTransformTitleObject_, camera_);
	} else if (GetSceneStateKind() == SceneStateKind::GameIntro || GetSceneStateKind() == SceneStateKind::Game || GetSceneStateKind() == SceneStateKind::TransitionFromGame || GetSceneStateKind() == SceneStateKind::Over) {

		mapRenderer_.Draw(camera_);
		DrawTrampolineSprings();
		player_->Draw();

		if (explosionEmitter_) {
			explosionEmitter_->Draw(camera_);
		}

		if ((GetSceneStateKind() == SceneStateKind::Game && isGameIntroFinished_) || GetSceneStateKind() == SceneStateKind::Over) {
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
	} else if (GetSceneStateKind() == SceneStateKind::Clear) {
		// draw skydome so background exists
		skydome_->Draw();
		// draw any particles for clear
		if (clearEmitter_) {
			clearEmitter_->Draw(camera_);
		}
	}

	KamataEngine::Model::PostDraw();

	KamataEngine::Sprite::PreDraw(commandList);

	if (GetSceneStateKind() == SceneStateKind::Start || GetSceneStateKind() == SceneStateKind::TransitionToGame) {
		if (taitoruSprite_) {
			taitoruSprite_->Draw();
		}
	}

	if (GetSceneStateKind() == SceneStateKind::TransitionToGame || GetSceneStateKind() == SceneStateKind::TransitionFromGame) {
		transitionSprite_->Draw();
	}

	// ミニマップはゲームシーンのみ表示
	if (GetSceneStateKind() == SceneStateKind::Game && isGameIntroFinished_) {
		if (minimapSprite_) {
			minimapSprite_->Draw(); // 背景枠
		}
		for (KamataEngine::Sprite* sprite : minimapGroundSprites_) {
			if (sprite) {
				sprite->Draw();
			}
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
	}

	// Draw score digits on top-right
	for (KamataEngine::Sprite* s : scoreDigitSprites_) {
		if (s) s->Draw();
	}

	if (GetSceneStateKind() == SceneStateKind::Clear) {
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
	assert(railCamera_ && "EnemySpawn: railCamera_ が null です");
	KamataEngine::Vector3 playerPos = railCamera_->GetWorldTransform().translation_;

	KamataEngine::Vector3 spawnPosWorld;
	spawnPosWorld.x = playerPos.x + position.x;
	spawnPosWorld.y = playerPos.y + position.y;
	spawnPosWorld.z = playerPos.z + position.z;

	Enemy* newEnemy = entityFactory_.CreateEnemy(modelEnemy_, spawnPosWorld, player_, this, &camera_);
	newEnemy->SetEventSubject(&gameEventSubject_);
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
		std::string command;
		getline(line_stream, command, ',');

		if (command.empty() || command[0] == '/' || command.find("//") == 0) {
			continue;
		}

		spawnCommandTable_.Execute(*this, command, line_stream);
	}

	hasSpawnedEnemies_ = true;
	enemyPopCommands.str("");
	enemyPopCommands.clear();
}

void GameScene::CheckAllCollisions() {
	if (!player_)
		return;

	if (GetSceneStateKind() == SceneStateKind::Over || GetSceneStateKind() == SceneStateKind::GameIntro) {
		return;
	}

	KamataEngine::Vector3 posA[3]{}, posB[3]{};
	const std::list<PlayerBullet*>& playerBullets = player_->GetBullets();

	// ポリモーフィズム: プレイヤーを基底クラス（GameCharacter）として扱う
	GameCharacter* playerCharacter = player_;

	// --- 自キャラ vs 敵弾（HP制） ---
	posA[0] = playerCharacter->GetWorldPosition();
	const float playerRadius = playerCharacter->GetCollisionRadius();
	
	// 回避中は無敵時間として、当たり判定を無効にする
	bool isPlayerRolling = player_->IsRolling();
	
	for (EnemyBullet* enemyBullet : enemyBullets_) {
		if (!enemyBullet || enemyBullet->IsDead())
			continue;

		// 回避中は当たり判定を無効にする
		if (isPlayerRolling) {
			continue;
		}

		// ホーミングを失った弾（回避された弾）は当たり判定を無効にする
		if (!enemyBullet->IsHoming() && enemyBullet->GetEvadedDeathTimer() >= 0) {
			continue;
		}

		// ポリモーフィズム: 敵弾を基底クラス（GameBullet）として扱う
		GameBullet* bullet = enemyBullet;
		posB[0] = bullet->GetWorldPosition();
		float distanceSquared = DistanceSquared(posA[0], posB[0]);
		float combinedRadius = playerRadius + bullet->GetCollisionRadius();
		float combinedRadiusSquared = combinedRadius * combinedRadius;
		if (distanceSquared <= combinedRadiusSquared) {

			// ポリモーフィズム: 実際の型（Player）の OnCollision が呼ばれる
			ApplyCollisionDamage(playerCharacter);
			MarkBulletDestroyed(bullet);

			if (playerCharacter->IsDead()) {
				TransitionToClearScene2();
				return;
			}
		}
	}

	/*
	// 自キャラ vs 隕石 の判定（再有効化する場合は GameCharacter* を使う）
	// posA[0] = playerCharacter->GetWorldPosition();
	// float meteorPlayerRadius = playerCharacter->GetCollisionRadius();
	// for (Meteorite* meteor : meteorites_) {
	//     ...
	//     ApplyCollisionDamage(playerCharacter);
	// }
	*/

	// --- 自弾 vs 敵キャラ ---
	for (Enemy* enemyObject : enemies_) {
		if (!enemyObject || enemyObject->IsDead())
			continue;

		// ポリモーフィズム: 敵を基底クラス（GameCharacter）として扱う
		GameCharacter* enemyCharacter = enemyObject;
		posA[1] = enemyCharacter->GetWorldPosition();
		const float enemyRadius = enemyCharacter->GetCollisionRadius();

		for (PlayerBullet* playerBullet : playerBullets) {
			if (!playerBullet || playerBullet->IsDead())
				continue;

			GameBullet* bullet = playerBullet;
			posB[1] = bullet->GetWorldPosition();
			float distanceSquared = DistanceSquared(posA[1], posB[1]);
			float combinedRadius = enemyRadius + bullet->GetCollisionRadius();
			float combinedRadiusSquared = combinedRadius * combinedRadius;
			if (distanceSquared <= combinedRadiusSquared) {
				// ポリモーフィズム: 実際の型（Enemy）の OnCollision が呼ばれる
				ApplyCollisionDamage(enemyCharacter);
				MarkBulletDestroyed(bullet);

				if (enemyCharacter->IsDead()) {
					hitCount++;
					audio_->playAudio(hitSound_, hitSoundHandle_, false, 0.7f);
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
	// Change: go to Clear scene so player sees clear screen instead of immediately returning to title
	ChangeSceneState(SceneStateClear::Instance());

	// reset score on clear
	score_ = 0;
	UpdateScoreSprites();
	requestSceneClear_ = false;

	gameOverTimer_ = 0; // (念のためタイマー系もリセット)
	hitCount = 0;       // 撃破数リセット
	hitCount2 = 0;

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
		player_->ResetBullets(); // (弾のリセットも行う)
	}

	for (Enemy* enemy : enemies_) {
		delete enemy;
	}
	enemies_.clear();
	for (EnemyBullet* bullet : enemyBullets_) {
		entityFactory_.ReleaseEnemyBullet(bullet);
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
	ChangeSceneState(SceneStateOver::Instance());
	if (player_) {
		player_->ChangeState(PlayerStateDead::Instance());
	}
	// reset score on game over
	score_ = 0;
	UpdateScoreSprites();
	requestSceneClear_ = false;
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
	const float kSpawnDistance = balanceTable_.GetFloat("meteoriteSpawnDistance", 800.0f);

	KamataEngine::Vector3 offset = randomDir * kSpawnDistance;
	KamataEngine::Vector3 spawnPos = cameraPos + offset;

	const float kBaseRadius = balanceTable_.GetFloat("meteoriteBaseRadius", 2.0f);
	const float kMinScale = balanceTable_.GetFloat("meteoriteMinScale", 1.0f);
	const float kMaxScale = balanceTable_.GetFloat("meteoriteMaxScale", 5.0f);

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
	// kVisualRadius は画面HEIGHTに対する比率なので、NDCでの半径は (2 * kVisualRadius)
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
	// WASDで視点移動中は吸い寄せを無効化
	KamataEngine::Input* input = KamataEngine::Input::GetInstance();
	bool isViewMoving = input->PushKey(DIK_W) || input->PushKey(DIK_S) || input->PushKey(DIK_A) || input->PushKey(DIK_D);
	
	if (bestTarget && !isViewMoving) {
		// アシスト自体は「判定」円で見つかったら実行（WASDが押されていない時のみ）
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

	explosionEmitter_->EmitBurst(
	    position, // 発生座標
	    10,       // 粒の数
	    4.0f,     // 速度
	    40.0f,    // 寿命 (30フレーム)
	    10.0f,    // 開始スケール
	    0.0f      // 終了スケール
	);
}

void GameScene::RebuildMinimapTiles() {
	for (KamataEngine::Sprite* sprite : minimapGroundSprites_) {
		delete sprite;
	}
	minimapGroundSprites_.clear();

	if (tileMap_.GetWidth() <= 0 || tileMap_.GetHeight() <= 0) {
		return;
	}

	const float tileW = kMinimapSize_.x / static_cast<float>(tileMap_.GetWidth());
	const float tileH = kMinimapSize_.y / static_cast<float>(tileMap_.GetHeight());

	for (int row = 0; row < tileMap_.GetHeight(); ++row) {
		for (int col = 0; col < tileMap_.GetWidth(); ++col) {
			if (!tileMap_.IsGround(col, row)) {
				continue;
			}

			KamataEngine::Sprite* sprite = KamataEngine::Sprite::Create(greenBoxTextureHandle_, {0.0f, 0.0f});
			if (!sprite) {
				continue;
			}
			sprite->SetAnchorPoint({0.0f, 0.0f});
			sprite->SetSize({tileW, tileH});
			sprite->SetPosition({kMinimapPosition_.x + static_cast<float>(col) * tileW, kMinimapPosition_.y + static_cast<float>(row) * tileH});
			sprite->SetColor({0.6f, 0.45f, 0.25f, 1.0f});
			minimapGroundSprites_.push_back(sprite);
		}
	}
}

KamataEngine::Vector2 GameScene::ConvertWorldToMinimapPosition(const KamataEngine::Vector3& worldPos) const {
	if (tileMap_.GetWidth() <= 0 || tileMap_.GetHeight() <= 0) {
		return kMinimapPosition_;
	}

	int col = 0;
	int row = 0;
	tileMap_.WorldToTile(worldPos.x, worldPos.y, col, row);

	const float tileW = kMinimapSize_.x / static_cast<float>(tileMap_.GetWidth());
	const float tileH = kMinimapSize_.y / static_cast<float>(tileMap_.GetHeight());

	KamataEngine::Vector2 pos;
	pos.x = kMinimapPosition_.x + (static_cast<float>(col) + 0.5f) * tileW;
	pos.y = kMinimapPosition_.y + (static_cast<float>(row) + 0.5f) * tileH;
	return pos;
}

KamataEngine::Vector2 GameScene::ConvertWorldToMinimap(const KamataEngine::Vector3& worldPos, const KamataEngine::Vector3& playerPos) {
	(void)playerPos;
	KamataEngine::Vector2 finalPos = ConvertWorldToMinimapPosition(worldPos);

	float minX = kMinimapPosition_.x;
	float maxX = kMinimapPosition_.x + kMinimapSize_.x;
	float minY = kMinimapPosition_.y;
	float maxY = kMinimapPosition_.y + kMinimapSize_.y;

	finalPos.x = std::clamp(finalPos.x, minX, maxX);
	finalPos.y = std::clamp(finalPos.y, minY, maxY);
	return finalPos;
}

void GameScene::UpdateTitleCamera() {
	camera_.translation_ = {0.0f, 0.0f, -50.0f};
	camera_.rotation_ = {0.0f, 0.0f, 0.0f};
	camera_.aspectRatio = static_cast<float>(WinApp::kWindowWidth) / static_cast<float>(WinApp::kWindowHeight);
	camera_.UpdateMatrix();
	camera_.TransferMatrix();
}

void GameScene::SyncFreeCameraFromPlayerScreen() {
	float left = 0.0f;
	float bottom = 0.0f;
	float right = 0.0f;
	float top = 0.0f;
	tileMap_.GetScreenViewportBounds(currentScreenX_, currentScreenY_, left, bottom, right, top);
	freeCameraCenterX_ = (left + right) * 0.5f;
	freeCameraCenterY_ = (bottom + top) * 0.5f;
}

void GameScene::ComputeFreeCameraViewSize(float& viewW, float& viewH) const {
	const float winW = static_cast<float>(WinApp::kWindowWidth);
	const float winH = static_cast<float>(WinApp::kWindowHeight);
	const float mapW = tileMap_.GetMapPixelWidth();
	const float mapH = tileMap_.GetMapPixelHeight();
	const float targetW = mapW > winW ? mapW : winW;
	const float targetH = mapH > winH ? mapH : winH;
	viewW = winW + (targetW - winW) * cameraZoomOut_;
	viewH = winH + (targetH - winH) * cameraZoomOut_;
}

void GameScene::ClampFreeCameraCenter(float viewW, float viewH) {
	float mapMinX = 0.0f;
	float mapMinY = 0.0f;
	float mapMaxX = 0.0f;
	float mapMaxY = 0.0f;
	tileMap_.GetMapWorldBounds(mapMinX, mapMinY, mapMaxX, mapMaxY);
	const float mapW = mapMaxX - mapMinX;
	const float mapH = mapMaxY - mapMinY;
	const float halfW = viewW * 0.5f;
	const float halfH = viewH * 0.5f;

	if (viewW >= mapW) {
		freeCameraCenterX_ = mapMinX + mapW * 0.5f;
	} else {
		freeCameraCenterX_ = std::clamp(freeCameraCenterX_, mapMinX + halfW, mapMaxX - halfW);
	}

	if (viewH >= mapH) {
		freeCameraCenterY_ = mapMinY + mapH * 0.5f;
	} else {
		freeCameraCenterY_ = std::clamp(freeCameraCenterY_, mapMinY + halfH, mapMaxY - halfH);
	}
}

void GameScene::ComputeCameraBounds(float& left, float& bottom, float& right, float& top) {
	left = 0.0f;
	bottom = 0.0f;
	right = 0.0f;
	top = 0.0f;
	tileMap_.GetScreenViewportBounds(currentScreenX_, currentScreenY_, left, bottom, right, top);

	if (!isFreeCamera_) {
		return;
	}

	float viewW = 0.0f;
	float viewH = 0.0f;
	ComputeFreeCameraViewSize(viewW, viewH);
	ClampFreeCameraCenter(viewW, viewH);

	left = freeCameraCenterX_ - viewW * 0.5f;
	right = freeCameraCenterX_ + viewW * 0.5f;
	bottom = freeCameraCenterY_ - viewH * 0.5f;
	top = freeCameraCenterY_ + viewH * 0.5f;
}

KamataEngine::Vector3 GameScene::ConvertScreenToWorld(float screenX, float screenY) {
	float left = 0.0f;
	float bottom = 0.0f;
	float right = 0.0f;
	float top = 0.0f;
	ComputeCameraBounds(left, bottom, right, top);

	const float winW = static_cast<float>(WinApp::kWindowWidth);
	const float winH = static_cast<float>(WinApp::kWindowHeight);
	const float worldX = left + (screenX / winW) * (right - left);
	const float worldY = top - (screenY / winH) * (top - bottom);

	KamataEngine::Vector3 pos;
	pos.x = worldX;
	pos.y = worldY;
	pos.z = 0.5f;
	return pos;
}

void GameScene::UpdateTrampolinePlacement() {
	if (!input_ || !isGameIntroFinished_ || !player_) {
		hasTrampolinePreview_ = false;
		return;
	}

	const KamataEngine::Vector2& mousePos = input_->GetMousePosition();
	trampolinePreviewPos_ = ConvertScreenToWorld(mousePos.x, mousePos.y);
	hasTrampolinePreview_ = true;

	const float playerHalfW = player_->GetHalfWidth();
	const float playerHalfH = player_->GetHalfHeight();
	const TrampolineSpringType nextType = TrampolineSpring::GetPlacementType(nextTrampolineTypeIndex_);

	trampolinePreview_.SetType(nextType);
	trampolinePreview_.SetCenter(trampolinePreviewPos_, playerHalfW, playerHalfH);

	float springHalfW = 0.0f;
	float springHalfH = 0.0f;
	trampolinePreview_.GetHalfSize(springHalfW, springHalfH);
	tileMap_.ClampPositionToMapBounds(trampolinePreviewPos_.x, trampolinePreviewPos_.y, springHalfW, springHalfH);
	trampolinePreview_.SetCenter(trampolinePreviewPos_, playerHalfW, playerHalfH);

	if (input_->IsTriggerMouse(0) && !input_->IsPressMouse(1)) {
		TrampolineSpring spring;
		spring.SetType(nextType);
		spring.SetCenter(trampolinePreviewPos_, playerHalfW, playerHalfH);
		trampolineSprings_.push_back(std::move(spring));
		nextTrampolineTypeIndex_++;
	}
}

void GameScene::DrawTrampolineSprings() {
	if (!modelCube_ || !isGameIntroFinished_) {
		return;
	}

	for (const TrampolineSpring& spring : trampolineSprings_) {
		spring.Draw(modelCube_, camera_);
	}

	if (hasTrampolinePreview_) {
		trampolinePreview_.Draw(modelCube_, camera_);
	}
}

void GameScene::UpdateMapCamera() {
	float left = 0.0f;
	float bottom = 0.0f;
	float right = 0.0f;
	float top = 0.0f;
	ComputeCameraBounds(left, bottom, right, top);

	Matrix4x4 identity = {};
	identity.m[0][0] = 1.0f;
	identity.m[1][1] = 1.0f;
	identity.m[2][2] = 1.0f;
	identity.m[3][3] = 1.0f;

	camera_.matView = identity;
	camera_.matProjection = MakeOrthographicMatrix(left, top, right, bottom, -100.0f, 100.0f);
	camera_.TransferMatrix();
}

void GameScene::UpdateCameraControl() {
	if (!input_) {
		return;
	}

	if (player_ && player_->IsMovingInput()) {
		isFreeCamera_ = false;
		cameraZoomOut_ = 0.0f;
		return;
	}

	const int32_t wheel = input_->GetWheel();
	if (wheel != 0) {
		if (!isFreeCamera_) {
			SyncFreeCameraFromPlayerScreen();
			isFreeCamera_ = true;
		}

		cameraZoomOut_ -= static_cast<float>(wheel) / 120.0f * 0.12f;
		cameraZoomOut_ = std::clamp(cameraZoomOut_, 0.0f, 1.0f);

		float viewW = 0.0f;
		float viewH = 0.0f;
		ComputeFreeCameraViewSize(viewW, viewH);
		ClampFreeCameraCenter(viewW, viewH);
	}

	if (input_->IsPressMouse(1)) {
		if (!isFreeCamera_) {
			SyncFreeCameraFromPlayerScreen();
			isFreeCamera_ = true;
		}

		float viewW = 0.0f;
		float viewH = 0.0f;
		ComputeFreeCameraViewSize(viewW, viewH);

		const float winW = static_cast<float>(WinApp::kWindowWidth);
		const float winH = static_cast<float>(WinApp::kWindowHeight);
		const Input::MouseMove mouseMove = input_->GetMouseMove();
		const float worldPerPixelX = viewW / winW;
		const float worldPerPixelY = viewH / winH;

		const float panSpeed = 1.2f;
		freeCameraCenterX_ -= static_cast<float>(mouseMove.lX) * worldPerPixelX * panSpeed;
		freeCameraCenterY_ += static_cast<float>(mouseMove.lY) * worldPerPixelY * panSpeed;
		ClampFreeCameraCenter(viewW, viewH);
	}
}

void GameScene::UpdatePlayerScreenTransition() {
	if (!player_) {
		return;
	}

	KamataEngine::Vector3 pos = player_->GetWorldPosition();
	const float halfW = player_->GetHalfWidth();
	const float halfH = player_->GetHalfHeight();

	float left = 0.0f;
	float bottom = 0.0f;
	float right = 0.0f;
	float top = 0.0f;
	tileMap_.GetScreenViewportBounds(currentScreenX_, currentScreenY_, left, bottom, right, top);

	int newScreenX = currentScreenX_;
	int newScreenY = currentScreenY_;
	bool transitioned = false;

	if (pos.x - halfW < left && currentScreenX_ > 0) {
		newScreenX = currentScreenX_ - 1;
		tileMap_.GetScreenViewportBounds(newScreenX, newScreenY, left, bottom, right, top);
		pos.x = right - halfW - 2.0f;
		transitioned = true;
	} else if (pos.x + halfW > right && currentScreenX_ < tileMap_.GetScreenCountX() - 1) {
		newScreenX = currentScreenX_ + 1;
		tileMap_.GetScreenViewportBounds(newScreenX, newScreenY, left, bottom, right, top);
		pos.x = left + halfW + 2.0f;
		transitioned = true;
	}

	if (pos.y - halfH < bottom && currentScreenY_ < tileMap_.GetScreenCountY() - 1) {
		newScreenY = currentScreenY_ + 1;
		tileMap_.GetScreenViewportBounds(newScreenX, newScreenY, left, bottom, right, top);
		pos.y = top - halfH - 4.0f;
		player_->SetVelocityY(-2.0f);
		player_->SetPosition(pos);
		currentScreenX_ = newScreenX;
		currentScreenY_ = newScreenY;
		return;
	}

	if (pos.y + halfH > top && currentScreenY_ > 0) {
		newScreenY = currentScreenY_ - 1;
		tileMap_.GetScreenViewportBounds(newScreenX, newScreenY, left, bottom, right, top);
		pos.y = bottom + halfH + 4.0f;
		player_->SetVelocityY(0.0f);
		transitioned = true;
	}

	if (transitioned) {
		currentScreenX_ = newScreenX;
		currentScreenY_ = newScreenY;
		player_->SetPosition(pos);
	} else {
		tileMap_.GetScreenFromWorld(pos.x, pos.y, currentScreenX_, currentScreenY_);
	}
}

// データドリブン: イベント種別ごとの処理テーブル（switch 分岐の代替）
namespace {
using GameEventHandlerFn = void (*)(GameScene&, const GameEvent&);

void HandleExplosionRequested(GameScene& scene, const GameEvent& event) { scene.RequestExplosion(event.position); }

void HandleEnemyDestroyed(GameScene& scene, const GameEvent& event) { scene.AddScore(event.scoreDelta); }

void HandleScoreChanged(GameScene& scene, const GameEvent& event) { scene.SetScoreValue(event.totalScore); }

const std::unordered_map<GameEventType, GameEventHandlerFn>& GetGameEventHandlerTable() {
	static const std::unordered_map<GameEventType, GameEventHandlerFn> table = {
	    {GameEventType::ExplosionRequested, HandleExplosionRequested},
	    {GameEventType::EnemyDestroyed, HandleEnemyDestroyed},
	    {GameEventType::ScoreChanged, HandleScoreChanged},
	};
	return table;
}
} // namespace

// Observer Pattern: 衝突応答などのゲームイベントを処理
void GameScene::OnGameEvent(const GameEvent& event) {
	const auto& table = GetGameEventHandlerTable();
	auto it = table.find(event.type);
	if (it != table.end()) {
		it->second(*this, event);
	}
}

// Score handling
void GameScene::SetScoreValue(int value) {
	score_ = value;
	if (score_ < 0) {
		score_ = 0;
	}
	if (score_ > kMaxScore_) {
		score_ = kMaxScore_;
	}
	UpdateScoreSprites();
}

void GameScene::AddScore(int points) {
	if (points <= 0) return;
	score_ += points;
	if (score_ > kMaxScore_) score_ = kMaxScore_;
	UpdateScoreSprites();

	// If score reaches threshold, request clear the scene at a safe point
	if (GetSceneStateKind() == SceneStateKind::Game && score_ >= balanceTable_.GetInt("clearScoreThreshold", 600)) {
		requestSceneClear_ = true;
	}
}

void GameScene::UpdateScoreSprites() {
	int display = score_;
	// clamp
	if (display < 0) display = 0;
	if (display > kMaxScore_) display = kMaxScore_;

	int digits[4] = {0, 0, 0, 0};
	digits[3] = display % 10;
	digits[2] = (display / 10) % 10;
	digits[1] = (display / 100) % 10;
	digits[0] = (display / 1000) % 10;

	// Always show four digits (use '0' texture for leading zeros if available)
	for (int i = 0; i < 4; ++i) {
		int d = digits[i];
		uint32_t handle = 0;
		if (d >= 0 && d < (int)digitTextureHandles_.size()) handle = digitTextureHandles_[d];
		if (handle != 0) {
			// recreate sprite with digit texture
			if (scoreDigitSprites_[i]) delete scoreDigitSprites_[i];
			scoreDigitSprites_[i] = KamataEngine::Sprite::Create(handle, {0,0});
			if (scoreDigitSprites_[i]) {
				scoreDigitSprites_[i]->SetAnchorPoint({0.0f,0.0f});
				scoreDigitSprites_[i]->SetSize({80.0f, 64.0f}); // 横に伸ばす
				// 数字の幅が80.0fなので、間隔を90.0fに設定して重ならないようにする
				// 画面右端から余白20.0fを引いた位置から左に配置
				scoreDigitSprites_[i]->SetPosition({(float)WinApp::kWindowWidth - (4 - i) * 70.0f - 20.0f, 20.0f});
			}
		} else {
			// texture missing: hide
			if (scoreDigitSprites_[i]) scoreDigitSprites_[i]->SetPosition({-100.0f, -100.0f});
		}
	}
}