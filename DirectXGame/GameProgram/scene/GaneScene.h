#pragma once
#include "AABB.h"
#include "Enemy.h"
#include "KamataEngine.h"
#include "Player.h"
#include "RailCamera.h"
#include "SceneStateBase.h"
#include "WaterPond.h"
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

class GameScene {
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

	void LoadStage(int stage);
	void AdvanceToNextStage();
	void SpawnGoalAt(const Vector3& worldPos);
	static Vector3 GetStageGoalPosition(int stage);

	static const int kMaxStage_ = 2;
	static inline const float kGoalScaleRadius_ = 4.5f;
	// 見た目より当たり判定だけ広げる（ホールインの取りこぼし防止）
	static inline const float kGoalCollisionScale_ = 1.3f;
	int currentStage_ = 1;

	void AddEnemyBullet(EnemyBullet* enemyBullet);
	const std::list<EnemyBullet*>& GetEnemyBullets() const { return enemyBullets_; }

	void LoadEnemyPopData();
	void UpdateEnemyPopCommands();
	void SpawnWaterPonds(const KamataEngine::Vector3& goalCenter);
	void InitMinimapPondSprites();
	void UpdateMinimapPonds(const KamataEngine::Vector3& playerPos);
	void SetPlayAreaCenter(const KamataEngine::Vector3& center);
	void DrawBoundaryWalls();
	void UpdateBoundaryWall();
	void EnemySpawn(const Vector3& position);

	void UpdateAimAssist();
	KamataEngine::Vector3 ProjectToNDC(const KamataEngine::Vector3& worldPos);

	void SpawnMeteorite();
	void UpdateMeteorites();

	void RequestExplosion(const KamataEngine::Vector3& position);

	bool hasSpawnedEnemies_ = false;

	// Score handling (made public so other game objects can award points)
	void AddScore(int points);
	void UpdateScoreSprites();

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
	Skydome* zimenn_ = nullptr;
	Model* modelZimenn_ = nullptr;
	RailCamera* railCamera_ = nullptr;

	// --- ゴルフ用: 地面（当たり判定のみ・描画はスカイドームで後から差し替え） ---
	Model* modelGround_ = nullptr;
	WorldTransform groundTransform_;
	// 地面のワールドY（ボールのバウンド下限と一致させる）
	const float kGroundLocalY_ = 0.0f;
	// ゴルフコースの横幅（X方向スケール）
	const float kGroundWidth_ = 20.0f;
	// ゴルフコースの奥行き（Z方向スケール）。大きいほど長いコースになる
	const float kGroundLengthZ_ = 1500.0f;
	// プレーエリア（ゴール中心・XZ 円）の半径
	const float kPlayAreaRadius_ = 2000.0f;
	// 池配置: ゴール中心からこの半径以内（移動上限より少し狭い）
	const float kPondSpawnRadius_ = 1900.0f;
	const float kPondGlobalScale_ = 40.0f;
	// 空中打ち直し回数表示（標準サイズ、ホイール引き時のみ縮小）
	const float kAirShotBaseSize_ = 64.0f;
	// プレーエリア境界壁（境界100以内で3Dパネル1枚を表示）
	const float kWallShowDistance_ = 150.0f;
	const float kBoundaryWallPanelSize_ = 42.0f;
	const float kBoundaryWallThickness_ = 0.8f;
	const float kBoundaryBallRadius_ = 0.5f;
	// 池の描画距離（移動制限半径と同じ＝プレーエリア内は常に表示）
	const float kPondDrawDistance_ = kPlayAreaRadius_;

	// --- ゴルフ: パワーゲージ UI ---
	// ゲージ外枠スプライト（ユーザーが "gage.png" を Resources に置く）
	KamataEngine::Sprite* gageSprite_ = nullptr;
	uint32_t gageTH_ = 0;
	// ゲージバースプライト（ユーザーが "bar.png" を Resources に置く）
	KamataEngine::Sprite* barSprite_ = nullptr;
	uint32_t barTH_ = 0;
	// ゲージ外枠のスクリーン位置（左上基準）
	const float kGaugePosX_   = 600.0f;   // 画面中央やや右
	const float kGaugePosY_   = 140.0f;   // 縦方向の開始Y
	const float kGaugeWidth_  = 60.0f;    // 外枠の横幅
	const float kGaugeHeight_ = 180.0f;   // 外枠の高さ（バーが動く範囲）
	// バーのサイズ
	const float kBarWidth_    = 60.0f;
	const float kBarHeight_   = 30.0f;

	// --- ゴルフ: 飛距離カウンター ---
	// ゲーム開始時のボールZ座標（飛距離の基準点）
	float ballStartZ_ = 0.0f;

	KamataEngine::Sprite* reticleSprite_ = nullptr;
	uint32_t reticleTextureHandle_ = 0;

	Model* modelPlayer_ = nullptr;
	Model* modelEnemy_ = nullptr;
	Model* modelGoal_  = nullptr; // ゴール専用モデル（ボールと同じ OBJ を使用）
	KamataEngine::Sprite* airShotCountSprite_ = nullptr; // 空中打ち直し残り回数（ボール上に表示）

	// --- ゴルフ: 池（落ちるとゲームオーバー） ---
	Model* modelWaterPond_ = nullptr;
	std::vector<WaterPond*> waterPonds_;
	// --- ゴルフ: プレーエリア境界壁 ---
	KamataEngine::Vector3 playAreaCenter_ = {0.0f, 10.0f, 1200.0f};
	KamataEngine::WorldTransform boundaryWallTransform_;
	KamataEngine::ObjectColor boundaryWallColor_;
	bool boundaryWallVisible_ = false;
	KamataEngine::Vector3 boundaryWallPos_ = {0.0f, 0.0f, 0.0f};
	float boundaryWallYaw_ = 0.0f;
	// 敵弾用の3Dモデル（OBJ）を格納するポインタ
	Model* modelEnemyBullet_ = nullptr;

	// ゴルフ: カメラ初期位置はボール後方上方。追従カメラが起動後は上書きされる
	Vector3 railcameraPos = {0.0f, 8.0f, -18.0f};
	// 下向き角度を大きくするとボールが画面の下方に映る
	Vector3 railcameraRad = {0.28f, 0.0f, 0.0f};

	std::list<EnemyBullet*> enemyBullets_;
	std::stringstream enemyPopCommands;
	std::list<Enemy*> enemies_;

	int32_t hitCount = 0;
	int32_t hitCount2 = 0;

	KamataEngine::Sprite* titleScreenSprite_ = nullptr;
	uint32_t titleScreenTextureHandle_ = 0;

	KamataEngine::Sprite* wasdSprite_ = nullptr;
	uint32_t wasdTextureHandle_ = 0;

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
	bool IsBoundaryWallNear(const KamataEngine::Vector3& ball, float* outDistToWall) const;

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
	KamataEngine::Sprite* gameOverSprite_ = nullptr;
	uint32_t gameOverTextureHandle_ = 0;
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
	uint32_t minimapPondTextureHandle_ = 0;
	KamataEngine::Sprite* minimapSprite_ = nullptr;       // ミニマップ背景
	KamataEngine::Sprite* minimapPlayerSprite_ = nullptr; // ミニマップ上の自機アイコン

	uint32_t minimapPlayerTextureHandle_ = 0;

	// ミニマップ上の敵アイコン (事前に最大数確保する)
	static const size_t kMaxMinimapEnemies_ = 100; // 例: 最大100体
	std::vector<KamataEngine::Sprite*> minimapEnemySprites_;

	// ミニマップ上の池アイコン（各パーツ1つ）
	std::vector<KamataEngine::Sprite*> minimapPondSprites_;

	// ミニマップの敵弾アイコン
	static const size_t kMaxMinimapEnemyBullets_ = 100;
	std::vector<KamataEngine::Sprite*> minimapEnemyBulletSprites_;
	uint32_t minimapEnemyBulletTextureHandle_ = 0;

	// ミニマップ設定値（左上配置）
	// ゴルフ: ゴールまで ~120 units → 0.75 px/unit で中心から90px（半径100px内に収まる）
	const KamataEngine::Vector2 kMinimapPosition_ = {10.0f, 10.0f}; // 描画基準位置 (左上)
	const KamataEngine::Vector2 kMinimapSize_ = {250.0f, 250.0f};   // 背景スプライトのサイズ
	const float kMinimapScale_ = 0.10f; // ワールド座標 -> ミニマップ座標の縮尺（小さいほど広範囲を表示）

	/// <returns>ミニマップ上のスクリーン座標</returns>
	KamataEngine::Vector2 ConvertWorldToMinimap(const KamataEngine::Vector3& worldPos, const KamataEngine::Vector3& playerPos);
	// ミニマップに映るワールド範囲内か（XZ のみ）
	bool IsWithinMinimapWorldRange(const KamataEngine::Vector3& worldPos, const KamataEngine::Vector3& playerPos) const;
	// マーカー中心とサイズをミニマップ矩形内に収める（ゴールなど小さいアイコン用）
	void ClampMinimapSpriteMarker(KamataEngine::Vector2& pos, float& size) const;
	// 池マーカー: 位置をずらさず、全体がミニマップ内に収まるときだけ表示
	bool IsMinimapMarkerFullyInside(const KamataEngine::Vector2& pos, float size) const;
	void CapMinimapMarkerSize(float& size) const;

	// 最後に記録したプレイヤー位置（ミニマップ回転の判定用）
	KamataEngine::Vector3 lastPlayerPos_ = {0.0f, 0.0f, 0.0f};

	int homingSpawnTimer_ = 0;
	// Enemyミサイルの間隔
	const int kHomingIntervalFrames_ = 60 * 10;
	const float kHomingMaxDistance_ = 3000.0f;
	const float kHomingBulletSpeed_ = 8.0f; // requested speed

	// デバッグ: ゲーム開始から指定秒数でタイトルに戻す
	bool debug10 = true;            // 有効化フラグ
	// デバッグ10秒タイマー
	float debug10ElapsedSec_ = 0.0f; // 経過秒数
	const float kDebug10Seconds = 100.0f; // タイトルへ戻すまでの秒数（100秒）

	// ゲームタイマー（秒）: 自動ゲームオーバー判定に使用
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
};