#pragma once
#include "AABB.h"
#include "EnemyBullet.h"
#include "KamataEngine.h"
#include "ParticleEmitter.h"
#include "PlayerBullet.h"
#include <list>
#include "EnemyBullet.h"

using namespace KamataEngine;

class Enemy;
class RailCamera;

/// プレイヤークラス
/// 目的: ゲーム内のプレイヤーキャラクターを管理し、操作・攻撃・回避を行う
/// 責務: プレイヤーの位置・状態管理、入力処理、弾発射、回避行動、被弾処理
class Player {
public:
	Player() = default;
	~Player();

	/// 初期化処理
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& pos);
	
	/// 更新処理
	void Update();
	
	/// 描画処理
	void Draw();
	
	/// 攻撃処理（弾発射）
	void Attack();
	
	/// 衝突時の処理
	void OnCollision();

	/// 死亡状態を取得
	bool IsDead() const { return isDead_; }
	
	/// ワールドトランスフォームを取得
	KamataEngine::WorldTransform& GetWorldTransform() { return worldtransform_; }

	/// ゲームオーバー時のアニメーション更新
	void UpdateGameOver(float animationTime);

	/// ワールド座標を取得
	KamataEngine::Vector3 GetWorldPosition();
	
	/// AABB（軸平行境界ボックス）を取得
	AABB GetAABB();
	
	/// 弾のリストを取得
	const std::list<PlayerBullet*>& GetBullets() const { return bullets_; }

	/// 親トランスフォームを設定
	void SetParent(const KamataEngine::WorldTransform* parent);
	
	/// レールカメラを設定
	void SetRailCamera(RailCamera* camera);
	
	/// 敵のリストを設定
	void SetEnemies(std::list<Enemy*>* enemies) { enemies_ = enemies; }

	/// 回転をリセット
	void ResetRotation();

	/// パーティクルをリセット
	void ResetParticles();
	
	/// 弾をリセット
	void ResetBullets();

	// 当たり判定用のサイズ
	static inline const float kWidth = 1.0f;
	static inline const float kHeight = 1.0f;

	/// 弾の回避処理
	void EvadeBullets(std::list<EnemyBullet*>& bullets);
	
	/// 回避中かどうかを取得
	bool IsRolling() const { return isRolling_; }

private:
	KamataEngine::WorldTransform worldtransform_;
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

	int hp_ = 3;
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