#pragma once
#include "AABB.h"
#include "EnemyBullet.h"
#include "GameCharacter.h"
#include "KamataEngine.h"
#include "PlayerBullet.h"
#include "PlayerState.h"
#include <list>
#include <vector>

using namespace KamataEngine;

class Enemy;
class RailCamera;

// Stickman Hook 風: 空中アンカーに糸を繋いで右へ進む
class Player : public GameCharacter {
public:
	Player();
	~Player() override;

	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& pos);
	void Update();
	void Draw();
	void Attack() {}

	KamataEngine::Vector3 GetWorldPosition() const override;
	bool IsDead() const override { return isDead_; }
	void OnCollision() override;
	int GetHp() const override { return hp_; }
	int GetMaxHp() const override { return kMaxHp_; }
	float GetCollisionRadius() const override { return 1.0f; }
	const char* GetKindName() const override { return "Player"; }

	void SetPosition(const KamataEngine::Vector3& position);
	const KamataEngine::Vector3& GetLocalPosition() const { return worldtransfrom_.translation_; }
	void RefreshWorldMatrix();

	void ResetStats();
	void SetGameOverAnimationTime(float time) { gameOverAnimationTime_ = time; }

	AABB GetAABB();
	const std::list<PlayerBullet*>& GetBullets() const { return bullets_; }

	void SetParent(const KamataEngine::WorldTransform* parent);
	void SetRailCamera(RailCamera* camera);
	void SetCameraYaw(float) {}
	void SetGoalPosition(const KamataEngine::Vector3&) {}
	const KamataEngine::Vector3& GetGoalPosition() const { return dummyGoal_; }
	void SetPlayAreaRadius(float) {}
	void SetEnemies(std::list<Enemy*>*) {}
	void SetGroundY(float) {}

	void ResetRotation() {}
	void ResetParticles() {}
	void ResetBullets();

	void BeginSwingFromWaiting();
	void UpdatePendulum();
	void CutRopeAndFly();
	void TryAttachNewAnchor();
	void UpdateFreeFlight();
	void Fail();
	bool ConsumeRestartRequest();
	bool IsAnchorOnScreen() const;
	bool IsWorldPosOnScreen(const KamataEngine::Vector3& worldPos) const;

	bool IsSpaceJustPressed() const;
	bool IsSpaceHeld() const;
	bool WasSpaceReleased() const;

	KamataEngine::Vector3 GetCameraFocusPosition() const;
	float GetProgressZ() const;
	// プレイヤー本体
	KamataEngine::Vector3 GetBobPosition() const { return playerPos_; }
	// いま繋がっているアンカー（未接続時は最近アンカー）
	KamataEngine::Vector3 GetAnchorBallPosition() const;
	float GetBallScale() const { return kBallScale_; }

	bool IsFlying() const;
	bool IsWaiting() const;
	bool IsSwinging() const;
	bool IsGauging() const { return false; }
	bool IsAirAiming() const { return false; }
	int GetAirShotsRemaining() const { return 0; }
	float GetGaugePower() const { return 0.0f; }

	bool IsRolling() const { return false; }
	void ChangeState(PlayerState* newState);
	PlayerState* GetState() const { return state_; }
	const char* GetStateName() const;
	void UpdateGameOverAnimation();

	bool IsFlyerOnScreen() const;

	static inline const float kWidth = 1.0f;
	static inline const float kHeight = 1.0f;

private:
	void SyncTransforms();
	void ApplyPlayerFromAngle();
	void ComputeAngleFromPositions();
	void UpdateRopeTransform();
	void UpdatePlayerVelocityFromAngle();
	void ClampAngularVelToLinearSpeed();
	void EnsureAnchorsAhead();
	void RebuildAnchorListFromStart();
	int FindNearestAnchorIndex() const;
	float MakeAnchorY(int index) const;

	PlayerState* state_ = nullptr;

	KamataEngine::WorldTransform worldtransfrom_;
	KamataEngine::WorldTransform playerTransform_;
	KamataEngine::WorldTransform ropeTransform_;
	// anchorDrawTransform_ は配列に置き換え（1つ使い回しだと描画が潰れる）

	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Model* modelRope_ = nullptr;
	KamataEngine::Model* modelAnchor_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::Input* input_ = nullptr;
	RailCamera* railCamera_ = nullptr;

	std::list<PlayerBullet*> bullets_;

	static const int kMaxHp_ = 3;
	int hp_ = kMaxHp_;
	bool isDead_ = false;
	float gameOverAnimationTime_ = 0.0f;

	KamataEngine::Vector3 playerPos_ = {};
	KamataEngine::Vector3 playerVel_ = {};
	KamataEngine::Vector3 anchorPos_ = {};
	int attachedAnchorIndex_ = -1;

	std::vector<KamataEngine::Vector3> anchors_;
	int nextAnchorIndex_ = 0;
	float nextAnchorZ_ = 0.0f;

	float ropeLength_ = 12.0f;
	float angle_ = 0.0f; // 0=真下、+ = 右（+Z）
	float angularVel_ = 0.0f;
	bool ropeConnected_ = true;

	const float kGravity_ = 0.045f;
	const float kPendulumGravity_ = 0.055f;
	const float kPendulumDamping_ = 0.9988f;
	const float kLaunchSpeedScale_ = 1.15f;
	const float kDefaultRopeLength_ = 21.0f;
	const float kBallScale_ = 1.2f;
	const float kAnchorScale_ = 4.0f;
	const float kRopeThickness_ = 0.10f;
	const float kRopeVisualLengthScale_ = 0.5f;
	const float kStartHangAngle_ = -1.20f;
	const float kStartAngularVel_ = 0.0f;
	const float kAttachKickAngularVel_ = 0.0f;
	const float kSwingLinearAccel_ = 0.0021f * kDefaultRopeLength_; // AD 2倍
	const float kMaxLinearSpeed_ = 0.12f * kDefaultRopeLength_;
	const float kAnchorSpacingZ_ = 120.0f;
	const float kAnchorBaseY_ = 18.0f; // 開始時の仮位置用
	const float kAnchorYMin_ = 28.0f;  // 上寄りの出現帯
	const float kAnchorYMax_ = 100.0f;
	const float kAnchorGenAhead_ = 500.0f;
	const float kAnchorCullBehind_ = 250.0f;
	static const int kMaxAnchorDraw_ = 40;
	KamataEngine::WorldTransform anchorDrawTransforms_[kMaxAnchorDraw_];
	bool anchorDrawReady_ = false;

	bool spaceWasHeld_ = false;
	bool restartRequested_ = false;
	KamataEngine::Vector3 dummyGoal_ = {0.0f, 0.0f, 0.0f};

	KamataEngine::ObjectColor playerTint_;
	KamataEngine::ObjectColor anchorTint_;
	KamataEngine::ObjectColor attachedTint_;
	bool colorsReady_ = false;
};
