#pragma once
#include "EntityFactory.h"
#include "GameBalanceTable.h"
#include "GameEvent.h"
#include "GameplayRewind.h"
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
#include <array>
#include <vector>
using namespace KamataEngine;

float Distance(const Vector3& v1, const Vector3& v2);
Vector3 Lerp(const Vector3& start, const Vector3& end, float t);

// State Pattern: シーン状態クラスから GameScene の内部にアクセスする
class SceneStateStart;
class SceneStateStageSelect;
class SceneStateTransitionToGame;
class SceneStateTransitionFromGame;
class SceneStateGameIntro;
class SceneStateGame;
class SceneStateClear;
class SceneStateOver;

class GameScene : public IGameEventListener {
	friend class SceneStateStart;
	friend class SceneStateStageSelect;
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

	void LoadStage(int stageIndex);
	void BeginStageFromSelect(int stageIndex);
	void AdvanceFromClearScreen();
	void AdvanceToNextStageFromClear();
	void ReturnToTitleFromStageClear();
	void ReturnToTitleScreen();
	void ResetCurrentStage();
	void HandleGameplayShortcuts();
	void UpdateGameplayRewind();
	void UpdateGameplayRewindInput();
	void SeedGameplayRewindSnapshot();
	void CaptureGameplaySnapshot(GameplaySnapshot& outSnapshot) const;
	void ApplyGameplaySnapshot(const GameplaySnapshot& snapshot, bool finalizeSideEffects = true);
	void FinalizeGameplayRewindScrub();
	void ApplyRewindScrubInterpolation(float tTowardTarget, bool undoDirection);
	void UpdateRewindFuelRecharge();
	void DrawRewindGauge();
	void DrawSpikeLifeHearts();
	bool TrampolinesMatchSnapshot(const std::vector<TrampolineSpringSnapshot>& snapshotSprings) const;
	void RestoreTrampolinesFromSnapshot(const std::vector<TrampolineSpringSnapshot>& snapshotSprings);
	void DrawSpikeRewindOverlay();
	bool HasNextStageAfterCurrent() const;
	static constexpr int kStageCount = 10;
	static constexpr int kStageSelectDisplayMin = 1;
	static constexpr int kStageSelectDisplayMax = 10;
	static constexpr int kStageSelectColsPerRow = 5;
	// 将来チュートリアル用スロットを追加する場合は kStageCount / UI 配置を拡張する
	int GetCurrentStageIndex() const { return currentStageIndex_; }

	void AddEnemyBullet(EnemyBullet* enemyBullet);
	const std::list<EnemyBullet*>& GetEnemyBullets() const { return enemyBullets_; }

	void LoadEnemyPopData();
	void UpdateEnemyPopCommands();
	void EnemySpawn(const Vector3& position);

	void UpdateAimAssist();
	KamataEngine::Vector3 ProjectToNDC(const KamataEngine::Vector3& worldPos);

	void SpawnMeteorite();
	void UpdateMeteorites();

	void DrawZoomOutMarginFill(float viewLeft, float viewBottom, float viewRight, float viewTop);
	void UpdateMapCamera();
	void UpdateTitleCamera();
	void UpdateCameraControl();
	void UpdatePlayerScreenTransition();
	void UpdateTrampolinePlacement();
	void AdvanceToNextAvailableTrampolineType();
	void SyncTrampolinePlacementType();
	bool HasAnySpringPlacementRemaining() const;
	void UpdateTrampolineArrowAnimations();
	void UpdateButtonGimmicks();
	void DrawTrampolineSprings();
	void DrawSpringPlacementHud();
	void InitializeSpringPlacementHud();
	int GetRemainingSpringPlacementCount(TrampolineSpringType type) const;
	void DrawJumpSpringChargeCircle();
	void DrawSpringTrajectoryPreview();
	void DrawStageSelectUi();
	void DrawStageClearUi();
	void LayoutStageClearButtons();
	bool IsScreenPointInSprite(const KamataEngine::Sprite* sprite, float screenX, float screenY) const;
	int HitTestStageSelectSlot(float screenX, float screenY) const;
	KamataEngine::Model* GetSpringModel(TrampolineSpringType type) const;
	KamataEngine::Vector3 ConvertScreenToWorld(float screenX, float screenY);
	KamataEngine::Vector2 ConvertWorldToScreen(float worldX, float worldY);
	void ComputeCameraBounds(float& left, float& bottom, float& right, float& top);
	void ComputeFreeCameraViewSize(float& viewW, float& viewH) const;

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
	Model* modelBlocks_ = nullptr;
	Model* modelDeleteBlocks_ = nullptr;
	Model* modelSpikeTile_ = nullptr;
	Model* modelPortal_ = nullptr;
	Model* modelSpringUp_ = nullptr;
	Model* modelSpringDown_ = nullptr;
	Model* modelSpringRight_ = nullptr;
	Model* modelSpringLeft_ = nullptr;
	Model* modelSpringArrow_ = nullptr;
	Model* modelRaycasting_ = nullptr;
	static constexpr int kTrajectoryDotPoolSize_ = 64;
	std::array<KamataEngine::WorldTransform, kTrajectoryDotPoolSize_> trajectoryDotTransforms_;
	bool isTrajectoryDotPoolReady_ = false;
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
	void UpdateStateBody_StageSelect();
	void UpdateStateBody_TransitionToGame();
	void UpdateStateBody_TransitionFromGame();
	void UpdateStateBody_GameIntro();
	void UpdateStateBody_Game();
	void UpdateStateBody_Clear();
	void UpdateStateBody_Over();
	void ResetToTitle();
	void ClearStageRuntimeEntities();
	void BeginGameplayWhileTransitionOverlay();
	void UpdateTransitionOverlayIfActive();
	void CommitPendingStageAfterTransitionExpand();
	void ResetTransitionExpandOverlay();
	KamataEngine::Vector2 GetClientMousePosition() const;

	float DistanceSquared(const KamataEngine::Vector3& v1, const KamataEngine::Vector3& v2);

	KamataEngine::Sprite* transitionSprite_ = nullptr;
	uint32_t transitionTextureHandle_ = 0;
	float transitionTimer_ = 0.0f;
	const float kTransitionTime = 30.0f;
	const float kTransitionGameplayUnlockTime = 15.0f;
	bool transitionOverlayActive_ = false;

	enum class TransitionExpandSource {
		StageSelect,
		ClearScreen,
	};
	TransitionExpandSource transitionExpandSource_ = TransitionExpandSource::StageSelect;
	int pendingStageIndex_ = 0;

	int hitSoundHandle_ = 0;
	int hitSound_ = -1;

	Vector3 playerIntroStartPosition_ = {0.0f, -3.0f, -30.0f};
	Vector3 playerIntroTargetPosition_ = {0.0f, -3.0f, 20.0f};
	float gameIntroTimer_ = 0.0f;
	const float kGameIntroDuration_ = 60.0f;
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
	KamataEngine::Sprite* gameOverSprite_ = nullptr;
	uint32_t gameOverTextureHandle_ = 0;
	KamataEngine::Sprite* clearScreenBackgroundSprite_ = nullptr;
	std::array<KamataEngine::Sprite*, 4> zoomMarginFillSprites_{};
	KamataEngine::Sprite* stageClearTitleReturnSprite_ = nullptr;
	KamataEngine::Sprite* stageClearNextStageSprite_ = nullptr;
	uint32_t stageClearTitleReturnTextureHandle_ = 0;
	uint32_t stageClearNextStageTextureHandle_ = 0;

	struct StageSelectSlot {
		float centerX = 0.0f;
		float centerY = 0.0f;
		float halfW = 72.0f;
		float halfH = 56.0f;
		int displayNumber = 1;
	};
	struct StageSelectSlotUi {
		KamataEngine::Sprite* digitSprites[2] = {nullptr, nullptr};
		int digitCount = 0;
	};
	std::array<StageSelectSlot, kStageCount> stageSelectSlots_{};
	std::array<StageSelectSlotUi, kStageCount> stageSelectSlotUi_{};
	int focusedStageSelectIndex_ = 0;
	static constexpr float kStageSelectDigitSize = 80.0f;
	static constexpr float kStageSelectMultiDigitSpacing = 4.0f;
	std::array<uint32_t, 10> stageSelectDigitTextureHandles_{};
	bool isStageSelectFontReady_ = false;
	KamataEngine::Sprite* stageSelectBackgroundSprite_ = nullptr;
	KamataEngine::Sprite* stageSelectCursorSprite_ = nullptr;
	void MoveStageSelectFocus(int deltaCol, int deltaRow);
	ParticleEmitter* clearEmitter_ = nullptr;
	ParticleEmitter* goalPortalEmitter_ = nullptr;
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
	static constexpr int kSpikeLivesMax_ = 20;
	KamataEngine::Sprite* minimapSprite_ = nullptr;       // ミニマップ背景
	KamataEngine::Sprite* minimapPlayerSprite_ = nullptr; // ミニマップ上の自機アイコン
	KamataEngine::Sprite* jumpSpringChargeSprite_ = nullptr;
	KamataEngine::Sprite* rewindGaugeBgSprite_ = nullptr;
	KamataEngine::Sprite* rewindGaugeFillSprite_ = nullptr;
	KamataEngine::Sprite* spikeLifeHeartIconSprite_ = nullptr;
	KamataEngine::Sprite* spikeLifeMultiplySprite_ = nullptr;
	KamataEngine::Sprite* spikeLifeTensDigitSprite_ = nullptr;
	KamataEngine::Sprite* spikeLifeOnesDigitSprite_ = nullptr;

	uint32_t minimapPlayerTextureHandle_ = 0;

	// ミニマップ上の敵アイコン (事前に最大数確保する)
	static const size_t kMaxMinimapEnemies_ = 100; // 例: 最大100体
	std::vector<KamataEngine::Sprite*> minimapEnemySprites_;

	// ミニマップの敵弾アイコン
	static const size_t kMaxMinimapEnemyBullets_ = 100;
	std::vector<KamataEngine::Sprite*> minimapEnemyBulletSprites_;
	uint32_t minimapEnemyBulletTextureHandle_ = 0;
	uint32_t heartTextureHandle_ = 0;

	// ミニマップ設定値（36×20タイルに合わせた縦横比）
	const KamataEngine::Vector2 kMinimapPosition_ = {10.0f, 10.0f}; // 描画基準位置 (左上)
	const KamataEngine::Vector2 kMinimapSize_ = {162.0f, 90.0f};     // 1タイル約4.5px
	const KamataEngine::Vector2 kRewindGaugeBarSize_ = {50.4f, 8.0f};
	const KamataEngine::Vector2 kHeartIconSize_ = {28.0f, 28.0f};
	const float kHeartRowGap_ = 10.0f;

	void RebuildMinimapTiles();
	void RebuildGoalPositions();
	void UpdateGoalPortalParticles();
	bool BeginPortalAbsorption();

	// ゴール吸い込み演出（PlayerSpin=自機軸 / OrbitSpiral=ポータル周回）
	static constexpr PortalAbsorptionStyle kPortalAbsorptionStyle = PortalAbsorptionStyle::OrbitSpiral;

	KamataEngine::Vector2 ConvertWorldToMinimapPosition(const KamataEngine::Vector3& worldPos) const;

	std::vector<KamataEngine::Sprite*> minimapGroundSprites_;
	std::vector<KamataEngine::Vector3> goalPositions_;

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
	static constexpr bool kShowScoreDigits_ = false;
	int score_ = 0; // 表示スコア
	const int kMaxScore_ = 9999;

	// 安全にシーンクリア遷移をリクエストするフラグ
	bool requestSceneClear_ = false;
	bool portalAbsorbFinishedPending_ = false;

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

	int currentStageIndex_ = 0;
	bool isPlayerGameInitialized_ = false;

	int currentScreenX_ = 0;
	int currentScreenY_ = 0;
	int screenTransitionCooldown_ = 0;
	static constexpr int kScreenTransitionCooldownFrames = 45;
	bool isFreeCamera_ = false;
	float freeCameraCenterX_ = 0.0f;
	float freeCameraCenterY_ = 0.0f;
	float cameraZoomOut_ = 0.0f;
	int middleMouseWheelSuppressFrames_ = 0;

	std::vector<TrampolineSpring> trampolineSprings_;
	static constexpr int kSpringPlacementLimitPerType_ = 10;
	static constexpr float kSpringHudMinimapGapX_ = 8.0f;
	static constexpr float kSpringHudIconAspectRatio_ = 2.2f;
	static constexpr float kSpringHudMultiplyScale_ = 0.55f;
	static constexpr float kSpringHudDigitScale_ = 0.65f;
	static constexpr float kSpringHudDigitSpacing_ = 2.0f;
	static constexpr float kSpringHudRowSpacing_ = 4.0f;
	static constexpr float kSpringHudInnerSpacing_ = 4.0f;
	struct SpringPlacementHudUi {
		KamataEngine::Sprite* iconSprite = nullptr;
		KamataEngine::Sprite* multiplySprite = nullptr;
		KamataEngine::Sprite* tensDigitSprite = nullptr;
		KamataEngine::Sprite* onesDigitSprite = nullptr;
	};
	std::array<SpringPlacementHudUi, 4> springPlacementHudUi_{};
	std::array<uint32_t, 4> springHudIconTextureHandles_{};
	uint32_t springMultiplyTextureHandle_ = 0;

	GameplayRewindBuffer gameplayRewindBuffer_;
	bool isGameplayRewinding_ = false;
	bool gameplayRewindSeeded_ = false;
	float gameplayRewindScrubAccumulator_ = 0.0f;
	bool isRewindGaugeVisible_ = false;
	float rewindFuelSeconds_ = GameplayRewindBuffer::kRewindFuelMaxSeconds;
	int spikeLivesRemaining_ = kSpikeLivesMax_;
	bool isSpikeRewindOverlayActive_ = false;
	float spikeRewindOverlayAlpha_ = 0.0f;
	KamataEngine::Sprite* spikeRewindDimSprite_ = nullptr;
	TrampolineSpring trampolinePreview_;
	KamataEngine::Vector3 trampolinePreviewPos_ = {};
	bool hasTrampolinePreview_ = false;
	int nextTrampolineTypeIndex_ = 0;
	float trampolineArrowAnimTime_ = 0.0f;
};