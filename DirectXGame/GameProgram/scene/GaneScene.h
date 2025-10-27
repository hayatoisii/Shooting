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

	int32_t timer = 0;
	bool timerflag = true;

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
	// Vector3 enemyPos = {0, 3, 100};
	Vector3 RailCamerPos = {0, 0, 0};
	Vector3 RailCamerRad = {0, 0, 0};

	Model* modelPlayer_ = nullptr;
	Model* modelEnemy_ = nullptr;

	// std::vector<std::vector<WorldTransform*>> worldTransformBlocks_;
	WorldTransform worldTransform_;
	Camera camera_;

	Vector3 railcameraPos = {0, 0, 0};
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

	Vector3 playerPos = {0, 0, 30};

	bool isGameIntroFinished_ = false;       // イントロが終わったかどうかのフラグ
	float gameIntroTimer_ = 0.0f;            // イントロ用タイマー
	const float kGameIntroDuration_ = 90.0f; // イントロの長さ（フレーム数、0.5秒）
	Vector3 playerIntroStartPosition_;       // イントロ開始時のプレイヤー座標
	Vector3 playerIntroTargetPosition_;      // イントロ終了時（＝ゲーム開始時）のプレイヤー座標

	KamataEngine::Sprite* reticleSprite_ = nullptr;
	uint32_t reticleTextureHandle_ = 0;

	float DistanceSquared(const Vector3& v1, const Vector3& v2);

	int32_t gameSceneTimer_ = 0;         
	const int32_t kGameTimeLimit_ = 660;
};