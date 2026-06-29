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
	float GetJumpSpringChargeLevel() const { return springChargeLevel_; }
	const KamataEngine::Vector3& GetJumpSpringAnchor() const { return springChargeAnchor_; }
	float GetJumpSpringCircleRadiusWorld() const;

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

	void HandleSpikeCollision(KamataEngine::Vector3& pos);
	void RespawnToSpawn(KamataEngine::Vector3& pos);

	bool UpdateSpringCharge(KamataEngine::Vector3& pos);
	void BeginSpringPenetration(int springIndex, SpringChargeKind kind, const KamataEngine::Vector3& pos);
	void BeginSpringPauseAt(const KamataEngine::Vector3& pos);
	void LaunchFromSpringCharge(bool useCharge);
	const TrampolineSpring* GetActiveSpring() const;
	TrampolineSpring* GetActiveSpringMutable();

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

	static constexpr float kMoveSpeed = 6.5f;
	static constexpr float kGravity = 0.6f;
	static constexpr float kJumpSpeed = 12.0f;
	static constexpr float kSpringPauseDuration = 0.3f;
	static constexpr float kSpringMaxChargeTime = 1.2f;
	static constexpr float kSpringMinLaunchSpeed = 7.0f;
	static constexpr float kSpringMaxLaunchSpeed = 28.0f;
	static constexpr float kSideSpringMinLaunchSpeed = 7.0f;
	static constexpr float kSideSpringMaxLaunchSpeed = 24.0f;
	static constexpr float kSpringPenetrationSpeed = 3.5f;
	static constexpr int kSpikeRespawnInvulnFrames = 45;
};
