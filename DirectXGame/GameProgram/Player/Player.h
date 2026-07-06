#pragma once
#include "AABB.h"
#include "EnemyBullet.h"
#include "GameCharacter.h"
#include "KamataEngine.h"
#include "ParticleEmitter.h"
#include "PlayerBullet.h"
#include "PlayerState.h"
#include <list>
#include <vector>

using namespace KamataEngine;

class Enemy;
class EntityFactory;
class RailCamera;
class TileMap;
class TrampolineSpring;

struct PlayerSnapshot;

// ゴール吸い込み演出の種類
enum class PortalAbsorptionStyle {
	PlayerSpin,  // 自機を軸にくるくる回転しながら吸い込み
	OrbitSpiral, // ポータル中心の周りを旋回、円が縮小しながら吸い込み
};

// プレイヤー（GameCharacter を継承。行動は State Pattern で切り替え）
class Player : public GameCharacter {
public:
	Player();
	~Player() override;

	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& pos);
	void Update();
	void Draw();
	void Attack();
	void UpdateMovement();

	enum class SpringChargePhase { None, Penetrating, Pause, Charging };
	enum class SpringChargeKind { Up, Right, Left, Down };

	SpringChargePhase GetSpringChargePhase() const { return springChargePhase_; }
	SpringChargeKind GetSpringChargeKind() const { return springChargeKind_; }
	float GetJumpSpringChargeLevel() const { return springChargeLevel_; }
	const KamataEngine::Vector3& GetJumpSpringAnchor() const { return springChargeAnchor_; }
	float GetJumpSpringCircleRadiusWorld() const;
	bool ShouldShowSpringTrajectory() const;
	float GetSpringTrajectoryPreviewCharge() const;
	static constexpr int kSpringTrajectoryMaxSamples = 48;
	bool ComputeSpringTrajectorySamples(KamataEngine::Vector3* outSamples, int maxSamples, int& outCount) const;
	KamataEngine::Vector3 GetSpringTrajectoryStart() const;

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
	void TeleportForScreenWrap(const KamataEngine::Vector3& position, bool preserveVelocity);
	bool IsSpikeInvulnerable() const { return spikeRespawnCooldown_ > 0; }
	bool IsSideSpringFlying() const { return isSideSpringFlying_; }
	const KamataEngine::Vector3& GetLocalPosition() const { return worldtransfrom_.translation_; }
	void RefreshWorldMatrix() { worldtransfrom_.UpdateMatrix(); }

	void ResetStats();

	// ゲームオーバー演出用タイマー（Dead 状態で使用）
	void SetGameOverAnimationTime(float time) { gameOverAnimationTime_ = time; }

	AABB GetAABB();
	const std::list<PlayerBullet*>& GetBullets() const { return bullets_; }

	void SetParent(const KamataEngine::WorldTransform* parent);
	void SetRailCamera(RailCamera* camera);
	void SetTileMap(TileMap* tileMap);
	void SetTrampolineSprings(std::vector<TrampolineSpring>* springs) { trampolineSprings_ = springs; }
	void SetEnemies(std::list<Enemy*>* enemies) { enemies_ = enemies; }
	void SetEntityFactory(EntityFactory* factory) { entityFactory_ = factory; }

	float GetHalfWidth() const { return halfWidth_; }
	float GetHalfHeight() const { return halfHeight_; }
	bool IsMovingInput() const;
	void SetVelocityY(float velocityY) { velocityY_ = velocityY; }
	void SetVelocityX(float velocityX) { velocityX_ = velocityX; }

	void SetVisualModel(KamataEngine::Model* model);
	void SetSpawnPosition(const KamataEngine::Vector3& pos);
	const KamataEngine::Vector3& GetSpawnPosition() const { return spawnPosition_; }

	void ResetRotation();
	void ResetVisualScaleFromTileMap();
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
	void BeginPortalAbsorption(const KamataEngine::Vector3& portalCenter, PortalAbsorptionStyle style);
	bool UpdatePortalAbsorption();
	bool IsPortalAbsorbing() const { return isPortalAbsorbing_; }
	PortalAbsorptionStyle GetPortalAbsorptionStyle() const { return portalAbsorbStyle_; }
	const KamataEngine::Vector3& GetPortalAbsorbCenter() const { return portalAbsorbCenter_; }
	void BeginRolling(float direction);

	void HandleSpikeCollision(KamataEngine::Vector3& pos);
	void RespawnToSpawn(KamataEngine::Vector3& pos);
	bool ConsumeSpikeHitEvent();

	void CaptureSnapshot(PlayerSnapshot& outSnapshot) const;
	void ApplySnapshot(const PlayerSnapshot& snapshot);

	bool UpdateSpringCharge(KamataEngine::Vector3& pos);
	void BeginSpringPenetration(int springIndex, SpringChargeKind kind, const KamataEngine::Vector3& pos);
	void BeginSpringPauseAt(const KamataEngine::Vector3& pos);
	void LaunchFromSpringCharge(bool useCharge);
	const TrampolineSpring* GetActiveSpring() const;
	TrampolineSpring* GetActiveSpringMutable();
	void GetSpringTrajectoryOrigin(KamataEngine::Vector3& outOrigin) const;

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
	EntityFactory* entityFactory_ = nullptr;

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

	TileMap* tileMap_ = nullptr;
	std::vector<TrampolineSpring>* trampolineSprings_ = nullptr;

	float velocityX_ = 0.0f;
	float velocityY_ = 0.0f;
	bool onGround_ = false;
	float halfWidth_ = 14.0f;
	float halfHeight_ = 14.0f;
	int spikeRespawnCooldown_ = 0;

	SpringChargePhase springChargePhase_ = SpringChargePhase::None;
	SpringChargeKind springChargeKind_ = SpringChargeKind::Up;
	int activeSpringIndex_ = -1;
	float springChargePauseTimer_ = 0.0f;
	float springChargeLevel_ = 0.0f;
	KamataEngine::Vector3 springChargeAnchor_ = {};
	KamataEngine::Vector3 springPenetrationPrevPos_ = {};
	bool isSideSpringFlying_ = false;
	KamataEngine::Vector3 spawnPosition_ = {};

	static constexpr float kMoveSpeed = 6.5f * 1.224744871391589f;  // 重力1.5倍時の滞空距離補正 (√1.5)
	static constexpr float kGravity = 0.6f * 1.5f;
	static constexpr float kJumpSpeed = 12.0f * 1.4491374849946407f; // 高さ1.4倍＋重力1.5倍補正 (√2.1)
	static constexpr float kPlayerVisualScale = 2.0f;
	static constexpr float kSpringPauseDuration = 0.3f;
	static constexpr float kSpringMaxChargeTime = 1.2f;
	static constexpr float kSpringMinLaunchSpeed = 7.0f;
	static constexpr float kSpringMaxLaunchSpeed = 28.0f * 1.1f;
	static constexpr float kSideSpringMinLaunchSpeed = 7.0f;
	static constexpr float kSideSpringMaxLaunchSpeed = 24.0f;
	static constexpr float kSpringPenetrationSpeed = 3.5f * 1.3f;
	static constexpr int kSpikeRespawnInvulnFrames = 45;
	static constexpr float kPortalAbsorbDuration = 105.0f;

	bool isPortalAbsorbing_ = false;
	PortalAbsorptionStyle portalAbsorbStyle_ = PortalAbsorptionStyle::PlayerSpin;
	float portalAbsorbTimer_ = 0.0f;
	KamataEngine::Vector3 portalAbsorbCenter_ = {};
	KamataEngine::Vector3 portalAbsorbStartPos_ = {};
	KamataEngine::Vector3 portalAbsorbStartScale_ = {};
	float portalAbsorbStartRotZ_ = 0.0f;
	float portalAbsorbSpinZ_ = 0.0f;
	float portalAbsorbStartRadius_ = 0.0f;
	float portalAbsorbStartAngle_ = 0.0f;

	bool spikeHitEvent_ = false;

	bool UpdatePortalAbsorptionPlayerSpin(float t, float ease, float oneMinusEase);
	bool UpdatePortalAbsorptionOrbitSpiral(float t, float ease, float oneMinusEase);

	void GetSpringPreviewVelocity(float chargeLevel, float& outVelX, float& outVelY) const;
};

struct PlayerSnapshot {
	KamataEngine::Vector3 position{};
	KamataEngine::Vector3 spawnPosition{};
	KamataEngine::Vector3 scale{1.0f, 1.0f, 1.0f};
	KamataEngine::Vector3 rotation{};
	float velocityX = 0.0f;
	float velocityY = 0.0f;
	bool onGround = true;
	int hp = 3;
	int spikeRespawnCooldown = 0;
	Player::SpringChargePhase springChargePhase = Player::SpringChargePhase::None;
	Player::SpringChargeKind springChargeKind = Player::SpringChargeKind::Up;
	int activeSpringIndex = -1;
	float springChargePauseTimer = 0.0f;
	float springChargeLevel = 0.0f;
	KamataEngine::Vector3 springChargeAnchor{};
	KamataEngine::Vector3 springPenetrationPrevPos{};
	bool isSideSpringFlying = false;
	bool isPortalAbsorbing = false;
	float portalAbsorbTimer = 0.0f;
	PortalAbsorptionStyle portalAbsorbStyle = PortalAbsorptionStyle::PlayerSpin;
	KamataEngine::Vector3 portalAbsorbCenter{};
	KamataEngine::Vector3 portalAbsorbStartPos{};
	KamataEngine::Vector3 portalAbsorbStartScale{1.0f, 1.0f, 1.0f};
	float portalAbsorbStartRotZ = 0.0f;
	float portalAbsorbSpinZ = 0.0f;
	float portalAbsorbStartRadius = 0.0f;
	float portalAbsorbStartAngle = 0.0f;
	bool isDead = false;
};
