#include "GaneScene.h"
#include "GameBullet.h"
#include "GameCharacter.h"
#include "3d/AxisIndicator.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <random>

namespace {
float PondSpawnRand01(int& seed) {
	seed = seed * 1103515245 + 12345;
	return static_cast<float>((seed / 65536) % 32768) / 32768.0f;
}
float EstimatePondExtent(const std::vector<WaterPond::PondPart>& parts) {
	float extent = 0.0f;
	for (const WaterPond::PondPart& part : parts) {
		const float e = std::sqrtf(part.offsetX * part.offsetX + part.offsetZ * part.offsetZ) + part.radius;
		if (e > extent) {
			extent = e;
		}
	}
	return extent;
}
} // namespace

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

GameScene::GameScene() { sceneState_ = SceneStateStart::Instance(); }

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
	delete modelPlayer_;
	delete modelEnemy_;
	delete modelGoal_;
	delete airShotCountSprite_;
	delete modelSkydome_;
	delete modelZimenn_;
	delete modelMeteorite_;
	delete modelGround_; // ゴルフ用の地面モデルを解放
	delete modelEnemyBullet_; // 敵弾モデルを解放（追加）
	for (Meteorite* meteor : meteorites_) {
		delete meteor;
	}
	delete player_;
	delete skydome_;
	delete zimenn_;
	delete railCamera_;
	delete reticleSprite_;
	delete transitionSprite_;
	delete titleScreenSprite_;
	delete wasdSprite_;
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
	delete gameOverSprite_;
	for (KamataEngine::Sprite* sprite : minimapEnemySprites_) {
		delete sprite;
	}
	minimapEnemySprites_.clear();
	for (KamataEngine::Sprite* sprite : minimapPondSprites_) {
		delete sprite;
	}
	minimapPondSprites_.clear();
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
	for (WaterPond* pond : waterPonds_) {
		delete pond;
	}
	waterPonds_.clear();
	delete modelWaterPond_;

	// delete score digit sprites
	for (KamataEngine::Sprite* s : scoreDigitSprites_) {
		delete s;
	}

	// ゴルフ: ゲージスプライト解放
	delete gageSprite_;
	delete barSprite_;
}

void GameScene::Initialize() {
	dxCommon_ = DirectXCommon::GetInstance();
	input_ = Input::GetInstance();
	audio_ = Audio::GetInstance();

	player_ = new Player();
	skydome_ = new Skydome();
	zimenn_ = new Skydome();

	modelPlayer_ = KamataEngine::Model::CreateFromOBJ("goruhu", true);
	modelEnemy_ = KamataEngine::Model::CreateFromOBJ("boat", true);
	// ゴール専用モデル: ボールと同じ OBJ を使ってサイズ・当たり判定を一致させる
	// ゴール専用モデル: ボールと同じ cube を使用して当たり判定を可視化
	modelGoal_ = KamataEngine::Model::CreateFromOBJ("cube", true);
	modelSkydome_ = Model::CreateFromOBJ("skydomesora", true);
	modelZimenn_ = Model::CreateFromOBJ("zimenn", true);

	// 敵弾用のOBJモデルを読み込む（ファイル名: Resources/bulletEnemy.obj を想定）
	modelEnemyBullet_ = KamataEngine::Model::CreateFromOBJ("bulletEnemy", true);

	modelMeteorite_ = KamataEngine::Model::CreateFromOBJ("meteorite", true);
	meteoriteSpawnTimer_ = 0;

	// ゴルフ用: 地面モデル（cube を横長・薄く変形して使う）
	modelGround_ = KamataEngine::Model::CreateFromOBJ("cube", true);

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

	titleScreenTextureHandle_ = KamataEngine::TextureManager::Load("taitoru2.png");
	titleScreenSprite_ = KamataEngine::Sprite::Create(titleScreenTextureHandle_, {0, 0});
	if (titleScreenSprite_) {
		titleScreenSprite_->SetAnchorPoint({0.5f, 0.5f});
		titleScreenSprite_->SetPosition(screenCenter);
		titleScreenSprite_->SetSize({(float)WinApp::kWindowWidth, (float)WinApp::kWindowHeight});
	}

	wasdTextureHandle_ = KamataEngine::TextureManager::Load("WASD.png");
	wasdSprite_ = KamataEngine::Sprite::Create(wasdTextureHandle_, {0, 0});
	if (wasdSprite_) {
		wasdSprite_->SetAnchorPoint({0.5f, 0.5f});
		wasdSprite_->SetPosition(screenCenter);
		wasdSprite_->SetSize({(float)WinApp::kWindowWidth, (float)WinApp::kWindowHeight});
	}

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

	gameOverTextureHandle_ = KamataEngine::TextureManager::Load("GameOver.png");
	gameOverSprite_ = KamataEngine::Sprite::Create(gameOverTextureHandle_, {0, 0});
	if (gameOverSprite_) {
		gameOverSprite_->SetAnchorPoint({0.5f, 0.5f});
		gameOverSprite_->SetPosition(screenCenter);
		gameOverSprite_->SetSize({(float)WinApp::kWindowWidth, (float)WinApp::kWindowHeight});
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

	minimapTextureHandle_ = KamataEngine::TextureManager::Load("mini.png");
	greenBoxTextureHandle_ = KamataEngine::TextureManager::Load("greenBox.png");
	minimapPondTextureHandle_ = KamataEngine::TextureManager::Load("aoCircle.png");
	minimapPlayerTextureHandle_ = KamataEngine::TextureManager::Load("player.png");
	// ミニマップ上の敵弾アイコンは元の赤いテクスチャを使用（変更を取り消し）
	minimapEnemyBulletTextureHandle_ = KamataEngine::TextureManager::Load("missileRedBox.png");

	// 1. ミニマップ背景
	minimapSprite_ = KamataEngine::Sprite::Create(minimapTextureHandle_, {0, 0});
	minimapSprite_->SetPosition(kMinimapPosition_);
	minimapSprite_->SetAnchorPoint({0.0f, 0.0f}); // 左上をアンカーに（ゴルフ: 左上配置）
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

	// 空中打ち直し残り回数（数字テクスチャを使用）
	{
		uint32_t defaultDigit = digitTextureHandles_[4] != 0 ? digitTextureHandles_[4] : digitTextureHandles_[1];
		airShotCountSprite_ = KamataEngine::Sprite::Create(defaultDigit, {-200.0f, -200.0f});
		if (airShotCountSprite_) {
			airShotCountSprite_->SetAnchorPoint({0.5f, 0.5f});
			airShotCountSprite_->SetSize({64.0f, 64.0f});
		}
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

	// ゴルフ: プレイヤーを地面（Y=0）に直接配置（空中から落下させない）
	playerIntroTargetPosition_ = {0.0f, 0.0f, 0.0f};
	playerIntroStartPosition_ = playerIntroTargetPosition_;

	player_->Initialize(modelPlayer_, &camera_, playerIntroStartPosition_);
	// ゴルフ: 地面のワールドY を設定（ボールはこれより下に行かない）
	player_->SetGroundY(kGroundLocalY_);
	// Initialize last player position for minimap rotation tracking
	lastPlayerPos_ = player_->GetWorldPosition();

	skydome_->Initialize(modelSkydome_, &camera_);
	zimenn_->Initialize(modelZimenn_, &camera_);
	KamataEngine::AxisIndicator::GetInstance()->SetVisible(true);

	railCamera_ = new RailCamera();
	railCamera_->Initialize(railcameraPos, railcameraRad);
	// ゴルフ: ボール追従カメラモードに設定
	railCamera_->SetGolfChaseMode(true);
	// ゴルフ: プレイヤーはワールド座標で動かすので親なし
	// （カメラ親を付けると循環依存になるため）
	player_->SetParent(nullptr);
	railCamera_->SetTarget(player_);
	cameraPositionAnchor_.Initialize();
	player_->SetRailCamera(railCamera_);
	player_->SetEnemies(&enemies_);

	// ゴルフ用: 地面の見た目を設定（ワールド座標）。
	// 地面をワールド座標で Z 方向に長く配置する。
	// 地面の上面が Y=0 になるよう Y=-0.5 に配置（cubeは1ユニット厚なので）
	// kGroundLengthZ_ でコース長を調整、kGroundWidth_ で横幅を調整。
	groundTransform_.Initialize();
	groundTransform_.translation_ = {
		0.0f,
		-10.0f,                       // 上面が Y=0 になるよう半分だけ沈める
		kGroundLengthZ_ * 0.5f       // コース全体の中心（Z: 0〜kGroundLengthZ_）
	};
	groundTransform_.scale_ = {kGroundWidth_, 1.0f, kGroundLengthZ_};
	groundTransform_.UpdateMatrix();

	// ゴルフ: 池（落ちるとゲームオーバー）
	modelWaterPond_ = KamataEngine::Model::CreateFromOBJ("ike", true);

	boundaryWallColor_.Initialize();
	boundaryWallColor_.SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	boundaryWallTransform_.Initialize();
	playAreaCenter_ = {0.0f, 10.0f, 1200.0f};
	player_->SetPlayAreaRadius(kPlayAreaRadius_);

	LoadEnemyPopData();
	hitSoundHandle_ = audio_->LoadWave("./sound/parry.wav");

	// ホーミング弾生成タイマー初期化
	homingSpawnTimer_ = kHomingIntervalFrames_; // 最初のショットが間隔後に発生するようタイマー初期化

	// ミニマップ用テクスチャ等の初期化を行った後に、右/左キー表示用スプライトを初期化
	// テクスチャ名は Resources に配置した "light.png" と "left.png" を想定
	lightTextureHandle_ = KamataEngine::TextureManager::Load("light.png");
	leftTextureHandle_ = KamataEngine::TextureManager::Load("left.png");
	shiftTextureHandle_ = KamataEngine::TextureManager::Load("shift.png"); // Shift画像

	// スプライト生成
	lightSprite_ = KamataEngine::Sprite::Create(lightTextureHandle_, {0, 0});
	leftSprite_ = KamataEngine::Sprite::Create(leftTextureHandle_, {0, 0});
	shiftSprite_ = KamataEngine::Sprite::Create(shiftTextureHandle_, {0, 0});

	if (lightSprite_) {
		// グループオフセットを適用して右にずらす
		float groupX = static_cast<float>(WinApp::kWindowWidth) - 2.0f * 80.0f + controlGroupOffset_;
		lightSprite_->SetAnchorPoint({1.0f, 1.0f});
		lightSprite_->SetSize({80.0f, 80.0f});
		lightSprite_->SetPosition({groupX, (float)WinApp::kWindowHeight - 20.0f});
		lightSprite_->SetColor({1.0f, 1.0f, 1.0f, 0.5f});
	}
	if (leftSprite_) {
		float groupX = static_cast<float>(WinApp::kWindowWidth) - 3.0f * 80.0f - 16.0f + controlGroupOffset_;
		leftSprite_->SetAnchorPoint({1.0f, 1.0f});
		leftSprite_->SetSize({80.0f, 80.0f});
		leftSprite_->SetPosition({groupX, (float)WinApp::kWindowHeight - 20.0f});
		leftSprite_->SetColor({1.0f, 1.0f, 1.0f, 0.5f});
	}
	if (shiftSprite_) {
		shiftSprite_->SetAnchorPoint({1.0f, 1.0f});
		// 横長: 幅1.5倍, 高さは矢印基準サイズ
		shiftSprite_->SetSize({80.0f * 1.5f, 80.0f});
		// 初期配置: light の右側に少しずらして上に置く（グループオフセット適用）
		float controlSizeInit = 80.0f;
		float verticalGapInit = 8.0f;
		float lightRightXInit = static_cast<float>(WinApp::kWindowWidth) - 2.0f * controlSizeInit + controlGroupOffset_;
		float shiftRightXInit = lightRightXInit + controlSizeInit + shiftExtraRight_;
		float shiftBottomYInit = static_cast<float>(WinApp::kWindowHeight) - 20.0f - controlSizeInit - verticalGapInit - shiftExtraUp_;
		shiftSprite_->SetPosition({shiftRightXInit, shiftBottomYInit});
		shiftSprite_->SetColor({1.0f, 1.0f, 1.0f, 0.5f});
	}

	// スプライトの初期位置を右下に設定（毎フレームの更新時に再計算されるため、ここではウィンドウサイズ依存の初期位置のみ設定）
	if (lightSprite_) {
		float groupX = static_cast<float>(WinApp::kWindowWidth) - 2.0f * 80.0f + controlGroupOffset_;
		lightSprite_->SetPosition({groupX, (float)WinApp::kWindowHeight - 20.0f});
	}
	if (leftSprite_) {
		float groupX = static_cast<float>(WinApp::kWindowWidth) - 3.0f * 80.0f - 16.0f + controlGroupOffset_;
		leftSprite_->SetPosition({groupX, (float)WinApp::kWindowHeight - 20.0f});
	}
	if (shiftSprite_) {
		float controlSizeInit = 80.0f;
		float verticalGapInit = 8.0f;
		float lightRightXInit = static_cast<float>(WinApp::kWindowWidth) - 2.0f * controlSizeInit + controlGroupOffset_;
		float shiftRightXInit = lightRightXInit + controlSizeInit + shiftExtraRight_;
		float shiftBottomYInit = static_cast<float>(WinApp::kWindowHeight) - 20.0f - controlSizeInit - verticalGapInit - shiftExtraUp_;
		shiftSprite_->SetPosition({shiftRightXInit, shiftBottomYInit});
	}

	// ゴルフ: パワーゲージスプライトを初期化（"gage.png" / "bar.png" を Resources に置く）
	gageTH_ = KamataEngine::TextureManager::Load("gage.png");
	barTH_  = KamataEngine::TextureManager::Load("bar.png");
	gageSprite_ = KamataEngine::Sprite::Create(gageTH_, {kGaugePosX_, kGaugePosY_});
	if (gageSprite_) {
		gageSprite_->SetAnchorPoint({0.5f, 0.0f}); // 上中央アンカー
		gageSprite_->SetSize({kGaugeWidth_, kGaugeHeight_});
	}
	barSprite_ = KamataEngine::Sprite::Create(barTH_, {kGaugePosX_, kGaugePosY_});
	if (barSprite_) {
		barSprite_->SetAnchorPoint({0.5f, 0.5f}); // 中央アンカー
		barSprite_->SetSize({kBarWidth_, kBarHeight_});
	}
}

void GameScene::Update() {

	// 天球・地面モデルをボールの XZ に追従（Y は固定）
	if (player_) {
		KamataEngine::Vector3 ballPos = player_->GetWorldPosition();
		if (skydome_) {
			skydome_->SetPositionXZ(ballPos.x, ballPos.z);
		}
		if (zimenn_) {
			zimenn_->SetPositionXZ(ballPos.x, ballPos.z);
		}
	}
	if (skydome_) {
		skydome_->Update();
	}
	if (zimenn_) {
		zimenn_->Update();
	}

	// 右／左キーの押下状態に応じてスプライトの明るさを切替
	if (input_) {
		bool rightPressed = input_->PushKey(DIK_RIGHT);
		bool leftPressed = input_->PushKey(DIK_LEFT);

		if (lightSprite_) {
			if (rightPressed) {
				// 明るく表示
				lightSprite_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
			} else {
				// 非押下はやや半透明
				lightSprite_->SetColor({1.0f, 1.0f, 1.0f, 0.5f});
			}
		}
		if (leftSprite_) {
			if (leftPressed) {
				leftSprite_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
			} else {
				leftSprite_->SetColor({1.0f, 1.0f, 1.0f, 0.5f});
			}
		}
	}

	// 常に右下に表示するため、毎フレーム位置を再設定する（ウィンドウリサイズ対応）
	float margin = 20.0f;
	float gap = 16.0f; // スプライト間のギャップ

	// 実際のサイズを取得（スプライトが無ければ基準値を使う）
	float controlSize = 80.0f;
	//float shiftW = shiftSprite_ ? shiftSprite_->GetSize().x : controlSize * 1.5f;
	//float lightW = lightSprite_ ? lightSprite_->GetSize().x : controlSize;
	//float leftW = leftSprite_ ? leftSprite_->GetSize().x : controlSize;

	float bottomY = static_cast<float>(WinApp::kWindowHeight) - margin; // 下辺の位置

	// 矢印（light/left）は下端に横並びで配置
	float lightRightX = static_cast<float>(WinApp::kWindowWidth) - 2.0f * controlSize + controlGroupOffset_; // グループオフセット適用
	float leftRightX = static_cast<float>(WinApp::kWindowWidth) - 3.0f * controlSize - gap + controlGroupOffset_;  // グループオフセット適用

	if (lightSprite_) {
		lightSprite_->SetAnchorPoint({1.0f, 1.0f});
		lightSprite_->SetSize({controlSize, controlSize});
		lightSprite_->SetPosition({lightRightX, bottomY});
	}
	if (leftSprite_) {
		leftSprite_->SetAnchorPoint({1.0f, 1.0f});
		leftSprite_->SetSize({controlSize, controlSize});
		leftSprite_->SetPosition({leftRightX, bottomY});
	}

	// Shift は light の上に表示する
	float verticalGap = 8.0f; // 縦方向の隙間
	if (shiftSprite_) {
		shiftSprite_->SetAnchorPoint({1.0f, 1.0f});
		shiftSprite_->SetSize({controlSize * 1.5f, controlSize});
		// 矢印スプライト一個分右にずらす + クラスメンバで追加オフセット
		float shiftRightX = lightRightX + controlSize + shiftExtraRight_;
		float shiftBottomY = bottomY - controlSize - verticalGap - shiftExtraUp_; // light の上に配置
		shiftSprite_->SetPosition({shiftRightX, shiftBottomY});
	}

	// Shift の表示（Shift 押下時は点滅）
	if (shiftSprite_ && input_) {
		bool shiftPressed = input_->PushKey(DIK_RSHIFT) || input_->PushKey(DIK_LSHIFT);
		bool aPressed = input_->PushKey(DIK_A);
		bool dPressed = input_->PushKey(DIK_D);
		if (shiftPressed && (aPressed || dPressed)) {
			shiftBlinkTimer_++;
			const int blinkPeriod = 8;
			bool visible = ((shiftBlinkTimer_ / blinkPeriod) % 2) == 0;
			float alpha = visible ? 1.0f : 0.3f;
			shiftSprite_->SetColor({1.0f, 1.0f, 1.0f, alpha});
		} else {
			shiftBlinkTimer_ = 0;
			shiftSprite_->SetColor({1.0f, 1.0f, 1.0f, 0.5f});
		}
	}

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
		// タイトルは 2D スプライト（3D モデルは使わない）
	} else if (GetSceneStateKind() == SceneStateKind::GameIntro || GetSceneStateKind() == SceneStateKind::Game || GetSceneStateKind() == SceneStateKind::TransitionFromGame || GetSceneStateKind() == SceneStateKind::Over) {

		// スカイドーム・地面を描画
		if (skydome_) { skydome_->Draw(); }
		if (zimenn_) { zimenn_->Draw(); }

		// ゴール（敵）と池を先に描画 → 半透明ボールの下に見えるようにする
		if (GetSceneStateKind() == SceneStateKind::Game && isGameIntroFinished_) {
			for (Enemy* enemy : enemies_) {
				if (enemy && !enemy->IsDead()) {
					enemy->Draw(camera_);
				}
			}
			const KamataEngine::Vector3 camPos = railCamera_->GetWorldTransform().translation_;
			struct PondDrawEntry {
				float distSq;
				WaterPond* pond;
			};
			std::vector<PondDrawEntry> pondDrawList;
			pondDrawList.reserve(waterPonds_.size());
			for (WaterPond* pond : waterPonds_) {
				if (!pond) {
					continue;
				}
				const KamataEngine::Vector3 pondCenter = pond->GetCenter();
				const float cdx = pondCenter.x - camPos.x;
				const float cdy = pondCenter.y - camPos.y;
				const float cdz = pondCenter.z - camPos.z;
				pondDrawList.push_back({cdx * cdx + cdy * cdy + cdz * cdz, pond});
			}
			for (size_t i = 0; i < pondDrawList.size(); ++i) {
				for (size_t j = i + 1; j < pondDrawList.size(); ++j) {
					if (pondDrawList[j].distSq > pondDrawList[i].distSq) {
						PondDrawEntry tmp = pondDrawList[i];
						pondDrawList[i] = pondDrawList[j];
						pondDrawList[j] = tmp;
					}
				}
			}
			for (const PondDrawEntry& entry : pondDrawList) {
				entry.pond->Draw(camera_);
			}
			DrawBoundaryWalls();
		}

		if (player_ && GetSceneStateKind() != SceneStateKind::Over) {
			player_->Draw();
		}
		// ゴルフ: 地面モデルはユーザーが天球で差し替えるため非表示
		// if (modelGround_) { modelGround_->Draw(groundTransform_, camera_); }

		if (explosionEmitter_) {
			explosionEmitter_->Draw(camera_);
		}
	} else if (GetSceneStateKind() == SceneStateKind::Clear) {
		// draw skydome / ground so background exists
		if (skydome_) { skydome_->Draw(); }
		if (zimenn_) { zimenn_->Draw(); }
		// draw any particles for clear
		if (clearEmitter_) {
			clearEmitter_->Draw(camera_);
		}
	}

	KamataEngine::Model::PostDraw();

	KamataEngine::Sprite::PreDraw(commandList);

	if (GetSceneStateKind() == SceneStateKind::Start || GetSceneStateKind() == SceneStateKind::TransitionToGame) {
		if (titleScreenSprite_) {
			titleScreenSprite_->Draw();
		}
	}

	if (GetSceneStateKind() == SceneStateKind::TransitionToGame || GetSceneStateKind() == SceneStateKind::TransitionFromGame) {
		transitionSprite_->Draw();
	}

	// ゴルフ: レティクルは使わないので描画しない
	// if (GetSceneStateKind() == SceneStateKind::GameIntro || GetSceneStateKind() == SceneStateKind::Game) {
	//     if (reticleSprite_) reticleSprite_->Draw();
	// }

	// ミニマップと矢印キー表示はゲームシーンのみ表示
	if (GetSceneStateKind() == SceneStateKind::Game && isGameIntroFinished_) {
		if (minimapSprite_) {
			minimapSprite_->Draw(); // 背景
		}
		for (KamataEngine::Sprite* sprite : minimapPondSprites_) {
			if (sprite) {
				sprite->Draw();
			}
		}
		for (KamataEngine::Sprite* sprite : minimapEnemyBulletSprites_) {
			if (sprite) {
				sprite->Draw();
			}
		}
		if (minimapPlayerSprite_) {
			minimapPlayerSprite_->Draw();
		}
		// ゴールは池などより手前（最前面）
		for (KamataEngine::Sprite* sprite : minimapEnemySprites_) {
			if (sprite) {
				sprite->Draw();
			}
		}

		// 右下操作ガイドスプライト: ゴルフゲームでは不要のため描画しない
		// (lightSprite_, leftSprite_, shiftSprite_)

		if (wasdSprite_) {
			wasdSprite_->Draw();
		}
	}

	// 飛距離（スコア）を右上に描画
	for (KamataEngine::Sprite* s : scoreDigitSprites_) {
		if (s) s->Draw();
	}

	// ゴルフ: パワーゲージをプレイヤーがゲージ中のみ描画
	// ゲージはボールのスクリーン座標の右横に配置する
	if (player_ && player_->IsGauging() &&
	    GetSceneStateKind() == SceneStateKind::Game && isGameIntroFinished_) {

		// ボールのワールド座標 → NDC → スクリーン座標
		KamataEngine::Vector3 ndc = ProjectToNDC(player_->GetWorldPosition());
		float gaugeX = kGaugePosX_;  // デフォルト値（NDC変換失敗時）
		float gaugeY = kGaugePosY_;
		if (ndc.z >= 0.0f) {
			// NDC [-1,1] → スクリーン座標 [0, W/H]
			float bsx = (ndc.x + 1.0f) * 0.5f * static_cast<float>(WinApp::kWindowWidth);
			float bsy = (1.0f - ndc.y) * 0.5f * static_cast<float>(WinApp::kWindowHeight);
			gaugeX = bsx - kGaugeWidth_ - 220.0f; // ボールの左隣（余白を広げてパター・矢印と重ならないよう）
			gaugeY = bsy - kGaugeHeight_ * 0.5f;  // 縦中央をボールに合わせる
		}

		// 外枠
		if (gageSprite_) {
			gageSprite_->SetPosition({gaugeX, gaugeY});
			gageSprite_->Draw();
		}
		// バー: power=1.0 → 枠の上端、power=0.0 → 枠の下端
		if (barSprite_) {
			float power      = player_->GetGaugePower();
			float barCenterY = gaugeY + (1.0f - power) * kGaugeHeight_;
			barSprite_->SetPosition({gaugeX, barCenterY});
			barSprite_->Draw();
		}
	}

	// 空中打ち直し残り回数: 打った瞬間から着地するまでボール中心に表示
	if (player_ && airShotCountSprite_ &&
	    GetSceneStateKind() == SceneStateKind::Game && isGameIntroFinished_) {
		bool showCounter = player_->IsFlying() || player_->IsAirAiming();
		int remaining    = player_->GetAirShotsRemaining();
		if (showCounter && remaining >= 0 && remaining <= 9) {
			uint32_t handle = (remaining < (int)digitTextureHandles_.size()) ? digitTextureHandles_[remaining] : 0;
			if (handle != 0) {
				const KamataEngine::Vector3 ballPos = player_->GetWorldPosition();
				KamataEngine::Vector3 ndc = ProjectToNDC(ballPos);
				if (ndc.z >= 0.0f) {
					// 標準64px。ホイールで引いたときだけ小さく（視点回転では変えない）
					float zoom = railCamera_ ? railCamera_->GetOrbitZoom() : 1.0f;
					if (zoom < 1.0f) {
						zoom = 1.0f;
					}
					float spriteSize = kAirShotBaseSize_ / zoom;

					float bsx = (ndc.x + 1.0f) * 0.5f * static_cast<float>(WinApp::kWindowWidth);
					float bsy = (1.0f - ndc.y) * 0.5f * static_cast<float>(WinApp::kWindowHeight);
					airShotCountSprite_->SetTextureHandle(handle);
					airShotCountSprite_->SetPosition({bsx, bsy});
					airShotCountSprite_->SetSize({spriteSize, spriteSize});
					airShotCountSprite_->Draw();
				}
			}
		}
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

	if (GetSceneStateKind() == SceneStateKind::Over) {
		if (gameOverSprite_) {
			gameOverSprite_->Draw();
		}
	}

	KamataEngine::Sprite::PostDraw();
}

void GameScene::AddEnemyBullet(EnemyBullet* bullet) {
	if (bullet)
		enemyBullets_.push_back(bullet);
}

void GameScene::SpawnWaterPonds(const Vector3& goalCenter) {
	for (WaterPond* pond : waterPonds_) {
		delete pond;
	}
	waterPonds_.clear();

	struct PlacedPond {
		float x = 0.0f;
		float z = 0.0f;
		float radius = 0.0f;
	};

	const float kMinCenterDist = 400.0f * (kPondGlobalScale_ / 10.0f);
	const int kTargetCount = 10;
	const float pondY = -0.40f;
	const float k2PI = 6.2831853f;

	std::vector<PlacedPond> placed;
	placed.reserve(static_cast<size_t>(kTargetCount));

	std::random_device rd;
	std::mt19937 rng(rd());
	const auto nowTicks = std::chrono::steady_clock::now().time_since_epoch().count();
	int seed = static_cast<int>(rng() ^ static_cast<uint32_t>(nowTicks) ^ static_cast<uint32_t>(nowTicks >> 32));
	seed ^= static_cast<int>(goalCenter.x * 73856093.0f + goalCenter.z * 19349663.0f);
	seed ^= currentStage_ * 83492791;
	if (seed == 0) {
		seed = 481516;
	}

	int pondIndex = 0;
	int attempts = 0;
	while (static_cast<int>(placed.size()) < kTargetCount && attempts < 600) {
		++attempts;

		const float angle = PondSpawnRand01(seed) * k2PI;
		const float dist = 150.0f + PondSpawnRand01(seed) * (kPondSpawnRadius_ - 150.0f);
		const float x = goalCenter.x + std::sinf(angle) * dist;
		const float z = goalCenter.z + std::cosf(angle) * dist;

		const float tierRoll = PondSpawnRand01(seed);
		float sizeMul = 1.0f;
		if (tierRoll < 0.10f) {
			sizeMul = 0.35f + PondSpawnRand01(seed) * 0.25f;
		} else if (tierRoll < 0.28f) {
			sizeMul = 0.60f + PondSpawnRand01(seed) * 0.35f;
		} else if (tierRoll < 0.52f) {
			sizeMul = 0.95f + PondSpawnRand01(seed) * 0.55f;
		} else if (tierRoll < 0.78f) {
			sizeMul = 1.55f + PondSpawnRand01(seed) * 0.95f;
		} else {
			sizeMul = 2.5f + PondSpawnRand01(seed) * 1.8f;
		}

		const int shapeSeed = static_cast<int>(PondSpawnRand01(seed) * 2000000000.0f) + pondIndex * 9973;
		std::vector<WaterPond::PondPart> parts = WaterPond::GenerateRandomShape(shapeSeed, sizeMul * kPondGlobalScale_);
		const float estRadius = EstimatePondExtent(parts);

		// ボール初期位置（ティー）に池がかからないよう除外
		const float spawnDx = x - playerIntroTargetPosition_.x;
		const float spawnDz = z - playerIntroTargetPosition_.z;
		constexpr float kSpawnClearanceMargin = 180.0f;
		const float spawnKeepOut = estRadius + kSpawnClearanceMargin;
		if (spawnDx * spawnDx + spawnDz * spawnDz < spawnKeepOut * spawnKeepOut) {
			continue;
		}

		const bool allowCluster = PondSpawnRand01(seed) < 0.50f;
		float minDist = kMinCenterDist;
		if (allowCluster) {
			minDist *= 0.06f;
		}

		bool tooClose = false;
		for (const PlacedPond& other : placed) {
			const float dx = x - other.x;
			const float dz = z - other.z;
			const float need = minDist + other.radius * 0.12f + estRadius * 0.12f;
			if (dx * dx + dz * dz < need * need) {
				tooClose = true;
				break;
			}
		}
		if (tooClose) {
			continue;
		}

		WaterPond* pond = new WaterPond();
		pond->Initialize(modelWaterPond_, {x, pondY, z}, parts, pondIndex);
		waterPonds_.push_back(pond);
		placed.push_back({x, z, estRadius});
		++pondIndex;
	}

	InitMinimapPondSprites();
}

void GameScene::InitMinimapPondSprites() {
	for (KamataEngine::Sprite* sprite : minimapPondSprites_) {
		delete sprite;
	}
	minimapPondSprites_.clear();

	size_t totalPonds = waterPonds_.size();
	minimapPondSprites_.resize(totalPonds);
	for (size_t i = 0; i < totalPonds; ++i) {
		KamataEngine::Sprite* sprite = KamataEngine::Sprite::Create(minimapPondTextureHandle_, {0, 0});
		if (sprite) {
			sprite->SetAnchorPoint({0.5f, 0.5f});
			sprite->SetSize({8.0f, 8.0f});
			sprite->SetColor({0.15f, 0.45f, 0.95f, 0.95f});
			sprite->SetPosition({-100.0f, -100.0f});
		}
		minimapPondSprites_[i] = sprite;
	}
}

void GameScene::UpdateMinimapPonds(const KamataEngine::Vector3& playerPos) {
	size_t spriteIndex = 0;
	for (WaterPond* pond : waterPonds_) {
		if (!pond) {
			continue;
		}
		if (spriteIndex >= minimapPondSprites_.size()) {
			break;
		}
		KamataEngine::Sprite* sprite = minimapPondSprites_[spriteIndex];
		if (!sprite) {
			++spriteIndex;
			continue;
		}
		const KamataEngine::Vector3 pondCenter = pond->GetCenter();
		if (!IsWithinMinimapWorldRange(pondCenter, playerPos)) {
			sprite->SetPosition({-100.0f, -100.0f});
			++spriteIndex;
			continue;
		}

		KamataEngine::Vector2 minimapPos = ConvertWorldToMinimap(pondCenter, playerPos);
		float dotSize = pond->GetBoundingRadius() * 2.0f * kMinimapScale_;
		if (dotSize < 2.0f) {
			dotSize = 2.0f;
		}
		CapMinimapMarkerSize(dotSize);
		if (IsMinimapMarkerFullyInside(minimapPos, dotSize)) {
			sprite->SetPosition(minimapPos);
			sprite->SetSize({dotSize, dotSize});
		} else {
			sprite->SetPosition({-100.0f, -100.0f});
		}
		++spriteIndex;
	}
	for (size_t i = spriteIndex; i < minimapPondSprites_.size(); ++i) {
		if (minimapPondSprites_[i]) {
			minimapPondSprites_[i]->SetPosition({-100.0f, -100.0f});
		}
	}
}

void GameScene::SetPlayAreaCenter(const Vector3& center) {
	playAreaCenter_ = center;
}

bool GameScene::IsBoundaryWallNear(const KamataEngine::Vector3& ball, float* outDistToWall) const {
	if (!player_) {
		return false;
	}

	const Vector3 center = player_->GetGoalPosition();
	const float dx = ball.x - center.x;
	const float dz = ball.z - center.z;
	const float dist = std::sqrtf(dx * dx + dz * dz);
	if (dist < 0.001f) {
		return false;
	}

	const float collisionRadius = kPlayAreaRadius_ - kBoundaryBallRadius_;
	const float distToWall = collisionRadius - dist;
	if (outDistToWall) {
		*outDistToWall = distToWall;
	}
	return distToWall <= kWallShowDistance_;
}

void GameScene::UpdateBoundaryWall() {
	boundaryWallVisible_ = false;
	if (!player_) {
		return;
	}

	const Vector3 ball = player_->GetWorldPosition();
	if (!IsBoundaryWallNear(ball, nullptr)) {
		return;
	}

	const Vector3 center = player_->GetGoalPosition();
	const float dx = ball.x - center.x;
	const float dz = ball.z - center.z;
	const float dist = std::sqrtf(dx * dx + dz * dz);
	if (dist < 0.001f) {
		return;
	}

	const float nx = dx / dist;
	const float nz = dz / dist;
	const float collisionRadius = kPlayAreaRadius_ - kBoundaryBallRadius_;
	const float wallCenterRadius = collisionRadius + kBoundaryWallThickness_ * 0.5f;

	boundaryWallVisible_ = true;
	boundaryWallYaw_ = std::atan2f(nx, nz);
	// XZは移動上限の円周上（跳ね返りライン）。Yだけボール高さに合わせる
	boundaryWallPos_ = {
	    center.x + nx * wallCenterRadius,
	    ball.y,
	    center.z + nz * wallCenterRadius
	};
}

void GameScene::DrawBoundaryWalls() {
	if (!boundaryWallVisible_ || !modelGround_) {
		return;
	}

	boundaryWallColor_.SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	boundaryWallTransform_.translation_ = boundaryWallPos_;
	boundaryWallTransform_.rotation_ = {0.0f, boundaryWallYaw_, 0.0f};
	boundaryWallTransform_.scale_ = {kBoundaryWallPanelSize_, kBoundaryWallPanelSize_, kBoundaryWallThickness_};
	boundaryWallTransform_.UpdateMatrix();
	modelGround_->Draw(boundaryWallTransform_, camera_, &boundaryWallColor_);
	boundaryWallTransform_.rotation_ = {0.0f, boundaryWallYaw_ + 3.14159265f, 0.0f};
	boundaryWallTransform_.UpdateMatrix();
	modelGround_->Draw(boundaryWallTransform_, camera_, &boundaryWallColor_);
}

void GameScene::SpawnGoalAt(const Vector3& worldPos) {
	Enemy* newEnemy = new Enemy();

	newEnemy->SetPlayer(player_);
	newEnemy->SetGameScene(this);
	newEnemy->SetCamera(&camera_);

	newEnemy->Initialize(modelGoal_ ? modelGoal_ : modelEnemy_, worldPos);
	newEnemy->Freeze();
	newEnemy->SetGoalScale(kGoalScaleRadius_, kGoalCollisionScale_);

	railCamera_->SetGoalPosition(worldPos);
	if (player_) {
		player_->SetGoalPosition(worldPos);
	}
	SetPlayAreaCenter(worldPos);
	SpawnWaterPonds(worldPos);

	enemies_.push_back(newEnemy);
}

Vector3 GameScene::GetStageGoalPosition(int stage) {
	switch (stage) {
	case 1:
		return {0.0f, 10.0f, 1200.0f};
	case 2:
		return {0.0f, 440.0f, 1200.0f};
	default:
		return {0.0f, 10.0f, 1200.0f};
	}
}

void GameScene::LoadStage(int stage) {
	currentStage_ = stage;

	for (Enemy* enemy : enemies_) {
		delete enemy;
	}
	enemies_.clear();
	for (WaterPond* pond : waterPonds_) {
		delete pond;
	}
	waterPonds_.clear();

	SpawnGoalAt(GetStageGoalPosition(stage));
	hasSpawnedEnemies_ = true;
}

void GameScene::AdvanceToNextStage() {
	if (currentStage_ >= kMaxStage_) {
		return;
	}

	LoadStage(currentStage_ + 1);

	gameIntroTimer_ = 0.0f;
	isGameIntroFinished_ = false;
	gameSceneTimer_ = 0.0f;
	confettiActive_ = false;
	score_ = 0;
	UpdateScoreSprites();

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
}

void GameScene::EnemySpawn(const Vector3& position) {
	assert(railCamera_ && "EnemySpawn: railCamera_ が null です");
	KamataEngine::Vector3 playerPos = railCamera_->GetWorldTransform().translation_;

	KamataEngine::Vector3 spawnPosWorld;
	spawnPosWorld.x = playerPos.x + position.x;
	spawnPosWorld.y = playerPos.y + position.y;
	spawnPosWorld.z = playerPos.z + position.z;

	SpawnGoalAt(spawnPosWorld);
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

	if (GetSceneStateKind() == SceneStateKind::Over || GetSceneStateKind() == SceneStateKind::GameIntro) {
		return;
	}

	KamataEngine::Vector3 posA[3]{}, posB[3]{};
	const std::list<PlayerBullet*>& playerBullets = player_->GetBullets();

	// ポリモーフィズム: プレイヤーを基底クラス（GameCharacter）として扱う
	GameCharacter* playerCharacter = player_;

	// ゴルフ用: ボール（自機）のワールド位置と半径
	posA[0] = playerCharacter->GetWorldPosition();
	const float playerRadius = playerCharacter->GetCollisionRadius();

	// ゴルフ化: 敵弾でHPが減ってゲームオーバーにならないよう、敵弾ダメージ判定は一旦無効化する
	// （ゴルフの「穴」は弾を撃たないため）
	// for (EnemyBullet* enemyBullet : enemyBullets_) {
	// 	if (!enemyBullet || enemyBullet->IsDead())
	// 		continue;
	// 	GameBullet* bullet = enemyBullet;
	// 	posB[0] = bullet->GetWorldPosition();
	// 	float distanceSquared = DistanceSquared(posA[0], posB[0]);
	// 	float combinedRadius = playerRadius + bullet->GetCollisionRadius();
	// 	float combinedRadiusSquared = combinedRadius * combinedRadius;
	// 	if (distanceSquared <= combinedRadiusSquared) {
	// 		ApplyCollisionDamage(playerCharacter);
	// 		MarkBulletDestroyed(bullet);
	// 		if (playerCharacter->IsDead()) {
	// 			TransitionToClearScene2();
	// 			return;
	// 		}
	// 	}
	// }

	// --- ゴルフ: 池に落ちたらゲームオーバー ---
	for (WaterPond* pond : waterPonds_) {
		if (!pond) {
			continue;
		}
		const KamataEngine::Vector3 pondCenter = pond->GetCenter();
		const float pdx = posA[0].x - pondCenter.x;
		const float pdz = posA[0].z - pondCenter.z;
		const float checkRadius = pond->GetBoundingRadius() + playerRadius;
		if (pdx * pdx + pdz * pdz > checkRadius * checkRadius) {
			continue;
		}
		if (pond->CheckBallFallIn(posA[0], playerRadius)) {
			TransitionToClearScene2();
			return;
		}
	}

	// --- ゴルフ: ボール（自機） vs 穴（敵） → ホールインワン ---
	// 穴に当たったらホールインワン成功としてクリア演出へ
	for (Enemy* holeObject : enemies_) {
		if (!holeObject || holeObject->IsDead())
			continue;

		// ポリモーフィズム: 穴（敵）を基底クラス（GameCharacter）として扱う
		GameCharacter* holeCharacter = holeObject;
		KamataEngine::Vector3 holePos = holeCharacter->GetWorldPosition();
		float combinedRadius = playerRadius + holeCharacter->GetCollisionRadius();
		float combinedRadiusSquared = combinedRadius * combinedRadius;
		if (DistanceSquared(posA[0], holePos) <= combinedRadiusSquared) {
			// ホールインワン！クリアシーンへ遷移
			TransitionToClearScene();
			return;
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
	ChangeSceneState(SceneStateClear::Instance());
	confettiActive_ = false;
	requestSceneClear_ = false;
	gameOverTimer_ = 0;
}

void GameScene::TransitionToClearScene2() {
	ChangeSceneState(SceneStateOver::Instance());
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
	const float kSpawnDistance = 800.0f;

	KamataEngine::Vector3 offset = randomDir * kSpawnDistance;
	KamataEngine::Vector3 spawnPos = cameraPos + offset;

	// スケールと半径をランダム
	const float kBaseRadius = 2.0f;
	const float kMinScale = 1.0f;
	const float kMaxScale = 5.0f;

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

KamataEngine::Vector2 GameScene::ConvertWorldToMinimap(const KamataEngine::Vector3& worldPos, const KamataEngine::Vector3& playerPos) {

	// 1. 自機からの相対座標（XZ のみ。高さ Y はミニマップに反映しない）
	float relativeX = worldPos.x - playerPos.x;
	float relativeZ = worldPos.z - playerPos.z;

	// 2. ミニマップのスケールを適用 (ワールドのZ+ を ミニマップのY+ (上) に)
	float minimapOffsetX = relativeX * kMinimapScale_;
	float minimapOffsetY = relativeZ * kMinimapScale_ * -1.0f; // Y軸反転

	// 3. ミニマップの中心座標を計算（左上アンカー基準）
	KamataEngine::Vector2 minimapCenterPos = {
	    kMinimapPosition_.x + kMinimapSize_.x * 0.5f,
	    kMinimapPosition_.y + kMinimapSize_.y * 0.5f // 左上アンカー基準
	};

	// 4. 中心の座標にオフセットを加える
	return {minimapCenterPos.x + minimapOffsetX, minimapCenterPos.y + minimapOffsetY};
}

bool GameScene::IsWithinMinimapWorldRange(const KamataEngine::Vector3& worldPos, const KamataEngine::Vector3& playerPos) const {
	const float halfRangeX = (kMinimapSize_.x * 0.5f) / kMinimapScale_;
	const float halfRangeZ = (kMinimapSize_.y * 0.5f) / kMinimapScale_;
	const float relX = worldPos.x - playerPos.x;
	const float relZ = worldPos.z - playerPos.z;
	return std::abs(relX) <= halfRangeX && std::abs(relZ) <= halfRangeZ;
}

void GameScene::CapMinimapMarkerSize(float& size) const {
	const float maxSize = (kMinimapSize_.x < kMinimapSize_.y) ? kMinimapSize_.x : kMinimapSize_.y;
	if (size > maxSize) {
		size = maxSize;
	}
	if (size < 2.0f) {
		size = 2.0f;
	}
}

bool GameScene::IsMinimapMarkerFullyInside(const KamataEngine::Vector2& pos, float size) const {
	const float minX = kMinimapPosition_.x;
	const float maxX = kMinimapPosition_.x + kMinimapSize_.x;
	const float minY = kMinimapPosition_.y;
	const float maxY = kMinimapPosition_.y + kMinimapSize_.y;
	const float half = size * 0.5f;
	return (pos.x - half >= minX) && (pos.x + half <= maxX) && (pos.y - half >= minY) &&
	       (pos.y + half <= maxY);
}

void GameScene::ClampMinimapSpriteMarker(KamataEngine::Vector2& pos, float& size) const {
	CapMinimapMarkerSize(size);

	const float minX = kMinimapPosition_.x;
	const float maxX = kMinimapPosition_.x + kMinimapSize_.x;
	const float minY = kMinimapPosition_.y;
	const float maxY = kMinimapPosition_.y + kMinimapSize_.y;

	const float half = size * 0.5f;
	if (maxX - minX <= size) {
		pos.x = (minX + maxX) * 0.5f;
	} else {
		pos.x = std::clamp(pos.x, minX + half, maxX - half);
	}
	if (maxY - minY <= size) {
		pos.y = (minY + maxY) * 0.5f;
	} else {
		pos.y = std::clamp(pos.y, minY + half, maxY - half);
	}
}

// Score handling
void GameScene::AddScore(int points) {
	if (points <= 0) return;
	score_ += points;
	if (score_ > kMaxScore_) score_ = kMaxScore_;
	UpdateScoreSprites();

	// If score reaches or exceeds 200, request clear the scene at a safe point
	if (GetSceneStateKind() == SceneStateKind::Game && score_ >= 600) {
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
