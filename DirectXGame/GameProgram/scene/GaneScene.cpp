#include "GaneScene.h"
#include "GameBalanceAccess.h"
#include "GameBullet.h"
#include "GameCharacter.h"
#include "MT.h"
#include "SpawnCommandTable.h"
#include "3d/AxisIndicator.h"
#include "base/WinApp.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

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
	StopStageBgm();
	if (osCursorHidden_) {
		while (ShowCursor(TRUE) < 0) {
		}
		osCursorHidden_ = false;
	}
	delete modelCube_;
	delete modelBlocks_;
	delete modelDeleteBlocks_;
	delete modelSpikeTile_;
	delete modelPortal_;
	delete modelKey_;
	delete modelSpringUp_;
	delete modelSpringDown_;
	delete modelSpringRight_;
	delete modelSpringLeft_;
	delete modelSpringArrow_;
	delete modelRaycasting_;
	modelPlayer_ = nullptr;
	delete modelEnemy_;
	delete modelWall_;
	delete title2DSprite_;
	title2DSprite_ = nullptr;
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
	delete aimAssistCircleSprite_;
	// 追加: 右/左キー表示用スプライトを解放
	delete lightSprite_;
	delete leftSprite_;
	delete shiftSprite_; // Shiftスプライトを解放
	delete explosionEmitter_;
	delete modelParticle_;
	delete minimapSprite_;
	delete minimapPlayerSprite_;
	delete jumpSpringChargeSprite_;
	// シーンのクリア
	delete clearEmitter_;
	delete goalPortalEmitter_;
	delete clearSprite_;
	delete gameOverSprite_;
	delete clearScreenBackgroundSprite_;
	for (KamataEngine::Sprite* sprite : zoomMarginFillSprites_) {
		delete sprite;
	}
	zoomMarginFillSprites_.fill(nullptr);
	delete stageClearTitleReturnSprite_;
	delete stageClearNextStageSprite_;
	delete stageSelectBackgroundSprite_;
	delete stageSelectCursorSprite_;
	delete pauseDimSprite_;
	delete pauseTitleSprite_;
	delete pausePlayMoveSprite_;
	delete pausePlayMovePointSprite_;
	delete gameplayPauseStopSprite_;
	delete spikeRewindDimSprite_;
	delete fontRenderer_;
	delete tutorialFrameSprite_;
	delete springCollectLabelSprite_;
	delete springCollectLabelVerticalSprite_;
	for (StageSelectSlotUi& ui : stageSelectSlotUi_) {
		for (int d = 0; d < ui.digitCount; ++d) {
			delete ui.digitSprites[d];
			ui.digitSprites[d] = nullptr;
		}
		ui.digitCount = 0;
	}
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
	delete spikeLifeHeartIconSprite_;
	delete spikeLifeMultiplySprite_;
	delete spikeLifeTensDigitSprite_;
	delete spikeLifeOnesDigitSprite_;
	delete tutorialTimerOnesSprite_;
	delete tutorialTimerTenthsSprite_;
	delete tutorialTimerHundredthsSprite_;
	delete tutorialTimerTensSprite_;
	spikeLifeHeartIconSprite_ = nullptr;
	spikeLifeMultiplySprite_ = nullptr;
	spikeLifeTensDigitSprite_ = nullptr;
	spikeLifeOnesDigitSprite_ = nullptr;
	tutorialTimerOnesSprite_ = nullptr;
	tutorialTimerTenthsSprite_ = nullptr;
	tutorialTimerHundredthsSprite_ = nullptr;
	tutorialTimerTensSprite_ = nullptr;
	for (SpringPlacementHudUi& ui : springPlacementHudUi_) {
		delete ui.iconSprite;
		delete ui.multiplySprite;
		delete ui.tensDigitSprite;
		delete ui.onesDigitSprite;
		ui = {};
	}
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

	skydome_ = new Skydome();

	modelCube_ = KamataEngine::Model::CreateFromOBJ("cube", true);
	modelBlocks_ = KamataEngine::Model::CreateFromOBJ("blocks", true);
	modelDeleteBlocks_ = KamataEngine::Model::CreateFromOBJ("deleteblocks", true);
	modelSpikeTile_ = KamataEngine::Model::CreateFromOBJ("needle", true);
	modelPortal_ = KamataEngine::Model::CreateFromOBJ("portal", true);
	modelKey_ = KamataEngine::Model::CreateFromOBJ("key", true);
	goalPortalEmitter_ = new ParticleEmitter();
	if (goalPortalEmitter_ && modelPortal_) {
		goalPortalEmitter_->Initialize(modelPortal_);
	}
	modelSpringUp_ = KamataEngine::Model::CreateFromOBJ("Up", true);
	modelSpringDown_ = KamataEngine::Model::CreateFromOBJ("down", true);
	modelSpringRight_ = KamataEngine::Model::CreateFromOBJ("light", true);
	modelSpringLeft_ = KamataEngine::Model::CreateFromOBJ("left", true);
	modelSpringArrow_ = KamataEngine::Model::CreateFromOBJ("whiteArrow", true);
	modelRaycasting_ = KamataEngine::Model::CreateFromOBJ("Raycasting", true);
	if (modelRaycasting_) {
		for (KamataEngine::WorldTransform& dotTransform : trajectoryDotTransforms_) {
			dotTransform.Initialize();
		}
		isTrajectoryDotPoolReady_ = true;
	}
	modelPlayer_ = modelCube_;
	modelEnemy_ = KamataEngine::Model::CreateFromOBJ("boat", true);
	modelWall_ = Model::CreateFromOBJ("wall2", false);

	// タイトル画面（2D）
	title2DTextureHandle_ = KamataEngine::TextureManager::Load("title2D.png");
	title2DSprite_ = KamataEngine::Sprite::Create(title2DTextureHandle_, {0, 0});
	if (title2DSprite_) {
		title2DSprite_->SetAnchorPoint({0.0f, 0.0f});
		title2DSprite_->SetPosition({0.0f, 0.0f});
		title2DSprite_->SetSize({static_cast<float>(WinApp::kWindowWidth), static_cast<float>(WinApp::kWindowHeight)});
	}

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

	spikeRewindDimSprite_ = KamataEngine::Sprite::Create(transitionTextureHandle_, {0, 0});
	if (spikeRewindDimSprite_) {
		spikeRewindDimSprite_->SetAnchorPoint({0.0f, 0.0f});
		spikeRewindDimSprite_->SetPosition({0.0f, 0.0f});
		spikeRewindDimSprite_->SetSize({static_cast<float>(WinApp::kWindowWidth), static_cast<float>(WinApp::kWindowHeight)});
		spikeRewindDimSprite_->SetColor({0.0f, 0.0f, 0.0f, 0.0f});
	}

	reticleTextureHandle_ = KamataEngine::TextureManager::Load("reticle.png");
	reticleSprite_ = KamataEngine::Sprite::Create(reticleTextureHandle_, {0, 0});
	reticleSprite_->SetPosition(screenCenter);
	reticleSprite_->SetAnchorPoint({0.5f, 0.5f});

	aimAssistCircleTextureHandle_ = KamataEngine::TextureManager::Load("aimCircle.png");
	aimAssistCircleSprite_ = KamataEngine::Sprite::Create(aimAssistCircleTextureHandle_, {0, 0});
	aimAssistCircleSprite_->SetSize({0.0f, 0.0f});

	modelParticle_ = KamataEngine::Model::CreateFromOBJ("flare", true);
	explosionEmitter_ = new ParticleEmitter();
	if (explosionEmitter_) {
		explosionEmitter_->Initialize(modelParticle_);
	}

	// ゲームオーバー用アセット（over.png を画面全体にかぶせる）
	gameOverTextureHandle_ = KamataEngine::TextureManager::Load("over.png");
	gameOverSprite_ = KamataEngine::Sprite::Create(gameOverTextureHandle_, {0, 0});
	if (gameOverSprite_) {
		gameOverSprite_->SetAnchorPoint({0.0f, 0.0f});
		gameOverSprite_->SetPosition({0.0f, 0.0f});
		gameOverSprite_->SetSize({static_cast<float>(WinApp::kWindowWidth), static_cast<float>(WinApp::kWindowHeight)});
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

	if (transitionTextureHandle_ != 0) {
		clearScreenBackgroundSprite_ = KamataEngine::Sprite::Create(transitionTextureHandle_, {0, 0});
		if (clearScreenBackgroundSprite_) {
			clearScreenBackgroundSprite_->SetAnchorPoint({0.0f, 0.0f});
			clearScreenBackgroundSprite_->SetPosition({0.0f, 0.0f});
			clearScreenBackgroundSprite_->SetSize(
			    {static_cast<float>(WinApp::kWindowWidth), static_cast<float>(WinApp::kWindowHeight)});
		}

		for (KamataEngine::Sprite*& sprite : zoomMarginFillSprites_) {
			sprite = KamataEngine::Sprite::Create(transitionTextureHandle_, {0, 0});
			if (sprite) {
				sprite->SetAnchorPoint({0.0f, 0.0f});
			}
		}
	}

	stageClearTitleReturnTextureHandle_ = KamataEngine::TextureManager::Load("title.png");
	stageClearTitleReturnSprite_ = KamataEngine::Sprite::Create(stageClearTitleReturnTextureHandle_, {0, 0});
	if (stageClearTitleReturnSprite_) {
		stageClearTitleReturnSprite_->SetAnchorPoint({0.0f, 0.0f});
		stageClearTitleReturnSprite_->SetSize({320.0f, 80.0f});
	}

	stageClearNextStageTextureHandle_ = KamataEngine::TextureManager::Load("nextstage.png");
	stageClearNextStageSprite_ = KamataEngine::Sprite::Create(stageClearNextStageTextureHandle_, {0, 0});
	if (stageClearNextStageSprite_) {
		stageClearNextStageSprite_->SetAnchorPoint({0.0f, 0.0f});
		stageClearNextStageSprite_->SetSize({320.0f, 80.0f});
	}
	LayoutStageClearButtons();

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
	heartTextureHandle_ = KamataEngine::TextureManager::Load("heart.png");

	// 1. ミニマップ背景
	minimapSprite_ = KamataEngine::Sprite::Create(minimapTextureHandle_, {0, 0});
	minimapSprite_->SetPosition(kMinimapPosition_);
	minimapSprite_->SetAnchorPoint({0.0f, 0.0f}); // 左上をアンカーに
	minimapSprite_->SetSize(kMinimapSize_);

	// 2. ミニマップ上の自機
	minimapPlayerSprite_ = KamataEngine::Sprite::Create(minimapPlayerTextureHandle_, {0, 0});
	minimapPlayerSprite_->SetAnchorPoint({0.5f, 0.5f}); // 中央をアンカーに
	minimapPlayerSprite_->SetSize({10.0f, 10.0f});      // 仮サイズ

	jumpSpringChargeSprite_ = KamataEngine::Sprite::Create(greenBoxTextureHandle_, {0.0f, 0.0f});
	jumpSpringChargeSprite_->SetAnchorPoint({0.5f, 0.5f});
	jumpSpringChargeSprite_->SetColor({0.6f, 0.45f, 0.25f, 0.75f});

	tutorialFrameSprite_ = KamataEngine::Sprite::Create(greenBoxTextureHandle_, {0.0f, 0.0f});
	if (tutorialFrameSprite_) {
		tutorialFrameSprite_->SetAnchorPoint({0.5f, 0.5f});
		tutorialFrameSprite_->SetColor({0.35f, 0.95f, 0.45f, 0.85f});
	}

	// 残機表示: ハート×数字（ビットマップフォント）
	spikeLifeHeartIconSprite_ = KamataEngine::Sprite::Create(heartTextureHandle_, {0.0f, 0.0f});
	if (spikeLifeHeartIconSprite_) {
		spikeLifeHeartIconSprite_->SetAnchorPoint({0.0f, 0.5f});
		spikeLifeHeartIconSprite_->SetSize(kHeartIconSize_);
	}
	const uint32_t heartMultiplyHandle = KamataEngine::TextureManager::Load("kakeru.png");
	spikeLifeMultiplySprite_ = KamataEngine::Sprite::Create(heartMultiplyHandle, {0.0f, 0.0f});
	if (spikeLifeMultiplySprite_) {
		spikeLifeMultiplySprite_->SetAnchorPoint({0.0f, 0.5f});
	}
	// 数字スプライトはビットマップフォント読み込み後に生成する（下部参照）

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

	LoadStage(0);

	skydome_->Initialize(&camera_);
	skydome_->SetWallBackdrop(modelWall_);

	KamataEngine::AxisIndicator::GetInstance()->SetVisible(false);

	railCamera_ = new RailCamera();
	railCamera_->Initialize(railcameraPos, railcameraRad);
	cameraPositionAnchor_.Initialize();
	player_->SetRailCamera(railCamera_);
	player_->SetEnemies(&enemies_);
	player_->SetEntityFactory(&entityFactory_);

	LoadEnemyPopData();
	hitSoundHandle_ = audio_->LoadWave("./sound/parry.wav");
	keyCollectSoundHandle_ = audio_->LoadWave("./sound/Attack.wav");
	stageBgmHandle_ = audio_->LoadWave("./sound/battle.wav");
	stageBgmVoice_ = -1;

	// ホーミング弾生成タイマー初期化
	homingSpawnTimer_ = balanceTable_.GetInt("homingIntervalFrames", kHomingIntervalFrames_);

	for (int i = 0; i < 10; ++i) {
		const std::string path = "bitmapfont/" + std::to_string(i) + ".png";
		stageSelectDigitTextureHandles_[static_cast<size_t>(i)] = KamataEngine::TextureManager::Load(path.c_str());
		if (stageSelectDigitTextureHandles_[static_cast<size_t>(i)] != 0) {
			isStageSelectFontReady_ = true;
		}
	}

	// Noto Sans JP (.ttf) から起動時に文字アトラスを自動生成して日本語描画を可能にする。
	// extraChars に入れた漢字がアトラスに焼き込まれる（英数字・かな・記号は常に収録）。
	fontRenderer_ = new FontRenderer();
	fontRenderer_->Initialize(
	    L"Resources/Noto_Sans_JP/static/NotoSansJP-Regular.ttf", // フォントファイル
	    L"Noto Sans JP",                                         // ファミリ名
	    48,                                                      // 焼き込む解像度(px)
	    "Resources/generatedfont/notosans_atlas.png",           // 生成先
	    "generatedfont/notosans_atlas.png",                     // 読み込み名
	    L"回収する開始選択決定戻次前操作説明終了再挑戦得点残機時間空中"
	    L"上下左右中大小新記録最高位置移動跳飛壁床天井敵弾避進退全消"
	    L"成功失敗一二三四五六七八九十百千万点数秒分速度力方向目標地面"
	    L"取設置置換押離長溜貯留放発射当判定復活反発跳返弾力調整難易度"
	    L"枠触種類変長棘巻戻回初赤時拡縮画面青解除停止乗紫重同切替間緑再"
	    L"行テキスト仕組不可能状態合中鍵消");

	// 残機の数字スプライト（ビットマップフォントのテクスチャで生成）
	{
		uint32_t digitHandle = stageSelectDigitTextureHandles_[0];
		if (digitHandle == 0 && !digitTextureHandles_.empty()) {
			digitHandle = digitTextureHandles_[0] != 0 ? digitTextureHandles_[0] : digitTextureHandles_[1];
		}
		if (digitHandle == 0) {
			digitHandle = heartTextureHandle_;
		}
		spikeLifeTensDigitSprite_ = KamataEngine::Sprite::Create(digitHandle, {0.0f, 0.0f});
		if (spikeLifeTensDigitSprite_) {
			spikeLifeTensDigitSprite_->SetAnchorPoint({0.0f, 0.5f});
		}
		spikeLifeOnesDigitSprite_ = KamataEngine::Sprite::Create(digitHandle, {0.0f, 0.0f});
		if (spikeLifeOnesDigitSprite_) {
			spikeLifeOnesDigitSprite_->SetAnchorPoint({0.0f, 0.5f});
		}
		tutorialTimerOnesSprite_ = KamataEngine::Sprite::Create(digitHandle, {0.0f, 0.0f});
		if (tutorialTimerOnesSprite_) {
			tutorialTimerOnesSprite_->SetAnchorPoint({0.0f, 0.5f});
		}
		tutorialTimerTenthsSprite_ = KamataEngine::Sprite::Create(digitHandle, {0.0f, 0.0f});
		if (tutorialTimerTenthsSprite_) {
			tutorialTimerTenthsSprite_->SetAnchorPoint({0.0f, 0.5f});
		}
		tutorialTimerHundredthsSprite_ = KamataEngine::Sprite::Create(digitHandle, {0.0f, 0.0f});
		if (tutorialTimerHundredthsSprite_) {
			tutorialTimerHundredthsSprite_->SetAnchorPoint({0.0f, 0.5f});
		}
		tutorialTimerTensSprite_ = KamataEngine::Sprite::Create(digitHandle, {0.0f, 0.0f});
		if (tutorialTimerTensSprite_) {
			tutorialTimerTensSprite_->SetAnchorPoint({0.0f, 0.5f});
		}
	}

	InitializeSpringPlacementHud();

	constexpr float kStageSelectRow1Y = 280.0f;
	constexpr float kStageSelectRow2Y = 520.0f;
	constexpr float kStageSelectColStartX = 160.0f;
	constexpr float kStageSelectColSpacing = 240.0f;
	constexpr float kStageSelectSlotHalfW = 96.0f;
	constexpr float kStageSelectSlotHalfH = 60.0f;
	for (int i = 0; i < kStageCount; ++i) {
		const int row = i / kStageSelectColsPerRow;
		const int col = i % kStageSelectColsPerRow;
		stageSelectSlots_[static_cast<size_t>(i)].displayNumber = i + 1;
		stageSelectSlots_[static_cast<size_t>(i)].centerX = kStageSelectColStartX + kStageSelectColSpacing * static_cast<float>(col);
		stageSelectSlots_[static_cast<size_t>(i)].centerY = (row == 0) ? kStageSelectRow1Y : kStageSelectRow2Y;
		stageSelectSlots_[static_cast<size_t>(i)].halfW = kStageSelectSlotHalfW;
		stageSelectSlots_[static_cast<size_t>(i)].halfH = kStageSelectSlotHalfH;
	}

	for (int i = 0; i < kStageCount; ++i) {
		StageSelectSlotUi& ui = stageSelectSlotUi_[static_cast<size_t>(i)];
		ui.digitCount = 0;
		const std::string label = std::to_string(i + 1);
		for (char ch : label) {
			if (ui.digitCount >= 2) {
				break;
			}
			if (ch < '0' || ch > '9') {
				continue;
			}
			const int digit = ch - '0';
			const uint32_t textureHandle = stageSelectDigitTextureHandles_[static_cast<size_t>(digit)];
			if (textureHandle == 0) {
				continue;
			}

			ui.digitSprites[ui.digitCount] = KamataEngine::Sprite::Create(textureHandle, {-100.0f, -100.0f});
			if (!ui.digitSprites[ui.digitCount]) {
				continue;
			}

			ui.digitSprites[ui.digitCount]->SetAnchorPoint({0.0f, 0.5f});
			ui.digitSprites[ui.digitCount]->SetSize({kStageSelectDigitSize, kStageSelectDigitSize});
			ui.digitCount++;
			isStageSelectFontReady_ = true;
		}
	}

	if (transitionTextureHandle_ != 0) {
		stageSelectBackgroundSprite_ = KamataEngine::Sprite::Create(transitionTextureHandle_, {0, 0});
		if (stageSelectBackgroundSprite_) {
			stageSelectBackgroundSprite_->SetAnchorPoint({0.0f, 0.0f});
			stageSelectBackgroundSprite_->SetPosition({0.0f, 0.0f});
			stageSelectBackgroundSprite_->SetSize(
			    {static_cast<float>(WinApp::kWindowWidth), static_cast<float>(WinApp::kWindowHeight)});
		}
	}

	if (greenBoxTextureHandle_ != 0) {
		stageSelectCursorSprite_ = KamataEngine::Sprite::Create(greenBoxTextureHandle_, {0, 0});
		if (stageSelectCursorSprite_) {
			stageSelectCursorSprite_->SetAnchorPoint({0.5f, 0.5f});
		}
	}

	if (transitionTextureHandle_ != 0) {
		pauseDimSprite_ = KamataEngine::Sprite::Create(transitionTextureHandle_, {0, 0});
		if (pauseDimSprite_) {
			pauseDimSprite_->SetAnchorPoint({0.0f, 0.0f});
			pauseDimSprite_->SetPosition({0.0f, 0.0f});
			pauseDimSprite_->SetSize(
			    {static_cast<float>(WinApp::kWindowWidth), static_cast<float>(WinApp::kWindowHeight)});
			pauseDimSprite_->SetColor({0.0f, 0.0f, 0.0f, kPauseMenuDimAlpha_});
		}
	}

	gameplayPauseStopTextureHandle_ = KamataEngine::TextureManager::Load("stop.png");
	if (gameplayPauseStopTextureHandle_ != 0) {
		gameplayPauseStopSprite_ = KamataEngine::Sprite::Create(gameplayPauseStopTextureHandle_, {0, 0});
		if (gameplayPauseStopSprite_) {
			gameplayPauseStopSprite_->SetAnchorPoint({0.5f, 0.5f});
			const KamataEngine::Vector2& nativeSize = gameplayPauseStopSprite_->GetSize();
			if (nativeSize.y > 0.0f) {
				gameplayPauseStopAspect_ = nativeSize.x / nativeSize.y;
			}
		}
	}

	if (stageClearTitleReturnTextureHandle_ != 0) {
		pauseTitleSprite_ = KamataEngine::Sprite::Create(stageClearTitleReturnTextureHandle_, {0, 0});
		if (pauseTitleSprite_) {
			pauseTitleSprite_->SetAnchorPoint({0.5f, 0.5f});
			pauseTitleSprite_->SetPosition({kPauseMenuLeftButtonX_, kPauseMenuButtonY_});
			pauseTitleSprite_->SetSize({kPauseMenuButtonW_, kPauseMenuButtonH_});
		}
	}

	pausePlayMoveTextureHandle_ = KamataEngine::TextureManager::Load("playmove.png");
	if (pausePlayMoveTextureHandle_ != 0) {
		pausePlayMoveSprite_ = KamataEngine::Sprite::Create(pausePlayMoveTextureHandle_, {0, 0});
		if (pausePlayMoveSprite_) {
			pausePlayMoveSprite_->SetAnchorPoint({0.5f, 0.5f});
			pausePlayMoveSprite_->SetPosition({kPauseMenuRightButtonX_, kPauseMenuButtonY_});
			pausePlayMoveSprite_->SetSize({kPauseMenuButtonW_, kPauseMenuButtonH_});
		}
	}

	pausePlayMovePointTextureHandle_ = KamataEngine::TextureManager::Load("playmovepoint.png");
	if (pausePlayMovePointTextureHandle_ != 0) {
		pausePlayMovePointSprite_ = KamataEngine::Sprite::Create(pausePlayMovePointTextureHandle_, {0, 0});
		if (pausePlayMovePointSprite_) {
			pausePlayMovePointSprite_->SetAnchorPoint({0.5f, 0.5f});
			pausePlayMovePointSprite_->SetPosition(
			    {static_cast<float>(WinApp::kWindowWidth) * 0.5f, static_cast<float>(WinApp::kWindowHeight) * 0.5f});
			pausePlayMovePointSprite_->SetSize(
			    {static_cast<float>(WinApp::kWindowWidth) * 0.85f, static_cast<float>(WinApp::kWindowHeight) * 0.75f});
		}
	}
}

namespace {
bool TryLoadStageMapFile(TileMap& tileMap, const std::string& relativePath) {
	const std::string paths[] = {
	    "Resources/" + relativePath,
	    "DirectXGame/Resources/" + relativePath,
	};
	for (const std::string& path : paths) {
		if (tileMap.LoadFromFile(path.c_str())) {
			return true;
		}
	}
	return false;
}

bool TryLoadStageMap(TileMap& tileMap, const std::string& relativePath, int fallbackIndex) {
	if (TryLoadStageMapFile(tileMap, relativePath)) {
		return true;
	}

	fallbackIndex = std::clamp(fallbackIndex, 0, GameScene::kStageCount - 1);
	const std::string fallbackRelativePath =
	    (fallbackIndex <= 0) ? std::string("map/maptutorial.csv") : ("map/map" + std::to_string(fallbackIndex + 1) + ".csv");
	return TryLoadStageMapFile(tileMap, fallbackRelativePath);
}
} // namespace

std::string GameScene::ResolveStageMapRelativePath() const {
	if (currentStageIndex_ <= 0) {
		return (tutorialMapMode_ == TutorialMapMode::FreePlay) ? "map/maptutorial.csv" : "map/maptutorial1.csv";
	}
	return "map/map" + std::to_string(currentStageIndex_ + 1) + ".csv";
}

void GameScene::LoadStage(int stageIndex) {
	stageIndex = std::clamp(stageIndex, 0, GameScene::kStageCount - 1);
	currentStageIndex_ = stageIndex;

	TryLoadStageMap(tileMap_, ResolveStageMapRelativePath(), stageIndex % 3);

	gameplayRewindBuffer_.Clear();
	gameplayRewindSeeded_ = false;
	isGameplayRewinding_ = false;
	gameplayRewindScrubAccumulator_ = 0.0f;
	gameplayRewindQHoldSeconds_ = 0.0f;
	gameplayRewindScrubbedDuringHold_ = false;
	spikeLivesRemaining_ = GetSpikeLivesMaxForCurrentStage();
	isSpikeRewindOverlayActive_ = false;
	spikeRewindOverlayAlpha_ = 0.0f;
	spikeRewindScrubStarted_ = false;
	rewindOverlayAlpha_ = 0.0f;
	rewindMinimapDirty_ = false;
	pendingRewindPostReleaseLock_ = false;
	rewindPostReleaseLockSeconds_ = 0.0f;
	rewindPostReleaseDimAlpha_ = 0.0f;

	mapRenderer_.Initialize(modelBlocks_, modelSpikeTile_, modelPortal_, modelDeleteBlocks_, modelKey_, tileMap_);
	RebuildMinimapTiles();
	RebuildGoalPositions();
	if (goalPortalEmitter_) {
		goalPortalEmitter_->Clear();
	}

	trampolineSprings_.clear();
	hasTrampolinePreview_ = false;
	nextTrampolineTypeIndex_ = 0;
	trampolineArrowAnimTime_ = 0.0f;
	requestSceneClear_ = false;
	portalAbsorbFinishedPending_ = false;
	isFreeCamera_ = false;
	cameraZoomOut_ = 0.0f;

	player_->SetTileMap(&tileMap_);
	const float playerHalfW = player_->GetHalfWidth();
	const float playerHalfH = player_->GetHalfHeight();
	const KamataEngine::Vector3 spawnPos = tileMap_.FindSpawnPosition(playerHalfW, playerHalfH);
	playerIntroTargetPosition_ = spawnPos;
	playerIntroStartPosition_ = spawnPos;

	// 縦・横マルチ画面でも、開始時点からスポーン位置の1画面を映す
	// （常に (0,0) だと別画面が一瞬映ってから切り替わる）
	tileMap_.GetScreenFromWorld(spawnPos.x, spawnPos.y, currentScreenX_, currentScreenY_);

	if (!isPlayerGameInitialized_) {
		player_->Initialize(modelCube_, &camera_, spawnPos);
		isPlayerGameInitialized_ = true;
	} else {
		player_->SetVisualModel(modelCube_);
		player_->SetPosition(spawnPos);
		player_->SetSpawnPosition(spawnPos);
		player_->ResetStats();
		player_->ResetRotation();
		player_->ResetVisualScaleFromTileMap();
		player_->ResetParticles();
		player_->ResetBullets();
	}

	player_->SetParent(nullptr);
	player_->SetTrampolineSprings(&trampolineSprings_);
	player_->SetSpawnPosition(spawnPos);

	lastPlayerPos_ = spawnPos;
	UpdateMapCamera();
	InitializeStageTutorial();
	InitializeKeyWallTutorial();
}

void GameScene::BeginStageFromSelect(int stageIndex) {
	pendingStageIndex_ = std::clamp(stageIndex, 0, kStageCount - 1);
	if (pendingStageIndex_ == 0) {
		tutorialMapMode_ = TutorialMapMode::Guided;
	}
	transitionExpandSource_ = TransitionExpandSource::StageSelect;
	transitionTimer_ = 0.0f;
	transitionOverlayActive_ = false;
	gameSceneTimer_ = 0;
	isGameIntroFinished_ = false;
	gameIntroTimer_ = 0.0f;
	ResetTransitionExpandOverlay();
	ChangeSceneState(SceneStateTransitionToGame::Instance());
}

void GameScene::ResetTransitionExpandOverlay() {
	if (transitionSprite_) {
		transitionSprite_->SetSize({0.0f, 0.0f});
	}
}

void GameScene::CommitPendingStageAfterTransitionExpand() {
	const int stageIndex = std::clamp(pendingStageIndex_, 0, kStageCount - 1);

	if (transitionExpandSource_ == TransitionExpandSource::ClearScreen) {
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
		hasSpawnedEnemies_ = false;

		if (railCamera_) {
			railCamera_->Reset();
			railCamera_->SetCanMove(false);
		}
	}

	currentStageIndex_ = stageIndex;
	LoadStage(stageIndex);
	gameSceneTimer_ = 0;
	isGameIntroFinished_ = false;
	gameIntroTimer_ = 0.0f;
	transitionOverlayActive_ = false;

	if (transitionSprite_) {
		const float maxScale =
		    sqrtf(powf(static_cast<float>(WinApp::kWindowWidth), 2.0f) + powf(static_cast<float>(WinApp::kWindowHeight), 2.0f));
		transitionSprite_->SetSize({maxScale, maxScale});
	}
}

bool GameScene::HasNextStageAfterCurrent() const {
	return currentStageIndex_ + 1 < kStageCount;
}

void GameScene::AdvanceToNextStageFromClear() {
	if (!HasNextStageAfterCurrent()) {
		return;
	}

	confettiActive_ = false;
	score_ = 0;
	UpdateScoreSprites();
	requestSceneClear_ = false;
	portalAbsorbFinishedPending_ = false;
	gameOverTimer_ = 0;
	hitCount = 0;
	hitCount2 = 0;

	pendingStageIndex_ = currentStageIndex_ + 1;
	transitionExpandSource_ = TransitionExpandSource::ClearScreen;
	isGameIntroFinished_ = false;
	gameIntroTimer_ = 0.0f;
	transitionOverlayActive_ = false;
	transitionTimer_ = 0.0f;
	ResetTransitionExpandOverlay();
	ChangeSceneState(SceneStateTransitionToGame::Instance());
}

void GameScene::ReturnToTitleScreen() {
	StopStageBgm();
	ClosePauseMenu();
	confettiActive_ = false;
	requestSceneClear_ = false;
	portalAbsorbFinishedPending_ = false;
	currentStageIndex_ = 0;
	LoadStage(0);
	ResetToTitle();
	focusedStageSelectIndex_ = 0;
	gameplayRewindBuffer_.Clear();
	gameplayRewindSeeded_ = false;
	isGameplayRewinding_ = false;
	gameplayRewindScrubAccumulator_ = 0.0f;
	gameplayRewindQHoldSeconds_ = 0.0f;
	gameplayRewindScrubbedDuringHold_ = false;
	spikeLivesRemaining_ = kSpikeLivesMax_;
	isSpikeRewindOverlayActive_ = false;
	spikeRewindOverlayAlpha_ = 0.0f;
	spikeRewindScrubStarted_ = false;
	rewindOverlayAlpha_ = 0.0f;
	rewindMinimapDirty_ = false;
	pendingRewindPostReleaseLock_ = false;
	rewindPostReleaseLockSeconds_ = 0.0f;
	rewindPostReleaseDimAlpha_ = 0.0f;
	ChangeSceneState(SceneStateStart::Instance());
}

void GameScene::ReturnToTitleFromStageClear() {
	ReturnToTitleScreen();
}

void GameScene::ClearStageRuntimeEntities() {
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
	hasSpawnedEnemies_ = false;
}

void GameScene::ResetCurrentStage() {
	requestSceneClear_ = false;
	portalAbsorbFinishedPending_ = false;
	score_ = 0;
	UpdateScoreSprites();
	gameOverTimer_ = 0;
	hitCount = 0;
	hitCount2 = 0;

	ClearStageRuntimeEntities();

	if (railCamera_) {
		railCamera_->Reset();
		railCamera_->SetCanMove(false);
	}

	LoadStage(currentStageIndex_);
	LoadEnemyPopData();

	isGameIntroFinished_ = false;
	gameIntroTimer_ = 0.0f;
	gameSceneTimer_ = 0;
	transitionOverlayActive_ = false;

	gameplayRewindBuffer_.Clear();
	gameplayRewindSeeded_ = false;
	isGameplayRewinding_ = false;
	gameplayRewindScrubAccumulator_ = 0.0f;
	gameplayRewindQHoldSeconds_ = 0.0f;
	gameplayRewindScrubbedDuringHold_ = false;
	spikeLivesRemaining_ = GetSpikeLivesMaxForCurrentStage();
	isSpikeRewindOverlayActive_ = false;
	spikeRewindOverlayAlpha_ = 0.0f;
	spikeRewindScrubStarted_ = false;
	rewindOverlayAlpha_ = 0.0f;
	rewindMinimapDirty_ = false;
	pendingRewindPostReleaseLock_ = false;
	rewindPostReleaseLockSeconds_ = 0.0f;
	rewindPostReleaseDimAlpha_ = 0.0f;
	isGameplayPaused_ = false;

	ChangeSceneState(SceneStateGameIntro::Instance());
}

void GameScene::HandleGameplayShortcuts() {
	if (!input_) {
		return;
	}

	if (input_->TriggerKey(DIK_ESCAPE)) {
		if (isPauseMenuOpen_) {
			return;
		}
		OpenPauseMenu();
		return;
	}

	if (isPauseMenuOpen_) {
		return;
	}

	if (input_->TriggerKey(DIK_R)) {
		if (IsKeyWallTutorialActive()) {
			return;
		}
		if (currentStageIndex_ == 0 && stageTutorialStep_ == StageTutorialStep::PressR) {
			tutorialMapMode_ = TutorialMapMode::FreePlay;
		}
		ResetCurrentStage();
	}
}

void GameScene::OpenPauseMenu() {
	const SceneStateKind kind = GetSceneStateKind();
	if (kind != SceneStateKind::Game && kind != SceneStateKind::GameIntro && kind != SceneStateKind::StageSelect) {
		return;
	}

	isPauseMenuOpen_ = true;
	pauseMenuPage_ = PauseMenuPage::Main;
	pauseReturnSceneKind_ = kind;

	// 巻き戻し・バネ更新を止めたままにする（シーン切替しないので飛行中の速度を保持）
	if (pauseDimSprite_) {
		pauseDimSprite_->SetColor({0.0f, 0.0f, 0.0f, kPauseMenuDimAlpha_});
	}
}

void GameScene::ClosePauseMenu() {
	if (!isPauseMenuOpen_) {
		return;
	}

	isPauseMenuOpen_ = false;
	pauseMenuPage_ = PauseMenuPage::Main;

	// ポーズ中に Q を離していた場合は、再開時に巻き戻しを確定してバネ速度を壊さない
	if (isGameplayRewinding_ && input_ && !input_->PushKey(DIK_Q)) {
		FinalizeGameplayRewindScrub();
	}
}

bool GameScene::CanUseGameplayPause() const {
	if (!player_ || !isGameIntroFinished_) {
		return false;
	}
	if (isPauseMenuOpen_ || isGameplayRewinding_ || IsRewindPostReleaseLocked()) {
		return false;
	}
	// トゲ被弾後の巻き戻し待ち中は一時停止不可（暗転の二重化＋Q阻害を防ぐ）
	if (isSpikeRewindOverlayActive_) {
		return false;
	}
	// めり込み中・チャージ中は一時停止不可（チャージ操作を優先）
	const Player::SpringChargePhase phase = player_->GetSpringChargePhase();
	if (phase == Player::SpringChargePhase::Penetrating || phase == Player::SpringChargePhase::Charging) {
		return false;
	}
	if (IsStageTutorialActive()) {
		return stageTutorialStep_ == StageTutorialStep::SpacePauseHint;
	}
	if (IsKeyWallTutorialActive()) {
		return false;
	}
	return GetSceneStateKind() == SceneStateKind::Game;
}

void GameScene::ClearIdleSpringContactsAfterPause() {
	// 一時停止中はPlayer更新が止まるため isPlayerInside_ が古いまま残り、解除後にすり抜ける
	if (!player_ || player_->GetSpringChargePhase() != Player::SpringChargePhase::None) {
		return;
	}
	for (TrampolineSpring& spring : trampolineSprings_) {
		spring.ResetPlayerContact();
	}
}

void GameScene::BeginGameplayPause() {
	if (player_) {
		gameplayPauseSavedVelX_ = player_->GetVelocityX();
		gameplayPauseSavedVelY_ = player_->GetVelocityY();
		player_->ClearSpringSpaceHold();
	}
	isGameplayPaused_ = true;
	ClearIdleSpringContactsAfterPause();
}

void GameScene::BeginGameplayPauseAfterRewind() {
	// 巻き戻し暗転は一時停止UIに引き継ぐ
	rewindOverlayAlpha_ = 0.0f;
	spikeRewindOverlayAlpha_ = 0.0f;
	rewindPostReleaseLockSeconds_ = 0.0f;
	pendingRewindPostReleaseLock_ = false;
	BeginGameplayPause();
	gameplayPauseIgnoreSpaceUntilRelease_ = true;
	gameplayPauseSpaceHoldFrames_ = 0;
}

void GameScene::EndGameplayPause() {
	isGameplayPaused_ = false;
	// 解除に使った押しの離し／続き押しを、短押し一時停止と誤認しない
	gameplayPauseIgnoreSpaceUntilRelease_ = true;
	gameplayPauseSpaceHoldFrames_ = 0;
	if (player_) {
		player_->SetVelocityX(gameplayPauseSavedVelX_);
		player_->SetVelocityY(gameplayPauseSavedVelY_);
		player_->ClearSpringSpaceHold();
	}
	if (IsStageTutorialActive() && stageTutorialStep_ == StageTutorialStep::SpacePauseHint) {
		tutorialSpacePauseReleased_ = true;
	}
	ClearIdleSpringContactsAfterPause();
}

void GameScene::UpdateGameplayPauseInput() {
	if (!input_ || isPauseMenuOpen_) {
		return;
	}

	// トゲ待ち中に一時停止が残っているとQ巻き戻しが効かないので解除する
	if (isSpikeRewindOverlayActive_) {
		isGameplayPaused_ = false;
		gameplayPauseSpaceWasDown_ = input_->PushKey(DIK_SPACE);
		gameplayPauseSpaceHoldFrames_ = 0;
		gameplayPauseIgnoreSpaceUntilRelease_ = false;
		gameplayPauseIgnoreQUntilRelease_ = false;
		return;
	}

	const bool spaceDown = input_->PushKey(DIK_SPACE);
	const bool spaceTrigger = input_->TriggerKey(DIK_SPACE);
	const bool spaceReleased = gameplayPauseSpaceWasDown_ && !spaceDown;
	const bool qTrigger = input_->TriggerKey(DIK_Q);

	if (isGameplayPaused_) {
		if (IsTutorialPauseIntroLocked()) {
			gameplayPauseSpaceWasDown_ = spaceDown;
			gameplayPauseSpaceHoldFrames_ = spaceDown ? (gameplayPauseSpaceHoldFrames_ + 1) : 0;
			return;
		}
		// チュートリアル「チャージ状態から再開」中は SPACE 長押しチャージ以外で解除しない
		const bool chargeOnlyResume =
		    IsStageTutorialActive() && stageTutorialStep_ == StageTutorialStep::SpacePauseHint &&
		    !tutorialPauseLandHintActive_;
		if (chargeOnlyResume) {
			if (spaceTrigger) {
				EndGameplayPause();
			}
			gameplayPauseSpaceWasDown_ = spaceDown;
			gameplayPauseSpaceHoldFrames_ = spaceDown ? (gameplayPauseSpaceHoldFrames_ + 1) : 0;
			return;
		}
		const bool wasdTrigger = input_->TriggerKey(DIK_W) || input_->TriggerKey(DIK_A) ||
		                         input_->TriggerKey(DIK_S) || input_->TriggerKey(DIK_D);
		if (spaceTrigger) {
			EndGameplayPause();
		} else if (qTrigger) {
			// Q短押しでも再開（この押しでは巻き戻ししない）
			EndGameplayPause();
			gameplayPauseIgnoreQUntilRelease_ = true;
		} else if (wasdTrigger) {
			// 隠し: WASDでも再開（巻き戻し後すぐ動き出せる）
			EndGameplayPause();
		}
		gameplayPauseSpaceWasDown_ = spaceDown;
		gameplayPauseSpaceHoldFrames_ = spaceDown ? (gameplayPauseSpaceHoldFrames_ + 1) : 0;
		return;
	}

	// 解除直後: SPACEを一度離すまで新規の一時停止を受け付けない
	if (gameplayPauseIgnoreSpaceUntilRelease_) {
		if (!spaceDown) {
			gameplayPauseIgnoreSpaceUntilRelease_ = false;
		}
		gameplayPauseSpaceWasDown_ = spaceDown;
		gameplayPauseSpaceHoldFrames_ = 0;
		return;
	}

	if (spaceDown) {
		++gameplayPauseSpaceHoldFrames_;
	}

	if (CanUseGameplayPause()) {
		// 短押し離しで一時停止。長押しは一時停止しない
		// （空中の前入力→次バネでチャージ／バネ上のチャージ待ちでもチャージ）
		if (spaceReleased && gameplayPauseSpaceHoldFrames_ > 0 &&
		    gameplayPauseSpaceHoldFrames_ <= kGameplayPauseTapMaxFrames_) {
			BeginGameplayPause();
		}
	}

	if (!spaceDown) {
		gameplayPauseSpaceHoldFrames_ = 0;
	}
	gameplayPauseSpaceWasDown_ = spaceDown;
}

void GameScene::DrawGameplayPauseOverlay() {
	if (!isGameplayPaused_ || !pauseDimSprite_) {
		return;
	}

	pauseDimSprite_->SetColor({0.0f, 0.0f, 0.0f, kGameplayPauseDimAlpha_});
	pauseDimSprite_->Draw();

	// 画面上部中央に「一時停止中」表示
	if (gameplayPauseStopSprite_) {
		const float winW = static_cast<float>(WinApp::kWindowWidth);
		const float labelH = kGameplayPauseStopHeight_;
		const float labelW = labelH * gameplayPauseStopAspect_;
		gameplayPauseStopSprite_->SetSize({labelW, labelH});
		gameplayPauseStopSprite_->SetPosition({winW * 0.5f, kGameplayPauseStopCenterY_});
		gameplayPauseStopSprite_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
		gameplayPauseStopSprite_->Draw();
	}

	// チュートリアル案内は暗転・「一時停止中」より手前に描く（バネの上・大きめ）
	if (IsStageTutorialActive() && stageTutorialStep_ == StageTutorialStep::SpacePauseHint &&
	    !tutorialSpacePauseReleased_ && fontRenderer_ && fontRenderer_->IsReady()) {
		const float fontH = kTutorialPauseHintFontHeight_;
		const KamataEngine::Vector4 textColor = {1.0f, 1.0f, 1.0f, 1.0f};
		const KamataEngine::Vector2 springScreen =
		    ConvertWorldToScreen(tutorialPlacedSpringWorldPos_.x, tutorialPlacedSpringWorldPos_.y);
		float springHalfW = TileMap::kSpringReferenceHalfW;
		float springHalfH = TileMap::kSpringReferenceHalfH;
		if (!trampolineSprings_.empty()) {
			trampolineSprings_.front().GetHalfSize(springHalfW, springHalfH);
		}
		float viewLeft = 0.0f, viewBottom = 0.0f, viewRight = 0.0f, viewTop = 0.0f;
		ComputeCameraBounds(viewLeft, viewBottom, viewRight, viewTop);
		const float winW = static_cast<float>(WinApp::kWindowWidth);
		const float pxPerWorldY =
		    (viewTop > viewBottom) ? (static_cast<float>(WinApp::kWindowHeight) / (viewTop - viewBottom)) : 1.0f;
		const float springTopScreenY = springScreen.y - springHalfH * pxPerWorldY;

		auto drawCenteredHint = [&](const char* text, float topYLine, float centerX) {
			const float width = fontRenderer_->MeasureWidth(text, fontH);
			float x = centerX - width * 0.5f;
			if (x < 8.0f) {
				x = 8.0f;
			}
			if (x + width > winW - 8.0f) {
				x = winW - 8.0f - width;
			}
			fontRenderer_->DrawString(text, x, topYLine - kTutorialTextOpticalOffsetY_, fontH, textColor);
		};

		const float topY = springTopScreenY - kTutorialPauseHintAboveSpringPx_ - fontH;
		const float hintCenterX = winW * 0.5f; // バネが左寄りでも読みやすく画面中央
		drawCenteredHint("SPACE長押しでチャージ状態から再開できます", topY, hintCenterX);
	}
}

void GameScene::UpdatePauseMenu() {
	if (!isPauseMenuOpen_ || !input_) {
		return;
	}

	if (input_->TriggerKey(DIK_ESCAPE)) {
		if (pauseMenuPage_ == PauseMenuPage::PlayMoveGuide) {
			pauseMenuPage_ = PauseMenuPage::Main;
		} else {
			ClosePauseMenu();
		}
		return;
	}

	if (pauseMenuPage_ != PauseMenuPage::Main) {
		return;
	}

	const KamataEngine::Vector2 mousePos = GetClientMousePosition();
	if (!input_->IsTriggerMouse(0)) {
		return;
	}

	if (IsScreenPointInSprite(pauseTitleSprite_, mousePos.x, mousePos.y)) {
		ReturnToTitleScreen();
		return;
	}

	if (IsScreenPointInSprite(pausePlayMoveSprite_, mousePos.x, mousePos.y)) {
		pauseMenuPage_ = PauseMenuPage::PlayMoveGuide;
	}
}

void GameScene::DrawPauseMenu() {
	if (!isPauseMenuOpen_) {
		return;
	}

	if (pauseDimSprite_) {
		pauseDimSprite_->SetColor({0.0f, 0.0f, 0.0f, kPauseMenuDimAlpha_});
		pauseDimSprite_->Draw();
	}

	if (pauseMenuPage_ == PauseMenuPage::PlayMoveGuide) {
		if (pausePlayMovePointSprite_) {
			pausePlayMovePointSprite_->Draw();
		}
		return;
	}

	if (pauseTitleSprite_) {
		pauseTitleSprite_->SetPosition({kPauseMenuLeftButtonX_, kPauseMenuButtonY_});
		pauseTitleSprite_->SetSize({kPauseMenuButtonW_, kPauseMenuButtonH_});
		pauseTitleSprite_->Draw();
	}
	if (pausePlayMoveSprite_) {
		pausePlayMoveSprite_->SetPosition({kPauseMenuRightButtonX_, kPauseMenuButtonY_});
		pausePlayMoveSprite_->SetSize({kPauseMenuButtonW_, kPauseMenuButtonH_});
		pausePlayMoveSprite_->Draw();
	}
}

void GameScene::AdvanceFromClearScreen() {
	AdvanceToNextStageFromClear();
}

void GameScene::UpdateOsCursorVisibility() {
	const SceneStateKind kind = GetSceneStateKind();
	// ゲームプレイ中（ポーズ中を除く）はOSの矢印カーソルを隠す
	const bool shouldHide = !isPauseMenuOpen_ && (kind == SceneStateKind::Game || kind == SceneStateKind::GameIntro);

	// ShowCursor の内部カウンタずれ（Alt+Tab 等）で表示が破綻しないよう、実表示状態を見て補正する
	CURSORINFO cursorInfo{};
	cursorInfo.cbSize = sizeof(cursorInfo);
	const bool queryOk = GetCursorInfo(&cursorInfo) != FALSE;
	const bool currentlyVisible = queryOk && (cursorInfo.flags & CURSOR_SHOWING) != 0;

	if (shouldHide) {
		if (!osCursorHidden_ || currentlyVisible) {
			int displayCount = 0;
			do {
				displayCount = ShowCursor(FALSE);
			} while (displayCount >= 0);
		}
		osCursorHidden_ = true;
	} else {
		if (osCursorHidden_ || !currentlyVisible) {
			int displayCount = -1;
			do {
				displayCount = ShowCursor(TRUE);
			} while (displayCount < 0);
		}
		osCursorHidden_ = false;
	}
}

void GameScene::Update() {
	// State Pattern: 現在のシーン状態に更新を委譲（ポリモーフィズム）
	if (sceneState_) {
		sceneState_->Update(*this);
	}
	UpdateTransitionOverlayIfActive();
	UpdateOsCursorVisibility();

	const SceneStateKind stateKind = GetSceneStateKind();
	if (!isPauseMenuOpen_ &&
	    (stateKind == SceneStateKind::Game || stateKind == SceneStateKind::GameIntro || stateKind == SceneStateKind::TransitionFromGame)) {
		UpdateGoalPortalParticles();
	}

	if (!isPauseMenuOpen_ && (stateKind == SceneStateKind::Game || stateKind == SceneStateKind::GameIntro)) {
		UpdateTrampolineArrowAnimations();
	}

	// ステージクリア → 次ステージへ直行（全クリア時のみタイトル）
	if (portalAbsorbFinishedPending_) {
		portalAbsorbFinishedPending_ = false;
		requestSceneClear_ = true;
	}
	if (requestSceneClear_ && GetSceneStateKind() == SceneStateKind::Game) {
		TransitionToClearScene();
	}

	UpdateStageBgm();
}

void GameScene::Draw() {
	ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

	dxCommon_->ClearDepthBuffer();

	KamataEngine::Model::PreDraw(commandList);

	if (GetSceneStateKind() == SceneStateKind::Start) {
		// タイトルは2Dスプライトのみ（3D描画なし）
	} else if (GetSceneStateKind() == SceneStateKind::GameIntro || GetSceneStateKind() == SceneStateKind::Game ||
	           GetSceneStateKind() == SceneStateKind::TransitionFromGame || GetSceneStateKind() == SceneStateKind::Over) {

		float viewLeft = 0.0f;
		float viewBottom = 0.0f;
		float viewRight = 0.0f;
		float viewTop = 0.0f;
		ComputeCameraBounds(viewLeft, viewBottom, viewRight, viewTop);

		float skydomeViewScale = 1.0f;
		if (cameraZoomOut_ > 0.0001f) {
			float zoomViewW = 0.0f;
			float zoomViewH = 0.0f;
			ComputeFreeCameraViewSize(zoomViewW, zoomViewH);
			float screenLeft = 0.0f;
			float screenBottom = 0.0f;
			float screenRight = 0.0f;
			float screenTop = 0.0f;
			tileMap_.GetScreenViewportBounds(currentScreenX_, currentScreenY_, screenLeft, screenBottom, screenRight, screenTop);
			const float baseW = screenRight - screenLeft;
			const float baseH = screenTop - screenBottom;
			if (baseW > 0.0f && baseH > 0.0f) {
				skydomeViewScale = (std::max)(zoomViewW / baseW, zoomViewH / baseH);
			}
		}

		if (skydome_) {
			skydome_->DrawAt((viewLeft + viewRight) * 0.5f, (viewBottom + viewTop) * 0.5f, skydomeViewScale);
			skydome_->Draw();
		}

		mapRenderer_.Draw(camera_);
		DrawTrampolineSprings();
		DrawSpringTrajectoryPreview();
		player_->Draw();

		if (goalPortalEmitter_) {
			goalPortalEmitter_->Draw(camera_);
		}

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
	} else if (GetSceneStateKind() == SceneStateKind::Clear ||
	           (GetSceneStateKind() == SceneStateKind::TransitionToGame &&
	            transitionExpandSource_ == TransitionExpandSource::ClearScreen)) {
		float viewLeft = 0.0f;
		float viewBottom = 0.0f;
		float viewRight = 0.0f;
		float viewTop = 0.0f;
		ComputeCameraBounds(viewLeft, viewBottom, viewRight, viewTop);
		if (skydome_) {
			skydome_->DrawAt((viewLeft + viewRight) * 0.5f, (viewBottom + viewTop) * 0.5f);
			skydome_->Draw();
		}
		if (clearEmitter_) {
			clearEmitter_->Draw(camera_);
		}
	}

	KamataEngine::Model::PostDraw();

	KamataEngine::Sprite::PreDraw(commandList);

	if (fontRenderer_) {
		fontRenderer_->NewFrame();
	}

	if (GetSceneStateKind() == SceneStateKind::GameIntro || GetSceneStateKind() == SceneStateKind::Game ||
	    GetSceneStateKind() == SceneStateKind::TransitionFromGame || GetSceneStateKind() == SceneStateKind::Over) {
		float viewLeft = 0.0f;
		float viewBottom = 0.0f;
		float viewRight = 0.0f;
		float viewTop = 0.0f;
		ComputeCameraBounds(viewLeft, viewBottom, viewRight, viewTop);
		DrawZoomOutMarginFill(viewLeft, viewBottom, viewRight, viewTop);
	}

	if (GetSceneStateKind() == SceneStateKind::Start) {
		if (title2DSprite_) {
			title2DSprite_->Draw();
		}
	}

	if (GetSceneStateKind() == SceneStateKind::StageSelect ||
	    (GetSceneStateKind() == SceneStateKind::TransitionToGame &&
	     transitionExpandSource_ == TransitionExpandSource::StageSelect)) {
		if (stageSelectBackgroundSprite_) {
			stageSelectBackgroundSprite_->Draw();
		}
		DrawStageSelectUi();
	}

	if (GetSceneStateKind() == SceneStateKind::TransitionToGame || GetSceneStateKind() == SceneStateKind::TransitionFromGame || transitionOverlayActive_) {
		transitionSprite_->Draw();
	}

	// ミニマップはゲームシーンのみ表示
	if (GetSceneStateKind() == SceneStateKind::Game && isGameIntroFinished_) {
		DrawJumpSpringChargeCircle();

		if (kShowMinimap_) {
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

		DrawSpikeLifeHearts();
		DrawSpringPreviewPlacementLabel();
		DrawSpringCollectLabel();
	}

	// Draw score digits on top-right（一時非表示・処理は維持）
	if (kShowScoreDigits_) {
		for (KamataEngine::Sprite* s : scoreDigitSprites_) {
			if (s) {
				s->Draw();
			}
		}
	}

	if (GetSceneStateKind() == SceneStateKind::Clear ||
	    (GetSceneStateKind() == SceneStateKind::TransitionToGame &&
	     transitionExpandSource_ == TransitionExpandSource::ClearScreen)) {
		if (clearScreenBackgroundSprite_) {
			clearScreenBackgroundSprite_->Draw();
		}
		DrawStageClearUi();
		for (auto& c : confettiParticles_) {
			if (c.active && c.sprite)
			 c.sprite->Draw();
		}
	}

	if (GetSceneStateKind() == SceneStateKind::Over) {
		if (gameOverSprite_) {
			gameOverSprite_->Draw();
		}
	}

	DrawSpikeRewindOverlay();
	DrawGameplayRewindStatusLabel();
	DrawStageTutorial();
	DrawKeyWallTutorial();
	DrawGameplayPauseOverlay();
	DrawPauseMenu();

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
	StopStageBgm();
	// Change: go to Clear scene so player sees clear screen instead of immediately returning to title
	ChangeSceneState(SceneStateClear::Instance());
	LayoutStageClearButtons();

	// reset score on clear
	score_ = 0;
	UpdateScoreSprites();
	requestSceneClear_ = false;

	confettiActive_ = false;
	confettiSpawnTimer_ = 0;

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
	StopStageBgm();
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
	if (tileMap_.GetWidth() <= 0 || tileMap_.GetHeight() <= 0) {
		for (KamataEngine::Sprite* sprite : minimapGroundSprites_) {
			delete sprite;
		}
		minimapGroundSprites_.clear();
		return;
	}

	const float tileW = kMinimapSize_.x / static_cast<float>(tileMap_.GetWidth());
	const float tileH = kMinimapSize_.y / static_cast<float>(tileMap_.GetHeight());

	struct NeededTile {
		int col = 0;
		int row = 0;
		float r = 1.0f;
		float g = 1.0f;
		float b = 1.0f;
		float a = 1.0f;
	};
	std::vector<NeededTile> needed;
	needed.reserve(static_cast<size_t>(tileMap_.GetWidth() * tileMap_.GetHeight() / 4));

	for (int row = 0; row < tileMap_.GetHeight(); ++row) {
		for (int col = 0; col < tileMap_.GetWidth(); ++col) {
			const bool isGround = tileMap_.GetTile(col, row) == 1;
			const bool isSpike = tileMap_.IsSpike(col, row);
			const bool isGoal = tileMap_.IsGoal(col, row);
			const bool isWall = tileMap_.IsDisappearingWallActive(col, row);
			const bool isButton = tileMap_.IsButton(col, row);
			if (!isGround && !isSpike && !isGoal && !isWall && !isButton) {
				continue;
			}

			NeededTile tile{};
			tile.col = col;
			tile.row = row;
			if (isSpike) {
				tile.r = 0.85f;
				tile.g = 0.2f;
				tile.b = 0.2f;
			} else if (isGoal) {
				tile.r = 0.2f;
				tile.g = 0.55f;
				tile.b = 0.95f;
			} else if (isButton) {
				if (tileMap_.IsButtonPressed(col, row)) {
					tile.r = 0.35f;
					tile.g = 0.35f;
					tile.b = 0.35f;
				} else {
					tile.r = 0.25f;
					tile.g = 0.9f;
					tile.b = 0.35f;
				}
			} else if (isWall) {
				tile.r = 0.75f;
				tile.g = 0.55f;
				tile.b = 0.15f;
			} else {
				tile.r = 0.6f;
				tile.g = 0.45f;
				tile.b = 0.25f;
			}
			needed.push_back(tile);
		}
	}

	// 鍵取得などで毎フレーム Create/Delete しないよう、既存スプライトを再利用する
	size_t writeIndex = 0;
	for (const NeededTile& tile : needed) {
		KamataEngine::Sprite* sprite = nullptr;
		if (writeIndex < minimapGroundSprites_.size()) {
			sprite = minimapGroundSprites_[writeIndex];
		} else {
			sprite = KamataEngine::Sprite::Create(greenBoxTextureHandle_, {0.0f, 0.0f});
			if (!sprite) {
				continue;
			}
			minimapGroundSprites_.push_back(sprite);
		}

		sprite->SetAnchorPoint({0.0f, 0.0f});
		sprite->SetSize({tileW, tileH});
		sprite->SetPosition(
		    {kMinimapPosition_.x + static_cast<float>(tile.col) * tileW, kMinimapPosition_.y + static_cast<float>(tile.row) * tileH});
		sprite->SetColor({tile.r, tile.g, tile.b, tile.a});
		++writeIndex;
	}

	while (minimapGroundSprites_.size() > writeIndex) {
		delete minimapGroundSprites_.back();
		minimapGroundSprites_.pop_back();
	}
}

void GameScene::RebuildGoalPositions() {
	goalPositions_.clear();

	for (int row = 0; row < tileMap_.GetHeight(); ++row) {
		for (int col = 0; col < tileMap_.GetWidth(); ++col) {
			if (!tileMap_.IsGoal(col, row)) {
				continue;
			}

			KamataEngine::Vector3 center = tileMap_.TileCenterToWorld(col, row);
			center.y += TileMap::GetGoalModelRaiseOffsetY(tileMap_.GetTileHeight());
			center.y += TileMap::GetGoalParticleBaseOffsetY(tileMap_.GetTileHeight());
			center.z = 1.0f;
			goalPositions_.push_back(center);
		}
	}
}

void GameScene::UpdateGoalPortalParticles() {
	if (!goalPortalEmitter_) {
		return;
	}

	for (const KamataEngine::Vector3& goalPos : goalPositions_) {
		goalPortalEmitter_->EmitPortal(goalPos);
	}

	if (player_ && player_->IsPortalAbsorbing()) {
		const KamataEngine::Vector3& absorbCenter = player_->GetPortalAbsorbCenter();
		for (int i = 0; i < 4; ++i) {
			goalPortalEmitter_->EmitPortal(absorbCenter);
		}
	}

	goalPortalEmitter_->Update();
}

bool GameScene::BeginPortalAbsorption() {
	if (!player_ || player_->IsPortalAbsorbing()) {
		return false;
	}

	const KamataEngine::Vector3 playerPos = player_->GetWorldPosition();
	const float playerHalfW = player_->GetHalfWidth();
	const float playerHalfH = player_->GetHalfHeight();

	KamataEngine::Vector3 portalCenter;
	if (!tileMap_.FindOverlappingGoalCenter(playerPos.x, playerPos.y, playerHalfW, playerHalfH, portalCenter)) {
		return false;
	}

	player_->BeginPortalAbsorption(portalCenter, kPortalAbsorptionStyle);

	if (isGameplayRewinding_) {
		FinalizeGameplayRewindScrub();
	}
	isGameplayRewinding_ = false;
	gameplayRewindScrubAccumulator_ = 0.0f;
	gameplayRewindQHoldSeconds_ = 0.0f;
	gameplayRewindScrubbedDuringHold_ = false;
	gameplayRewindBuffer_.Clear();
	gameplayRewindSeeded_ = false;
	isSpikeRewindOverlayActive_ = false;
	spikeRewindOverlayAlpha_ = 0.0f;
	spikeRewindScrubStarted_ = false;
	rewindOverlayAlpha_ = 0.0f;
	rewindMinimapDirty_ = false;
	pendingRewindPostReleaseLock_ = false;
	rewindPostReleaseLockSeconds_ = 0.0f;
	rewindPostReleaseDimAlpha_ = 0.0f;

	return true;
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

void GameScene::ComputeFreeCameraViewSize(float& viewW, float& viewH) const {
	float screenLeft = 0.0f;
	float screenBottom = 0.0f;
	float screenRight = 0.0f;
	float screenTop = 0.0f;
	tileMap_.GetScreenViewportBounds(currentScreenX_, currentScreenY_, screenLeft, screenBottom, screenRight, screenTop);
	const float baseW = screenRight - screenLeft;
	const float baseH = screenTop - screenBottom;

	float mapMinX = 0.0f;
	float mapMinY = 0.0f;
	float mapMaxX = 0.0f;
	float mapMaxY = 0.0f;
	tileMap_.GetMapWorldBounds(mapMinX, mapMinY, mapMaxX, mapMaxY);
	const float mapW = mapMaxX - mapMinX;
	const float mapH = mapMaxY - mapMinY;

	const float scaleToFitMapW = baseW > 0.0f ? mapW / baseW : 1.0f;
	const float scaleToFitMapH = baseH > 0.0f ? mapH / baseH : 1.0f;
	const float maxScale = (std::max)(scaleToFitMapW, scaleToFitMapH);
	const float scale = 1.0f + (maxScale - 1.0f) * cameraZoomOut_;

	viewW = baseW * scale;
	viewH = baseH * scale;
}

void GameScene::ComputeCameraBounds(float& left, float& bottom, float& right, float& top) {
	left = 0.0f;
	bottom = 0.0f;
	right = 0.0f;
	top = 0.0f;
	tileMap_.GetScreenViewportBounds(currentScreenX_, currentScreenY_, left, bottom, right, top);

	if (cameraZoomOut_ <= 0.0001f) {
		return;
	}

	float viewW = 0.0f;
	float viewH = 0.0f;
	ComputeFreeCameraViewSize(viewW, viewH);

	// Playerのいる画面の中心
	const float screenCenterX = (left + right) * 0.5f;
	const float screenCenterY = (bottom + top) * 0.5f;

	// マップ全体の中心
	float mapMinX = 0.0f;
	float mapMinY = 0.0f;
	float mapMaxX = 0.0f;
	float mapMaxY = 0.0f;
	tileMap_.GetMapWorldBounds(mapMinX, mapMinY, mapMaxX, mapMaxY);
	const float mapCenterX = (mapMinX + mapMaxX) * 0.5f;
	const float mapCenterY = (mapMinY + mapMaxY) * 0.5f;

	// ズームアウト時は全体の中心、ズームインで戻るとPlayerの画面中心へ補間する
	const float t = std::clamp(cameraZoomOut_, 0.0f, 1.0f);
	const float centerX = screenCenterX + (mapCenterX - screenCenterX) * t;
	const float centerY = screenCenterY + (mapCenterY - screenCenterY) * t;
	freeCameraCenterX_ = centerX;
	freeCameraCenterY_ = centerY;

	left = centerX - viewW * 0.5f;
	right = centerX + viewW * 0.5f;
	bottom = centerY - viewH * 0.5f;
	top = centerY + viewH * 0.5f;
}

namespace {
void GetClientSize(float& width, float& height) {
	RECT clientRect{};
	GetClientRect(WinApp::GetInstance()->GetHwnd(), &clientRect);
	width = static_cast<float>(clientRect.right - clientRect.left);
	height = static_cast<float>(clientRect.bottom - clientRect.top);
}
} // namespace

KamataEngine::Vector2 GameScene::GetClientMousePosition() const {
	POINT cursorPos{};
	GetCursorPos(&cursorPos);
	ScreenToClient(WinApp::GetInstance()->GetHwnd(), &cursorPos);

	// スプライト／UIは kWindowWidth×kWindowHeight 固定座標。
	// 実クライアントサイズ（リサイズ・DPI）とずれるとクリックが外れるので論理座標へ変換する。
	float clientW = 0.0f;
	float clientH = 0.0f;
	GetClientSize(clientW, clientH);
	if (clientW <= 1.0f || clientH <= 1.0f) {
		return {static_cast<float>(cursorPos.x), static_cast<float>(cursorPos.y)};
	}

	const float logicalX = static_cast<float>(cursorPos.x) * (static_cast<float>(WinApp::kWindowWidth) / clientW);
	const float logicalY = static_cast<float>(cursorPos.y) * (static_cast<float>(WinApp::kWindowHeight) / clientH);
	return {logicalX, logicalY};
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

KamataEngine::Vector2 GameScene::ConvertWorldToScreen(float worldX, float worldY) {
	float left = 0.0f;
	float bottom = 0.0f;
	float right = 0.0f;
	float top = 0.0f;
	ComputeCameraBounds(left, bottom, right, top);

	// スプライトは固定のウィンドウ座標系（kWindowWidth/kWindowHeight）で描画されるため、
	// 実クライアントサイズではなくこの固定値で変換しないと、ウィンドウリサイズ時にずれる
	const float winW = static_cast<float>(WinApp::kWindowWidth);
	const float winH = static_cast<float>(WinApp::kWindowHeight);
	const float screenX = (worldX - left) / (right - left) * winW;
	const float screenY = (top - worldY) / (top - bottom) * winH;
	return {screenX, screenY};
}

void GameScene::DrawJumpSpringChargeCircle() {
	if (!player_ || !jumpSpringChargeSprite_) {
		return;
	}

	if (player_->GetSpringChargePhase() != Player::SpringChargePhase::Charging) {
		return;
	}

	const KamataEngine::Vector3& anchor = player_->GetJumpSpringAnchor();
	const KamataEngine::Vector2 screenPos = ConvertWorldToScreen(anchor.x, anchor.y);
	const float radiusWorld = player_->GetJumpSpringCircleRadiusWorld();

	float left = 0.0f;
	float bottom = 0.0f;
	float right = 0.0f;
	float top = 0.0f;
	ComputeCameraBounds(left, bottom, right, top);
	// 位置(ConvertWorldToScreen)と同じ固定ウィンドウ座標系で直径を求める
	const float winW = static_cast<float>(WinApp::kWindowWidth);
	const float worldPerPixel = (right - left) / winW;
	const float diameterPx = (radiusWorld * 2.0f) / worldPerPixel;

	jumpSpringChargeSprite_->SetPosition(screenPos);
	jumpSpringChargeSprite_->SetSize({diameterPx, diameterPx});
	jumpSpringChargeSprite_->Draw();
}

void GameScene::DrawSpringTrajectoryPreview() {
	if (!player_ || !modelRaycasting_ || !isTrajectoryDotPoolReady_ || !isGameIntroFinished_) {
		return;
	}

	if (!player_->ShouldShowSpringTrajectory()) {
		return;
	}

	std::array<KamataEngine::Vector3, Player::kSpringTrajectoryMaxSamples> samples{};
	int sampleCount = 0;
	if (!player_->ComputeSpringTrajectorySamples(samples.data(), Player::kSpringTrajectoryMaxSamples, sampleCount)) {
		return;
	}

	static constexpr float kRaycastingModelExtent = 1.76f;
	static constexpr float kDotWorldSize = 7.5f;
	static constexpr float kDotWorldScale = kDotWorldSize / kRaycastingModelExtent;
	static constexpr float kDotSpacing = 20.0f;
	static constexpr float kGapSpacing = 16.0f;
	static constexpr float kTrajectoryDepthZ = 1.35f;

	auto drawDotAt = [&](const KamataEngine::Vector3& pos, float scale, int poolIndex) -> bool {
		if (poolIndex >= kTrajectoryDotPoolSize_) {
			return false;
		}
		KamataEngine::WorldTransform& dotTransform = trajectoryDotTransforms_[static_cast<size_t>(poolIndex)];
		dotTransform.parent_ = nullptr;
		dotTransform.rotation_ = {0.0f, 0.0f, 0.0f};
		dotTransform.translation_ = {pos.x, pos.y, kTrajectoryDepthZ};
		dotTransform.scale_ = {scale, scale, scale};
		dotTransform.UpdateMatrix();
		modelRaycasting_->Draw(dotTransform, camera_);
		return true;
	};

	int dotCount = 0;
	float distanceSinceLast = 0.0f;
	bool placeDot = true;

	if (drawDotAt(samples[0], kDotWorldScale, dotCount)) {
		++dotCount;
	}

	for (int seg = 0; seg < sampleCount - 1 && dotCount < kTrajectoryDotPoolSize_; ++seg) {
		const KamataEngine::Vector3& a = samples[static_cast<size_t>(seg)];
		const KamataEngine::Vector3& b = samples[static_cast<size_t>(seg + 1)];
		const float dx = b.x - a.x;
		const float dy = b.y - a.y;
		const float segLen = std::sqrt(dx * dx + dy * dy);
		if (segLen < 0.001f) {
			continue;
		}

		float traveled = 0.0f;
		while (traveled < segLen && dotCount < kTrajectoryDotPoolSize_) {
			const float spacing = placeDot ? kDotSpacing : kGapSpacing;
			const float remaining = segLen - traveled;
			const float needed = spacing - distanceSinceLast;

			if (remaining < needed) {
				distanceSinceLast += remaining;
				break;
			}

			traveled += needed;
			distanceSinceLast = 0.0f;

			if (placeDot) {
				const float t = traveled / segLen;
				KamataEngine::Vector3 pos = {
				    a.x + dx * t,
				    a.y + dy * t,
				    1.0f,
				};
				if (drawDotAt(pos, kDotWorldScale, dotCount)) {
					++dotCount;
				}
			}

			placeDot = !placeDot;
		}
	}
}

void GameScene::CaptureGameplaySnapshot(GameplaySnapshot& outSnapshot) const {
	if (player_) {
		player_->CaptureSnapshot(outSnapshot.player);
	}
	tileMap_.CaptureGimmickSnapshot(outSnapshot.gimmick);
	outSnapshot.trampolines.clear();
	outSnapshot.trampolines.reserve(trampolineSprings_.size());
	for (const TrampolineSpring& spring : trampolineSprings_) {
		TrampolineSpringSnapshot springSnapshot;
		springSnapshot.type = spring.GetType();
		springSnapshot.center = spring.GetCenter();
		springSnapshot.usedByPlayer = spring.IsUsedByPlayer();
		outSnapshot.trampolines.push_back(springSnapshot);
	}
	outSnapshot.currentScreenX = currentScreenX_;
	outSnapshot.currentScreenY = currentScreenY_;
	outSnapshot.isFreeCamera = isFreeCamera_;
	outSnapshot.freeCameraCenterX = freeCameraCenterX_;
	outSnapshot.freeCameraCenterY = freeCameraCenterY_;
	outSnapshot.cameraZoomOut = cameraZoomOut_;
	outSnapshot.score = score_;
	outSnapshot.hitCount = hitCount;
	outSnapshot.hitCount2 = hitCount2;
	outSnapshot.nextTrampolineTypeIndex = nextTrampolineTypeIndex_;
}

void GameScene::RestoreTrampolinesFromSnapshot(const std::vector<TrampolineSpringSnapshot>& snapshotSprings) {
	trampolineSprings_.clear();
	const float springLayoutHalfW = TileMap::kSpringReferenceHalfW;
	const float springLayoutHalfH = TileMap::kSpringReferenceHalfH;
	trampolineSprings_.reserve(snapshotSprings.size());
	for (const TrampolineSpringSnapshot& springSnapshot : snapshotSprings) {
		TrampolineSpring spring;
		spring.SetType(springSnapshot.type);
		spring.SetCenter(springSnapshot.center, springLayoutHalfW, springLayoutHalfH);
		spring.SetUsedByPlayer(springSnapshot.usedByPlayer);
		trampolineSprings_.push_back(std::move(spring));
	}
}

bool GameScene::TrampolinesMatchSnapshot(const std::vector<TrampolineSpringSnapshot>& snapshotSprings) const {
	if (snapshotSprings.size() != trampolineSprings_.size()) {
		return false;
	}
	for (size_t i = 0; i < snapshotSprings.size(); ++i) {
		if (snapshotSprings[i].type != trampolineSprings_[i].GetType()) {
			return false;
		}
		if (snapshotSprings[i].usedByPlayer != trampolineSprings_[i].IsUsedByPlayer()) {
			return false;
		}
		const KamataEngine::Vector3 center = trampolineSprings_[i].GetCenter();
		const KamataEngine::Vector3& snapCenter = snapshotSprings[i].center;
		if (std::abs(center.x - snapCenter.x) > 0.01f || std::abs(center.y - snapCenter.y) > 0.01f ||
		    std::abs(center.z - snapCenter.z) > 0.01f) {
			return false;
		}
	}
	return true;
}

void GameScene::ApplyGameplaySnapshot(const GameplaySnapshot& snapshot, bool finalizeSideEffects) {
	// ズームはプレイヤー視点の設定なので巻き戻しで上書きしない（ズームイン不能バグの原因）
	const bool preserveFreeCamera = isFreeCamera_;
	const float preserveZoom = cameraZoomOut_;
	const float preserveCenterX = freeCameraCenterX_;
	const float preserveCenterY = freeCameraCenterY_;

	if (player_) {
		player_->ApplySnapshot(snapshot.player);
	}

	TileMapGimmickSnapshot gimmickBefore;
	tileMap_.CaptureGimmickSnapshot(gimmickBefore);
	tileMap_.ApplyGimmickSnapshot(snapshot.gimmick);
	if (gimmickBefore.deactivatedWalls != snapshot.gimmick.deactivatedWalls ||
	    gimmickBefore.pressedButtons != snapshot.gimmick.pressedButtons) {
		mapRenderer_.ApplyGimmickVisualsFromTileMap(tileMap_);
		if (!finalizeSideEffects) {
			rewindMinimapDirty_ = true;
		}
	}

	if (!TrampolinesMatchSnapshot(snapshot.trampolines)) {
		RestoreTrampolinesFromSnapshot(snapshot.trampolines);
	}

	currentScreenX_ = snapshot.currentScreenX;
	currentScreenY_ = snapshot.currentScreenY;
	// 位置と画面番号のズレを防ぐ（巻き戻し中の画面遅れ対策）
	if (player_) {
		tileMap_.GetScreenFromWorld(
		    player_->GetWorldPosition().x, player_->GetWorldPosition().y, currentScreenX_, currentScreenY_);
	}
	isFreeCamera_ = preserveFreeCamera;
	freeCameraCenterX_ = preserveCenterX;
	freeCameraCenterY_ = preserveCenterY;
	cameraZoomOut_ = preserveZoom;
	score_ = snapshot.score;
	hitCount = snapshot.hitCount;
	hitCount2 = snapshot.hitCount2;
	nextTrampolineTypeIndex_ = snapshot.nextTrampolineTypeIndex;

	lastPlayerPos_ = player_ ? player_->GetWorldPosition() : lastPlayerPos_;
	UpdateMapCamera();

	if (player_ && minimapPlayerSprite_) {
		minimapPlayerSprite_->SetPosition(ConvertWorldToMinimapPosition(lastPlayerPos_));
	}

	if (!finalizeSideEffects) {
		return;
	}

	UpdateScoreSprites();
	RebuildMinimapTiles();

	ClearRewindOverlayImmediate();
	rewindMinimapDirty_ = false;
	portalAbsorbFinishedPending_ = false;
	requestSceneClear_ = false;
}

void GameScene::ClearRewindOverlayImmediate() {
	rewindOverlayAlpha_ = 0.0f;
	spikeRewindOverlayAlpha_ = 0.0f;
	isSpikeRewindOverlayActive_ = false;
	spikeRewindScrubStarted_ = false;
	pendingRewindPostReleaseLock_ = false;
	rewindPostReleaseLockSeconds_ = 0.0f;
	rewindPostReleaseDimAlpha_ = 0.0f;
}

void GameScene::FinalizeGameplayRewindScrub() {
	// Qを離してからの短い余韻用に、暗転レベルを覚えておく（タイマーはQ離し時に開始）
	rewindPostReleaseDimAlpha_ = rewindOverlayAlpha_;
	if (rewindPostReleaseDimAlpha_ <= 0.0f) {
		rewindPostReleaseDimAlpha_ =
		    (isSpikeRewindOverlayActive_ || spikeRewindScrubStarted_) ? kSpikeRewindDimAlpha_ : kNormalRewindDimAlpha_;
	}
	pendingRewindPostReleaseLock_ = true;
	isSpikeRewindOverlayActive_ = false;
	spikeRewindScrubStarted_ = false;

	UpdateScoreSprites();

	// 巻き戻し中はPlayer更新が止まるため、バネ側の接触フラグ(isPlayerInside_)が
	// 巻き戻し前の状態のまま残ってしまう。Playerがバネと無関係な状態(None)に戻ったら
	// 接触状態をクリアして、再びバネへ触れたときに確実に発火できるようにする。
	if (player_ && player_->GetSpringChargePhase() == Player::SpringChargePhase::None) {
		for (TrampolineSpring& spring : trampolineSprings_) {
			spring.ResetPlayerContact();
		}
	}

	// 地上停止時のみ運動量をクリア（空中のバネ飛行などはスナップショットの速度を維持）
	if (player_) {
		player_->CancelMotionAfterRewindStop();
	}
}

void GameScene::SeedGameplayRewindSnapshot() {
	if (gameplayRewindSeeded_ || !player_ || !isGameIntroFinished_) {
		return;
	}
	GameplaySnapshot snapshot;
	CaptureGameplaySnapshot(snapshot);
	gameplayRewindBuffer_.ForceRecord(snapshot);
	gameplayRewindSeeded_ = true;
}

void GameScene::ApplyRewindScrubInterpolation(float tTowardTarget, bool undoDirection) {
	if (!player_ || tTowardTarget <= 0.0f) {
		return;
	}

	const int timelineIndex = gameplayRewindBuffer_.GetTimelineIndex();
	if (timelineIndex < 0) {
		return;
	}

	GameplaySnapshot fromSnapshot;
	GameplaySnapshot toSnapshot;
	if (!gameplayRewindBuffer_.GetSnapshotAt(timelineIndex, fromSnapshot)) {
		return;
	}

	const int targetIndex = undoDirection ? timelineIndex - 1 : timelineIndex + 1;
	if (undoDirection && targetIndex < gameplayRewindBuffer_.GetMinUndoTimelineIndex()) {
		return;
	}
	if (!gameplayRewindBuffer_.GetSnapshotAt(targetIndex, toSnapshot)) {
		return;
	}

	PlayerSnapshot lerpedPlayer;
	LerpPlayerSnapshot(fromSnapshot.player, toSnapshot.player, tTowardTarget, lerpedPlayer);
	player_->ApplySnapshot(lerpedPlayer);
	lastPlayerPos_ = lerpedPlayer.position;
	// 補間中も Player 位置から画面を同期（スナップショット跨ぎで数フレーム画面が戻らないのを防ぐ）
	tileMap_.GetScreenFromWorld(lastPlayerPos_.x, lastPlayerPos_.y, currentScreenX_, currentScreenY_);
	UpdateMapCamera();

	if (minimapPlayerSprite_) {
		minimapPlayerSprite_->SetPosition(ConvertWorldToMinimapPosition(lastPlayerPos_));
	}
}

void GameScene::DrawSpikeLifeHearts() {
	const float iconH = kHeartIconSize_.y;
	const float rowCenterY = kHeartUiPosition_.y;
	const float multiplySize = iconH * 0.55f;
	const float digitSize = iconH * 0.75f;
	const float innerGap = 4.0f;
	const float digitGap = 2.0f;

	float x = kHeartUiPosition_.x;

	// ハートアイコン
	if (spikeLifeHeartIconSprite_) {
		spikeLifeHeartIconSprite_->SetSize(kHeartIconSize_);
		spikeLifeHeartIconSprite_->SetPosition({x, rowCenterY});
		spikeLifeHeartIconSprite_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
		spikeLifeHeartIconSprite_->Draw();
	}
	x += kHeartIconSize_.x + innerGap;

	// ×
	if (spikeLifeMultiplySprite_) {
		spikeLifeMultiplySprite_->SetSize({multiplySize, multiplySize});
		spikeLifeMultiplySprite_->SetPosition({x, rowCenterY});
		spikeLifeMultiplySprite_->Draw();
	}
	x += multiplySize + innerGap;

	// 残機の数字（ビットマップフォント）
	const int lives = (std::max)(0, spikeLivesRemaining_);
	const int tens = lives / 10;
	const int ones = lives % 10;

	auto drawDigit = [&](KamataEngine::Sprite* sprite, int digit, float digitX) {
		if (!sprite || digit < 0 || digit > 9) {
			return;
		}
		uint32_t handle = stageSelectDigitTextureHandles_[static_cast<size_t>(digit)];
		if (handle == 0 && static_cast<size_t>(digit) < digitTextureHandles_.size()) {
			handle = digitTextureHandles_[static_cast<size_t>(digit)];
		}
		if (handle == 0) {
			return;
		}
		sprite->SetTextureHandle(handle);
		sprite->SetSize({digitSize, digitSize});
		sprite->SetPosition({digitX, rowCenterY});
		sprite->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
		sprite->Draw();
	};

	if (tens > 0) {
		drawDigit(spikeLifeTensDigitSprite_, tens, x);
		drawDigit(spikeLifeOnesDigitSprite_, ones, x + digitSize + digitGap);
	} else {
		drawDigit(spikeLifeOnesDigitSprite_, ones, x);
	}
}

void GameScene::UpdateGameplayRewindInput() {
	if (!input_ || GetSceneStateKind() != SceneStateKind::Game || !isGameIntroFinished_ || !player_) {
		if (isGameplayRewinding_) {
			FinalizeGameplayRewindScrub();
		}
		isGameplayRewinding_ = false;
		gameplayRewindScrubAccumulator_ = 0.0f;
		return;
	}

	const bool rewindBlocked =
	    player_->IsPortalAbsorbing() || portalAbsorbFinishedPending_ || requestSceneClear_ || isGameplayPaused_ ||
	    IsKeyWallTutorialActive();
	gameplayRewindBuffer_.SetMaxRewindSeconds(IsStageTutorialActive() ? GetTutorialRewindMaxSeconds() : -1.0f);
	if (rewindBlocked) {
		if (isGameplayRewinding_) {
			FinalizeGameplayRewindScrub();
		}
		isGameplayRewinding_ = false;
		gameplayRewindScrubAccumulator_ = 0.0f;
		gameplayRewindQHoldSeconds_ = 0.0f;
		return;
	}

	const bool qHeld = input_->PushKey(DIK_Q);

	// 一時停止解除に使ったQ押し／履歴先頭到達後の押しっぱなしは、離すまで巻き戻ししない
	if (gameplayPauseIgnoreQUntilRelease_) {
		if (qHeld) {
			if (isGameplayRewinding_) {
				FinalizeGameplayRewindScrub();
			}
			isGameplayRewinding_ = false;
			gameplayRewindScrubAccumulator_ = 0.0f;
			gameplayRewindQHoldSeconds_ = 0.0f;
			UpdateRewindOverlay();
			return;
		}
		// 離したらフラグを下ろし、下の通常の離し処理（一時停止など）へ進む
		gameplayPauseIgnoreQUntilRelease_ = false;
	}

	// チュートリアル中は「Qを押して…」が出るまで巻き戻し不可。3秒使い切り後も不可。
	const bool tutorialQStepReady =
	    stageTutorialStep_ == StageTutorialStep::PressQ ||
	    (stageTutorialStep_ == StageTutorialStep::HitSpike && isSpikeRewindOverlayActive_);
	const bool tutorialRewindBlocked =
	    IsStageTutorialActive() && (tutorialRewindExhausted_ || !tutorialQStepReady);
	bool scrubbingThisFrame = false;

	const bool spikeRewindActive = isSpikeRewindOverlayActive_;

	if (qHeld && !tutorialRewindBlocked) {
		gameplayRewindQHoldSeconds_ += 1.0f / 60.0f;
		float scrubSpeed = 1.0f;
		if (gameplayRewindQHoldSeconds_ >= kRewindHoldSpeed2xSeconds_) {
			scrubSpeed = 2.0f;
		} else if (gameplayRewindQHoldSeconds_ >= kRewindHoldSpeed15xSeconds_) {
			scrubSpeed = 1.5f;
		} else if (gameplayRewindQHoldSeconds_ >= kRewindHoldSpeed12xSeconds_) {
			scrubSpeed = 1.2f;
		}

		if (gameplayRewindBuffer_.CanUndo()) {
			gameplayRewindScrubAccumulator_ += GameplayRewindBuffer::kScrubSnapshotsPerFrame * scrubSpeed;
			scrubbingThisFrame = true;
			gameplayRewindScrubbedDuringHold_ = true;

			while (gameplayRewindScrubAccumulator_ >= 1.0f && gameplayRewindBuffer_.CanUndo()) {
				GameplaySnapshot snapshot;
				if (!gameplayRewindBuffer_.Undo(snapshot)) {
					gameplayRewindScrubAccumulator_ = 0.0f;
					break;
				}

				if (spikeRewindActive) {
					spikeRewindScrubStarted_ = true;
				}

				ApplyGameplaySnapshot(snapshot, false);
				gameplayRewindScrubAccumulator_ -= 1.0f;
				UpdateTutorialRewindExhaustion();

				// 棘チュートリアルの瞬間移動地点まで戻ったら、そこで巻き戻し終了
				if (tutorialSpikeTeleportRewindClamp_ && !gameplayRewindBuffer_.CanUndo()) {
					gameplayRewindScrubAccumulator_ = 0.0f;
					break;
				}
			}

			if (!gameplayRewindBuffer_.CanUndo()) {
				// 履歴の先頭（ステージ開始相当）まで戻った。
				// 押し続けたままだと直後に新規記録が積まれて再巻き戻し→チカチカするため、
				// いったん解除し、Qを離してから再度押すまで巻き戻し不可にする。
				scrubbingThisFrame = false;
				gameplayRewindScrubAccumulator_ = 0.0f;
				gameplayRewindQHoldSeconds_ = 0.0f;
				gameplayPauseIgnoreQUntilRelease_ = true;
			} else if (gameplayRewindScrubAccumulator_ > 0.0f) {
				ApplyRewindScrubInterpolation(gameplayRewindScrubAccumulator_, true);
			}
			UpdateTutorialRewindExhaustion();
		} else if (isGameplayRewinding_ || gameplayRewindScrubbedDuringHold_) {
			// すでに先頭にいる／今フレームで先頭到達した直後
			gameplayRewindQHoldSeconds_ = 0.0f;
			gameplayPauseIgnoreQUntilRelease_ = true;
		}
	} else {
		gameplayRewindScrubAccumulator_ = 0.0f;
		gameplayRewindQHoldSeconds_ = 0.0f;
	}

	// 先に Finalize（pending を立てる）してから、Q離し時の一時停止へ移行する
	if (isGameplayRewinding_ && !scrubbingThisFrame) {
		FinalizeGameplayRewindScrub();
	}

	if (!qHeld) {
		if (pendingRewindPostReleaseLock_) {
			pendingRewindPostReleaseLock_ = false;
			if (gameplayRewindScrubbedDuringHold_) {
				const bool pressQTutorial =
				    IsStageTutorialActive() && stageTutorialStep_ == StageTutorialStep::PressQ;
				const bool onRewindSpring = pressQTutorial && IsPlayerOnTutorialRewindSpring();
				if (pressQTutorial && onRewindSpring) {
					tutorialRewindReachedSpring_ = true;
					tutorialRewindEarlyReleaseHint_ = false;
					BeginGameplayPauseAfterRewind();
				} else if (pressQTutorial) {
					// バネに触れる前に離した → 一時停止せず案内
					tutorialRewindEarlyReleaseHint_ = true;
					rewindOverlayAlpha_ = 0.0f;
					spikeRewindOverlayAlpha_ = 0.0f;
					rewindPostReleaseLockSeconds_ = 0.0f;
				} else {
					BeginGameplayPauseAfterRewind();
				}
			} else {
				rewindOverlayAlpha_ = 0.0f;
				spikeRewindOverlayAlpha_ = 0.0f;
				rewindPostReleaseLockSeconds_ = 0.0f;
			}
		}
		gameplayRewindScrubbedDuringHold_ = false;
	}

	isGameplayRewinding_ = scrubbingThisFrame;
	UpdateRewindOverlay();
}

void GameScene::UpdateGameplayRewind() {
	if (GetSceneStateKind() != SceneStateKind::Game || !isGameIntroFinished_ || !player_) {
		return;
	}

	SeedGameplayRewindSnapshot();

	if (isGameplayRewinding_) {
		return;
	}

	// ボタン/消える壁の状態が巻き戻しで変わった場合のみ、次フレーム以降でミニマップを再構築する
	if (rewindMinimapDirty_) {
		rewindMinimapDirty_ = false;
		RebuildMinimapTiles();
	}

	// 針被弾後・巻き戻し開始前の待機中は記録しない
	if (isSpikeRewindOverlayActive_ && !spikeRewindScrubStarted_) {
		return;
	}

	// 一時停止中は巻き戻しバッファを進めない（停止時間が履歴を食い潰さない）
	if (isGameplayPaused_) {
		return;
	}

	// 巻き戻し誘導中は用意した履歴を上書きしない
	if (IsStageTutorialActive() &&
	    (stageTutorialStep_ == StageTutorialStep::PressQ || stageTutorialStep_ == StageTutorialStep::RewindIntro)) {
		return;
	}

	if (player_->IsPortalAbsorbing() || portalAbsorbFinishedPending_ || requestSceneClear_) {
		return;
	}

	gameplayRewindBuffer_.SetMaxRewindSeconds(IsStageTutorialActive() ? GetTutorialRewindMaxSeconds() : -1.0f);

	GameplaySnapshot snapshot;
	CaptureGameplaySnapshot(snapshot);
	gameplayRewindBuffer_.Record(snapshot);
}

void GameScene::UpdateRewindOverlay() {
	if (rewindPostReleaseLockSeconds_ > 0.0f) {
		rewindPostReleaseLockSeconds_ -= 1.0f / 60.0f;
		if (rewindPostReleaseLockSeconds_ <= 0.0f) {
			rewindPostReleaseLockSeconds_ = 0.0f;
			rewindOverlayAlpha_ = 0.0f;
		} else {
			rewindOverlayAlpha_ = rewindPostReleaseDimAlpha_;
		}
	} else if (pendingRewindPostReleaseLock_) {
		rewindOverlayAlpha_ = rewindPostReleaseDimAlpha_;
	} else if (isGameplayRewinding_) {
		const float target = (isSpikeRewindOverlayActive_ || spikeRewindScrubStarted_) ? kSpikeRewindDimAlpha_
		                                                                                 : kNormalRewindDimAlpha_;
		rewindOverlayAlpha_ = (std::min)(rewindOverlayAlpha_ + kRewindOverlayFadeInSpeed_, target);
		rewindStatusDotTimer_ += 1.0f / 60.0f;
		const float cycleSec = kRewindStatusDotStepSec_ * static_cast<float>(kRewindStatusDotMax_ + 1);
		if (rewindStatusDotTimer_ >= cycleSec) {
			rewindStatusDotTimer_ -= cycleSec;
		}
	} else if (isSpikeRewindOverlayActive_ && !spikeRewindScrubStarted_) {
		// 針被弾後、Qで巻き戻すまでの待機中
		rewindOverlayAlpha_ = (std::min)(rewindOverlayAlpha_ + kRewindOverlayFadeInSpeed_, kSpikeRewindDimAlpha_);
		rewindStatusDotTimer_ = 0.0f;
	} else {
		rewindOverlayAlpha_ = 0.0f;
		rewindStatusDotTimer_ = 0.0f;
	}

	spikeRewindOverlayAlpha_ = rewindOverlayAlpha_;
}

void GameScene::DrawSpikeRewindOverlay() {
	if (!spikeRewindDimSprite_ || rewindOverlayAlpha_ <= 0.0f) {
		return;
	}

	spikeRewindDimSprite_->SetColor({0.0f, 0.0f, 0.0f, rewindOverlayAlpha_});
	spikeRewindDimSprite_->Draw();
}

void GameScene::DrawGameplayRewindStatusLabel() {
	if (!isGameplayRewinding_ || !fontRenderer_ || !fontRenderer_->IsReady()) {
		return;
	}

	constexpr const char* kBase = "巻き戻し中";
	constexpr const char* kDots[4] = {"", "・", "・・", "・・・"};
	int dotCount = static_cast<int>(rewindStatusDotTimer_ / kRewindStatusDotStepSec_);
	if (dotCount < 0) {
		dotCount = 0;
	}
	if (dotCount > kRewindStatusDotMax_) {
		dotCount = kRewindStatusDotMax_;
	}

	const float fontH = kRewindStatusFontHeight_;
	const KamataEngine::Vector4 textColor = {1.0f, 1.0f, 1.0f, 1.0f};
	const float winW = static_cast<float>(WinApp::kWindowWidth);
	const float fullW = fontRenderer_->MeasureWidth("巻き戻し中・・・", fontH);
	const float baseW = fontRenderer_->MeasureWidth(kBase, fontH);
	float x = (winW - fullW) * 0.5f;
	if (x < 8.0f) {
		x = 8.0f;
	}
	const float y = kGameplayPauseStopCenterY_ - fontH * 0.5f - kTutorialTextOpticalOffsetY_;
	fontRenderer_->DrawString(kBase, x, y, fontH, textColor);
	if (dotCount > 0) {
		fontRenderer_->DrawString(kDots[dotCount], x + baseW, y, fontH, textColor);
	}
}

void GameScene::UpdateButtonGimmicks() {
	if (!player_ || player_->IsPortalAbsorbing()) {
		return;
	}

	const KamataEngine::Vector3 playerPos = player_->GetWorldPosition();
	const float playerHalfW = player_->GetHalfWidth();
	const float playerHalfH = player_->GetHalfHeight();

	int buttonCol = 0;
	int buttonRow = 0;
	if (!tileMap_.FindOverlappingUnpressedButton(playerPos.x, playerPos.y, playerHalfW, playerHalfH, buttonCol, buttonRow)) {
		return;
	}

	if (!tileMap_.PressButton(buttonCol, buttonRow)) {
		return;
	}

	tileMap_.DeactivateAllDisappearingWalls();
	mapRenderer_.DeactivateAllDisappearingWalls();
	mapRenderer_.SetButtonPressed(buttonCol, buttonRow);
	// 全スプライト作り直しは重いので遅延（巻き戻しと同じ経路で次フレーム更新）
	rewindMinimapDirty_ = true;

	if (audio_) {
		audio_->playAudioOneShot(keyCollectSound_, keyCollectSoundHandle_, 0.8f);
	}
}

void GameScene::UpdateStageBgm() {
	if (!audio_) {
		return;
	}

	const SceneStateKind kind = GetSceneStateKind();
	const bool inStage =
	    kind == SceneStateKind::Game || kind == SceneStateKind::GameIntro || kind == SceneStateKind::TransitionFromGame;
	if (!inStage) {
		StopStageBgm();
		return;
	}

	audio_->ensureLoopPlaying(stageBgmVoice_, stageBgmHandle_, 0.213f);
}

void GameScene::StopStageBgm() {
	if (!audio_) {
		stageBgmVoice_ = -1;
		return;
	}
	audio_->stopAudio(stageBgmVoice_);
}

void GameScene::UpdateTrampolinePlacement() {
	if (!input_ || !isGameIntroFinished_ || !player_) {
		hasTrampolinePreview_ = false;
		showSpringCollectLabel_ = false;
		return;
	}

	// 巻き戻し説明中は設置・種類変更・回収操作を受け付けない
	if (IsStageTutorialActive() && stageTutorialStep_ == StageTutorialStep::RewindIntro) {
		hasTrampolinePreview_ = false;
		showSpringCollectLabel_ = false;
		return;
	}
	if (IsKeyWallTutorialActive()) {
		hasTrampolinePreview_ = false;
		showSpringCollectLabel_ = false;
		return;
	}

	if (input_->IsPressMouse(2)) {
		hasTrampolinePreview_ = false;
		showSpringCollectLabel_ = false;
		return;
	}

	if (!HasAnySpringPlacementRemaining() && !HasAnyCollectableSpringOnMap() && !ShouldForceTutorialSpringPreview()) {
		hasTrampolinePreview_ = false;
		showSpringCollectLabel_ = false;
		return;
	}

	if (input_->IsTriggerMouse(1) && !input_->IsPressMouse(2)) {
		// 設置チュートリアル中は上バネ固定
		if (!(IsStageTutorialActive() && stageTutorialStep_ == StageTutorialStep::PlaceSpring)) {
			if (IsStageTutorialActive()) {
				nextTrampolineTypeIndex_++;
			} else {
				AdvanceToNextAvailableTrampolineType();
			}
		}
	}

	if (IsStageTutorialActive() && stageTutorialStep_ == StageTutorialStep::PlaceSpring) {
		nextTrampolineTypeIndex_ = 0;
	}

	SyncTrampolinePlacementType();

	const TrampolineSpringType nextType = TrampolineSpring::GetPlacementType(nextTrampolineTypeIndex_);

	const KamataEngine::Vector2 mousePos = GetClientMousePosition();
	trampolinePreviewPos_ = ConvertScreenToWorld(mousePos.x, mousePos.y);
	hasTrampolinePreview_ = true;

	const float springLayoutHalfW = TileMap::kSpringReferenceHalfW;
	const float springLayoutHalfH = TileMap::kSpringReferenceHalfH;

	if (trampolinePreview_.GetType() != nextType) {
		trampolinePreview_.SetType(nextType);
	}
	trampolinePreview_.SetCenter(trampolinePreviewPos_, springLayoutHalfW, springLayoutHalfH);

	float springHalfW = 0.0f;
	float springHalfH = 0.0f;
	trampolinePreview_.GetHalfSize(springHalfW, springHalfH);
	tileMap_.ClampPositionToMapBounds(trampolinePreviewPos_.x, trampolinePreviewPos_.y, springHalfW, springHalfH);
	trampolinePreview_.SetCenter(trampolinePreviewPos_, springLayoutHalfW, springLayoutHalfH);

	size_t collectIndex = 0;
	bool canCollect = false;
	if (IsStageTutorialActive() && stageTutorialStep_ == StageTutorialStep::CollectSpring) {
		// 回収チュートリアル: 使用済みでも回収可だが、カーソルと同じ種類のみ
		for (TrampolineSpring& spring : trampolineSprings_) {
			spring.SetUsedByPlayer(false);
		}
		canCollect = FindCollectableSpringAtCursor(
		    nextType, trampolinePreviewPos_.x, trampolinePreviewPos_.y, springLayoutHalfW, springLayoutHalfH, collectIndex);
		// 判定を少し広めに再チェック（種類一致は必須）
		if (!canCollect) {
			for (size_t i = 0; i < trampolineSprings_.size(); ++i) {
				TrampolineSpring& spring = trampolineSprings_[i];
				if (spring.GetType() != nextType) {
					continue;
				}
				float hitSpringHalfW = 0.0f;
				float hitSpringHalfH = 0.0f;
				spring.GetHalfSize(hitSpringHalfW, hitSpringHalfH);
				const float hitHalfW = hitSpringHalfW + springLayoutHalfW;
				const float hitHalfH = hitSpringHalfH + springLayoutHalfH;
				if (std::abs(trampolinePreviewPos_.x - spring.GetCenter().x) <= hitHalfW &&
				    std::abs(trampolinePreviewPos_.y - spring.GetCenter().y) <= hitHalfH) {
					collectIndex = i;
					canCollect = true;
					break;
				}
			}
		}
	} else {
		canCollect = FindCollectableSpringAtCursor(
		    nextType, trampolinePreviewPos_.x, trampolinePreviewPos_.y, springLayoutHalfW, springLayoutHalfH, collectIndex);
	}

	showSpringCollectLabel_ = canCollect;
	if (canCollect) {
		const TrampolineSpring& collectSpring = trampolineSprings_[collectIndex];
		const TrampolineSpringType collectType = collectSpring.GetType();
		springCollectLabelIsVertical_ =
		    (collectType == TrampolineSpringType::Left || collectType == TrampolineSpringType::Right);
		springCollectLabelWorldPos_ = collectSpring.GetCenter();
		collectSpring.GetHalfSize(springCollectLabelSpringHalfW_, springCollectLabelSpringHalfH_);
	}
	const bool placementBlocked =
	    IsSpringPlacementBlockedAtCursor(nextType, trampolinePreviewPos_.x, trampolinePreviewPos_.y, springLayoutHalfW, springLayoutHalfH);
	const bool outsideTutorialFrame = IsStageTutorialActive() && stageTutorialStep_ == StageTutorialStep::PlaceSpring &&
	                                  !IsTutorialPlacementAllowedAt(trampolinePreviewPos_.x, trampolinePreviewPos_.y, springHalfW, springHalfH);
	const bool tutorialPlacementLocked = IsTutorialSpringPlacementLocked();
	const bool canPlace =
	    GetRemainingSpringPlacementCount(nextType) > 0 && !placementBlocked && !outsideTutorialFrame && !tutorialPlacementLocked;

	if (input_->IsTriggerMouse(0) && !input_->IsPressMouse(1) && !input_->IsPressMouse(2)) {
		if (canCollect) {
			// 回収チュートリアル以外の誘導中は回収させない
			if (IsStageTutorialActive() && stageTutorialStep_ != StageTutorialStep::CollectSpring) {
				return;
			}
			trampolineSprings_.erase(trampolineSprings_.begin() + static_cast<std::ptrdiff_t>(collectIndex));
			showSpringCollectLabel_ = false;
			SyncTrampolinePlacementType();
			return;
		}
		if (canPlace) {
			TrampolineSpring spring;
			spring.SetType(nextType);
			spring.SetCenter(trampolinePreviewPos_, springLayoutHalfW, springLayoutHalfH);
			trampolineSprings_.push_back(std::move(spring));
			SyncTrampolinePlacementType();
		}
	}
}

bool GameScene::HasAnyCollectableSpringOnMap() const {
	for (const TrampolineSpring& spring : trampolineSprings_) {
		if (!spring.IsUsedByPlayer()) {
			return true;
		}
	}
	return false;
}

bool GameScene::FindCollectableSpringAtCursor(TrampolineSpringType cursorType, float cursorX, float cursorY, float cursorHalfW, float cursorHalfH,
    size_t& outIndex) const {
	if (player_ && player_->GetSpringChargePhase() != Player::SpringChargePhase::None) {
		return false;
	}

	for (size_t i = 0; i < trampolineSprings_.size(); ++i) {
		const TrampolineSpring& spring = trampolineSprings_[i];
		if (spring.IsUsedByPlayer()) {
			continue;
		}
		if (spring.GetType() != cursorType) {
			continue;
		}
		if (!spring.IsPlayerOverlapping(cursorX, cursorY, cursorHalfW, cursorHalfH)) {
			continue;
		}
		outIndex = i;
		return true;
	}
	return false;
}

bool GameScene::IsSpringPlacementBlockedAtCursor(TrampolineSpringType cursorType, float cursorX, float cursorY, float cursorHalfW, float cursorHalfH) const {
	for (const TrampolineSpring& spring : trampolineSprings_) {
		if (!spring.IsPlayerOverlapping(cursorX, cursorY, cursorHalfW, cursorHalfH)) {
			continue;
		}
		if (!spring.IsUsedByPlayer() && spring.GetType() == cursorType) {
			continue;
		}
		return true;
	}
	return false;
}

bool GameScene::HasAnySpringPlacementRemaining() const {
	static constexpr TrampolineSpringType kTypes[] = {
	    TrampolineSpringType::Up,
	    TrampolineSpringType::Down,
	    TrampolineSpringType::Left,
	    TrampolineSpringType::Right,
	};
	for (TrampolineSpringType type : kTypes) {
		if (GetRemainingSpringPlacementCount(type) > 0) {
			return true;
		}
	}
	return false;
}

bool GameScene::IsTrampolineTypeSelectable(TrampolineSpringType type) const {
	// 誘導チュートリアル中は種類一周などのため全種類選べる
	if (IsStageTutorialActive()) {
		return true;
	}
	// ステージ上限0の種類だけ選べない。残数0（使い切り）は選べて ×0 表示する
	return GetSpringPlacementLimit(type) > 0;
}

void GameScene::AdvanceToNextAvailableTrampolineType() {
	const int startIndex = nextTrampolineTypeIndex_;
	for (int attempt = 0; attempt < 4; ++attempt) {
		nextTrampolineTypeIndex_++;
		const TrampolineSpringType type = TrampolineSpring::GetPlacementType(nextTrampolineTypeIndex_);
		if (IsTrampolineTypeSelectable(type)) {
			return;
		}
	}
	nextTrampolineTypeIndex_ = startIndex;
}

void GameScene::SyncTrampolinePlacementType() {
	if (IsStageTutorialActive()) {
		return;
	}

	if (IsTrampolineTypeSelectable(TrampolineSpring::GetPlacementType(nextTrampolineTypeIndex_))) {
		return;
	}

	const int startIndex = nextTrampolineTypeIndex_;
	do {
		nextTrampolineTypeIndex_++;
		const TrampolineSpringType type = TrampolineSpring::GetPlacementType(nextTrampolineTypeIndex_);
		if (IsTrampolineTypeSelectable(type)) {
			return;
		}
	} while (nextTrampolineTypeIndex_ != startIndex);
}

void GameScene::UpdateTrampolineArrowAnimations() {
	trampolineArrowAnimTime_ += 1.0f / 60.0f;
}

void GameScene::InitializeSpringPlacementHud() {
	static const char* kSpringHudIconPaths[4] = {"Up/Up.png", "down/down.png", "left/left.png", "light/light.png"};
	for (int i = 0; i < 4; ++i) {
		springHudIconTextureHandles_[static_cast<size_t>(i)] = KamataEngine::TextureManager::Load(kSpringHudIconPaths[i]);
	}

	springMultiplyTextureHandle_ = KamataEngine::TextureManager::Load("kakeru.png");

	springCollectLabelTextureHandle_ = KamataEngine::TextureManager::Load("respawn.png");
	if (springCollectLabelTextureHandle_ != 0) {
		springCollectLabelSprite_ = KamataEngine::Sprite::Create(springCollectLabelTextureHandle_, {0.0f, 0.0f});
		if (springCollectLabelSprite_) {
			springCollectLabelSprite_->SetAnchorPoint({0.5f, 0.5f});
			const KamataEngine::Vector2& nativeSize = springCollectLabelSprite_->GetSize();
			if (nativeSize.y > 0.0f) {
				springCollectLabelAspect_ = nativeSize.x / nativeSize.y;
			}
		}
	}

	springCollectLabelVerticalTextureHandle_ = KamataEngine::TextureManager::Load("respawn2.png");
	if (springCollectLabelVerticalTextureHandle_ != 0) {
		springCollectLabelVerticalSprite_ = KamataEngine::Sprite::Create(springCollectLabelVerticalTextureHandle_, {0.0f, 0.0f});
		if (springCollectLabelVerticalSprite_) {
			springCollectLabelVerticalSprite_->SetAnchorPoint({0.5f, 0.5f});
			const KamataEngine::Vector2& nativeSize = springCollectLabelVerticalSprite_->GetSize();
			if (nativeSize.y > 0.0f) {
				springCollectLabelVerticalAspect_ = nativeSize.x / nativeSize.y;
			}
		}
	}

	const uint32_t defaultDigitHandle =
	    stageSelectDigitTextureHandles_[0] != 0 ? stageSelectDigitTextureHandles_[0] : digitTextureHandles_[0];

	for (int i = 0; i < 4; ++i) {
		SpringPlacementHudUi& ui = springPlacementHudUi_[static_cast<size_t>(i)];
		const uint32_t iconHandle = springHudIconTextureHandles_[static_cast<size_t>(i)];
		if (iconHandle != 0) {
			ui.iconSprite = KamataEngine::Sprite::Create(iconHandle, {0.0f, 0.0f});
			if (ui.iconSprite) {
				ui.iconSprite->SetAnchorPoint({0.0f, 0.5f});
				const KamataEngine::Vector2& nativeSize = ui.iconSprite->GetSize();
				if (nativeSize.y > 0.0f) {
					ui.iconAspect = nativeSize.x / nativeSize.y;
				}
			}
		}

		if (springMultiplyTextureHandle_ != 0) {
			ui.multiplySprite = KamataEngine::Sprite::Create(springMultiplyTextureHandle_, {0.0f, 0.0f});
			if (ui.multiplySprite) {
				ui.multiplySprite->SetAnchorPoint({0.0f, 0.5f});
			}
		}

		if (defaultDigitHandle != 0) {
			ui.tensDigitSprite = KamataEngine::Sprite::Create(defaultDigitHandle, {0.0f, 0.0f});
			ui.onesDigitSprite = KamataEngine::Sprite::Create(defaultDigitHandle, {0.0f, 0.0f});
			if (ui.tensDigitSprite) {
				ui.tensDigitSprite->SetAnchorPoint({0.0f, 0.5f});
			}
			if (ui.onesDigitSprite) {
				ui.onesDigitSprite->SetAnchorPoint({0.0f, 0.5f});
			}
		}
	}
}

int GameScene::GetSpikeLivesMaxForCurrentStage() const {
	// ステージ8（currentStageIndex_ == 7）はハート30
	if (currentStageIndex_ == 7) {
		return kSpikeLivesMaxStage8_;
	}
	return kSpikeLivesMax_;
}

int GameScene::GetSpringPlacementLimit(TrampolineSpringType type) const {
	// displayStage = currentStageIndex_ + 1
	// ステージ1は誘導後 FreePlay（maptutorial）のみ適用
	struct Limits {
		int up;
		int down;
		int left;
		int right;
	};

	Limits limits = {kSpringPlacementLimitPerType_, kSpringPlacementLimitPerType_, kSpringPlacementLimitPerType_,
	                 kSpringPlacementLimitPerType_};

	if (currentStageIndex_ == 0) {
		if (tutorialMapMode_ != TutorialMapMode::FreePlay) {
			return kSpringPlacementLimitPerType_;
		}
		// ステージ1: 上1・右2
		limits = {1, 0, 0, 2};
	} else {
		switch (currentStageIndex_) {
		case 1: // ステージ2
			limits = {1, 0, 1, 1};
			break;
		case 2: // ステージ3
			limits = {1, 0, 1, 1};
			break;
		case 3: // ステージ4
			limits = {1, 1, 1, 1};
			break;
		case 4: // ステージ5
			limits = {1, 0, 0, 0};
			break;
		case 5: // ステージ6
			limits = {4, 1, 2, 2};
			break;
		case 6: // ステージ7
			limits = {6, 2, 5, 7};
			break;
		case 7: // ステージ8
			limits = {6, 1, 4, 3};
			break;
		case 8: // ステージ9
			limits = {6, 4, 1, 3};
			break;
		case 9: // ステージ10
			limits = {3, 0, 2, 2};
			break;
		default:
			break;
		}
	}

	switch (type) {
	case TrampolineSpringType::Up:
		return limits.up;
	case TrampolineSpringType::Down:
		return limits.down;
	case TrampolineSpringType::Left:
		return limits.left;
	case TrampolineSpringType::Right:
		return limits.right;
	}
	return 0;
}

int GameScene::GetRemainingSpringPlacementCount(TrampolineSpringType type) const {
	if (IsStageTutorialActive()) {
		(void)type;
		return (std::max)(0, kTutorialSpringPlacementLimit_ - static_cast<int>(trampolineSprings_.size()));
	}

	const int limit = GetSpringPlacementLimit(type);
	if (limit <= 0) {
		return 0;
	}

	int placedCount = 0;
	for (const TrampolineSpring& spring : trampolineSprings_) {
		if (spring.GetType() == type) {
			++placedCount;
		}
	}
	return (std::max)(0, limit - placedCount);
}

bool GameScene::IsStageTutorialActive() const {
	return stageTutorialStep_ != StageTutorialStep::Inactive && stageTutorialStep_ != StageTutorialStep::Done;
}

bool GameScene::IsKeyWallTutorialActive() const {
	return keyWallTutorialPhase_ != KeyWallTutorialPhase::Inactive && keyWallTutorialPhase_ != KeyWallTutorialPhase::Done;
}

bool GameScene::ShouldFreezePlayerForTutorial() const {
	return IsKeyWallTutorialActive() ||
	       (IsStageTutorialActive() &&
	        (stageTutorialStep_ == StageTutorialStep::PlaceSpring ||
	         (stageTutorialStep_ == StageTutorialStep::RedSpringHint && tutorialRedSpringHintFrozen_) ||
	         stageTutorialStep_ == StageTutorialStep::ChangeType || tutorialPlayerControlLocked_));
}

bool GameScene::IsTutorialSpringReactive() const {
	if (!IsStageTutorialActive()) {
		return true;
	}
	return stageTutorialStep_ == StageTutorialStep::TouchSpring || stageTutorialStep_ == StageTutorialStep::ChargeSpring ||
	       stageTutorialStep_ == StageTutorialStep::SpacePauseHint;
}

bool GameScene::IsTutorialSpikeReactive() const {
	if (!IsStageTutorialActive()) {
		return true;
	}
	if (tutorialSpikeAwaitingExit_) {
		return false;
	}
	return stageTutorialStep_ == StageTutorialStep::HitSpike || stageTutorialStep_ == StageTutorialStep::ScrollWheel ||
	       stageTutorialStep_ == StageTutorialStep::ShowGoal || stageTutorialStep_ == StageTutorialStep::PressR;
}

bool GameScene::IsPlayerOverlappingSpike() const {
	if (!player_) {
		return false;
	}
	const KamataEngine::Vector3 pos = player_->GetWorldPosition();
	return tileMap_.OverlapsSpike(pos.x, pos.y, player_->GetHalfWidth(), player_->GetHalfHeight());
}

bool GameScene::ShouldForceTutorialSpringPreview() const {
	if (!IsStageTutorialActive()) {
		return false;
	}
	switch (stageTutorialStep_) {
	case StageTutorialStep::WaitLand:
	case StageTutorialStep::ChangeType:
	case StageTutorialStep::HitSpike:
	case StageTutorialStep::RewindIntro:
	case StageTutorialStep::PressQ:
	case StageTutorialStep::RideSpringAgain:
	case StageTutorialStep::SpacePauseHint:
	case StageTutorialStep::CollectSpring:
	case StageTutorialStep::ScrollWheel:
	case StageTutorialStep::ShowGoal:
	case StageTutorialStep::PressR:
		return true;
	default:
		return false;
	}
}

bool GameScene::IsTutorialSpringPlacementLocked() const {
	if (!IsStageTutorialActive()) {
		return false;
	}
	return stageTutorialStep_ != StageTutorialStep::PlaceSpring;
}

float GameScene::GetTutorialRewindMaxSeconds() const { return kTutorialMaxRewindSeconds_; }

void GameScene::UpdateTutorialRewindExhaustion() {
	if (!IsStageTutorialActive() || tutorialRewindExhausted_) {
		return;
	}

	const int snapshotCount = gameplayRewindBuffer_.GetSnapshotCount();
	if (snapshotCount <= 0) {
		return;
	}

	const int presentIndex = snapshotCount - 1;
	const int timelineIndex = gameplayRewindBuffer_.GetTimelineIndex();
	const int maxUndoSteps = static_cast<int>(kTutorialMaxRewindSeconds_ / GameplayRewindBuffer::kSecondsPerSnapshot);
	if (presentIndex - timelineIndex >= maxUndoSteps) {
		tutorialRewindExhausted_ = true;
	}
}

bool GameScene::IsTutorialPlacementAllowedAt(float worldX, float worldY, float springHalfW, float springHalfH) const {
	// 描画の緑枠より一回り小さい範囲に、バネ全体が収まるときだけ許可
	const float halfW = kTutorialFrameHalfW_ * kTutorialFramePlaceScale_;
	const float halfH = kTutorialFrameHalfH_ * kTutorialFramePlaceScale_;
	const float minX = kTutorialFrameCenterX_ - halfW;
	const float maxX = kTutorialFrameCenterX_ + halfW;
	const float minY = kTutorialFrameCenterY_ - halfH;
	const float maxY = kTutorialFrameCenterY_ + halfH;
	return (worldX - springHalfW) >= minX && (worldX + springHalfW) <= maxX &&
	       (worldY - springHalfH) >= minY && (worldY + springHalfH) <= maxY;
}

bool GameScene::IsTutorialGoalVisibleInView(KamataEngine::Vector3& outGoalPos) {
	if (goalPositions_.empty() || cameraZoomOut_ < 0.12f) {
		return false;
	}

	float viewLeft = 0.0f, viewBottom = 0.0f, viewRight = 0.0f, viewTop = 0.0f;
	ComputeCameraBounds(viewLeft, viewBottom, viewRight, viewTop);

	for (const KamataEngine::Vector3& goalPos : goalPositions_) {
		int screenX = 0;
		int screenY = 0;
		tileMap_.GetScreenFromWorld(goalPos.x, goalPos.y, screenX, screenY);
		if (screenX < 1) {
			continue; // 右マップ側のゴールを対象にする
		}
		if (goalPos.x > viewLeft && goalPos.x < viewRight && goalPos.y > viewBottom && goalPos.y < viewTop) {
			outGoalPos = goalPos;
			return true;
		}
	}
	return false;
}

void GameScene::BeginTutorialTimedBanner(const char* text, float seconds, bool centerOnScreen) {
	tutorialTimedBannerText_ = text;
	tutorialTimedBannerRemainSec_ = (seconds > 0.0f) ? seconds : 0.0f;
	tutorialTimedBannerCentered_ = centerOnScreen;
}

void GameScene::UpdateTutorialTimedBanner() {
	if (tutorialTimedBannerRemainSec_ <= 0.0f) {
		tutorialTimedBannerText_ = nullptr;
		return;
	}
	tutorialTimedBannerRemainSec_ -= 1.0f / 60.0f;
	if (tutorialTimedBannerRemainSec_ <= 0.0f) {
		tutorialTimedBannerRemainSec_ = 0.0f;
		tutorialTimedBannerText_ = nullptr;
	}
}

void GameScene::DrawTutorialTimedBanner() {
	if (tutorialTimedBannerRemainSec_ <= 0.0f || !tutorialTimedBannerText_ || !fontRenderer_ ||
	    !fontRenderer_->IsReady()) {
		return;
	}

	const float fontH = kTutorialPauseHintFontHeight_;
	const KamataEngine::Vector4 textColor = {1.0f, 1.0f, 1.0f, 1.0f};
	const float winW = static_cast<float>(WinApp::kWindowWidth);
	const float winH = static_cast<float>(WinApp::kWindowHeight);
	const float textW = fontRenderer_->MeasureWidth(tutorialTimedBannerText_, fontH);

	float anchorX = winW * 0.5f;
	float textTopY = winH * kTutorialPauseLandHintTextAnchorY_ - fontH * 0.5f;
	if (!tutorialTimedBannerCentered_) {
		const KamataEngine::Vector2 springScreen =
		    ConvertWorldToScreen(tutorialPlacedSpringWorldPos_.x, tutorialPlacedSpringWorldPos_.y);
		float springHalfH = TileMap::kSpringReferenceHalfH;
		if (!trampolineSprings_.empty()) {
			float springHalfW = 0.0f;
			trampolineSprings_.front().GetHalfSize(springHalfW, springHalfH);
		}
		float viewLeft = 0.0f, viewBottom = 0.0f, viewRight = 0.0f, viewTop = 0.0f;
		ComputeCameraBounds(viewLeft, viewBottom, viewRight, viewTop);
		const float pxPerWorldY =
		    (viewTop > viewBottom) ? (winH / (viewTop - viewBottom)) : 1.0f;
		const float springTopScreenY = springScreen.y - springHalfH * pxPerWorldY;
		anchorX = springScreen.x;
		textTopY = springTopScreenY - kTutorialTimedBannerTextAboveSpringPx_ - fontH;
	}

	float textX = anchorX - textW * 0.5f;
	if (textX < 8.0f) {
		textX = 8.0f;
	}
	if (textX + textW > winW - 8.0f) {
		textX = winW - 8.0f - textW;
	}
	fontRenderer_->DrawString(
	    tutorialTimedBannerText_, textX, textTopY - kTutorialTextOpticalOffsetY_, fontH, textColor);
}

float GameScene::FindTutorialGroundCenterY(float worldX, float halfW, float halfH) const {
	float y = tileMap_.GetOffsetY() + static_cast<float>(tileMap_.GetHeight()) * TileMap::kTileHeight - halfH;
	float velY = -1.0f;
	for (int i = 0; i < 256; ++i) {
		bool landed = false;
		const float nextY = y - TileMap::kTileHeight * 0.5f;
		float resolveY = nextY;
		tileMap_.ResolveCollisionY(resolveY, worldX, halfW, halfH, velY, landed);
		y = resolveY;
		if (landed) {
			return y;
		}
	}
	return y;
}

bool GameScene::IsPlayerOnTutorialRewindSpring() const {
	if (!player_ || trampolineSprings_.empty()) {
		return false;
	}
	const KamataEngine::Vector3 pos = player_->GetWorldPosition();
	for (const TrampolineSpring& spring : trampolineSprings_) {
		if (spring.IsPlayerOverlapping(pos.x, pos.y, player_->GetHalfWidth(), player_->GetHalfHeight())) {
			return true;
		}
	}
	return false;
}

void GameScene::SetupTutorialRewindToSpringCourse() {
	if (!player_) {
		return;
	}

	isSpikeRewindOverlayActive_ = false;
	spikeRewindScrubStarted_ = false;
	spikeRewindOverlayAlpha_ = 0.0f;
	rewindOverlayAlpha_ = 0.0f;
	isGameplayPaused_ = false;
	isGameplayRewinding_ = false;

	tutorialPreRewindPlayerPos_ = player_->GetWorldPosition();
	tutorialPreRewindSprings_.clear();
	tutorialPreRewindSprings_.reserve(trampolineSprings_.size());
	for (const TrampolineSpring& spring : trampolineSprings_) {
		TrampolineSpringSnapshot springSnapshot;
		springSnapshot.type = spring.GetType();
		springSnapshot.center = spring.GetCenter();
		springSnapshot.usedByPlayer = spring.IsUsedByPlayer();
		tutorialPreRewindSprings_.push_back(springSnapshot);
	}
	tutorialHasPreRewindSnapshot_ = true;

	const float halfH = player_->GetHalfHeight();
	const float playerHeight = halfH * 2.0f;
	const float springHalfW = TileMap::kSpringReferenceHalfW;
	const float springHalfH = TileMap::kSpringReferenceHalfH;

	// 現在位置のまま（瞬間移動しない）
	const float playerX = tutorialPreRewindPlayerPos_.x;
	const float playerGroundY = tutorialPreRewindPlayerPos_.y;

	// カメラをプレイヤー基準にしてから、画面左へ右向きバネを置く
	UpdateMapCamera();
	float viewLeft = 0.0f, viewBottom = 0.0f, viewRight = 0.0f, viewTop = 0.0f;
	ComputeCameraBounds(viewLeft, viewBottom, viewRight, viewTop);

	const float springX = viewLeft + (viewRight - viewLeft) * 0.22f;
	const float groundYAtSpring = FindTutorialGroundCenterY(springX, springHalfW, springHalfH);
	// 地面よりプレイヤー約2個分上（バネ中心）
	const float springY = (groundYAtSpring - halfH) + playerHeight * 2.0f + springHalfH;

	trampolineSprings_.clear();
	TrampolineSpring spring;
	spring.SetType(TrampolineSpringType::Right);
	spring.SetCenter({springX, springY, 1.0f}, springHalfW, springHalfH);
	spring.SetUsedByPlayer(false);
	trampolineSprings_.push_back(std::move(spring));
	tutorialPlacedSpringWorldPos_ = {springX, springY, 1.0f};
	nextTrampolineTypeIndex_ = 2; // GetPlacementType: Right

	// バネ位置 → 現在位置まで履歴を作り、Qでバネまで戻せるようにする
	gameplayRewindBuffer_.Clear();
	gameplayRewindBuffer_.SetMaxRewindSeconds(kTutorialMaxRewindSeconds_);
	constexpr int kPathSnapshots = 72;
	for (int i = 0; i <= kPathSnapshots; ++i) {
		const float t = static_cast<float>(i) / static_cast<float>(kPathSnapshots);
		const float x = springX + (playerX - springX) * t;
		const float y = springY + (playerGroundY - springY) * t;
		player_->SetPosition({x, y, 1.0f});
		player_->RefreshWorldMatrix();
		GameplaySnapshot snapshot;
		CaptureGameplaySnapshot(snapshot);
		snapshot.player.onGround = (i == kPathSnapshots);
		snapshot.player.velocityX = 0.0f;
		snapshot.player.velocityY = 0.0f;
		gameplayRewindBuffer_.ForceRecord(snapshot);
	}

	player_->SetPosition({playerX, playerGroundY, 1.0f});
	player_->RefreshWorldMatrix();
	gameplayRewindSeeded_ = true;
	gameplayRewindScrubAccumulator_ = 0.0f;
	gameplayRewindQHoldSeconds_ = 0.0f;
	tutorialRewindExhausted_ = false;
	tutorialRewindReachedSpring_ = false;
	tutorialRewindEarlyReleaseHint_ = false;
}

void GameScene::RestoreTutorialPreRewindCourse() {
	if (!tutorialHasPreRewindSnapshot_ || !player_) {
		return;
	}

	player_->SetPosition(tutorialPreRewindPlayerPos_);
	player_->RefreshWorldMatrix();
	RestoreTrampolinesFromSnapshot(tutorialPreRewindSprings_);
	SyncTrampolinePlacementType();
	if (!trampolineSprings_.empty()) {
		tutorialPlacedSpringWorldPos_ = trampolineSprings_.front().GetCenter();
	}
	gameplayRewindBuffer_.Clear();
	gameplayRewindSeeded_ = false;
	isGameplayPaused_ = false;
	isGameplayRewinding_ = false;
	tutorialHasPreRewindSnapshot_ = false;
}

void GameScene::ResetStageTutorial() {
	stageTutorialStep_ = StageTutorialStep::Inactive;
	ResetKeyWallTutorial();
	tutorialTouchedSpring_ = false;
	tutorialChargeReady_ = false;
	tutorialChargeFlying_ = false;
	tutorialChargeLaunched_ = false;
	tutorialChargePeakY_ = 0.0f;
	tutorialTypeCycleAway_ = false;
	tutorialDidRewind_ = false;
	tutorialRewindExhausted_ = false;
	tutorialPlayerControlLocked_ = false;
	tutorialSpikeAwaitingExit_ = false;
	tutorialSpikeTeleportRewindClamp_ = false;
	tutorialWasOnSpringForSecondRide_ = false;
	tutorialSpacePauseApexReached_ = false;
	tutorialSpacePauseReleased_ = false;
	tutorialSpacePauseAirFrames_ = 0;
	tutorialSpacePausePeakY_ = 0.0f;
	tutorialRideSpringLaunchY_ = 0.0f;
	tutorialRewindReachedSpring_ = false;
	tutorialRewindEarlyReleaseHint_ = false;
	tutorialRewindIntroRemainSec_ = 0.0f;
	tutorialPauseChargeLaunched_ = false;
	tutorialPauseLandHintActive_ = false;
	tutorialPauseLandHintRemainSec_ = 0.0f;
	tutorialPauseLandGroundFrames_ = 0;
	tutorialPauseAirborneFrames_ = 0;
	tutorialHasPreRewindSnapshot_ = false;
	tutorialPreRewindPlayerPos_ = {};
	tutorialPreRewindSprings_.clear();
	tutorialPauseIntroLockRemainSec_ = 0.0f;
	tutorialTimedBannerRemainSec_ = 0.0f;
	tutorialTimedBannerText_ = nullptr;
	tutorialTimedBannerCentered_ = false;
	isGameplayPaused_ = false;
	gameplayPauseSavedVelX_ = 0.0f;
	gameplayPauseSavedVelY_ = 0.0f;
	gameplayPauseSpaceWasDown_ = false;
	gameplayPauseSpaceHoldFrames_ = 0;
	gameplayPauseIgnoreSpaceUntilRelease_ = false;
	gameplayPauseIgnoreQUntilRelease_ = false;
	tutorialWheelMoved_ = false;
	tutorialSeenTypeMask_ = 0;
	tutorialTypeCycleStartIndex_ = 0;
	tutorialPrevTypeIndex_ = 0;
	tutorialGoalVisibleFrames_ = 0;
	tutorialPlacedSpringWorldPos_ = {};
	tutorialGoalWorldPos_ = {};
	tutorialZoomAtWheelStep_ = 0.0f;
	tutorialRedSpringHintFrozen_ = false;
	tutorialRedSpringHintAirFrames_ = 0;
	tutorialRedSpringHintLaunchY_ = 0.0f;
	if (player_) {
		player_->SetMinSpringChargeToLaunch(0.0f);
		player_->SetBlockUnchargedSpringLaunch(false);
		player_->SetSpringInteractionEnabled(true);
		player_->SetSpikeInteractionEnabled(true);
		player_->SetHorizontalMoveEnabled(true);
		player_->SetInstantSpringLaunch(false);
	}
}

void GameScene::ResetKeyWallTutorial() {
	keyWallTutorialPhase_ = KeyWallTutorialPhase::Inactive;
	keyWallTutorialTimer_ = 0.0f;
	keyWallTutorialKeyWorld_ = {};
	keyWallTutorialWallWorld_ = {};
	keyWallTutorialTextScreen_ = {};
	keyWallTutorialFromScreen_ = {};
	keyWallTutorialToScreen_ = {};
	keyWallTutorialText_ = nullptr;
	keyWallTutorialTextAlpha_ = 1.0f;
}

bool GameScene::FindKeyWallTutorialAnchors() {
	keyWallTutorialKeyWorld_ = {};
	keyWallTutorialWallWorld_ = {};
	bool foundKey = false;
	bool foundWall = false;
	float wallSumX = 0.0f;
	float wallSumY = 0.0f;
	int wallCount = 0;

	const int mapW = tileMap_.GetWidth();
	const int mapH = tileMap_.GetHeight();
	for (int row = 0; row < mapH; ++row) {
		for (int col = 0; col < mapW; ++col) {
			float minX = 0.0f;
			float minY = 0.0f;
			float maxX = 0.0f;
			float maxY = 0.0f;
			tileMap_.GetTileWorldRect(col, row, minX, minY, maxX, maxY);
			const float cx = (minX + maxX) * 0.5f;
			const float cy = (minY + maxY) * 0.5f;

			if (!foundKey && tileMap_.IsButton(col, row) && !tileMap_.IsButtonPressed(col, row)) {
				keyWallTutorialKeyWorld_ = {cx, cy, 1.0f};
				foundKey = true;
			}
			if (tileMap_.IsDisappearingWallActive(col, row)) {
				wallSumX += cx;
				wallSumY += cy;
				++wallCount;
				foundWall = true;
			}
		}
	}

	if (foundWall && wallCount > 0) {
		keyWallTutorialWallWorld_ = {wallSumX / static_cast<float>(wallCount), wallSumY / static_cast<float>(wallCount), 1.0f};
	}
	return foundKey && foundWall;
}

void GameScene::InitializeKeyWallTutorial() {
	ResetKeyWallTutorial();
	if (currentStageIndex_ != kKeyWallTutorialStageIndex_) {
		return;
	}
	if (!FindKeyWallTutorialAnchors()) {
		keyWallTutorialPhase_ = KeyWallTutorialPhase::Done;
		return;
	}

	keyWallTutorialPhase_ = KeyWallTutorialPhase::ShowAtKey;
	keyWallTutorialTimer_ = 0.0f;
	keyWallTutorialText_ = "鍵を取る";
	keyWallTutorialTextAlpha_ = 0.0f;

	// 画面座標は intro 後に確定するので、Update 初回で同期する
	keyWallTutorialTextScreen_ = ConvertWorldToScreen(keyWallTutorialKeyWorld_.x, keyWallTutorialKeyWorld_.y);
	keyWallTutorialFromScreen_ = keyWallTutorialTextScreen_;
	keyWallTutorialToScreen_ = ConvertWorldToScreen(keyWallTutorialWallWorld_.x, keyWallTutorialWallWorld_.y);
}

void GameScene::UpdateKeyWallTutorial() {
	if (!isGameIntroFinished_ || !player_) {
		return;
	}
	if (currentStageIndex_ != kKeyWallTutorialStageIndex_) {
		if (keyWallTutorialPhase_ != KeyWallTutorialPhase::Inactive) {
			ResetKeyWallTutorial();
		}
		return;
	}
	if (keyWallTutorialPhase_ == KeyWallTutorialPhase::Inactive || keyWallTutorialPhase_ == KeyWallTutorialPhase::Done) {
		return;
	}

	const float dt = 1.0f / 60.0f;
	keyWallTutorialTimer_ += dt;

	auto easeOutCubic = [](float t) {
		t = std::clamp(t, 0.0f, 1.0f);
		const float u = 1.0f - t;
		return 1.0f - u * u * u;
	};
	auto easeInOutCubic = [](float t) {
		t = std::clamp(t, 0.0f, 1.0f);
		if (t < 0.5f) {
			return 4.0f * t * t * t;
		}
		const float u = -2.0f * t + 2.0f;
		return 1.0f - u * u * u * 0.5f;
	};

	const float fontH = kTutorialFontHeight_ + 4.0f;
	const KamataEngine::Vector2 keyScreen = ConvertWorldToScreen(keyWallTutorialKeyWorld_.x, keyWallTutorialKeyWorld_.y);
	const KamataEngine::Vector2 wallScreen = ConvertWorldToScreen(keyWallTutorialWallWorld_.x, keyWallTutorialWallWorld_.y);

	const char* kTextKey = "鍵を取る";
	const char* kTextWall = "鍵を取ると壁が消えます";
	const float keyTextW = (fontRenderer_ && fontRenderer_->IsReady()) ? fontRenderer_->MeasureWidth(kTextKey, fontH) : 120.0f;
	const float wallTextW = (fontRenderer_ && fontRenderer_->IsReady()) ? fontRenderer_->MeasureWidth(kTextWall, fontH) : 280.0f;
	(void)keyTextW;

	// テキスト左上付近のアンカー（鍵の右／壁の左）
	const KamataEngine::Vector2 keyTextAnchor = {keyScreen.x + 48.0f, keyScreen.y - fontH * 0.5f};
	const KamataEngine::Vector2 wallTextAnchor = {wallScreen.x - 48.0f - wallTextW, wallScreen.y - fontH * 0.5f};

	switch (keyWallTutorialPhase_) {
	case KeyWallTutorialPhase::ShowAtKey: {
		keyWallTutorialText_ = kTextKey;
		keyWallTutorialTextScreen_ = keyTextAnchor;
		keyWallTutorialTextAlpha_ = easeOutCubic(keyWallTutorialTimer_ / kKeyWallTutorialFadeInSec_);
		if (keyWallTutorialTimer_ >= kKeyWallTutorialHoldSec_) {
			keyWallTutorialPhase_ = KeyWallTutorialPhase::MoveToWall;
			keyWallTutorialTimer_ = 0.0f;
			keyWallTutorialFromScreen_ = keyTextAnchor;
			keyWallTutorialToScreen_ = wallTextAnchor;
			keyWallTutorialText_ = kTextWall;
			keyWallTutorialTextAlpha_ = 1.0f;
		}
		break;
	}
	case KeyWallTutorialPhase::MoveToWall: {
		keyWallTutorialFromScreen_ = keyTextAnchor;
		keyWallTutorialToScreen_ = wallTextAnchor;
		const float t = easeInOutCubic(keyWallTutorialTimer_ / kKeyWallTutorialMoveSec_);
		keyWallTutorialTextScreen_.x = keyWallTutorialFromScreen_.x + (keyWallTutorialToScreen_.x - keyWallTutorialFromScreen_.x) * t;
		keyWallTutorialTextScreen_.y = keyWallTutorialFromScreen_.y + (keyWallTutorialToScreen_.y - keyWallTutorialFromScreen_.y) * t;
		keyWallTutorialText_ = kTextWall;
		keyWallTutorialTextAlpha_ = 1.0f;
		if (keyWallTutorialTimer_ >= kKeyWallTutorialMoveSec_) {
			keyWallTutorialPhase_ = KeyWallTutorialPhase::ShowAtWall;
			keyWallTutorialTimer_ = 0.0f;
			keyWallTutorialTextScreen_ = wallTextAnchor;
		}
		break;
	}
	case KeyWallTutorialPhase::ShowAtWall: {
		keyWallTutorialText_ = kTextWall;
		keyWallTutorialTextScreen_ = wallTextAnchor;
		keyWallTutorialTextAlpha_ = 1.0f;
		if (keyWallTutorialTimer_ >= kKeyWallTutorialHoldSec_) {
			keyWallTutorialPhase_ = KeyWallTutorialPhase::Done;
			keyWallTutorialTimer_ = 0.0f;
			keyWallTutorialText_ = nullptr;
			keyWallTutorialTextAlpha_ = 0.0f;
		}
		break;
	}
	default:
		break;
	}
}

void GameScene::DrawKeyWallTutorial() {
	if (!IsKeyWallTutorialActive() || GetSceneStateKind() != SceneStateKind::Game || !isGameIntroFinished_) {
		return;
	}
	if (!fontRenderer_ || !fontRenderer_->IsReady() || !keyWallTutorialText_) {
		return;
	}

	if (pauseDimSprite_) {
		pauseDimSprite_->SetColor({0.0f, 0.0f, 0.0f, kTutorialDimAlpha_ * 0.65f * keyWallTutorialTextAlpha_});
		pauseDimSprite_->Draw();
	}

	const float fontH = kTutorialFontHeight_ + 4.0f;
	const float width = fontRenderer_->MeasureWidth(keyWallTutorialText_, fontH);
	const float winW = static_cast<float>(WinApp::kWindowWidth);
	const float winH = static_cast<float>(WinApp::kWindowHeight);

	float drawX = std::clamp(keyWallTutorialTextScreen_.x, 8.0f, winW - width - 8.0f);
	float drawY = std::clamp(keyWallTutorialTextScreen_.y - kTutorialTextOpticalOffsetY_, 8.0f, winH - fontH - 8.0f);

	const KamataEngine::Vector4 textColor = {1.0f, 1.0f, 1.0f, keyWallTutorialTextAlpha_};
	fontRenderer_->DrawString(keyWallTutorialText_, drawX, drawY, fontH, textColor);
}

void GameScene::InitializeStageTutorial() {
	ResetStageTutorial();
	if (currentStageIndex_ != 0) {
		return;
	}
	if (tutorialMapMode_ == TutorialMapMode::FreePlay) {
		stageTutorialStep_ = StageTutorialStep::Done;
		if (player_) {
			player_->SetSpringInteractionEnabled(true);
			player_->SetSpikeInteractionEnabled(true);
			player_->SetHorizontalMoveEnabled(true);
		}
		return;
	}
	stageTutorialStep_ = StageTutorialStep::PlaceSpring;
	nextTrampolineTypeIndex_ = 0;
	if (player_) {
		player_->SetSpringInteractionEnabled(false);
		player_->SetSpikeInteractionEnabled(false);
		player_->SetHorizontalMoveEnabled(true);
	}
}

void GameScene::UpdateStageTutorial() {
	if (!isGameIntroFinished_ || !player_) {
		return;
	}
	if (currentStageIndex_ != 0) {
		if (stageTutorialStep_ != StageTutorialStep::Inactive) {
			ResetStageTutorial();
		}
		return;
	}
	if (stageTutorialStep_ == StageTutorialStep::Inactive || stageTutorialStep_ == StageTutorialStep::Done) {
		player_->SetSpringInteractionEnabled(true);
		player_->SetSpikeInteractionEnabled(true);
		player_->SetHorizontalMoveEnabled(true);
		player_->SetInstantSpringLaunch(false);
		return;
	}

	player_->SetSpringInteractionEnabled(IsTutorialSpringReactive());
	player_->SetSpikeInteractionEnabled(IsTutorialSpikeReactive());
	player_->SetInstantSpringLaunch(false);

	switch (stageTutorialStep_) {
	case StageTutorialStep::PlaceSpring:
		if (!trampolineSprings_.empty()) {
			tutorialPlacedSpringWorldPos_ = trampolineSprings_.back().GetCenter();
			stageTutorialStep_ = StageTutorialStep::TouchSpring;
			player_->SetSpringInteractionEnabled(true);
		}
		break;

	case StageTutorialStep::TouchSpring:
		if (player_->GetSpringChargePhase() != Player::SpringChargePhase::None) {
			tutorialTouchedSpring_ = true;
			stageTutorialStep_ = StageTutorialStep::ChargeSpring;
			player_->SetMinSpringChargeToLaunch(kTutorialMinCharge_);
			player_->SetBlockUnchargedSpringLaunch(true);
		}
		break;

	case StageTutorialStep::ChargeSpring: {
		player_->SetMinSpringChargeToLaunch(kTutorialMinCharge_);
		player_->SetBlockUnchargedSpringLaunch(true);
		const Player::SpringChargePhase phase = player_->GetSpringChargePhase();

		// 少しでもチャージしたら発射可能扱い（すぐ離しても飛ぶ）
		if (phase == Player::SpringChargePhase::Charging && player_->GetJumpSpringChargeLevel() >= kTutorialMinCharge_) {
			tutorialChargeReady_ = true;
		}

		// 発射した瞬間を記録
		if (tutorialChargeReady_ && !tutorialChargeFlying_ && phase == Player::SpringChargePhase::None &&
		    !player_->IsOnGround()) {
			tutorialChargeFlying_ = true;
			tutorialRedSpringHintLaunchY_ = player_->GetWorldPosition().y;
			tutorialRedSpringHintAirFrames_ = 0;
			tutorialChargePeakY_ = tutorialRedSpringHintLaunchY_;
			player_->SetMinSpringChargeToLaunch(0.0f);
			player_->SetBlockUnchargedSpringLaunch(false);
			player_->SetSpringInteractionEnabled(false);
			player_->SetHorizontalMoveEnabled(false);
		}

		// このジャンプの最高到達点で次へ
		if (tutorialChargeFlying_) {
			++tutorialRedSpringHintAirFrames_;
			const float y = player_->GetWorldPosition().y;
			if (y > tutorialChargePeakY_) {
				tutorialChargePeakY_ = y;
			}
			const float rise = tutorialChargePeakY_ - tutorialRedSpringHintLaunchY_;
			const bool reachedApex =
			    tutorialRedSpringHintAirFrames_ >= 4 && player_->GetVelocityY() <= 0.0f && rise >= 16.0f;
			if (reachedApex) {
				stageTutorialStep_ = StageTutorialStep::RedSpringHint;
				tutorialRedSpringHintFrozen_ = true; // 頂点でそのまま案内
				tutorialChargeLaunched_ = true;
			}
		}
		break;
	}

	case StageTutorialStep::RedSpringHint:
		if (!tutorialRedSpringHintFrozen_) {
			++tutorialRedSpringHintAirFrames_;
			player_->SetHorizontalMoveEnabled(false);
			const KamataEngine::Vector3 playerPos = player_->GetWorldPosition();
			const KamataEngine::Vector2 playerScreen = ConvertWorldToScreen(playerPos.x, playerPos.y);
			const float midY = static_cast<float>(WinApp::kWindowHeight) * 0.5f;
			const float rise = playerPos.y - tutorialRedSpringHintLaunchY_;
			// 少し飛んでから画面中央付近（または十分上昇／タイムアウト）で止めて案内
			const bool nearScreenMid = tutorialRedSpringHintAirFrames_ >= 8 && playerScreen.y <= midY + 24.0f;
			const bool midAirRise = tutorialRedSpringHintAirFrames_ >= 12 && rise >= 90.0f;
			const bool timeout = tutorialRedSpringHintAirFrames_ >= 45;
			if (nearScreenMid || midAirRise || timeout) {
				tutorialRedSpringHintFrozen_ = true;
			}
		} else if (input_ && (input_->TriggerKey(DIK_A) || input_->TriggerKey(DIK_D))) {
			// 案内表示後にA/Dを押した瞬間から左右移動を解放
			tutorialRedSpringHintFrozen_ = false;
			player_->SetHorizontalMoveEnabled(true);
			stageTutorialStep_ = StageTutorialStep::WaitLand;
		}
		break;

	case StageTutorialStep::WaitLand:
		if (player_->IsOnGround()) {
			stageTutorialStep_ = StageTutorialStep::ChangeType;
			tutorialTypeCycleStartIndex_ = nextTrampolineTypeIndex_;
			tutorialPrevTypeIndex_ = nextTrampolineTypeIndex_;
			tutorialSeenTypeMask_ = 1u << static_cast<uint32_t>((nextTrampolineTypeIndex_ % 4 + 4) % 4);
			tutorialTypeCycleAway_ = false;
			player_->SetSpringInteractionEnabled(false);
		}
		break;

	case StageTutorialStep::ChangeType: {
		const int cur = nextTrampolineTypeIndex_;
		if (cur != tutorialPrevTypeIndex_) {
			const int typeIdx = ((cur % 4) + 4) % 4;
			const int startIdx = ((tutorialTypeCycleStartIndex_ % 4) + 4) % 4;
			tutorialSeenTypeMask_ |= 1u << static_cast<uint32_t>(typeIdx);
			if (typeIdx != startIdx) {
				tutorialTypeCycleAway_ = true;
			}
			if (tutorialTypeCycleAway_ && typeIdx == startIdx && tutorialSeenTypeMask_ == 0xFu) {
				// 巻き戻し→バネ→一時停止→チャージのコースへ
				SetupTutorialRewindToSpringCourse();
				stageTutorialStep_ = StageTutorialStep::RewindIntro;
				tutorialRewindIntroRemainSec_ = kTutorialRewindIntroSec_;
				tutorialDidRewind_ = false;
				tutorialRewindReachedSpring_ = false;
				tutorialRewindEarlyReleaseHint_ = false;
				tutorialRewindExhausted_ = false;
				tutorialPlayerControlLocked_ = true;
				tutorialSpikeAwaitingExit_ = false;
				tutorialSpikeTeleportRewindClamp_ = false;
				player_->SetSpringInteractionEnabled(false);
				player_->SetHorizontalMoveEnabled(false);
				player_->SetSpikeInteractionEnabled(false);
			}
			tutorialPrevTypeIndex_ = cur;
		}
		break;
	}

	case StageTutorialStep::HitSpike:
		// 誘導ルートではスキップ（旧ステップ）
		if (isSpikeRewindOverlayActive_) {
			stageTutorialStep_ = StageTutorialStep::PressQ;
			tutorialDidRewind_ = false;
			tutorialSpikeAwaitingExit_ = false;
		}
		break;

	case StageTutorialStep::RewindIntro:
		tutorialRewindIntroRemainSec_ -= 1.0f / 60.0f;
		if (tutorialRewindIntroRemainSec_ <= 0.0f) {
			tutorialRewindIntroRemainSec_ = 0.0f;
			stageTutorialStep_ = StageTutorialStep::PressQ;
			tutorialDidRewind_ = false;
			// 説明中にQを押し続けていた場合は、一度離してから巻き戻せるようにする
			if (input_ && input_->PushKey(DIK_Q)) {
				gameplayPauseIgnoreQUntilRelease_ = true;
			}
		}
		break;

	case StageTutorialStep::PressQ:
		if (isGameplayRewinding_ || (input_ && input_->PushKey(DIK_Q))) {
			tutorialDidRewind_ = true;
			if (IsPlayerOnTutorialRewindSpring()) {
				tutorialRewindReachedSpring_ = true;
				tutorialRewindEarlyReleaseHint_ = false;
			}
		}
		// バネ上でQを離して一時停止に入ったら次へ
		if (tutorialRewindReachedSpring_ && isGameplayPaused_) {
			tutorialPlayerControlLocked_ = false;
			tutorialPauseChargeLaunched_ = false;
			tutorialPauseLandGroundFrames_ = 0;
			tutorialPauseAirborneFrames_ = 0;
			tutorialChargeReady_ = false;
			tutorialSpacePauseReleased_ = false;
			tutorialSpikeTeleportRewindClamp_ = false;
			tutorialPauseIntroLockRemainSec_ = 0.0f;
			tutorialPauseLandHintActive_ = false;
			tutorialPauseLandHintRemainSec_ = 0.0f;
			tutorialTimedBannerRemainSec_ = 0.0f;
			tutorialTimedBannerText_ = nullptr;
			stageTutorialStep_ = StageTutorialStep::SpacePauseHint;
			player_->SetHorizontalMoveEnabled(true);
			player_->SetSpringInteractionEnabled(true);
			player_->SetInstantSpringLaunch(false);
			player_->SetMinSpringChargeToLaunch(kTutorialMinCharge_);
			player_->SetBlockUnchargedSpringLaunch(true);
		}
		break;

	case StageTutorialStep::RideSpringAgain:
		stageTutorialStep_ = StageTutorialStep::SpacePauseHint;
		tutorialSpacePauseApexReached_ = true;
		tutorialSpacePauseReleased_ = false;
		break;

	case StageTutorialStep::SpacePauseHint: {
		if (tutorialPauseLandHintActive_) {
			UpdateTutorialTimedBanner();
			tutorialPauseLandHintRemainSec_ = tutorialTimedBannerRemainSec_;
			if (tutorialTimedBannerRemainSec_ <= 0.0f) {
				tutorialPauseLandHintRemainSec_ = 0.0f;
				tutorialPauseLandHintActive_ = false;
				RestoreTutorialPreRewindCourse();
				tutorialPlayerControlLocked_ = true;
				player_->SetHorizontalMoveEnabled(false);
				player_->SetSpringInteractionEnabled(false);
				player_->SetInstantSpringLaunch(false);
				player_->SetMinSpringChargeToLaunch(0.0f);
				player_->SetBlockUnchargedSpringLaunch(false);
				for (TrampolineSpring& spring : trampolineSprings_) {
					spring.SetUsedByPlayer(false);
				}
				if (!trampolineSprings_.empty()) {
					tutorialPlacedSpringWorldPos_ = trampolineSprings_.front().GetCenter();
				}
				stageTutorialStep_ = StageTutorialStep::CollectSpring;
			}
			break;
		}

		player_->SetMinSpringChargeToLaunch(kTutorialMinCharge_);
		player_->SetBlockUnchargedSpringLaunch(true);
		const Player::SpringChargePhase phase = player_->GetSpringChargePhase();
		if (phase == Player::SpringChargePhase::Charging &&
		    player_->GetJumpSpringChargeLevel() >= kTutorialMinCharge_) {
			tutorialChargeReady_ = true;
		}
		if (tutorialSpacePauseReleased_ && !isGameplayPaused_ && !player_->IsOnGround()) {
			if (!tutorialPauseChargeLaunched_) {
				tutorialPauseChargeLaunched_ = true;
				tutorialPauseAirborneFrames_ = 0;
				player_->SetMinSpringChargeToLaunch(0.0f);
				player_->SetBlockUnchargedSpringLaunch(false);
				// 着地判定をバネ再接触で潰さない
				player_->SetSpringInteractionEnabled(false);
			}
		}
		if (tutorialPauseChargeLaunched_ && !isGameplayPaused_) {
			if (!player_->IsOnGround()) {
				++tutorialPauseAirborneFrames_;
				tutorialPauseLandGroundFrames_ = 0;
			} else if (tutorialPauseAirborneFrames_ >= 8) {
				++tutorialPauseLandGroundFrames_;
			} else {
				tutorialPauseLandGroundFrames_ = 0;
			}
			if (tutorialPauseLandGroundFrames_ >= 2) {
				tutorialPauseLandGroundFrames_ = 0;
				tutorialPauseAirborneFrames_ = 0;
				tutorialPauseLandHintActive_ = true;
				tutorialPauseLandHintRemainSec_ = kTutorialPauseLandHintSec_;
				BeginTutorialTimedBanner(
				    "一時停止はWASD・SPACE・Qでも再開できます", kTutorialPauseLandHintSec_, true);
				tutorialPlayerControlLocked_ = true;
				player_->SetHorizontalMoveEnabled(false);
				player_->SetSpringInteractionEnabled(false);
			}
		}
		break;
	}

	case StageTutorialStep::CollectSpring:
		if (trampolineSprings_.empty()) {
			stageTutorialStep_ = StageTutorialStep::ScrollWheel;
			tutorialZoomAtWheelStep_ = cameraZoomOut_;
			tutorialWheelMoved_ = false;
		}
		break;

	case StageTutorialStep::ScrollWheel:
		if (std::abs(cameraZoomOut_ - tutorialZoomAtWheelStep_) > 0.03f) {
			tutorialWheelMoved_ = true;
			stageTutorialStep_ = StageTutorialStep::ShowGoal;
			tutorialGoalVisibleFrames_ = 0;
		}
		break;

	case StageTutorialStep::ShowGoal: {
		KamataEngine::Vector3 goalPos{};
		if (IsTutorialGoalVisibleInView(goalPos)) {
			tutorialGoalWorldPos_ = goalPos;
			++tutorialGoalVisibleFrames_;
			if (tutorialGoalVisibleFrames_ >= kTutorialGoalVisibleFramesToAdvance_) {
				stageTutorialStep_ = StageTutorialStep::PressR;
			}
		}
		break;
	}

	case StageTutorialStep::PressR:
		break;

	default:
		break;
	}
}

void GameScene::DrawStageTutorial() {
	if (!IsStageTutorialActive() || GetSceneStateKind() != SceneStateKind::Game || !isGameIntroFinished_) {
		return;
	}

	// 画面全体を薄暗くする（SPACE一時停止中は専用オーバーレイに任せる／着地後の案内中は暗転する）
	if (pauseDimSprite_ &&
	    (stageTutorialStep_ != StageTutorialStep::SpacePauseHint || tutorialPauseLandHintActive_)) {
		pauseDimSprite_->SetColor({0.0f, 0.0f, 0.0f, kTutorialDimAlpha_});
		pauseDimSprite_->Draw();
	}

	const KamataEngine::Vector2 frameScreen = ConvertWorldToScreen(kTutorialFrameCenterX_, kTutorialFrameCenterY_);
	// 画面上で完全な正方形にする（X/Yのピクセル換算が微妙に違うため、一辺はX基準で揃える）
	float viewLeft = 0.0f, viewBottom = 0.0f, viewRight = 0.0f, viewTop = 0.0f;
	ComputeCameraBounds(viewLeft, viewBottom, viewRight, viewTop);
	const float winW = static_cast<float>(WinApp::kWindowWidth);
	const float pxPerWorldX = (viewRight > viewLeft) ? (winW / (viewRight - viewLeft)) : 1.0f;
	const float frameSidePx = kTutorialFrameHalfW_ * 2.0f * pxPerWorldX;
	const float frameW = frameSidePx;
	const float frameH = frameSidePx;

	// バネ・プレイヤーを暗幕の上に再描画（3Dパス）
	const bool spotlightWorld =
	    stageTutorialStep_ == StageTutorialStep::PlaceSpring || stageTutorialStep_ == StageTutorialStep::TouchSpring ||
	    stageTutorialStep_ == StageTutorialStep::ChargeSpring || stageTutorialStep_ == StageTutorialStep::RedSpringHint ||
	    stageTutorialStep_ == StageTutorialStep::WaitLand || stageTutorialStep_ == StageTutorialStep::ChangeType ||
	    stageTutorialStep_ == StageTutorialStep::HitSpike || stageTutorialStep_ == StageTutorialStep::RewindIntro ||
	    stageTutorialStep_ == StageTutorialStep::PressQ ||
	    stageTutorialStep_ == StageTutorialStep::RideSpringAgain || stageTutorialStep_ == StageTutorialStep::SpacePauseHint ||
	    stageTutorialStep_ == StageTutorialStep::CollectSpring ||
	    stageTutorialStep_ == StageTutorialStep::ScrollWheel || stageTutorialStep_ == StageTutorialStep::ShowGoal ||
	    stageTutorialStep_ == StageTutorialStep::PressR;
	if (spotlightWorld) {
		ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();
		KamataEngine::Sprite::PostDraw();
		dxCommon_->ClearDepthBuffer();
		KamataEngine::Model::PreDraw(commandList);
		DrawTrampolineSprings();
		if (player_ && stageTutorialStep_ != StageTutorialStep::PlaceSpring) {
			player_->Draw();
		}
		KamataEngine::Model::PostDraw();
		KamataEngine::Sprite::PreDraw(commandList);
		if (fontRenderer_) {
			fontRenderer_->NewFrame();
		}
	}

	// 設置枠（greenBox）を最前面に
	if (stageTutorialStep_ == StageTutorialStep::PlaceSpring && tutorialFrameSprite_) {
		tutorialFrameSprite_->SetPosition(frameScreen);
		tutorialFrameSprite_->SetSize({frameW, frameH});
		tutorialFrameSprite_->SetColor({0.35f, 0.95f, 0.45f, 0.7f});
		tutorialFrameSprite_->Draw();
	}

	// チャージ中の枠も暗幕の上に再表示
	if (stageTutorialStep_ == StageTutorialStep::ChargeSpring) {
		DrawJumpSpringChargeCircle();
	}

	if (!fontRenderer_ || !fontRenderer_->IsReady()) {
		return;
	}

	const float fontH = kTutorialFontHeight_;
	const KamataEngine::Vector4 textColor = {1.0f, 1.0f, 1.0f, 1.0f};
	const float winH = static_cast<float>(WinApp::kWindowHeight);

	// 対象の左側・縦中央。文字の末尾が左端の少し手前に来る（はみ出す場合は少し小さく）
	auto drawTextLeftOf = [&](const char* text, float leftEdgeX, float centerY) {
		float useH = fontH;
		float width = fontRenderer_->MeasureWidth(text, useH);
		float x = leftEdgeX - kTutorialTextGapLeftOfFrame_ - width;
		while (x < 8.0f && useH > 15.0f) {
			useH -= 1.0f;
			width = fontRenderer_->MeasureWidth(text, useH);
			x = leftEdgeX - kTutorialTextGapLeftOfFrame_ - width;
		}
		if (x < 8.0f) {
			x = 8.0f;
		}
		fontRenderer_->DrawString(text, x, centerY - useH * 0.5f - kTutorialTextOpticalOffsetY_, useH, textColor);
	};
	auto drawCenteredTop = [&](const char* text) {
		const float cy = winH * 0.22f;
		float useH = fontH;
		float width = fontRenderer_->MeasureWidth(text, useH);
		while (width > winW - 24.0f && useH > 15.0f) {
			useH -= 1.0f;
			width = fontRenderer_->MeasureWidth(text, useH);
		}
		fontRenderer_->DrawString(
		    text, (winW - width) * 0.5f, cy - useH * 0.5f - kTutorialTextOpticalOffsetY_, useH, textColor);
	};
	// バネ上（一時停止案内と同じ高さ。extraLowerPx で少し下へ）
	auto drawCenteredAboveSpringHint = [&](const char* text, float hintFontH, float extraLowerPx = 0.0f) {
		const KamataEngine::Vector2 springScreen =
		    ConvertWorldToScreen(tutorialPlacedSpringWorldPos_.x, tutorialPlacedSpringWorldPos_.y);
		float springHalfH = TileMap::kSpringReferenceHalfH;
		if (!trampolineSprings_.empty()) {
			float springHalfW = 0.0f;
			trampolineSprings_.front().GetHalfSize(springHalfW, springHalfH);
		}
		const float pxPerWorldY =
		    (viewTop > viewBottom) ? (winH / (viewTop - viewBottom)) : 1.0f;
		const float springTopScreenY = springScreen.y - springHalfH * pxPerWorldY;
		const float lineGap = 6.0f;
		const float blockH = hintFontH * 2.0f + lineGap;
		const float topY = springTopScreenY - kTutorialPauseHintAboveSpringPx_ - blockH + extraLowerPx;
		const float width = fontRenderer_->MeasureWidth(text, hintFontH);
		float x = springScreen.x - width * 0.5f;
		if (x < 8.0f) {
			x = 8.0f;
		}
		if (x + width > winW - 8.0f) {
			x = winW - 8.0f - width;
		}
		fontRenderer_->DrawString(text, x, topY - kTutorialTextOpticalOffsetY_, hintFontH, textColor);
	};

	const float frameLeftX = frameScreen.x - frameW * 0.5f;

	switch (stageTutorialStep_) {
	case StageTutorialStep::PlaceSpring:
		drawTextLeftOf("緑の枠の中にバネを置いてみよう", frameLeftX, frameScreen.y);
		break;
	case StageTutorialStep::TouchSpring: {
		const KamataEngine::Vector2 springScreen =
		    ConvertWorldToScreen(tutorialPlacedSpringWorldPos_.x, tutorialPlacedSpringWorldPos_.y);
		float springHalfW = TileMap::kSpringReferenceHalfW;
		float springHalfH = TileMap::kSpringReferenceHalfH;
		if (!trampolineSprings_.empty()) {
			trampolineSprings_.front().GetHalfSize(springHalfW, springHalfH);
		}
		const float springLeftX = springScreen.x - springHalfW * pxPerWorldX;
		drawTextLeftOf("WASDで動いて、バネに触れてみよう", springLeftX, springScreen.y);
		break;
	}
	case StageTutorialStep::ChargeSpring:
		if (player_) {
			const KamataEngine::Vector3 playerPos = player_->GetWorldPosition();
			const KamataEngine::Vector2 playerScreen = ConvertWorldToScreen(playerPos.x, playerPos.y);
			drawTextLeftOf("SPACEを長押しして飛ばしてみよう", playerScreen.x - 40.0f, playerScreen.y);
		}
		break;
	case StageTutorialStep::RedSpringHint:
		// 中央で止まったあと、Playerの左に案内を出す
		if (tutorialRedSpringHintFrozen_ && player_) {
			const KamataEngine::Vector3 playerPos = player_->GetWorldPosition();
			const KamataEngine::Vector2 playerScreen = ConvertWorldToScreen(playerPos.x, playerPos.y);
			drawTextLeftOf("空中でもA・Dで左右に動けるよ", playerScreen.x - 40.0f, playerScreen.y);
		}
		break;
	case StageTutorialStep::WaitLand:
		break;
	case StageTutorialStep::ChangeType:
		drawTextLeftOf("右クリックでバネの種類を変えよう", frameLeftX, frameScreen.y);
		break;
	case StageTutorialStep::HitSpike:
		if (player_) {
			const KamataEngine::Vector3 playerPos = player_->GetWorldPosition();
			const KamataEngine::Vector2 playerScreen = ConvertWorldToScreen(playerPos.x, playerPos.y);
			drawTextLeftOf("紫のトゲに当たってみよう", playerScreen.x - 40.0f, playerScreen.y - 28.0f);
		}
		break;
	case StageTutorialStep::RewindIntro: {
		const float hintH = kTutorialPauseHintFontHeight_;
		const float centerY = winH * kTutorialPauseLandHintTextAnchorY_;
		constexpr const char* kRewindIntroText = "Qでいつでも時間を巻き戻せます";
		float useH = hintH;
		float width = fontRenderer_->MeasureWidth(kRewindIntroText, useH);
		while (width > winW - 32.0f && useH > 20.0f) {
			useH -= 1.0f;
			width = fontRenderer_->MeasureWidth(kRewindIntroText, useH);
		}
		fontRenderer_->DrawString(
		    kRewindIntroText, (winW - width) * 0.5f, centerY - useH * 0.5f - kTutorialTextOpticalOffsetY_, useH,
		    textColor);
		break;
	}
	case StageTutorialStep::PressQ:
		if (player_) {
			const KamataEngine::Vector3 playerPos = player_->GetWorldPosition();
			const KamataEngine::Vector2 playerScreen = ConvertWorldToScreen(playerPos.x, playerPos.y);
			drawCenteredAboveSpringHint(
			    "Qで戻したあと、離すと一時停止します", kTutorialPauseHintFontHeight_,
			    kTutorialRewindMechanismHintLowerPx_);
			if (tutorialRewindEarlyReleaseHint_ && !tutorialRewindReachedSpring_) {
				drawTextLeftOf("バネに触れるまで巻き戻してみよう", playerScreen.x - 40.0f, playerScreen.y);
			} else if (!tutorialDidRewind_) {
				drawTextLeftOf("Qで巻き戻してみよう", playerScreen.x - 40.0f, playerScreen.y);
			}
		}
		break;
	case StageTutorialStep::RideSpringAgain:
		break;
	case StageTutorialStep::SpacePauseHint:
		if (tutorialPauseLandHintActive_) {
			DrawTutorialTimedBanner();
		}
		break;
	case StageTutorialStep::CollectSpring: {
		const float hintH = kTutorialPauseHintFontHeight_;
		const float baseY = winH * 0.26f;
		auto drawCenteredLine = [&](const char* text, float centerY) {
			float useH = hintH;
			float width = fontRenderer_->MeasureWidth(text, useH);
			while (width > winW - 32.0f && useH > 20.0f) {
				useH -= 1.0f;
				width = fontRenderer_->MeasureWidth(text, useH);
			}
			fontRenderer_->DrawString(
			    text, (winW - width) * 0.5f, centerY - useH * 0.5f - kTutorialTextOpticalOffsetY_, useH, textColor);
		};
		drawCenteredLine("バネを同じ種類にして、カーソルを合わせて回収しよう", baseY);
		drawCenteredLine("一度跳ねたら回収不可（Qで巻き戻せば回収可能）", baseY + hintH + 10.0f);
		DrawSpringCollectLabel();
		break;
	}
	case StageTutorialStep::ScrollWheel: {
		drawCenteredTop("マウスホイールで画面を拡大・縮小できるよ");
		break;
	}
	case StageTutorialStep::ShowGoal: {
		if (tutorialGoalVisibleFrames_ > 0) {
			const KamataEngine::Vector2 goalScreen = ConvertWorldToScreen(tutorialGoalWorldPos_.x, tutorialGoalWorldPos_.y);
			drawTextLeftOf("青いところがゴールです", goalScreen.x - 20.0f, goalScreen.y);
		} else {
			drawCenteredTop("マウスホイールで画面を拡大・縮小できるよ");
		}
		break;
	}
	case StageTutorialStep::PressR: {
		drawCenteredTop("Rを押して、最初からクリアに挑戦しよう！");
		break;
	}
	default:
		break;
	}

	// SpacePauseHint の着地案内は case 内の DrawTutorialTimedBanner で描画する
}

void GameScene::DrawSpringPreviewPlacementLabel() {
	if (!hasTrampolinePreview_ || !isGameIntroFinished_) {
		return;
	}

	const TrampolineSpringType springType = trampolinePreview_.GetType();
	const int remaining = GetRemainingSpringPlacementCount(springType);
	// 残数0（使い切り）でも ×0 を表示する。ステージ上限0の種類は選択自体しない

	int hudIndex = 0;
	switch (springType) {
	case TrampolineSpringType::Up:
		hudIndex = 0;
		break;
	case TrampolineSpringType::Down:
		hudIndex = 1;
		break;
	case TrampolineSpringType::Left:
		hudIndex = 2;
		break;
	case TrampolineSpringType::Right:
		hudIndex = 3;
		break;
	}

	SpringPlacementHudUi& ui = springPlacementHudUi_[static_cast<size_t>(hudIndex)];
	if (!ui.multiplySprite) {
		return;
	}

	float zoomScale = 1.0f;
	{
		float sl = 0.0f, sb = 0.0f, sr = 0.0f, st = 0.0f;
		tileMap_.GetScreenViewportBounds(currentScreenX_, currentScreenY_, sl, sb, sr, st);
		const float baseW = sr - sl;
		float vl = 0.0f, vb = 0.0f, vr = 0.0f, vt = 0.0f;
		ComputeCameraBounds(vl, vb, vr, vt);
		const float viewW = vr - vl;
		if (viewW > 0.0f && baseW > 0.0f) {
			zoomScale = baseW / viewW;
		}
	}

	const KamataEngine::Vector2 screenPos = ConvertWorldToScreen(trampolinePreviewPos_.x, trampolinePreviewPos_.y);
	const float labelH = kSpringPreviewLabelBaseH_ * zoomScale;
	const float multiplySize = labelH * kSpringHudMultiplyScale_;
	const float digitSize = labelH * kSpringHudDigitScale_ * kSpringPreviewDigitScaleBoost_;
	const float innerGap = 3.0f * zoomScale;
	const float digitGap = kSpringHudDigitSpacing_ * zoomScale;
	const float digitY = screenPos.y + kSpringPreviewDigitOffsetY_ * zoomScale;
	const float boldPx = kSpringPreviewLabelBoldPx_ * zoomScale;

	const int tens = remaining / 10;
	const int ones = remaining % 10;
	const KamataEngine::Vector4 outlineColor = {1.0f, 1.0f, 1.0f, 1.0f};
	const KamataEngine::Vector4 labelColor = {0.0f, 0.0f, 0.0f, 1.0f};

	const bool isVertical = (springType == TrampolineSpringType::Left || springType == TrampolineSpringType::Right);

	// 白縁＋黒本体をずらして重ね、太く読みやすくする
	auto drawSpriteBold = [&](KamataEngine::Sprite* sprite, float cx, float cy, float size,
	                          const KamataEngine::Vector2& anchor) {
		if (!sprite) {
			return;
		}
		sprite->SetAnchorPoint(anchor);
		sprite->SetSize({size, size});
		// 縁は上下左右のみ（斜めは付けない＝控えめな太さ）
		static const float kOffsets[4][2] = {{-1.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, -1.0f}, {0.0f, 1.0f}};
		sprite->SetColor(outlineColor);
		for (const float* o : kOffsets) {
			sprite->SetPosition({cx + o[0] * boldPx, cy + o[1] * boldPx});
			sprite->Draw();
		}
		sprite->SetColor(labelColor);
		sprite->SetPosition({cx, cy});
		sprite->Draw();
	};

	auto drawDigit = [&](KamataEngine::Sprite* sprite, int digit, float cx, float cy, const KamataEngine::Vector2& anchor) {
		if (!sprite || digit < 0 || digit > 9) {
			return;
		}
		const uint32_t handle = stageSelectDigitTextureHandles_[static_cast<size_t>(digit)] != 0
		                            ? stageSelectDigitTextureHandles_[static_cast<size_t>(digit)]
		                            : digitTextureHandles_[static_cast<size_t>(digit)];
		if (handle == 0) {
			return;
		}
		sprite->SetTextureHandle(handle);
		drawSpriteBold(sprite, cx, cy, digitSize, anchor);
	};

	if (isVertical) {
		// 左右バネは縦長なので ×N を縦に積む
		float contentH = multiplySize + innerGap + digitSize;
		if (tens > 0) {
			contentH += digitGap + digitSize;
		}
		float y = screenPos.y - contentH * 0.5f;

		drawSpriteBold(ui.multiplySprite, screenPos.x, y, multiplySize, {0.5f, 0.0f});
		y += multiplySize + innerGap;

		if (tens > 0) {
			drawDigit(ui.tensDigitSprite, tens, screenPos.x, y, {0.5f, 0.0f});
			y += digitSize + digitGap;
			drawDigit(ui.onesDigitSprite, ones, screenPos.x, y, {0.5f, 0.0f});
		} else {
			drawDigit(ui.onesDigitSprite, ones, screenPos.x, y, {0.5f, 0.0f});
		}
		return;
	}

	float contentW = multiplySize + innerGap + digitSize;
	if (tens > 0) {
		contentW += digitGap + digitSize;
	}

	float x = screenPos.x - contentW * 0.5f;
	const float multiplyY = screenPos.y;

	drawSpriteBold(ui.multiplySprite, x, multiplyY, multiplySize, {0.0f, 0.5f});
	x += multiplySize + innerGap;

	if (tens > 0) {
		drawDigit(ui.tensDigitSprite, tens, x, digitY, {0.0f, 0.5f});
		x += digitSize + digitGap;
		drawDigit(ui.onesDigitSprite, ones, x, digitY, {0.0f, 0.5f});
	} else {
		drawDigit(ui.onesDigitSprite, ones, x, digitY, {0.0f, 0.5f});
	}
}

void GameScene::DrawSpringCollectLabel() {
	if (!showSpringCollectLabel_ || !isGameIntroFinished_) {
		return;
	}

	float zoomScale = 1.0f;
	{
		float sl = 0.0f, sb = 0.0f, sr = 0.0f, st = 0.0f;
		tileMap_.GetScreenViewportBounds(currentScreenX_, currentScreenY_, sl, sb, sr, st);
		const float baseW = sr - sl;
		float vl = 0.0f, vb = 0.0f, vr = 0.0f, vt = 0.0f;
		ComputeCameraBounds(vl, vb, vr, vt);
		const float viewW = vr - vl;
		if (viewW > 0.0f && baseW > 0.0f) {
			zoomScale = baseW / viewW;
		}
	}

	const KamataEngine::Vector2 springScreen = ConvertWorldToScreen(springCollectLabelWorldPos_.x, springCollectLabelWorldPos_.y);

	// 縦バネ/横バネどちらもバネの真上（中心）に「回収する」を重ねる
	KamataEngine::Sprite* label = springCollectLabelIsVertical_ ? springCollectLabelVerticalSprite_ : springCollectLabelSprite_;
	if (!label) {
		return;
	}

	// バネの当たり判定サイズにぴったり重なるようにラベルサイズを合わせる
	const float labelW = springCollectLabelSpringHalfW_ * 2.0f * zoomScale;
	const float labelH = springCollectLabelSpringHalfH_ * 2.0f * zoomScale;

	label->SetAnchorPoint({0.5f, 0.5f});
	label->SetSize({labelW, labelH});
	label->SetPosition({springScreen.x, springScreen.y});
	label->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	label->Draw();
}

void GameScene::DrawTrampolineSprings() {
	if (!modelSpringUp_ || !isGameIntroFinished_) {
		return;
	}

	for (const TrampolineSpring& spring : trampolineSprings_) {
		KamataEngine::Model* springModel = GetSpringModel(spring.GetType());
		if (springModel) {
			spring.Draw(springModel, camera_);
		}
		if (modelSpringArrow_) {
			spring.DrawArrowMarkers(modelSpringArrow_, trampolineArrowAnimTime_, camera_);
		}
	}

	if (hasTrampolinePreview_) {
		KamataEngine::Model* previewModel = GetSpringModel(trampolinePreview_.GetType());
		if (previewModel) {
			trampolinePreview_.Draw(previewModel, camera_);
		}
		if (modelSpringArrow_) {
			trampolinePreview_.DrawArrowMarkers(modelSpringArrow_, trampolineArrowAnimTime_, camera_);
		}
	}
}

KamataEngine::Model* GameScene::GetSpringModel(TrampolineSpringType type) const {
	switch (type) {
	case TrampolineSpringType::Up:
		return modelSpringUp_;
	case TrampolineSpringType::Down:
		return modelSpringDown_;
	case TrampolineSpringType::Right:
		return modelSpringRight_;
	case TrampolineSpringType::Left:
		return modelSpringLeft_;
	}
	return modelSpringUp_;
}

void GameScene::DrawZoomOutMarginFill(float viewLeft, float viewBottom, float viewRight, float viewTop) {
	if ((tileMap_.GetScreenCountX() <= 1 && tileMap_.GetScreenCountY() <= 1) || cameraZoomOut_ <= 0.0f) {
		return;
	}

	float mapMinX = 0.0f;
	float mapMinY = 0.0f;
	float mapMaxX = 0.0f;
	float mapMaxY = 0.0f;
	tileMap_.GetMapWorldBounds(mapMinX, mapMinY, mapMaxX, mapMaxY);

	const float viewW = viewRight - viewLeft;
	const float viewH = viewTop - viewBottom;
	if (viewW <= 0.0f || viewH <= 0.0f) {
		return;
	}

	const float winW = static_cast<float>(WinApp::kWindowWidth);
	const float winH = static_cast<float>(WinApp::kWindowHeight);

	// 各余白は専用スプライトで描画する（1個を使い回すと定数バッファ共有で最後の矩形だけになる）
	int spriteIndex = 0;
	auto drawScreenRect = [&](float worldLeft, float worldRight, float worldBottom, float worldTop) {
		if (spriteIndex >= static_cast<int>(zoomMarginFillSprites_.size())) {
			return;
		}
		KamataEngine::Sprite* sprite = zoomMarginFillSprites_[static_cast<size_t>(spriteIndex)];
		if (!sprite) {
			return;
		}

		const float screenX = (worldLeft - viewLeft) / viewW * winW;
		const float screenY = (viewTop - worldTop) / viewH * winH;
		const float screenW = (worldRight - worldLeft) / viewW * winW;
		const float screenH = (worldTop - worldBottom) / viewH * winH;
		if (screenW <= 0.5f || screenH <= 0.5f) {
			return;
		}

		sprite->SetColor({0.0f, 0.0f, 0.0f, 1.0f});
		sprite->SetPosition({screenX, screenY});
		sprite->SetSize({screenW, screenH});
		sprite->Draw();
		++spriteIndex;
	};
	auto drawPixelRect = [&](float screenX, float screenY, float screenW, float screenH) {
		if (spriteIndex >= static_cast<int>(zoomMarginFillSprites_.size())) {
			return;
		}
		KamataEngine::Sprite* sprite = zoomMarginFillSprites_[static_cast<size_t>(spriteIndex)];
		if (!sprite) {
			return;
		}
		if (screenW <= 0.5f || screenH <= 0.5f) {
			return;
		}

		sprite->SetColor({0.0f, 0.0f, 0.0f, 1.0f});
		sprite->SetPosition({screenX, screenY});
		sprite->SetSize({screenW, screenH});
		sprite->Draw();
		++spriteIndex;
	};

	if (viewLeft < mapMinX) {
		drawScreenRect(viewLeft, mapMinX, viewBottom, viewTop);
	}
	if (viewRight > mapMaxX) {
		drawScreenRect(mapMaxX, viewRight, viewBottom, viewTop);
	}
	if (viewBottom < mapMinY) {
		drawScreenRect(mapMinX, mapMaxX, viewBottom, mapMinY);
	}
	if (viewTop > mapMaxY) {
		drawScreenRect(mapMinX, mapMaxX, mapMaxY, viewTop);
	}

	// マップが画面いっぱいに収まらないときのレターボックスも黒で埋める
	const float mapScreenLeft = (mapMinX - viewLeft) / viewW * winW;
	const float mapScreenRight = (mapMaxX - viewLeft) / viewW * winW;
	const float mapScreenTop = (viewTop - mapMaxY) / viewH * winH;
	const float mapScreenBottom = (viewTop - mapMinY) / viewH * winH;
	if (mapScreenLeft > 0.5f) {
		drawPixelRect(0.0f, 0.0f, mapScreenLeft, winH);
	}
	if (mapScreenRight < winW - 0.5f) {
		drawPixelRect(mapScreenRight, 0.0f, winW - mapScreenRight, winH);
	}
	if (mapScreenTop > 0.5f) {
		drawPixelRect(0.0f, 0.0f, winW, mapScreenTop);
	}
	if (mapScreenBottom < winH - 0.5f) {
		drawPixelRect(0.0f, mapScreenBottom, winW, winH - mapScreenBottom);
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
	camera_.matProjection = MakeOrthographicMatrix(left, top, right, bottom, -250.0f, 250.0f);
	camera_.TransferMatrix();
}

bool GameScene::CanCameraZoomOut() const {
	if (tileMap_.GetScreenCountX() <= 1 && tileMap_.GetScreenCountY() <= 1) {
		return false;
	}

	float screenLeft = 0.0f;
	float screenBottom = 0.0f;
	float screenRight = 0.0f;
	float screenTop = 0.0f;
	tileMap_.GetScreenViewportBounds(currentScreenX_, currentScreenY_, screenLeft, screenBottom, screenRight, screenTop);
	const float baseW = screenRight - screenLeft;
	const float baseH = screenTop - screenBottom;

	float mapMinX = 0.0f;
	float mapMinY = 0.0f;
	float mapMaxX = 0.0f;
	float mapMaxY = 0.0f;
	tileMap_.GetMapWorldBounds(mapMinX, mapMinY, mapMaxX, mapMaxY);
	const float mapW = mapMaxX - mapMinX;
	const float mapH = mapMaxY - mapMinY;

	const float scaleToFitMapW = baseW > 0.0f ? mapW / baseW : 1.0f;
	const float scaleToFitMapH = baseH > 0.0f ? mapH / baseH : 1.0f;
	const float maxScale = (std::max)(scaleToFitMapW, scaleToFitMapH);
	return maxScale > 1.01f;
}

void GameScene::UpdateCameraControl() {
	if (!input_) {
		return;
	}

	const SceneStateKind stateKind = GetSceneStateKind();
	if (stateKind != SceneStateKind::Game && stateKind != SceneStateKind::GameIntro) {
		return;
	}

	if (!CanCameraZoomOut()) {
		isFreeCamera_ = false;
		cameraZoomOut_ = 0.0f;
		return;
	}

	// 誘導チュートリアル中は、ホイール案内のステップまでズーム操作を受け付けない
	if (IsStageTutorialActive() && stageTutorialStep_ < StageTutorialStep::ScrollWheel) {
		isFreeCamera_ = false;
		cameraZoomOut_ = 0.0f;
		return;
	}

	const int32_t wheel = input_->GetWheel();
	if (wheel != 0) {
		cameraZoomOut_ -= static_cast<float>(wheel) / 120.0f * 0.12f;
		cameraZoomOut_ = std::clamp(cameraZoomOut_, 0.0f, 1.0f);
	}
	isFreeCamera_ = cameraZoomOut_ > 0.0001f;
}

void GameScene::UpdatePlayerScreenTransition() {
	if (!player_) {
		return;
	}

	if (player_->IsSpikeInvulnerable()) {
		return;
	}

	if (tileMap_.GetScreenCountX() <= 1 && tileMap_.GetScreenCountY() <= 1) {
		return;
	}

	// 1枚のCSVで連続したワールド座標なので、プレイヤーはテレポートしない。
	// いる座標から画面番号だけを更新し、カメラ追従を切り替える。
	const KamataEngine::Vector3 pos = player_->GetLocalPosition();
	int screenX = 0;
	int screenY = 0;
	tileMap_.GetScreenFromWorld(pos.x, pos.y, screenX, screenY);

	if (screenX == currentScreenX_ && screenY == currentScreenY_) {
		return;
	}

	currentScreenX_ = screenX;
	currentScreenY_ = screenY;
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

bool GameScene::IsScreenPointInSprite(const KamataEngine::Sprite* sprite, float screenX, float screenY) const {
	if (!sprite) {
		return false;
	}
	const KamataEngine::Vector2& pos = sprite->GetPosition();
	const KamataEngine::Vector2& size = sprite->GetSize();
	const KamataEngine::Vector2& anchor = sprite->GetAnchorPoint();
	const float left = pos.x - size.x * anchor.x;
	const float top = pos.y - size.y * anchor.y;
	const float right = left + size.x;
	const float bottom = top + size.y;
	return screenX >= left && screenX <= right && screenY >= top && screenY <= bottom;
}

int GameScene::HitTestStageSelectSlot(float screenX, float screenY) const {
	for (int i = 0; i < kStageCount; ++i) {
		const StageSelectSlot& slot = stageSelectSlots_[static_cast<size_t>(i)];
		if (screenX >= slot.centerX - slot.halfW && screenX <= slot.centerX + slot.halfW &&
		    screenY >= slot.centerY - slot.halfH && screenY <= slot.centerY + slot.halfH) {
			return i;
		}
	}
	return -1;
}

void GameScene::MoveStageSelectFocus(int deltaCol, int deltaRow) {
	int col = focusedStageSelectIndex_ % kStageSelectColsPerRow;
	int row = focusedStageSelectIndex_ / kStageSelectColsPerRow;
	col = std::clamp(col + deltaCol, 0, kStageSelectColsPerRow - 1);
	row = std::clamp(row + deltaRow, 0, (kStageCount - 1) / kStageSelectColsPerRow);
	focusedStageSelectIndex_ = row * kStageSelectColsPerRow + col;
	focusedStageSelectIndex_ = std::clamp(focusedStageSelectIndex_, 0, kStageCount - 1);
}

void GameScene::DrawStageSelectUi() {
	if (!isStageSelectFontReady_) {
		return;
	}

	static constexpr float kFocusedScale = 1.15f;

	if (stageSelectCursorSprite_ && focusedStageSelectIndex_ >= 0 && focusedStageSelectIndex_ < kStageCount) {
		const StageSelectSlot& focusedSlot = stageSelectSlots_[static_cast<size_t>(focusedStageSelectIndex_)];
		stageSelectCursorSprite_->SetPosition({focusedSlot.centerX, focusedSlot.centerY});
		stageSelectCursorSprite_->SetSize({focusedSlot.halfW * 2.0f, focusedSlot.halfH * 2.0f});
		stageSelectCursorSprite_->Draw();
	}

	for (int i = 0; i < kStageCount; ++i) {
		const StageSelectSlot& slot = stageSelectSlots_[static_cast<size_t>(i)];
		const StageSelectSlotUi& ui = stageSelectSlotUi_[static_cast<size_t>(i)];
		if (ui.digitCount <= 0) {
			continue;
		}

		const bool isFocused = (i == focusedStageSelectIndex_);
		const float scale = isFocused ? kFocusedScale : 1.0f;
		const float digitSize = kStageSelectDigitSize * scale;
		const float digitSpacing = ui.digitCount >= 2 ? kStageSelectMultiDigitSpacing : 0.0f;
		const float totalWidth =
		    digitSize * static_cast<float>(ui.digitCount) + digitSpacing * static_cast<float>(ui.digitCount - 1);
		float cursorX = slot.centerX - totalWidth * 0.5f;

		for (int d = 0; d < ui.digitCount; ++d) {
			KamataEngine::Sprite* digitSprite = ui.digitSprites[d];
			if (!digitSprite) {
				continue;
			}

			digitSprite->SetSize({digitSize, digitSize});
			digitSprite->SetPosition({cursorX, slot.centerY});
			digitSprite->Draw();
			cursorX += digitSize + digitSpacing;
		}
	}
}

void GameScene::LayoutStageClearButtons() {
	static constexpr float kBtnW = 320.0f;
	static constexpr float kBtnH = 80.0f;
	static constexpr float kBtnGap = 48.0f;
	static constexpr float kBtnY = 320.0f;
	const float screenCenterX = static_cast<float>(WinApp::kWindowWidth) * 0.5f;

	if (HasNextStageAfterCurrent()) {
		const float totalW = kBtnW * 2.0f + kBtnGap;
		const float leftX = screenCenterX - totalW * 0.5f;
		if (stageClearTitleReturnSprite_) {
			stageClearTitleReturnSprite_->SetPosition({leftX, kBtnY});
			stageClearTitleReturnSprite_->SetSize({kBtnW, kBtnH});
		}
		if (stageClearNextStageSprite_) {
			stageClearNextStageSprite_->SetPosition({leftX + kBtnW + kBtnGap, kBtnY});
			stageClearNextStageSprite_->SetSize({kBtnW, kBtnH});
		}
	} else if (stageClearTitleReturnSprite_) {
		stageClearTitleReturnSprite_->SetPosition({screenCenterX - kBtnW * 0.5f, kBtnY});
		stageClearTitleReturnSprite_->SetSize({kBtnW, kBtnH});
	}
}

void GameScene::DrawStageClearUi() {
	LayoutStageClearButtons();
	if (stageClearTitleReturnSprite_) {
		stageClearTitleReturnSprite_->Draw();
	}
	if (HasNextStageAfterCurrent() && stageClearNextStageSprite_) {
		stageClearNextStageSprite_->Draw();
	}
}