#pragma once
#include "AABB.h"
#include "EnemyBullet.h"
#include "GameCharacter.h"
#include "KamataEngine.h"
#include "ParticleEmitter.h"
#include "PlayerBullet.h"
#include "PlayerState.h"
#include <list>

using namespace KamataEngine;

class Enemy;
class RailCamera;

// プレイヤー（GameCharacter を継承。行動は State Pattern で切り替え）
class Player : public GameCharacter {
public:
	Player();
	~Player() override;

	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& pos);
	void Update();
	void Draw();
	void Attack();

	// --- GameCharacter の仮想関数（override） ---
	KamataEngine::Vector3 GetWorldPosition() const override;
	bool IsDead() const override;
	void OnCollision() override;
	int GetHp() const override;
	int GetMaxHp() const override;
	float GetCollisionRadius() const override;
	const char* GetKindName() const override;

	// 位置は setter 経由でのみ変更
	void SetPosition(const KamataEngine::Vector3& position);
	const KamataEngine::Vector3& GetLocalPosition() const { return worldtransfrom_.translation_; }
	void RefreshWorldMatrix() { worldtransfrom_.UpdateMatrix(); }

	void ResetStats();

	// ゲームオーバー演出用タイマー（Dead 状態で使用）
	void SetGameOverAnimationTime(float time) { gameOverAnimationTime_ = time; }

	AABB GetAABB();
	const std::list<PlayerBullet*>& GetBullets() const { return bullets_; }

	void SetParent(const KamataEngine::WorldTransform* parent);
	void SetRailCamera(RailCamera* camera);
	void SetEnemies(std::list<Enemy*>* enemies) { enemies_ = enemies; }

	void ResetRotation();
	void ResetParticles();
	void ResetBullets();

	static inline const float kWidth = 1.0f;
	static inline const float kHeight = 1.0f;

	void EvadeBullets(std::list<EnemyBullet*>& bullets);

	// ポリモーフィズム: 現在の状態オブジェクトに委譲
	bool IsRolling() const;

	// State Pattern: 状態遷移（状態クラスから呼ばれる）
	void ChangeState(PlayerState* newState);
	PlayerState* GetState() const { return state_; }
	const char* GetStateName() const;

	// --- 状態クラスから呼ばれる処理（public: 状態が Player のデータを更新する） ---
	void UpdateBullets();
	void ProcessDodgeInput();
	void UpdateRotationNormal();
	bool UpdateRotationRolling();
	void UpdateHitShake();
	void FinalizeFrameUpdate();
	void UpdateGameOverAnimation();
	void BeginRolling(float direction);

private:
	PlayerState* state_ = nullptr;

	KamataEngine::WorldTransform worldtransfrom_;
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::Input* input_ = nullptr;
	RailCamera* railCamera_ = nullptr;

	Audio* audio_ = nullptr;

	KamataEngine::Model* modelbullet_ = nullptr;
	std::list<PlayerBullet*> bullets_;

	std::list<Enemy*>* enemies_ = nullptr;

	int specialTimer = 20;
	bool isParry_ = false;

	int hitPlayerSoundHandle_ = 0;
	int hitPlayerSound_ = -1;

	KamataEngine::Model* modelParticle_ = nullptr;
	ParticleEmitter* engineExhaust_ = nullptr;

	static const int kMaxHp_ = 3;
	int hp_ = kMaxHp_;
	bool isDead_ = false;
	int shotTimer_;

	int dodgeTimer_ = 0;

	float rollTimer_ = 0.0f;
	float rollDirection_ = 0.0f;
	const float kRollDuration_ = 60.0f;

	float gameOverAnimationTime_ = 0.0f;

	// 被弾時の揺れ
	float hitShakeTime_ = 0.0f;
	float hitShakeAmplitude_ = 0.0f;
	float hitShakeDecay_ = 0.08f;
	float hitShakeFrequency_ = 0.3f;
	float hitShakeVerticalAmplitude_ = 0.0f;
	float hitShakePrevVerticalOffset_ = 0.0f;
	float hitShakeHorizontalAmplitude_ = 0.0f;
	float hitShakePrevHorizontalOffset_ = 0.0f;

	float spawnBaseY_ = 0.0f;
};
