#pragma once
#include "AABB.h"
#include "EnemyBullet.h"
#include "GameCharacter.h"
#include "KamataEngine.h"
#include "ParticleEmitter.h"
#include "PlayerBullet.h"
#include <list>

using namespace KamataEngine;

class Enemy;
class RailCamera;

// プレイヤー（GameCharacter を継承。ポリモーフィズムの派生クラス）
class Player : public GameCharacter {
public:
	Player() = default;
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

	// 位置は setter 経由でのみ変更（内部 WorldTransform を直接公開しない）
	void SetPosition(const KamataEngine::Vector3& position);
	const KamataEngine::Vector3& GetLocalPosition() const { return worldtransfrom_.translation_; }
	void RefreshWorldMatrix() { worldtransfrom_.UpdateMatrix(); }

	// ゲーム再開時に HP・死亡フラグを初期化
	void ResetStats();

	void UpdateGameOver(float animationTime);

	AABB GetAABB();
	const std::list<PlayerBullet*>& GetBullets() const { return bullets_; }

	void SetParent(const KamataEngine::WorldTransform* parent);
	void SetRailCamera(RailCamera* camera);
	void SetEnemies(std::list<Enemy*>* enemies) { enemies_ = enemies; }

	void ResetRotation();
	void ResetParticles();
	void ResetBullets();

	// 当たり判定用のサイズ
	static inline const float kWidth = 1.0f;
	static inline const float kHeight = 1.0f;

	void EvadeBullets(std::list<EnemyBullet*>& bullets);

	// 回避中かどうかを取得
	bool IsRolling() const { return isRolling_; }

private:
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

	// パーティクル
	KamataEngine::Model* modelParticle_ = nullptr;
	ParticleEmitter* engineExhaust_ = nullptr;

	static const int kMaxHp_ = 3;
	int hp_ = kMaxHp_;
	bool isDead_ = false;
	int shotTimer_;

	int dodgeTimer_ = 0;

	bool isRolling_ = false;     // 回転中か
	float rollTimer_ = 0.0f;     // 回転タイマー
	float rollDirection_ = 0.0f; // 回転方向
	const float kRollDuration_ = 60.0f;

	// --- 被弾時の揺れ ---
	float hitShakeTime_ = 0.0f;       // 経過フレーム数
	float hitShakeAmplitude_ = 0.0f;
	float hitShakeDecay_ = 0.08f;    // 減衰係数
	float hitShakeFrequency_ = 0.3f;

	// 垂直方向（上下）揺れ用
	float hitShakeVerticalAmplitude_ = 0.0f; // 垂直振幅
	float hitShakePrevVerticalOffset_ = 0.0f;

	// 水平方向（左右）揺れ用
	float hitShakeHorizontalAmplitude_ = 0.0f; // 水平
	float hitShakePrevHorizontalOffset_ = 0.0f;

	// 弾発射時に固定するY座標（上下移動で発射位置がずれないようにする）
	float spawnBaseY_ = 0.0f;
};
