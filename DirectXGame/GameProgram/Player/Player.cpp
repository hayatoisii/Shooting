#include "Player.h"
#include "EntityFactory.h"
#include "GameBalanceAccess.h"
#include "Enemy.h"
#include "PlayerState.h"
#include "RailCamera.h"
#include "TileMap.h"
#include "TrampolineSpring.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <dinput.h>
// For window size constants
#include "base/WinApp.h"
#include <Windows.h>
#include <cstdio>
#include <vector>

Player::Player() { state_ = PlayerStateNormal::Instance(); }

Player::~Player() {
	delete modelbullet_;
	delete modelParticle_;
	delete engineExhaust_;
	for (PlayerBullet* bullet : bullets_) {
		delete bullet;
	}
}

void Player::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& pos) {
	assert(model);
	model_ = model;
	camera_ = camera;
	modelbullet_ = KamataEngine::Model::CreateFromOBJ("Bullet", true);
	worldtransfrom_.translation_ = pos;
	worldtransfrom_.translation_.z = 1.0f;
	velocityX_ = 0.0f;
	velocityY_ = 0.0f;
	onGround_ = true;
	input_ = KamataEngine::Input::GetInstance();
	audio_ = KamataEngine::Audio::GetInstance();
	if (audio_)
		hitPlayerSoundHandle_ = audio_->LoadWave("./sound/parry.wav");


	worldtransfrom_.Initialize();

	modelParticle_ = KamataEngine::Model::CreateFromOBJ("flare", true);
	engineExhaust_ = new ParticleEmitter();
	engineExhaust_->Initialize(modelParticle_);

	hp_ = kMaxHp_;
	isDead_ = false;
	shotTimer_ = 0;

	hitShakePrevVerticalOffset_ = 0.0f;
	hitShakePrevHorizontalOffset_ = 0.0f;

	// 初期状態は通常飛行
	ChangeState(PlayerStateNormal::Instance());
}

void Player::SetPosition(const KamataEngine::Vector3& position) {
	worldtransfrom_.translation_ = position;
	worldtransfrom_.translation_.z = 1.0f;
	velocityX_ = 0.0f;
	velocityY_ = 0.0f;
	spikeRespawnCooldown_ = 0;

	if (tileMap_) {
		tileMap_->ClampPositionToMapBounds(worldtransfrom_.translation_.x, worldtransfrom_.translation_.y, halfWidth_, halfHeight_);
	}
}

void Player::TeleportForScreenWrap(const KamataEngine::Vector3& position, bool preserveVelocity) {
	worldtransfrom_.translation_ = position;
	worldtransfrom_.translation_.z = 1.0f;

	if (!preserveVelocity) {
		velocityX_ = 0.0f;
		velocityY_ = 0.0f;
	}

	if (tileMap_) {
		tileMap_->ClampPositionToMapBounds(worldtransfrom_.translation_.x, worldtransfrom_.translation_.y, halfWidth_, halfHeight_);
	}
}

void Player::ResetStats() {
	hp_ = kMaxHp_;
	isDead_ = false;
	spikeRespawnCooldown_ = 0;
	springChargePhase_ = SpringChargePhase::None;
	activeSpringIndex_ = -1;
	springChargePauseTimer_ = 0.0f;
	springChargeLevel_ = 0.0f;
	isSideSpringFlying_ = false;
	gameOverAnimationTime_ = 0.0f;
	rollTimer_ = 0.0f;
	// タイトル復帰時は通常状態へ
	ChangeState(PlayerStateNormal::Instance());
}

void Player::ChangeState(PlayerState* newState) {
	if (newState == nullptr || newState == state_) {
		return;
	}
	state_ = newState;
}

bool Player::IsRolling() const {
	return state_ != nullptr && state_->IsRolling();
}

const char* Player::GetStateName() const {
	return state_ ? state_->GetStateName() : "None";
}

// ポリモーフィズム: Player 固有の被弾処理（GameCharacter::OnCollision の override）
void Player::OnCollision() {
	// isDead_ = true; // 即死テスト用（通常は HP を減らす）
	hp_--;
	if (hp_ <= 0) {
		isDead_ = true;
	}

	// 被弾時に左右に揺れる
	hitShakeTime_ = 0.0f;
	hitShakeAmplitude_ = 0.6f;           // 最大振幅
	hitShakeVerticalAmplitude_ = 1.5f;   // 垂直
	hitShakeHorizontalAmplitude_ = 1.0f; // 水平
}

void Player::Attack() {
	// 2Dマップ移動モードでは発射しない
}

const TrampolineSpring* Player::GetActiveSpring() const {
	if (!trampolineSprings_ || activeSpringIndex_ < 0 || activeSpringIndex_ >= static_cast<int>(trampolineSprings_->size())) {
		return nullptr;
	}
	return &(*trampolineSprings_)[static_cast<size_t>(activeSpringIndex_)];
}

TrampolineSpring* Player::GetActiveSpringMutable() {
	if (!trampolineSprings_ || activeSpringIndex_ < 0 || activeSpringIndex_ >= static_cast<int>(trampolineSprings_->size())) {
		return nullptr;
	}
	return &(*trampolineSprings_)[static_cast<size_t>(activeSpringIndex_)];
}

void Player::BeginSpringPenetration(int springIndex, SpringChargeKind kind, const KamataEngine::Vector3& pos) {
	activeSpringIndex_ = springIndex;
	springChargeKind_ = kind;
	springChargePhase_ = SpringChargePhase::Penetrating;
	springChargeLevel_ = 0.0f;
	onGround_ = false;
	springPenetrationPrevPos_ = pos;

	TrampolineSpring* spring = GetActiveSpringMutable();
	if (!spring) {
		return;
	}

	if (kind == SpringChargeKind::Up || kind == SpringChargeKind::Down) {
		velocityX_ = 0.0f;
		if (!spring->IsPlayerCenterInStopZone(pos.x, pos.y)) {
			const float centerY = spring->GetCenter().y;
			if (centerY > pos.y + 0.5f) {
				velocityY_ = kSpringPenetrationSpeed;
			} else if (centerY < pos.y - 0.5f) {
				velocityY_ = -kSpringPenetrationSpeed;
			} else {
				velocityY_ = 0.0f;
			}
		} else {
			velocityY_ = 0.0f;
		}
	} else {
		if (!spring->IsPlayerCenterInStopZone(pos.x, pos.y)) {
			const float centerX = spring->GetCenter().x;
			if (centerX > pos.x + 0.5f) {
				velocityX_ = kSpringPenetrationSpeed;
			} else if (centerX < pos.x - 0.5f) {
				velocityX_ = -kSpringPenetrationSpeed;
			} else {
				velocityX_ = 0.0f;
			}
		} else {
			velocityX_ = 0.0f;
		}
		velocityY_ = 0.0f;
	}
}

void Player::BeginSpringPauseAt(const KamataEngine::Vector3& pos) {
	springChargeAnchor_ = pos;
	springChargeAnchor_.z = 1.0f;
	springChargePhase_ = SpringChargePhase::Pause;
	springChargePauseTimer_ = kSpringPauseDuration;
	velocityX_ = 0.0f;
	velocityY_ = 0.0f;
}

void Player::LaunchFromSpringCharge(bool useCharge) {
	const SpringChargeKind kind = springChargeKind_;
	springChargePhase_ = SpringChargePhase::None;
	activeSpringIndex_ = -1;

	const float t = useCharge ? std::clamp(springChargeLevel_, 0.0f, 1.0f) : 0.0f;

	switch (kind) {
	case SpringChargeKind::Up:
		velocityY_ = useCharge ? (kSpringMinLaunchSpeed + (kSpringMaxLaunchSpeed - kSpringMinLaunchSpeed) * t) : TrampolineSpring::kBounceSpeedY;
		velocityX_ = 0.0f;
		isSideSpringFlying_ = false;
		break;
	case SpringChargeKind::Right:
		velocityX_ = useCharge ? (kSideSpringMinLaunchSpeed + (kSideSpringMaxLaunchSpeed - kSideSpringMinLaunchSpeed) * t) : TrampolineSpring::kSideBounceSpeedX;
		velocityY_ = TrampolineSpring::kSideBounceLiftY;
		isSideSpringFlying_ = true;
		break;
	case SpringChargeKind::Left:
		velocityX_ = useCharge ? -(kSideSpringMinLaunchSpeed + (kSideSpringMaxLaunchSpeed - kSideSpringMinLaunchSpeed) * t) : -TrampolineSpring::kSideBounceSpeedX;
		velocityY_ = TrampolineSpring::kSideBounceLiftY;
		isSideSpringFlying_ = true;
		break;
	case SpringChargeKind::Down:
		velocityY_ = -(useCharge ? (kSpringMinLaunchSpeed + (kSpringMaxLaunchSpeed - kSpringMinLaunchSpeed) * t) : TrampolineSpring::kDownBounceSpeedY);
		velocityX_ = 0.0f;
		isSideSpringFlying_ = false;
		break;
	}

	onGround_ = false;
}

void Player::RespawnToSpawn(KamataEngine::Vector3& pos) {
	pos = spawnPosition_;
	velocityX_ = 0.0f;
	velocityY_ = 0.0f;
	onGround_ = true;
	isSideSpringFlying_ = false;
	springChargePhase_ = SpringChargePhase::None;
	activeSpringIndex_ = -1;
	springChargeLevel_ = 0.0f;
}

void Player::HandleSpikeCollision(KamataEngine::Vector3& pos) {
	if (spikeRespawnCooldown_ > 0 || !tileMap_) {
		return;
	}

	if (!tileMap_->OverlapsSpike(pos.x, pos.y, halfWidth_, halfHeight_)) {
		return;
	}

	RespawnToSpawn(pos);
	spikeRespawnCooldown_ = kSpikeRespawnInvulnFrames;
}

bool Player::UpdateSpringCharge(KamataEngine::Vector3& pos) {
	const float kDeltaSec = 1.0f / 60.0f;
	const TrampolineSpring* spring = GetActiveSpring();

	if (springChargePhase_ == SpringChargePhase::Penetrating) {
		if (!spring) {
			springChargePhase_ = SpringChargePhase::None;
			return false;
		}

		const float prevX = springPenetrationPrevPos_.x;
		const float prevY = springPenetrationPrevPos_.y;

		if (springChargeKind_ == SpringChargeKind::Up || springChargeKind_ == SpringChargeKind::Down) {
			if (spring && !spring->IsPlayerCenterInStopZone(pos.x, pos.y)) {
				const float centerY = spring->GetCenter().y;
				if (centerY > pos.y + 0.5f) {
					velocityY_ = kSpringPenetrationSpeed;
				} else if (centerY < pos.y - 0.5f) {
					velocityY_ = -kSpringPenetrationSpeed;
				}
			}
			velocityX_ = 0.0f;
		} else {
			if (spring && !spring->IsPlayerCenterInStopZone(pos.x, pos.y)) {
				const float centerX = spring->GetCenter().x;
				if (centerX > pos.x + 0.5f) {
					velocityX_ = kSpringPenetrationSpeed;
				} else if (centerX < pos.x - 0.5f) {
					velocityX_ = -kSpringPenetrationSpeed;
				}
			}
			velocityY_ = 0.0f;
		}
		pos.x += velocityX_;
		tileMap_->ResolveCollisionX(pos.x, pos.y, halfWidth_, halfHeight_);

		pos.y += velocityY_;
		bool landed = false;
		tileMap_->ResolveCollisionY(pos.y, pos.x, halfWidth_, halfHeight_, velocityY_, landed);
		if (landed) {
			velocityY_ = 0.0f;
			if (springChargeKind_ == SpringChargeKind::Right || springChargeKind_ == SpringChargeKind::Left) {
				onGround_ = false;
			} else {
				velocityX_ = 0.0f;
				onGround_ = true;
			}
		} else {
			onGround_ = false;
		}

		if (!spring->IsPlayerOverlapping(pos.x, pos.y, halfWidth_, halfHeight_)) {
			TrampolineSpring* mutableSpring = GetActiveSpringMutable();
			if (mutableSpring) {
				mutableSpring->ResetPlayerContact();
			}
			springChargePhase_ = SpringChargePhase::None;
			activeSpringIndex_ = -1;
			return false;
		}

		if (spring->DidEnterOrCrossStopZone(prevX, prevY, pos.x, pos.y)) {
			BeginSpringPauseAt(pos);
			springPenetrationPrevPos_ = pos;
			return true;
		}

		springPenetrationPrevPos_ = pos;
		HandleSpikeCollision(pos);
		return true;
	}

	if (springChargePhase_ == SpringChargePhase::Pause || springChargePhase_ == SpringChargePhase::Charging) {
		pos = springChargeAnchor_;
		velocityX_ = 0.0f;
		velocityY_ = 0.0f;

		if (springChargePhase_ == SpringChargePhase::Pause) {
			springChargePauseTimer_ -= kDeltaSec;
			if (input_->PushKey(DIK_SPACE)) {
				springChargePhase_ = SpringChargePhase::Charging;
				springChargeLevel_ = 0.0f;
				return true;
			}
			if (springChargePauseTimer_ <= 0.0f) {
				LaunchFromSpringCharge(false);
				return false;
			}
			return true;
		}

		if (input_->PushKey(DIK_SPACE)) {
			springChargeLevel_ += kDeltaSec / kSpringMaxChargeTime;
			springChargeLevel_ = std::clamp(springChargeLevel_, 0.0f, 1.0f);
			return true;
		}

		LaunchFromSpringCharge(true);
		return false;
	}

	return false;
}

float Player::GetJumpSpringCircleRadiusWorld() const {
	const float maxRadius = halfWidth_ * 2.0f;
	const float minRadius = halfWidth_ * 0.35f;
	return maxRadius + (minRadius - maxRadius) * std::clamp(springChargeLevel_, 0.0f, 1.0f);
}

void Player::UpdateMovement() {
	if (!tileMap_ || !input_) {
		return;
	}

	KamataEngine::Vector3 pos = worldtransfrom_.translation_;

	if (spikeRespawnCooldown_ > 0) {
		spikeRespawnCooldown_--;
		RespawnToSpawn(pos);
		worldtransfrom_.translation_ = pos;
		worldtransfrom_.translation_.z = 1.0f;
		worldtransfrom_.rotation_ = {0.0f, 0.0f, 0.0f};
		worldtransfrom_.UpdateMatrix();
		return;
	}

	bool launchedFromSpringThisFrame = false;
	if (springChargePhase_ != SpringChargePhase::None) {
		const SpringChargePhase phaseBefore = springChargePhase_;
		if (UpdateSpringCharge(pos)) {
			worldtransfrom_.translation_ = pos;
			worldtransfrom_.translation_.z = 1.0f;
			worldtransfrom_.rotation_ = {0.0f, 0.0f, 0.0f};
			return;
		}
		launchedFromSpringThisFrame = phaseBefore == SpringChargePhase::Pause || phaseBefore == SpringChargePhase::Charging;
	}

	float moveX = 0.0f;
	const bool canUseMoveInput = !launchedFromSpringThisFrame && !isSideSpringFlying_;
	if (canUseMoveInput) {
		if (input_->PushKey(DIK_A)) {
			moveX -= kMoveSpeed;
		}
		if (input_->PushKey(DIK_D)) {
			moveX += kMoveSpeed;
		}
	}

	if (canUseMoveInput && input_->TriggerKey(DIK_W) && onGround_ && springChargePhase_ == SpringChargePhase::None) {
		velocityY_ = kJumpSpeed;
		onGround_ = false;
	}

	velocityY_ -= kGravity;

	const float prevX = pos.x;
	const float prevY = pos.y;
	const float approachVelX = velocityX_ + moveX;

	pos.x += approachVelX;
	tileMap_->ResolveCollisionX(pos.x, pos.y, halfWidth_, halfHeight_);

	pos.y += velocityY_;
	const float yAfterMove = pos.y;

	bool landed = false;
	tileMap_->ResolveCollisionY(pos.y, pos.x, halfWidth_, halfHeight_, velocityY_, landed);
	if (landed) {
		velocityY_ = 0.0f;
		velocityX_ = 0.0f;
		onGround_ = true;
		isSideSpringFlying_ = false;
	} else if (velocityY_ > 0.0f && pos.y < yAfterMove) {
		velocityY_ = 0.0f;
		onGround_ = false;
	} else {
		onGround_ = false;
	}

	if (springChargePhase_ == SpringChargePhase::None && !launchedFromSpringThisFrame && trampolineSprings_) {
		for (size_t i = 0; i < trampolineSprings_->size(); ++i) {
			TrampolineSpring& spring = (*trampolineSprings_)[i];
			const TrampolineBounceResult result = spring.TryBounce(prevX, prevY, pos.x, pos.y, halfWidth_, halfHeight_);
			if (result == TrampolineBounceResult::EnterUp) {
				BeginSpringPenetration(static_cast<int>(i), SpringChargeKind::Up, pos);
				if (spring.DidEnterOrCrossStopZone(pos.x, pos.y, pos.x, pos.y)) {
					BeginSpringPauseAt(pos);
				}
				if (UpdateSpringCharge(pos)) {
					worldtransfrom_.translation_ = pos;
					worldtransfrom_.translation_.z = 1.0f;
					worldtransfrom_.rotation_ = {0.0f, 0.0f, 0.0f};
					return;
				}
				break;
			}
			if (result == TrampolineBounceResult::EnterDown) {
				BeginSpringPenetration(static_cast<int>(i), SpringChargeKind::Down, pos);
				if (spring.DidEnterOrCrossStopZone(pos.x, pos.y, pos.x, pos.y)) {
					BeginSpringPauseAt(pos);
				}
				if (UpdateSpringCharge(pos)) {
					worldtransfrom_.translation_ = pos;
					worldtransfrom_.translation_.z = 1.0f;
					worldtransfrom_.rotation_ = {0.0f, 0.0f, 0.0f};
					return;
				}
				break;
			}
			if (result == TrampolineBounceResult::EnterSide) {
				const SpringChargeKind kind = spring.GetType() == TrampolineSpringType::Right ? SpringChargeKind::Right : SpringChargeKind::Left;
				BeginSpringPenetration(static_cast<int>(i), kind, pos);
				if (spring.DidEnterOrCrossStopZone(pos.x, pos.y, pos.x, pos.y)) {
					BeginSpringPauseAt(pos);
				}
				if (UpdateSpringCharge(pos)) {
					worldtransfrom_.translation_ = pos;
					worldtransfrom_.translation_.z = 1.0f;
					worldtransfrom_.rotation_ = {0.0f, 0.0f, 0.0f};
					return;
				}
				break;
			}
		}
	}

	HandleSpikeCollision(pos);

	const float clampedY = pos.y;
	tileMap_->ClampPositionToMapBounds(pos.x, pos.y, halfWidth_, halfHeight_);
	if (pos.y != clampedY) {
		velocityY_ = 0.0f;
	}

	worldtransfrom_.translation_ = pos;
	worldtransfrom_.translation_.z = 1.0f;
	worldtransfrom_.rotation_ = {0.0f, 0.0f, 0.0f};
}

bool Player::IsMovingInput() const {
	if (!input_) {
		return false;
	}
	if (springChargePhase_ != SpringChargePhase::None) {
		return false;
	}
	if (isSideSpringFlying_) {
		return false;
	}
	return input_->PushKey(DIK_W) || input_->PushKey(DIK_A) || input_->PushKey(DIK_D);
}

bool Player::IsDead() const { return isDead_; }

int Player::GetHp() const { return hp_; }

int Player::GetMaxHp() const { return kMaxHp_; }

float Player::GetCollisionRadius() const { return 0.8f; }

const char* Player::GetKindName() const { return "Player"; }

KamataEngine::Vector3 Player::GetWorldPosition() const {
	KamataEngine::Vector3 worldPos;
	worldPos.x = worldtransfrom_.matWorld_.m[3][0];
	worldPos.y = worldtransfrom_.matWorld_.m[3][1];
	worldPos.z = worldtransfrom_.matWorld_.m[3][2];
	return worldPos;
}

AABB Player::GetAABB() {
	KamataEngine::Vector3 worldPos = GetWorldPosition();
	AABB aabb;
	aabb.min = {worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f};
	aabb.max = {worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f};
	return aabb;
}

void Player::SetParent(const KamataEngine::WorldTransform* parent) { worldtransfrom_.parent_ = parent; }

// State Pattern: 現在の状態オブジェクトに更新処理を委譲（ポリモーフィズム）
void Player::Update() {
	if (state_) {
		state_->Update(*this);
	}
}

void Player::UpdateBullets() {
	// 弾更新中にリストが変わっても安全なようスナップショットで回す
	std::vector<PlayerBullet*> bulletSnapshot;
	bulletSnapshot.reserve(bullets_.size());
	for (PlayerBullet* b : bullets_) {
		if (b)
			bulletSnapshot.push_back(b);
	}

	for (PlayerBullet* b : bulletSnapshot) {
		if (b)
			b->Update();
	}

	bullets_.remove_if([this](PlayerBullet* bullet) {
		if (!bullet)
			return true;
		if (bullet->IsDead()) {
			if (entityFactory_) {
				entityFactory_->ReleasePlayerBullet(bullet);
			}
			return true;
		}
		return false;
	});
}

void Player::ProcessDodgeInput() {
	if (dodgeTimer_ > 0) {
		dodgeTimer_--;
		return;
	}

	if (!input_->PushKey(DIK_LSHIFT)) {
		return;
	}

	float dodgeDir = 0.0f;
	if (input_->PushKey(DIK_A))
		dodgeDir = -1.0f;
	else if (input_->PushKey(DIK_D))
		dodgeDir = 1.0f;

	if (dodgeDir != 0.0f && railCamera_) {
		BeginRolling(dodgeDir);
	}
}

void Player::BeginRolling(float direction) {
	railCamera_->Dodge(direction);
	rollTimer_ = 0.0f;
	rollDirection_ = direction;
	dodgeTimer_ = 10; // クールタイム
	// 状態遷移: 回避開始時に Rolling へ（Normal 状態クラスから呼ばれる）
	ChangeState(PlayerStateRolling::Instance());
}

void Player::UpdateRotationNormal() {
	Vector3 currentRotation = {0, 0, 0};
	worldtransfrom_.rotation_ = currentRotation;

	if (!railCamera_) {
		return;
	}

	const float lerpFactor = 0.1f;

	// ロール（横の傾き）
	float rollVelocity = railCamera_->GetRotationVelocity().z;
	const float tiltFactor = 5.0f;
	float targetRoll = rollVelocity * tiltFactor;

	float yawVelocity = railCamera_->GetRotationVelocity().y;
	const float yawTiltFactor = 50.0f;
	targetRoll -= yawVelocity * yawTiltFactor;

	const float maxRollAngle = 4.0f;
	targetRoll = std::clamp(targetRoll, -maxRollAngle, maxRollAngle);
	worldtransfrom_.rotation_.z += (targetRoll - worldtransfrom_.rotation_.z) * lerpFactor;

	float pitchVelocity = railCamera_->GetRotationVelocity().x;
	const float pitchFactor = 12.0f;
	float targetPitch = pitchVelocity * pitchFactor;
	const float maxPitchAngle = 1.5f;
	targetPitch = std::clamp(targetPitch, -maxPitchAngle, maxPitchAngle);
	worldtransfrom_.rotation_.x += (targetPitch - worldtransfrom_.rotation_.x) * lerpFactor;
}

bool Player::UpdateRotationRolling() {
	rollTimer_ += 1.0f;
	float t = rollTimer_ / kRollDuration_;
	const bool finished = t >= 1.0f;
	if (finished) {
		t = 1.0f;
	}

	float easeT = 1.0f - std::pow(1.0f - t, 3.0f);
	float maxAngle = 2.0f * 3.14159265f;

	Vector3 currentRotation = {0, 0, 0};
	currentRotation.z = maxAngle * easeT * rollDirection_ * -1.0f;
	worldtransfrom_.rotation_ = currentRotation;

	return finished;
}

void Player::UpdateHitShake() {
	worldtransfrom_.translation_.x -= hitShakePrevHorizontalOffset_;
	worldtransfrom_.translation_.y -= hitShakePrevVerticalOffset_;
	hitShakePrevHorizontalOffset_ = 0.0f;
	hitShakePrevVerticalOffset_ = 0.0f;

	if (hitShakeAmplitude_ <= 0.001f && hitShakeVerticalAmplitude_ <= 0.0001f && hitShakeHorizontalAmplitude_ <= 0.0001f) {
		return;
	}

	hitShakeTime_ += 1.0f;
	float damping = std::exp(-hitShakeDecay_ * hitShakeTime_);

	float angle = hitShakeAmplitude_ * damping * std::sin(hitShakeFrequency_ * hitShakeTime_ * 2.0f * 3.14159265f);
	worldtransfrom_.rotation_.y += angle;
	worldtransfrom_.rotation_.z += angle * 0.25f;

	float verticalOffset = hitShakeVerticalAmplitude_ * damping * std::sin(hitShakeFrequency_ * hitShakeTime_ * 2.0f * 3.14159265f);
	worldtransfrom_.translation_.y += verticalOffset;
	hitShakePrevVerticalOffset_ = verticalOffset;

	float horizontalOffset = hitShakeHorizontalAmplitude_ * damping * std::cos(hitShakeFrequency_ * hitShakeTime_ * 2.0f * 3.14159265f);
	worldtransfrom_.translation_.x += horizontalOffset;
	hitShakePrevHorizontalOffset_ = horizontalOffset;

	if (damping < 0.01f) {
		hitShakeAmplitude_ = 0.0f;
		hitShakeVerticalAmplitude_ = 0.0f;
		hitShakeHorizontalAmplitude_ = 0.0f;
		hitShakeTime_ = 0.0f;
		hitShakePrevVerticalOffset_ = 0.0f;
		hitShakePrevHorizontalOffset_ = 0.0f;
	}
}

void Player::FinalizeFrameUpdate() {
	worldtransfrom_.UpdateMatrix();
}

void Player::Draw() {
	model_->Draw(worldtransfrom_, *camera_);
}

void Player::SetVisualModel(KamataEngine::Model* model) {
	if (model) {
		model_ = model;
	}
}

void Player::SetSpawnPosition(const KamataEngine::Vector3& pos) {
	spawnPosition_ = pos;
	spawnPosition_.z = 1.0f;
}

void Player::SetRailCamera(RailCamera* camera) { railCamera_ = camera; }

void Player::SetTileMap(TileMap* tileMap) {
	tileMap_ = tileMap;
	if (tileMap_) {
		const float kModelExtent = 8.0f;
		const float tw = tileMap_->GetTileWidth();
		const float th = tileMap_->GetTileHeight();
		if (tw > 0.0f && th > 0.0f) {
			worldtransfrom_.scale_ = {tw / kModelExtent * 0.75f, th / kModelExtent * 0.75f, 1.0f};
			halfWidth_ = tw * 0.375f;
			halfHeight_ = th * 0.375f;
		}
	}
}

void Player::ResetRotation() {
	worldtransfrom_.rotation_ = {0.0f, 0.0f, 0.0f};
	worldtransfrom_.UpdateMatrix();
}

void Player::ResetParticles() {
	if (engineExhaust_) {
		engineExhaust_->Clear();
	}
}

// Dead 状態から呼ばれるゲームオーバー演出
void Player::UpdateGameOverAnimation() {
	const float animationTime = gameOverAnimationTime_;

	const float pitchDownAngle = 3.14159265f / 4.0f;
	worldtransfrom_.rotation_.x = pitchDownAngle;

	const float baseSpinSpeed = 0.01f;
	const float spinAcceleration = 0.0005f;
	float currentSpinSpeed = baseSpinSpeed + spinAcceleration * animationTime;
	worldtransfrom_.rotation_.y += currentSpinSpeed;

	const float baseFallSpeed = 0.02f;
	const float fallAcceleration = 0.001f;
	float currentFallSpeed = baseFallSpeed + fallAcceleration * animationTime;
	worldtransfrom_.translation_.y -= currentFallSpeed;

	worldtransfrom_.UpdateMatrix();

	if (engineExhaust_) {
		KamataEngine::Vector3 emitOffset = {0.8f, 0.0f, -0.8f};
		KamataEngine::Vector3 worldEmitPos = KamataEngine::MathUtility::Transform(emitOffset, worldtransfrom_.matWorld_);
		KamataEngine::Vector3 localVelocityDir = {1.0f, 1.0f, -0.5f};
		localVelocityDir = KamataEngine::MathUtility::Normalize(localVelocityDir);
		KamataEngine::Vector3 worldVelocityDir = KamataEngine::MathUtility::TransformNormal(localVelocityDir, worldtransfrom_.matWorld_);
		const float smokeSpeed = 0.5f;
		KamataEngine::Vector3 smokeVelocity = worldVelocityDir * smokeSpeed;
		engineExhaust_->Emit(worldEmitPos, smokeVelocity);
		engineExhaust_->Update();
	}
}

void Player::ResetBullets() {
	if (entityFactory_) {
		for (PlayerBullet* bullet : bullets_) {
			entityFactory_->ReleasePlayerBullet(bullet);
		}
	}
	bullets_.clear();
}

void Player::EvadeBullets(std::list<EnemyBullet*>& bullets) {

	if (IsRolling()) {

		// 回避中: 近距離のホーミング弾を無効化
		KamataEngine::Vector3 playerPos = GetWorldPosition();

		for (EnemyBullet* bullet : bullets) {
			if (!bullet)
				continue;
			if (!bullet->IsHoming())
				continue;

			KamataEngine::Vector3 bulletPos = bullet->GetWorldPosition();

			// 距離を計算
			float dx = playerPos.x - bulletPos.x;
			float dy = playerPos.y - bulletPos.y;
			float dz = playerPos.z - bulletPos.z;
			float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

			// 200以内の時に回避行動をしたらホーミングを失う
			const float kEvasionRange = 200.0f;
			if (dist < kEvasionRange) {
				bullet->OnEvaded();
			}
		}
	}
}