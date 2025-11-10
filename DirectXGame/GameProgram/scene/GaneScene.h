#pragma once
#include "AABB.h"
#include "Enemy.h"
#include "KamataEngine.h"
#include "Player.h"
#include "RailCamera.h"
#include "Skydome.h"
#include <sstream>
using namespace KamataEngine;

float Distance(const Vector3& v1, const Vector3& v2);
Vector3 Lerp(const Vector3& start, const Vector3& end, float t);

class GameScene {
public:
	GameScene();
	~GameScene();

	void Initialize();
	void Update();
	void Draw();
	// 衝突判定と応答
	void CheckAllCollisions();

	void TransitionToClearScene();

	void TransitionToClearScene2();

	// 弾を追加
	void AddEnemyBullet(EnemyBullet* enemyBullet);
	const std::list<EnemyBullet*>& GetEnemyBullets() const { return enemyBullets_; }

	void LoadEnemyPopData();
	void UpdateEnemyPopCommands();
	void EnemySpawn(const Vector3& position);

	// 敵一括生成フラグ
	bool hasSpawnedEnemies_ = false;

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
	Model* modelEnemy_ = nullptr;

	int32_t gameSceneTimer_ = 0;
	const int32_t kGameTimeLimit_ = 60 * 30; // 30秒

	Vector3 railcameraPos = {0, 5, -50};
	Vector3 railcameraRad = {0, 0, 0};

	std::list<EnemyBullet*> enemyBullets_;
	std::stringstream enemyPopCommands;
	std::list<Enemy*> enemies_;

	int32_t titleAnimationTimer_ = 0;
	const int32_t kTitleRotateFrames = 60;
	const int32_t kTitlePauseFrames = 60;

	int hitCount = 0;
	int hitCount2 = 0; // ★ HP制にするならこのカウンターは不要かも

	Model* modelTitleObject_ = nullptr;
	WorldTransform worldTransformTitleObject_;

	enum class SceneState { Start, TransitionToGame, TransitionFromGame, GameIntro, Game, Clear, over };
	SceneState sceneState = SceneState::Start;

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
	const float kGameIntroDuration_ = 120.0f; // 2秒
	bool isGameIntroFinished_ = false;

	Camera camera_ = {};

	// ▼▼▼ 追加 ▼▼▼
	float gameOverTimer_ = 0.0f; // ゲームオーバー演出用タイマー
	
	// カメラ位置アンカー
	WorldTransform cameraPositionAnchor_;
};