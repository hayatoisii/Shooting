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
#include "FontRenderer.h"
#include <sstream>
#include <string>
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
	void DrawSpikeLifeHearts();
	bool TrampolinesMatchSnapshot(const std::vector<TrampolineSpringSnapshot>& snapshotSprings) const;
	void RestoreTrampolinesFromSnapshot(const std::vector<TrampolineSpringSnapshot>& snapshotSprings);
	void DrawSpikeRewindOverlay();
	void DrawGameplayRewindStatusLabel();
	void UpdateRewindOverlay();
	void ClearRewindOverlayImmediate();
	bool IsRewindPostReleaseLocked() const { return rewindPostReleaseLockSeconds_ > 0.0f; }
	bool HasNextStageAfterCurrent() const;
	static constexpr int kStageCount = 10;
	static constexpr int kStageSelectDisplayMin = 1;
	static constexpr int kStageSelectDisplayMax = 10;
	static constexpr int kStageSelectColsPerRow = 5;
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
	bool CanCameraZoomOut() const;
	void UpdatePlayerScreenTransition();
	void UpdateTrampolinePlacement();
	void AdvanceToNextAvailableTrampolineType();
	void SyncTrampolinePlacementType();
	bool HasAnySpringPlacementRemaining() const;
	bool IsTrampolineTypeSelectable(TrampolineSpringType type) const;
	int GetSpringPlacementLimit(TrampolineSpringType type) const;
	bool HasAnyCollectableSpringOnMap() const;
	bool FindCollectableSpringAtCursor(TrampolineSpringType cursorType, float cursorX, float cursorY, float cursorHalfW, float cursorHalfH,
	    size_t& outIndex) const;
	bool IsSpringPlacementBlockedAtCursor(TrampolineSpringType cursorType, float cursorX, float cursorY, float cursorHalfW, float cursorHalfH) const;
	void UpdateTrampolineArrowAnimations();
	void UpdateButtonGimmicks();
	void UpdateStageBgm();
	void StopStageBgm();
	void DrawTrampolineSprings();
	void DrawSpringPreviewPlacementLabel();
	void DrawSpringCollectLabel();
	void InitializeSpringPlacementHud();
	void InitializeStageTutorial();
	void ResetStageTutorial();
	void UpdateStageTutorial();
	void DrawStageTutorial();
	void InitializeKeyWallTutorial();
	void ResetKeyWallTutorial();
	void UpdateKeyWallTutorial();
	void DrawKeyWallTutorial();
	bool IsKeyWallTutorialActive() const;
	bool FindKeyWallTutorialAnchors();
	void SetupTutorialRewindToSpringCourse();
	void RestoreTutorialPreRewindCourse();
	bool IsPlayerOnTutorialRewindSpring() const;
	float FindTutorialGroundCenterY(float worldX, float halfW, float halfH) const;
	void BeginTutorialTimedBanner(const char* text, float seconds, bool centerOnScreen = false);
	void UpdateTutorialTimedBanner();
	void DrawTutorialTimedBanner();
	bool IsTutorialPauseIntroLocked() const { return tutorialPauseIntroLockRemainSec_ > 0.0f; }
	bool IsStageTutorialActive() const;
	bool IsTutorialPlacementAllowedAt(float worldX, float worldY, float springHalfW = 0.0f, float springHalfH = 0.0f) const;
	bool ShouldFreezePlayerForTutorial() const;
	int GetRemainingSpringPlacementCount(TrampolineSpringType type) const;
	void DrawJumpSpringChargeCircle();
	void DrawSpringTrajectoryPreview();
	void DrawStageSelectUi();
	void DrawStageClearUi();
	void LayoutStageClearButtons();
	bool IsScreenPointInSprite(const KamataEngine::Sprite* sprite, float screenX, float screenY) const;
	int HitTestStageSelectSlot(float screenX, float screenY) const;
	void UpdateOsCursorVisibility();
	void OpenPauseMenu();
	void ClosePauseMenu();
	void UpdatePauseMenu();
	void DrawPauseMenu();
	bool IsPauseMenuOpen() const { return isPauseMenuOpen_; }
	void UpdateGameplayPauseInput();
	void DrawGameplayPauseOverlay();
	bool CanUseGameplayPause() const;
	bool IsGameplayPaused() const { return isGameplayPaused_; }
	void BeginGameplayPause();
	void EndGameplayPause();
	void BeginGameplayPauseAfterRewind();
	void ClearIdleSpringContactsAfterPause();
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
	Model* modelWall_ = nullptr;
	RailCamera* railCamera_ = nullptr;

	KamataEngine::Sprite* reticleSprite_ = nullptr;
	uint32_t reticleTextureHandle_ = 0;

	Model* modelPlayer_ = nullptr;
	Model* modelCube_ = nullptr;
	Model* modelBlocks_ = nullptr;
	Model* modelDeleteBlocks_ = nullptr;
	Model* modelKey_ = nullptr;
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

	int hitCount = 0;
	int hitCount2 = 0;

	KamataEngine::Sprite* title2DSprite_ = nullptr;
	uint32_t title2DTextureHandle_ = 0;

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
	int keyCollectSoundHandle_ = 0;
	int keyCollectSound_ = -1;
	int stageBgmHandle_ = 0;
	int stageBgmVoice_ = -1;

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

	KamataEngine::Sprite* aimAssistCircleSprite_ = nullptr;
	uint32_t aimAssistCircleTextureHandle_ = 0;

	KamataEngine::Model* modelParticle_ = nullptr;
	ParticleEmitter* explosionEmitter_ = nullptr;

	KamataEngine::Sprite* clearSprite_ = nullptr;
	uint32_t clearTextureHandle_ = 0;
	KamataEngine::Sprite* gameOverSprite_ = nullptr;
	uint32_t gameOverTextureHandle_ = 0;
	KamataEngine::Sprite* clearScreenBackgroundSprite_ = nullptr;
	std::array<KamataEngine::Sprite*, 8> zoomMarginFillSprites_{};
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

	enum class PauseMenuPage {
		Main = 0,
		PlayMoveGuide = 1,
	};
	bool isPauseMenuOpen_ = false;
	PauseMenuPage pauseMenuPage_ = PauseMenuPage::Main;
	SceneStateKind pauseReturnSceneKind_ = SceneStateKind::Game;
	KamataEngine::Sprite* pauseDimSprite_ = nullptr;
	KamataEngine::Sprite* pauseTitleSprite_ = nullptr;
	KamataEngine::Sprite* pausePlayMoveSprite_ = nullptr;
	KamataEngine::Sprite* pausePlayMovePointSprite_ = nullptr;
	KamataEngine::Sprite* gameplayPauseStopSprite_ = nullptr;
	uint32_t pausePlayMoveTextureHandle_ = 0;
	uint32_t pausePlayMovePointTextureHandle_ = 0;
	uint32_t gameplayPauseStopTextureHandle_ = 0;
	float gameplayPauseStopAspect_ = 4.0f;
	static constexpr float kPauseMenuDimAlpha_ = 0.22f;
	static constexpr float kPauseMenuButtonW_ = 320.0f;
	static constexpr float kPauseMenuButtonH_ = 80.0f;
	static constexpr float kPauseMenuButtonY_ = 360.0f;
	static constexpr float kPauseMenuLeftButtonX_ = 360.0f;
	static constexpr float kPauseMenuRightButtonX_ = 920.0f;

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
	static constexpr int kSpikeLivesMaxStage8_ = 30;
	int GetSpikeLivesMaxForCurrentStage() const;
	KamataEngine::Sprite* minimapSprite_ = nullptr;       // ミニマップ背景
	KamataEngine::Sprite* minimapPlayerSprite_ = nullptr; // ミニマップ上の自機アイコン
	KamataEngine::Sprite* jumpSpringChargeSprite_ = nullptr;
	KamataEngine::Sprite* spikeLifeHeartIconSprite_ = nullptr;
	KamataEngine::Sprite* spikeLifeMultiplySprite_ = nullptr;
	KamataEngine::Sprite* spikeLifeTensDigitSprite_ = nullptr;
	KamataEngine::Sprite* spikeLifeOnesDigitSprite_ = nullptr;
	// チュートリアル上部タイマー用（残機数字と共用すると重なって見える）
	KamataEngine::Sprite* tutorialTimerOnesSprite_ = nullptr;
	KamataEngine::Sprite* tutorialTimerTenthsSprite_ = nullptr;
	KamataEngine::Sprite* tutorialTimerHundredthsSprite_ = nullptr;
	KamataEngine::Sprite* tutorialTimerTensSprite_ = nullptr;

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
	static constexpr bool kShowMinimap_ = false;
	const KamataEngine::Vector2 kMinimapPosition_ = {10.0f, 10.0f}; // 描画基準位置 (左上)
	const KamataEngine::Vector2 kMinimapSize_ = {162.0f, 90.0f};     // 1タイル約4.5px
	const KamataEngine::Vector2 kHeartUiPosition_ = {10.0f, 24.0f}; // 残機表示（左上）
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
	bool isFreeCamera_ = false;
	float freeCameraCenterX_ = 0.0f;
	float freeCameraCenterY_ = 0.0f;
	float cameraZoomOut_ = 0.0f;

	std::vector<TrampolineSpring> trampolineSprings_;
	// 未設定ステージのデフォルト上限（ステージ別は GetSpringPlacementLimit）
	static constexpr int kSpringPlacementLimitPerType_ = 9;
	static constexpr float kSpringHudMultiplyScale_ = 0.55f;
	static constexpr float kSpringHudDigitScale_ = 0.65f;
	static constexpr float kSpringHudDigitSpacing_ = 2.0f;
	static constexpr float kSpringPreviewDigitScaleBoost_ = 1.28f;
	static constexpr float kSpringPreviewDigitOffsetY_ = -0.4f;
	static constexpr float kSpringPreviewLabelBaseH_ = 20.0f;
	static constexpr float kSpringPreviewLabelBoldPx_ = 1.0f; // 太字用のずらし量（画面px）
	struct SpringPlacementHudUi {
		KamataEngine::Sprite* iconSprite = nullptr;
		KamataEngine::Sprite* multiplySprite = nullptr;
		KamataEngine::Sprite* tensDigitSprite = nullptr;
		KamataEngine::Sprite* onesDigitSprite = nullptr;
		float iconAspect = 1.0f;
	};
	std::array<SpringPlacementHudUi, 4> springPlacementHudUi_{};
	std::array<uint32_t, 4> springHudIconTextureHandles_{};
	uint32_t springMultiplyTextureHandle_ = 0;

	// バネ回収可能時に表示する「回収する」ラベル
	KamataEngine::Sprite* springCollectLabelSprite_ = nullptr;
	KamataEngine::Sprite* springCollectLabelVerticalSprite_ = nullptr;
	uint32_t springCollectLabelTextureHandle_ = 0;
	uint32_t springCollectLabelVerticalTextureHandle_ = 0;
	bool osCursorHidden_ = false;
	FontRenderer* fontRenderer_ = nullptr;

	// ===== ステージ1チュートリアル =====
	enum class StageTutorialStep {
		Inactive = 0,
		PlaceSpring,   // 枠内にバネを1つ設置
		TouchSpring,   // WASDで触れる
		ChargeSpring,  // SPACE短押しでも発射可→最高到達点で次へ
		RedSpringHint, // 頂点で停止→A/Dで解除＆左右移動案内
		WaitLand,      // 着地待ち
		ChangeType,    // 右クリックで種類を一周（Player停止）
		HitSpike,      // （誘導ではスキップ）棘に当たってみよう
		RewindIntro,   // Q巻き戻しの説明（2秒・操作不可）
		PressQ,        // 左の右向きバネまでQで巻き戻す
		RideSpringAgain, // （未使用）
		SpacePauseHint,  // バネ接触後の一時停止→長押しチャージ→着地で次へ
		CollectSpring,   // 設置バネを左クリック回収
		ScrollWheel,   // ホイールでズーム
		ShowGoal,      // ゴールを見せる
		PressR,        // Rで通しプレイへ
		Done,
	};
	enum class TutorialMapMode {
		Guided = 0,   // maptutorial1.csv + 誘導
		FreePlay = 1, // maptutorial.csv + チュートリアルなし
	};
	StageTutorialStep stageTutorialStep_ = StageTutorialStep::Inactive;
	TutorialMapMode tutorialMapMode_ = TutorialMapMode::Guided;

	// ===== ステージ5（map5）鍵・消える壁チュートリアル =====
	enum class KeyWallTutorialPhase {
		Inactive = 0,
		ShowAtKey,  // 「鍵を取る」を鍵横に2秒
		MoveToWall, // 壁へイージング移動＋文言変更
		ShowAtWall, // 「鍵を取ると壁が消えます」を壁横に2秒
		Done,
	};
	KeyWallTutorialPhase keyWallTutorialPhase_ = KeyWallTutorialPhase::Inactive;
	float keyWallTutorialTimer_ = 0.0f;
	KamataEngine::Vector3 keyWallTutorialKeyWorld_ = {};
	KamataEngine::Vector3 keyWallTutorialWallWorld_ = {};
	KamataEngine::Vector2 keyWallTutorialTextScreen_ = {};
	KamataEngine::Vector2 keyWallTutorialFromScreen_ = {};
	KamataEngine::Vector2 keyWallTutorialToScreen_ = {};
	const char* keyWallTutorialText_ = nullptr;
	float keyWallTutorialTextAlpha_ = 1.0f;
	static constexpr int kKeyWallTutorialStageIndex_ = 4; // map5.csv
	static constexpr float kKeyWallTutorialHoldSec_ = 2.0f;
	static constexpr float kKeyWallTutorialMoveSec_ = 0.85f;
	static constexpr float kKeyWallTutorialFadeInSec_ = 0.35f;

	KamataEngine::Sprite* tutorialFrameSprite_ = nullptr;
	bool tutorialTouchedSpring_ = false;
	bool tutorialChargeReady_ = false;
	bool tutorialChargeFlying_ = false;
	bool tutorialChargeLaunched_ = false;
	float tutorialChargePeakY_ = 0.0f;
	bool tutorialTypeCycleAway_ = false;
	bool tutorialDidRewind_ = false;
	bool tutorialRewindExhausted_ = false;
	bool tutorialPlayerControlLocked_ = false;
	// HitSpike開始時に棘に重なっていたら、一度離れるまで棘判定を無効化
	bool tutorialSpikeAwaitingExit_ = false;
	// 棘チュートリアルで中央へ瞬間移動した場合、その地点より前へは巻き戻せない
	bool tutorialSpikeTeleportRewindClamp_ = false;
	bool tutorialWasOnSpringForSecondRide_ = false;
	bool tutorialSpacePauseApexReached_ = false;
	bool tutorialSpacePauseReleased_ = false;
	int tutorialSpacePauseAirFrames_ = 0;
	float tutorialSpacePausePeakY_ = 0.0f;
	float tutorialRideSpringLaunchY_ = 0.0f;
	// PressQ: 左バネまで巻き戻せたか／途中離し案内
	bool tutorialRewindReachedSpring_ = false;
	bool tutorialRewindEarlyReleaseHint_ = false;
	float tutorialRewindIntroRemainSec_ = 0.0f;
	// SpacePauseHint: チャージジャンプして着地したか
	bool tutorialPauseChargeLaunched_ = false;
	bool tutorialPauseLandHintActive_ = false;
	float tutorialPauseLandHintRemainSec_ = 0.0f;
	int tutorialPauseLandGroundFrames_ = 0;
	int tutorialPauseAirborneFrames_ = 0;
	bool tutorialHasPreRewindSnapshot_ = false;
	KamataEngine::Vector3 tutorialPreRewindPlayerPos_ = {};
	std::vector<TrampolineSpringSnapshot> tutorialPreRewindSprings_;
	// Q離し直後: 「一時停止されるぞ」案内中は操作ロック
	float tutorialPauseIntroLockRemainSec_ = 0.0f;
	// 画面上部・時間で消える案内バナー
	float tutorialTimedBannerRemainSec_ = 0.0f;
	const char* tutorialTimedBannerText_ = nullptr;
	bool tutorialTimedBannerCentered_ = false;
	bool isGameplayPaused_ = false;
	float gameplayPauseSavedVelX_ = 0.0f;
	float gameplayPauseSavedVelY_ = 0.0f;
	bool gameplayPauseSpaceWasDown_ = false;
	int gameplayPauseSpaceHoldFrames_ = 0;
	// 一時停止解除に使ったSPACE押しの「離し」で、すぐ再一時停止しないためのガード
	bool gameplayPauseIgnoreSpaceUntilRelease_ = false;
	// 一時停止解除に使ったQ押しで、すぐ巻き戻しが始まらないようにする
	bool gameplayPauseIgnoreQUntilRelease_ = false;
	static constexpr int kGameplayPauseTapMaxFrames_ = 9; // 短押し判定（約0.15秒）
	bool tutorialWheelMoved_ = false;
	uint32_t tutorialSeenTypeMask_ = 0;
	int tutorialTypeCycleStartIndex_ = 0;
	int tutorialPrevTypeIndex_ = 0;
	int tutorialGoalVisibleFrames_ = 0;
	KamataEngine::Vector3 tutorialPlacedSpringWorldPos_ = {};
	KamataEngine::Vector3 tutorialGoalWorldPos_ = {};
	float tutorialZoomAtWheelStep_ = 0.0f;
	bool tutorialRedSpringHintFrozen_ = false;
	int tutorialRedSpringHintAirFrames_ = 0;
	float tutorialRedSpringHintLaunchY_ = 0.0f;
	std::string ResolveStageMapRelativePath() const;
	bool IsTutorialSpringReactive() const;
	bool IsTutorialSpikeReactive() const;
	bool IsPlayerOverlappingSpike() const;
	bool IsTutorialGoalVisibleInView(KamataEngine::Vector3& outGoalPos);
	bool ShouldForceTutorialSpringPreview() const;
	bool IsTutorialSpringPlacementLocked() const;
	float GetTutorialRewindMaxSeconds() const;
	void UpdateTutorialRewindExhaustion();
	// 枠のワールド座標（maptutorial1: 列28の縦棘のすぐ左に隙間を空けて配置）
	static constexpr float kTutorialFrameHalfW_ = 84.0f; // 枠の一辺 168（正方形）
	static constexpr float kTutorialFrameHalfH_ = 84.0f;
	static constexpr int kTutorialSpikeCol_ = 28;            // maptutorial1 の縦棘列
	static constexpr float kTutorialFrameGapFromSpike_ = 36.0f; // 棘との隙間
	static constexpr float kTutorialFrameCenterX_ =
	    static_cast<float>(kTutorialSpikeCol_) * TileMap::kTileWidth - kTutorialFrameGapFromSpike_ - kTutorialFrameHalfW_; // 1056
	static constexpr float kTutorialFrameCenterY_ =
	    420.0f - (kTutorialFrameHalfH_ * 2.0f) - (kTutorialFrameHalfH_ * 2.0f) / 3.0f; // 196（以前の高さ）
	static constexpr float kTutorialDimAlpha_ = 0.55f;
	static constexpr float kTutorialMinCharge_ = 0.08f; // すぐ離しても飛べる程度
	static constexpr float kTutorialMaxRewindSeconds_ = 3.0f;
	static constexpr float kTutorialFontHeight_ = 21.0f;
	static constexpr float kTutorialPauseHintFontHeight_ = 28.0f; // バネ到達後の一時停止案内
	static constexpr float kTutorialPauseHintAboveSpringPx_ = 64.0f; // 36 + 文字約1個分
	static constexpr float kTutorialRewindMechanismHintLowerPx_ = 18.0f; // 一時停止説明を少し下へ
	static constexpr float kTutorialPauseIntroLockSec_ = 4.0f;
	static constexpr float kTutorialPauseLandHintSec_ = 3.0f;
	static constexpr float kTutorialRewindIntroSec_ = 2.0f;
	static constexpr float kTutorialPauseLandHintTextAnchorY_ = 0.30f; // 画面中央より上
	static constexpr float kTutorialTimedBannerFontHeight_ = 30.0f;
	static constexpr float kTutorialTimedBannerTextAboveSpringPx_ = 48.0f; // バネ上端からの隙間
	static constexpr float kTutorialTextGapLeftOfFrame_ = 12.0f; // 枠左端と文字末尾の隙間
	// グリフ余白で見た目が下ずれる分を上へ補正
	static constexpr float kTutorialTextOpticalOffsetY_ = 6.0f;
	// 設置判定は見た目の緑枠より一回り小さく（枠線に触れるだけでは置けない）
	static constexpr float kTutorialFramePlaceScale_ = 0.72f;
	static constexpr int kTutorialSpringPlacementLimit_ = 1;
	static constexpr int kTutorialGoalVisibleFramesToAdvance_ = 180;
	static constexpr float kGameplayPauseDimAlpha_ = 0.5f;
	static constexpr float kGameplayPauseStopHeight_ = 43.2f; // 36 × 1.2
	static constexpr float kGameplayPauseStopCenterY_ = 52.0f; // 画面上部中央
	static constexpr float kRewindStatusFontHeight_ = 28.0f;
	static constexpr float kRewindStatusDotStepSec_ = 0.35f; // ・が1個増える間隔
	static constexpr int kRewindStatusDotMax_ = 3;

	bool showSpringCollectLabel_ = false;
	bool springCollectLabelIsVertical_ = false;
	KamataEngine::Vector3 springCollectLabelWorldPos_ = {};
	float springCollectLabelSpringHalfW_ = 0.0f;
	float springCollectLabelSpringHalfH_ = 0.0f;
	float springCollectLabelAspect_ = 1.0f;
	float springCollectLabelVerticalAspect_ = 1.0f;

	GameplayRewindBuffer gameplayRewindBuffer_;
	bool isGameplayRewinding_ = false;
	bool gameplayRewindSeeded_ = false;
	float gameplayRewindScrubAccumulator_ = 0.0f;
	float gameplayRewindQHoldSeconds_ = 0.0f;
	bool gameplayRewindScrubbedDuringHold_ = false;
	static constexpr float kRewindHoldSpeed12xSeconds_ = 3.0f;
	static constexpr float kRewindHoldSpeed15xSeconds_ = 5.0f;
	static constexpr float kRewindHoldSpeed2xSeconds_ = 8.0f;
	int spikeLivesRemaining_ = kSpikeLivesMax_;
	bool isSpikeRewindOverlayActive_ = false;
	float spikeRewindOverlayAlpha_ = 0.0f;
	bool spikeRewindScrubStarted_ = false;
	float rewindOverlayAlpha_ = 0.0f;
	float rewindStatusDotTimer_ = 0.0f;
	bool rewindMinimapDirty_ = false;
	bool pendingRewindPostReleaseLock_ = false;
	float rewindPostReleaseLockSeconds_ = 0.0f;
	float rewindPostReleaseDimAlpha_ = 0.0f;
	static constexpr float kRewindPostReleaseLockSeconds_ = 0.2f;
	static constexpr float kNormalRewindDimAlpha_ = 0.28f;
	static constexpr float kSpikeRewindDimAlpha_ = 0.62f;
	static constexpr float kRewindOverlayFadeInSpeed_ = 0.06f;
	KamataEngine::Sprite* spikeRewindDimSprite_ = nullptr;
	TrampolineSpring trampolinePreview_;
	KamataEngine::Vector3 trampolinePreviewPos_ = {};
	bool hasTrampolinePreview_ = false;
	int nextTrampolineTypeIndex_ = 0;
	float trampolineArrowAnimTime_ = 0.0f;
};