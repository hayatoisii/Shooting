#pragma once
#include "AABB.h"
#include "Enemy.h"
#include "KamataEngine.h"
#include "Player.h"
#include "RailCamera.h"
#include "Skydome.h"
#include <sstream>
using namespace KamataEngine;

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

	// ▼▼▼ 修正 ▼▼▼
	// int32_t timer = 0; // <-- 削除
	// bool timerflag = true; // <-- 削除
	bool hasSpawnedEnemies_ = false; // ★ 敵をスポーンさせたかどうかのフラグ
	                                 // ▲▲▲ 修正完了 ▲▲▲

private:
	DirectXCommon* dxCommon_ = nullptr;
	Input* input_ = nullptr;
	Audio* audio_ = nullptr;

	Player* player_ = nullptr;
	// Enemy* enemy_ = nullptr;
	Skydome* skydome_ = nullptr;
	Model* modelSkydome_ = nullptr;
	RailCamera* railCamera_ = nullptr;

	// Vector3 playerPos = {};

	KamataEngine::Sprite* reticleSprite_ = nullptr;
	uint32_t reticleTextureHandle_ = 0;

	// 自機モデル
	Model* modelPlayer_ = nullptr;
	// 敵モデル
	Model* modelEnemy_ = nullptr;

	// ゲームタイマー
	int32_t gameSceneTimer_ = 0;
	// ゲームの時間制限（例：30秒）
	const int32_t kGameTimeLimit_ = 60 * 30;

	Vector3 railcameraPos = {0, 5, -50};
	Vector3 railcameraRad = {0, 0, 0};

	// 敵弾リストを追加
	std::list<EnemyBullet*> enemyBullets_;

	// 敵発生コマンド
	std::stringstream enemyPopCommands;

	std::list<Enemy*> enemies_;

	int32_t titleAnimationTimer_ = 0;
	const int32_t kTitleRotateFrames = 60;
	const int32_t kTitlePauseFrames = 60;

	int hitCount = 0;
	int hitCount2 = 0;

	Model* modelTitleObject_ = nullptr;
	WorldTransform worldTransformTitleObject_;

	// シーンの状態を管理する列挙型
	enum class SceneState { Start, TransitionToGame, TransitionFromGame, GameIntro, Game, Clear, over };

	// 現在のシーンの状態を管理する変数
	SceneState sceneState = SceneState::Start;

	// 遷移用のスプライトとタイマー
	KamataEngine::Sprite* transitionSprite_ = nullptr;
	uint32_t transitionTextureHandle_ = 0;
	float transitionTimer_ = 0.0f;
	// 遷移にかかる時間（フレーム数）
	const float kTransitionTime = 30.0f;

	// オーディオ関連のメンバ変数
	int hitSoundHandle_ = 0;
	int hitSound_ = -1;

	Vector3 playerIntroStartPosition_ = {0.0f, -3.0f, -30.0f};
	Vector3 playerIntroTargetPosition_ = {0.0f, -3.0f, 20.0f};
	float gameIntroTimer_ = 0.0f;
	const float kGameIntroDuration_ = 120.0f; // 2秒
	bool isGameIntroFinished_ = false;

	float DistanceSquared(const Vector3& v1, const Vector3& v2);

	// 基底クラスのカメラ
	Camera camera_ = {};
};