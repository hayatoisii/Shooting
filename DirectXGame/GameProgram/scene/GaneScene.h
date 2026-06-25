#pragma once
#include "EntityFactory.h"
#include "GameBalanceTable.h"
#include "GameEvent.h"
#include "MapRenderer.h"
#include "SpawnCommandTable.h"
#include "TileMap.h"
#include "TrampolineSpring.h"
#include "Enemy.h"
#include "KamataEngine.h"
#include "Player.h"
#include "RailCamera.h"
#include "SceneStateBase.h"
#include "Skydome.h"
#include "Meteorite.h"
#include <sstream>
#include <vector>
using namespace KamataEngine;

float Distance(const Vector3& v1, const Vector3& v2);
Vector3 Lerp(const Vector3& start, const Vector3& end, float t);

// State Pattern: シーン状態クラスから GameScene の内部にアクセスする
class SceneStateStart;
class SceneStateTransitionToGame;
class SceneStateTransitionFromGame;
class SceneStateGameIntro;
class SceneStateGame;
class SceneStateClear;
class SceneStateOver;

class GameScene : public IGameEventListener {
	friend class SceneStateStart;
	friend class SceneStateTransitionToGame;
	friend class SceneStateTransitionFromGame;
	friend class SceneStateGameIntro;
	friend class SceneStateGame;
	friend class SceneStateClear;
	friend class SceneStateOver;

public:
	GameScene();
	~GameScene();

	void Initialize();
	void Update();
	void Draw();

	void CheckAllCollisions();

	void TransitionToClearScene();

	void TransitionToClearScene2();

	void AddEnemyBullet(EnemyBullet* enemyBullet);
	const std::list<EnemyBullet*>& GetEnemyBullets() const { return enemyBullets_; }

	void LoadEnemyPopData();
	void UpdateEnemyPopCommands();
	void EnemySpawn(const Vector3& position);

	void UpdateAimAssist();
	KamataEngine::Vector3 ProjectToNDC(const KamataEngine::Vector3& worldPos);

	void SpawnMeteorite();
	void UpdateMeteorites();

	void UpdateMapCamera();
	void UpdateTitleCamera();
	void UpdateCameraControl();
	void UpdatePlayerScreenTransition();
	void UpdateTrampolinePlacement();
	void DrawTrampolineSprings();
	KamataEngine::Vector3 ConvertScreenToWorld(float screenX, float screenY);
	void ComputeCameraBounds(float& left, float& bottom, float& right, float& top);
	void SyncFreeCameraFromPlayerScreen();
	void ComputeFreeCameraViewSize(float& viewW, float& viewH) const;
	void ClampFreeCameraCenter(float viewW, float viewH);

	void RequestExplosion(const KamataEngine::Vector3& position);

	bool hasSpawnedEnemies_ = false;

	// Score handling (made public so other game objects can award points)
	void AddScore(int points);
	void SetScoreValue(int value);
	void UpdateScoreSprites();

	// Observer Pattern: ゲームイベントの受信
	void OnGameEvent(const GameEvent& event) override;

	EntityFactory& GetEntityFactory() { return entityFactory_; }
	const GameBalanceTable& GetBalanceTable() const { return balanceTable_; }

	// State Pattern: 状態遷移と現在状態の取得
	void ChangeSceneState(SceneStateBase* newState);
	SceneStateKind GetSceneStateKind() const;

private:
	DirectXCommon* dxCommon_ = nullptr;
	Input* input_ = nullptr;
	Audio* audio_ = nullptr;

	Player* player_ = nullptr;
	Skydome* skydome_ = nullptr;
	Model* modelSkydome_ = nullptr;
	RailCamera* railCamera_ = nullptr;

	KamataEngine::Sprite* reticleSprite_ = nullptr;
	uint32_t reticleTextureHandle_ = 0;

	Model* modelPlayer_ = nullptr;
	Model* modelCube_ = nullptr;
	Model* modelEnemy_ = nullptr;
	// 敵弾用の3Dモデル（OBJ）を格納するポインタ
	Model* modelEnemyBullet_ = nullptr;

	Vector3 railcameraPos = {0, 5, -50};
	Vector3 railcameraRad = {0, 0, 0};

	std::list<EnemyBullet*> enemyBullets_;
	std::stringstream enemyPopCommands;
	std::list<Enemy*> enemies_;

	int32_t titleAnimationTimer_ = 0;
	const int32_t kTitleRotateFrames = 60;
	const int32_t kTitlePauseFrames = 60;

	int hitCount = 0;
	int hitCount2 = 0;

	Model* modelTitleObject_ = nullptr;
	WorldTransform worldTransformTitleObject_;

	// 右／左キーを示すスプライト
	KamataEngine::Sprite* lightSprite_ = nullptr; // 右キー用（表示名: light）
	KamataEngine::Sprite* leftSprite_ = nullptr;  // 左キー用
	uint32_t lightTextureHandle_ = 0;
	uint32_t leftTextureHandle_ = 0;

	KamataEngine::Sprite* shiftSprite_ = nullptr; // 追加: Shiftキー表示用スプライト
	uint32_t shiftTextureHandle_ = 0;

	// Shift スプライトの点滅用タイマー
	int shiftBlinkTimer_ = 0;

	// Shift スプライトの右上オフセット（ピクセル）
	float shiftExtraRight_ = 16.0f; // Shift をさらに右にずらす量
	float shiftExtraUp_ = 6.0f;     // Shift をさらに上にずらす量

	// 矢印グループ全体を右に移動するオフセット（ピクセル）
	float controlGroupOffset_ = 16.0f; // 矢印と Shift を一緒に右へ移動する量

	SceneStateBase* sceneState_ = nullptr;

	// 各シーン状態の更新本体（状態クラスから呼ばれる）
	void UpdateStateBody_Start();
	void UpdateStateBody_TransitionToGame();
	void UpdateStateBody_TransitionFromGame();
	void UpdateStateBody_GameIntro();
	void UpdateStateBody_Game();
	void UpdateStateBody_Clear();
	void UpdateStateBody_Over();
	void ResetToTitle();

	float DistanceSquared(const KamataEngine::Vector3& v1, const KamataEngine::Vector3& v2);

	KamataEngine::Sprite* transitionSprite_ = nullptr;
	uint32_t transitionTextureHandle_ = 0;
	float transitionTimer_ = 0.0f;
	const float kTransitionTime = 30.0f;

	int hitSoundHandle_ = 0;
	int hitSound_ = -1;

	Vector3 playerIntroStartPosition_ = {0.0f, -3.0f, -30.0f};
	Vector3 playerIntroTargetPosition_ = {0.0f, -3.0f, 20.0f};
	float gameIntroTimer_ = 0.0f;
	const float kGameIntroDuration_ = 120.0f;
	bool isGameIntroFinished_ = false;

	Camera camera_ = {};

	float gameOverTimer_ = 0.0f;
	bool debugAutoClearEnabled_ = true;
	int debugAutoClearTimer_ = 0; // frames
	const int kDebugAutoClearFrames = 60; // ~1 second at 60 FPS

	// カメラ位置アンカー
	WorldTransform cameraPositionAnchor_;
	
	KamataEngine::Model* modelMeteorite_;
	std::list<Meteorite*> meteorites_;
	int meteoriteSpawnTimer_;
	int meteoriteUpdateCounter_;

	KamataEngine::Sprite* taitoruSprite_ = nullptr;
	uint32_t taitoruTextureHandle_ = 0;

	KamataEngine::Sprite* aimAssistCircleSprite_ = nullptr;
	uint32_t aimAssistCircleTextureHandle_ = 0;

	KamataEngine::Model* modelParticle_ = nullptr;
	ParticleEmitter* explosionEmitter_ = nullptr;

	KamataEngine::Sprite* clearSprite_ = nullptr;
	uint32_t clearTextureHandle_ = 0;
	ParticleEmitter* clearEmitter_ = nullptr;
	int confettiSpawnTimer_ = 0;
	bool confettiActive_ = false;

	struct ConfettiParticle {
		KamataEngine::Sprite* sprite = nullptr;
		bool active = false;
		KamataEngine::Vector2 pos = {0.0f, 0.0f};
		KamataEngine::Vector2 vel = {0.0f, 0.0f};
		float rotation = 0.0f;
		float rotVel = 0.0f;
		int life = 0;
		int age = 0;
	};
	std::vector<ConfettiParticle> confettiParticles_;
	uint32_t confettiTextureHandle_ = 0;
	const size_t kMaxConfetti_ = 200;

	uint32_t minimapTextureHandle_ = 0;
	uint32_t greenBoxTextureHandle_ = 0;
	KamataEngine::Sprite* minimapSprite_ = nullptr;       // ミニマップ背景
	KamataEngine::Sprite* minimapPlayerSprite_ = nullptr; // ミニマップ上の自機アイコン

	uint32_t minimapPlayerTextureHandle_ = 0;

	// ミニマップ上の敵アイコン (事前に最大数確保する)
	static const size_t kMaxMinimapEnemies_ = 100; // 例: 最大100体
	std::vector<KamataEngine::Sprite*> minimapEnemySprites_;

	// ミニマップの敵弾アイコン
	static const size_t kMaxMinimapEnemyBullets_ = 100;
	std::vector<KamataEngine::Sprite*> minimapEnemyBulletSprites_;
	uint32_t minimapEnemyBulletTextureHandle_ = 0;

	// ミニマップ設定値（30×17タイルに合わせた縦横比）
	const KamataEngine::Vector2 kMinimapPosition_ = {10.0f, 10.0f}; // 描画基準位置 (左上)
	const KamataEngine::Vector2 kMinimapSize_ = {150.0f, 85.0f};   // 1タイル約5px

	void RebuildMinimapTiles();
	KamataEngine::Vector2 ConvertWorldToMinimapPosition(const KamataEngine::Vector3& worldPos) const;

	std::vector<KamataEngine::Sprite*> minimapGroundSprites_;

	/// <returns>ミニマップ上のスクリーン座標（レーダー用・旧）</returns>
	KamataEngine::Vector2 ConvertWorldToMinimap(const KamataEngine::Vector3& worldPos, const KamataEngine::Vector3& playerPos);

	// 最後に記録したプレイヤー位置（ミニマップ回転の判定用）
	KamataEngine::Vector3 lastPlayerPos_ = {0.0f, 0.0f, 0.0f};

	int homingSpawnTimer_ = 0;
	// Enemyミサイルの間隔
	const int kHomingIntervalFrames_ = 60 * 10;
	const float kHomingMaxDistance_ = 3000.0f;
	const float kHomingBulletSpeed_ = 8.0f; // requested speed

	// ゲームタイマー（秒）
	float gameSceneTimer_ = 0.0f;

	// カウント表示 (ビットマップフォント用)
	int score_ = 0; // 表示スコア
	const int kMaxScore_ = 9999;

	// 安全にシーンクリア遷移をリクエストするフラグ
	bool requestSceneClear_ = false;

	// デジットテクスチャハンドル (0..9)
	std::vector<uint32_t> digitTextureHandles_;
	// 表示用スプライト (4桁)
	std::vector<KamataEngine::Sprite*> scoreDigitSprites_;

	// Factory Method + Object Pool: エンティティ生成
	EntityFactory entityFactory_;
	GameEventSubject gameEventSubject_;

	// データドリブン: バランス値・スポーンコマンド
	GameBalanceTable balanceTable_;
	SpawnCommandTable spawnCommandTable_;

	TileMap tileMap_;
	MapRenderer mapRenderer_;

	int currentScreenX_ = 0;
	int currentScreenY_ = 0;
	bool isFreeCamera_ = false;
	float freeCameraCenterX_ = 0.0f;
	float freeCameraCenterY_ = 0.0f;
	float cameraZoomOut_ = 0.0f;

	std::vector<TrampolineSpring> trampolineSprings_;
	TrampolineSpring trampolinePreview_;
	KamataEngine::Vector3 trampolinePreviewPos_ = {};
	bool hasTrampolinePreview_ = false;
	int nextTrampolineTypeIndex_ = 0;
};