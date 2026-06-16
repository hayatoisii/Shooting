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
	// カメラの現在ヨー角をセット（照準の基準方向に使用）
	void SetCameraYaw(float yaw) { cameraYaw_ = yaw; }
	// ゴール位置をセット（方向表示矢印の向き計算に使用）
	void SetGoalPosition(const KamataEngine::Vector3& pos) { goalPosition_ = pos; }
	const KamataEngine::Vector3& GetGoalPosition() const { return goalPosition_; }
	// プレーエリア半径（ゴール中心の XZ 円。0 以下で無制限）
	void SetPlayAreaRadius(float radius) { playAreaRadius_ = radius; }
	void SetEnemies(std::list<Enemy*>* enemies) { enemies_ = enemies; }

	void ResetRotation();
	void ResetParticles();
	void ResetBullets();

	// --- ゴルフ用: 状態クラスから呼ばれる処理 ---
	// 物理更新（重力落下・地面クランプ・バウンド・摩擦）
	void UpdateGolfBall();
	// スイング演出（パターが回転してボールを打つ）。完了したら true を返す
	bool UpdateSwingAnimation();
	// 照準を開始する（Normal → Aiming 遷移）
	void BeginAiming();
	// 方向矢印アニメを1フレーム進める（Aiming 状態から呼ぶ）
	void UpdateAimArrow();
	// 矢印の現在角度を打撃方向として確定する
	void LockAimDirection();
	// 照準状態かどうか（UI 描画判定用）
	bool IsAiming() const;

	// 高さ照準を開始する（Aiming → AimingHeight 遷移）
	void BeginAimingHeight();
	// 高さ矢印アニメを1フレーム進める（AimingHeight 状態から呼ぶ）
	void UpdateAimHeight();
	// 現在の高さ角度を確定する（AimingHeight → Gauging 遷移前）
	void LockAimHeight();
	// 高さ照準状態かどうか
	bool IsAimingHeight() const;

	// ゲージを開始する（AimingHeight → Gauging 遷移）
	void BeginGauging();
	// スイングを開始する（Gauging → Swing 遷移）
	void BeginSwing();
	// 打撃: ボールにZ＋Y の初速を与える
	void LaunchBall();
	// ボールが地面で静止したときの SE
	void PlayBallRestSe();
	void PlayLandingBurst(float impactSpeed);
	// 地面に接地しているか
	bool IsOnGround() const { return worldtransfrom_.translation_.y <= groundY_ + 0.01f; }
	// 速度がほぼゼロか（完全停止判定）
	bool IsVelocityNearZero() const;
	// 空中再照準モードのフラグ設定・取得
	void SetAirAiming(bool v) { isAirAiming_ = v; }
	bool IsAirAiming() const  { return isAirAiming_; }
	// 空中打ち直し残り回数
	void DecrementAirShots()      { if (airShotsRemaining_ > 0) --airShotsRemaining_; }
	void ResetAirShots()          { airShotsRemaining_ = kMaxAirShots_; }
	int  GetAirShotsRemaining() const { return airShotsRemaining_; }

	// ゴールと重なったときボールを半透明にするためのアルファ値（1.0=不透明）
	float GetBallDrawAlpha() const;
	// SPACE が今フレームに押されたか（初回だけ true）
	bool IsSpaceJustPressed() const;
	// ゲージバーの現在位置（0.0 = 底・最弱、1.0 = 頂・最強）
	float GetGaugePower() const { return gaugePower_; }
	// ゲージ状態かどうか（UI 描画判定用）
	bool IsGauging() const;
	// ゲージアニメを1フレーム進める（Gauging 状態から呼ぶ）
	void UpdateGauge();
	// 地面の高さ（ローカルY。これより下に行かない）を設定
	void SetGroundY(float y) { groundY_ = y; }
	// ボールの速度を設定（外部から直接セットしたい場合）
	void SetVelocity(const KamataEngine::Vector3& v) { velocity_ = v; }
	const KamataEngine::Vector3& GetVelocity() const { return velocity_; }

	static inline const float kWidth = 1.0f;
	static inline const float kHeight = 1.0f;

	void EvadeBullets(std::list<EnemyBullet*>& bullets);

	// ポリモーフィズム: 現在の状態オブジェクトに委譲
	bool IsRolling() const;

	// State Pattern: 状態遷移（状態クラスから呼ばれる）
	void ChangeState(PlayerState* newState);
	PlayerState* GetState() const { return state_; }
	const char* GetStateName() const;
	bool IsFlying() const;

	// --- 状態クラスから呼ばれる処理（public: 状態が Player のデータを更新する） ---
	void UpdateBullets();
	void ProcessDodgeInput();
	void UpdateRotationNormal();
	bool UpdateRotationRolling();
	void UpdateHitShake();
	void FinalizeFrameUpdate();
	void UpdateGameOverAnimation();
	void BeginRolling(float direction);

	void ClampToPlayArea();

private:
	PlayerState* state_ = nullptr;

	KamataEngine::WorldTransform worldtransfrom_;
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Model* modelArrow_ = nullptr;
	KamataEngine::Model* modelPutter_ = nullptr;
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
	int ballRestSoundHandle_ = 0;
	int ballRestSound_ = -1;

	KamataEngine::Model* modelParticle_ = nullptr;
	ParticleEmitter* engineExhaust_ = nullptr;
	ParticleEmitter* landingEmitter_ = nullptr;
	ParticleEmitter* trailEmitter_ = nullptr;

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

	// --- ゴルフ用 ---
	// ボールの速度ベクトル
	KamataEngine::Vector3 velocity_ = {0.0f, 0.0f, 0.0f};
	// 地面の高さ（ローカルY。これより下には行かない）
	float groundY_ = -6.0f;
	// 下向きの重力加速度
	const float kGravity_ = 0.015f;
	// バウンドの反発係数（1.0 = 完全弾性）
	const float kBounceRestitution_ = 0.45f;
	// 転がり摩擦（毎フレーム水平速度にかける係数）
	const float kRollFriction_ = 0.92f;

	// --- パター ---
	KamataEngine::WorldTransform putterTransform_;
	float swingTimer_ = 0.0f;
	const float kSwingDuration_ = 18.0f;

	// 打撃速度（高さ角度と組み合わせて使う）
	// ゲージMAX での最大合成初速
	const float kShotSpeedZ_    = 3.0f;
	const float kShotSpeedZMin_ = 0.7f;

	// ゲージ関連
	const float kGaugeSpeed_ = 0.07f;
	float gaugeTimer_ = 0.0f;
	float gaugePower_ = 0.0f;

	// 水平照準（矢印が ±90°を往復して方向を決める）
	const float kAimMaxAngle_    = 3.14159265f * 0.5f;  // 通常: ±90°
	const float kAimMaxAngleAir_ = 3.14159265f * 0.85f; // 空中: ±153°（より広い範囲）
	const float kAimSpeed_       = 0.045f;
	float aimTimer_           = 0.0f;
	float aimAngle_           = 0.0f;
	float aimBaseYaw_         = 0.0f; // 照準開始時のカメラヨー角（矢印の基準方向）
	float cameraYaw_          = 0.0f; // 毎フレーム GameScene からセットされるカメラヨー
	float lockedArrowAngle_   = 0.0f;
	KamataEngine::WorldTransform arrowTransform_;

	// 高さ照準（矢印がロフト角を往復して高さを決める）
	const float kHeightMaxAngle_    = 3.14159265f * 65.0f / 180.0f;  // 65° 上向き
	const float kHeightMinAngle_    = 0.0f;                           // 0° = 水平（転がし）
	const float kHeightAirMinAngle_ = -3.14159265f * 70.0f / 180.0f; // 空中: -70° 下向き
	const float kHeightSpeed_       = 0.04f;
	bool isAirAiming_ = false;           // true のとき空中で照準中（重力無効・角度範囲拡張）
	const int kMaxAirShots_ = 4;         // 空中で打ち直せる最大回数
	int airShotsRemaining_ = kMaxAirShots_; // 残り空中打ち直し回数（着地でリセット）

	// ボール半透明描画用（ゴールと重なったとき）
	KamataEngine::ObjectColor ballObjectColor_;
	float heightTimer_           = 0.0f;
	float heightAngle_           = 3.14159265f * 30.0f / 180.0f; // 現在のロフト角
	float lockedHeightAngle_     = 3.14159265f * 30.0f / 180.0f; // 確定ロフト角
	KamataEngine::WorldTransform arrowHeightTransform_;

	// ゴール方向インジケーター矢印（常にゴールを指す）
	KamataEngine::WorldTransform goalDirArrowTransform_;
	KamataEngine::Vector3 goalPosition_ = {0.0f, 10.0f, 1200.0f};
	float playAreaRadius_ = 2000.0f; // ゴール中心 XZ 円の半径

	static constexpr float kTrailDrawAlpha_      = 0.1f;
	static constexpr float kTrailDotLife_        = 22.0f;
	static constexpr float kTrailCrossScale_     = 0.65f;
	static constexpr float kTrailJoinOverlap_    = 0.55f;
	static constexpr float kTrailJoinOverlapRate_= 0.52f;
	static constexpr float kTrailInsetMin_       = 1.70f;
	static constexpr float kTrailInsetMax_       = 3.0f;
	static constexpr float kTrailInsetSpeedMul_  = 0.55f;

	KamataEngine::Vector3 trailPrevPos_ = {0.0f, 0.0f, 0.0f};
	bool trailHasPrevPos_ = false;
	float trailSmoothedInset_ = 1.70f;
};
