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
	isPortalAbsorbing_ = false;
	portalAbsorbTimer_ = 0.0f;
	portalAbsorbSpinZ_ = 0.0f;
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
	// 実際の移動量は UpdateSpringCharge() が毎フレーム中央へ向けて計算する
	velocityX_ = 0.0f;
	velocityY_ = 0.0f;
}

void Player::BeginSpringPauseAt(const KamataEngine::Vector3& pos) {
	springChargeAnchor_ = pos;
	if (const TrampolineSpring* spring = GetActiveSpring()) {
		// 端に張り付かないよう、バネの中央にぴったり移動する
		const KamataEngine::Vector3& center = spring->GetCenter();
		springChargeAnchor_.x = center.x;
		springChargeAnchor_.y = center.y;
	}
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

	velocityX_ = 0.0f;
	velocityY_ = 0.0f;
	spikeHitEvent_ = true;
}

bool Player::ConsumeSpikeHitEvent() {
	const bool hit = spikeHitEvent_;
	spikeHitEvent_ = false;
	return hit;
}

void Player::CaptureSnapshot(PlayerSnapshot& outSnapshot) const {
	outSnapshot.position = worldtransfrom_.translation_;
	outSnapshot.spawnPosition = spawnPosition_;
	outSnapshot.scale = worldtransfrom_.scale_;
	outSnapshot.rotation = worldtransfrom_.rotation_;
	outSnapshot.velocityX = velocityX_;
	outSnapshot.velocityY = velocityY_;
	outSnapshot.onGround = onGround_;
	outSnapshot.hp = hp_;
	outSnapshot.spikeRespawnCooldown = spikeRespawnCooldown_;
	outSnapshot.springChargePhase = springChargePhase_;
	outSnapshot.springChargeKind = springChargeKind_;
	outSnapshot.activeSpringIndex = activeSpringIndex_;
	outSnapshot.springChargePauseTimer = springChargePauseTimer_;
	outSnapshot.springChargeLevel = springChargeLevel_;
	outSnapshot.springChargeAnchor = springChargeAnchor_;
	outSnapshot.springPenetrationPrevPos = springPenetrationPrevPos_;
	outSnapshot.isSideSpringFlying = isSideSpringFlying_;
	outSnapshot.isPortalAbsorbing = isPortalAbsorbing_;
	outSnapshot.portalAbsorbTimer = portalAbsorbTimer_;
	outSnapshot.portalAbsorbStyle = portalAbsorbStyle_;
	outSnapshot.portalAbsorbCenter = portalAbsorbCenter_;
	outSnapshot.portalAbsorbStartPos = portalAbsorbStartPos_;
	outSnapshot.portalAbsorbStartScale = portalAbsorbStartScale_;
	outSnapshot.portalAbsorbStartRotZ = portalAbsorbStartRotZ_;
	outSnapshot.portalAbsorbSpinZ = portalAbsorbSpinZ_;
	outSnapshot.portalAbsorbStartRadius = portalAbsorbStartRadius_;
	outSnapshot.portalAbsorbStartAngle = portalAbsorbStartAngle_;
	outSnapshot.isDead = isDead_;
}

void Player::ApplySnapshot(const PlayerSnapshot& snapshot) {
	worldtransfrom_.translation_ = snapshot.position;
	spawnPosition_ = snapshot.spawnPosition;
	worldtransfrom_.scale_ = snapshot.scale;
	worldtransfrom_.rotation_ = snapshot.rotation;
	velocityX_ = snapshot.velocityX;
	velocityY_ = snapshot.velocityY;
	onGround_ = snapshot.onGround;
	hp_ = snapshot.hp;
	spikeRespawnCooldown_ = snapshot.spikeRespawnCooldown;
	springChargePhase_ = snapshot.springChargePhase;
	springChargeKind_ = snapshot.springChargeKind;
	activeSpringIndex_ = snapshot.activeSpringIndex;
	springChargePauseTimer_ = snapshot.springChargePauseTimer;
	springChargeLevel_ = snapshot.springChargeLevel;
	springChargeAnchor_ = snapshot.springChargeAnchor;
	springPenetrationPrevPos_ = snapshot.springPenetrationPrevPos;
	isSideSpringFlying_ = snapshot.isSideSpringFlying;
	isPortalAbsorbing_ = snapshot.isPortalAbsorbing;
	portalAbsorbTimer_ = snapshot.portalAbsorbTimer;
	portalAbsorbStyle_ = snapshot.portalAbsorbStyle;
	portalAbsorbCenter_ = snapshot.portalAbsorbCenter;
	portalAbsorbStartPos_ = snapshot.portalAbsorbStartPos;
	portalAbsorbStartScale_ = snapshot.portalAbsorbStartScale;
	portalAbsorbStartRotZ_ = snapshot.portalAbsorbStartRotZ;
	portalAbsorbSpinZ_ = snapshot.portalAbsorbSpinZ;
	portalAbsorbStartRadius_ = snapshot.portalAbsorbStartRadius;
	portalAbsorbStartAngle_ = snapshot.portalAbsorbStartAngle;
	isDead_ = snapshot.isDead;
	spikeHitEvent_ = false;
	worldtransfrom_.parent_ = nullptr;
	ChangeState(PlayerStateNormal::Instance());
	worldtransfrom_.UpdateMatrix();
}

namespace {
float LerpFloat(float from, float to, float t) { return from + (to - from) * t; }

KamataEngine::Vector3 LerpVector3(const KamataEngine::Vector3& from, const KamataEngine::Vector3& to, float t) {
	return {LerpFloat(from.x, to.x, t), LerpFloat(from.y, to.y, t), LerpFloat(from.z, to.z, t)};
}
} // namespace

void LerpPlayerSnapshot(const PlayerSnapshot& from, const PlayerSnapshot& to, float t, PlayerSnapshot& outSnapshot) {
	const float clampedT = (std::max)(0.0f, (std::min)(t, 1.0f));
	outSnapshot.position = LerpVector3(from.position, to.position, clampedT);
	outSnapshot.spawnPosition = LerpVector3(from.spawnPosition, to.spawnPosition, clampedT);
	outSnapshot.scale = LerpVector3(from.scale, to.scale, clampedT);
	outSnapshot.rotation = LerpVector3(from.rotation, to.rotation, clampedT);
	outSnapshot.velocityX = LerpFloat(from.velocityX, to.velocityX, clampedT);
	outSnapshot.velocityY = LerpFloat(from.velocityY, to.velocityY, clampedT);
	outSnapshot.springChargePauseTimer = LerpFloat(from.springChargePauseTimer, to.springChargePauseTimer, clampedT);
	outSnapshot.springChargeLevel = LerpFloat(from.springChargeLevel, to.springChargeLevel, clampedT);
	outSnapshot.springChargeAnchor = LerpVector3(from.springChargeAnchor, to.springChargeAnchor, clampedT);
	outSnapshot.springPenetrationPrevPos = LerpVector3(from.springPenetrationPrevPos, to.springPenetrationPrevPos, clampedT);
	outSnapshot.portalAbsorbTimer = LerpFloat(from.portalAbsorbTimer, to.portalAbsorbTimer, clampedT);
	outSnapshot.portalAbsorbCenter = LerpVector3(from.portalAbsorbCenter, to.portalAbsorbCenter, clampedT);
	outSnapshot.portalAbsorbStartPos = LerpVector3(from.portalAbsorbStartPos, to.portalAbsorbStartPos, clampedT);
	outSnapshot.portalAbsorbStartScale = LerpVector3(from.portalAbsorbStartScale, to.portalAbsorbStartScale, clampedT);
	outSnapshot.portalAbsorbStartRotZ = LerpFloat(from.portalAbsorbStartRotZ, to.portalAbsorbStartRotZ, clampedT);
	outSnapshot.portalAbsorbSpinZ = LerpFloat(from.portalAbsorbSpinZ, to.portalAbsorbSpinZ, clampedT);
	outSnapshot.portalAbsorbStartRadius = LerpFloat(from.portalAbsorbStartRadius, to.portalAbsorbStartRadius, clampedT);
	outSnapshot.portalAbsorbStartAngle = LerpFloat(from.portalAbsorbStartAngle, to.portalAbsorbStartAngle, clampedT);

	const bool useTo = clampedT >= 0.5f;
	outSnapshot.onGround = useTo ? to.onGround : from.onGround;
	outSnapshot.hp = useTo ? to.hp : from.hp;
	outSnapshot.spikeRespawnCooldown = useTo ? to.spikeRespawnCooldown : from.spikeRespawnCooldown;
	outSnapshot.springChargePhase = useTo ? to.springChargePhase : from.springChargePhase;
	outSnapshot.springChargeKind = useTo ? to.springChargeKind : from.springChargeKind;
	outSnapshot.activeSpringIndex = useTo ? to.activeSpringIndex : from.activeSpringIndex;
	outSnapshot.isSideSpringFlying = useTo ? to.isSideSpringFlying : from.isSideSpringFlying;
	outSnapshot.isPortalAbsorbing = useTo ? to.isPortalAbsorbing : from.isPortalAbsorbing;
	outSnapshot.portalAbsorbStyle = useTo ? to.portalAbsorbStyle : from.portalAbsorbStyle;
	outSnapshot.isDead = useTo ? to.isDead : from.isDead;
}

bool Player::UpdateSpringCharge(KamataEngine::Vector3& pos) {
	const float kDeltaSec = 1.0f / 60.0f;
	const TrampolineSpring* spring = GetActiveSpring();

	if (springChargePhase_ == SpringChargePhase::Penetrating) {
		if (!spring) {
			springChargePhase_ = SpringChargePhase::None;
			return false;
		}

		// バネの中央へ両軸をなめらかに寄せる（端に張り付かず、ぬるっと中央へめり込む）
		const KamataEngine::Vector3 center = spring->GetCenter();
		auto moveToward = [](float current, float target, float maxStep) -> float {
			const float diff = target - current;
			if (std::abs(diff) <= maxStep) {
				return target;
			}
			return current + (diff > 0.0f ? maxStep : -maxStep);
		};

		const float newX = moveToward(pos.x, center.x, kSpringPenetrationSpeed);
		const float newY = moveToward(pos.y, center.y, kSpringPenetrationSpeed);
		velocityX_ = newX - pos.x;
		velocityY_ = newY - pos.y;

		pos.x = newX;
		tileMap_->ResolveCollisionX(pos.x, pos.y, halfWidth_, halfHeight_);

		pos.y = newY;
		bool landed = false;
		tileMap_->ResolveCollisionY(pos.y, pos.x, halfWidth_, halfHeight_, velocityY_, landed);
		onGround_ = false;

		if (!spring->IsPlayerOverlapping(pos.x, pos.y, halfWidth_, halfHeight_)) {
			TrampolineSpring* mutableSpring = GetActiveSpringMutable();
			if (mutableSpring) {
				mutableSpring->ResetPlayerContact();
			}
			springChargePhase_ = SpringChargePhase::None;
			activeSpringIndex_ = -1;
			return false;
		}

		// 中央に到達してからチャージ待ちを開始する（端で即チャージさせない）
		const float kCenterEpsilon = 0.75f;
		if (std::abs(pos.x - center.x) <= kCenterEpsilon && std::abs(pos.y - center.y) <= kCenterEpsilon) {
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

bool Player::ShouldShowSpringTrajectory() const {
	return springChargePhase_ == SpringChargePhase::Charging;
}

float Player::GetSpringTrajectoryPreviewCharge() const {
	if (springChargePhase_ == SpringChargePhase::Charging) {
		return springChargeLevel_;
	}
	return 0.0f;
}

void Player::GetSpringTrajectoryOrigin(KamataEngine::Vector3& outOrigin) const {
	if (ShouldShowSpringTrajectory()) {
		outOrigin = springChargeAnchor_;
	} else {
		outOrigin = worldtransfrom_.translation_;
	}
	outOrigin.z = 1.0f;
}

void Player::GetSpringPreviewVelocity(float chargeLevel, float& outVelX, float& outVelY) const {
	const bool isCharging = springChargePhase_ == SpringChargePhase::Charging;
	const float t = isCharging ? std::clamp(chargeLevel, 0.0f, 1.0f) : 0.0f;

	switch (springChargeKind_) {
	case SpringChargeKind::Up:
		outVelY = isCharging ? (kSpringMinLaunchSpeed + (kSpringMaxLaunchSpeed - kSpringMinLaunchSpeed) * t) : TrampolineSpring::kBounceSpeedY;
		outVelX = 0.0f;
		break;
	case SpringChargeKind::Right:
		outVelX = isCharging ? (kSideSpringMinLaunchSpeed + (kSideSpringMaxLaunchSpeed - kSideSpringMinLaunchSpeed) * t) : TrampolineSpring::kSideBounceSpeedX;
		outVelY = TrampolineSpring::kSideBounceLiftY;
		break;
	case SpringChargeKind::Left:
		outVelX = isCharging ? -(kSideSpringMinLaunchSpeed + (kSideSpringMaxLaunchSpeed - kSideSpringMinLaunchSpeed) * t) : -TrampolineSpring::kSideBounceSpeedX;
		outVelY = TrampolineSpring::kSideBounceLiftY;
		break;
	case SpringChargeKind::Down:
		outVelY = -(isCharging ? (kSpringMinLaunchSpeed + (kSpringMaxLaunchSpeed - kSpringMinLaunchSpeed) * t) : TrampolineSpring::kDownBounceSpeedY);
		outVelX = 0.0f;
		break;
	}
}

bool Player::ComputeSpringTrajectorySamples(KamataEngine::Vector3* outSamples, int maxSamples, int& outCount) const {
	if (!tileMap_ || !ShouldShowSpringTrajectory() || outSamples == nullptr || maxSamples < 2) {
		outCount = 0;
		return false;
	}

	float launchVelX = 0.0f;
	float launchVelY = 0.0f;
	GetSpringPreviewVelocity(GetSpringTrajectoryPreviewCharge(), launchVelX, launchVelY);

	KamataEngine::Vector3 origin{};
	GetSpringTrajectoryOrigin(origin);
	const float startX = origin.x;
	const float startY = origin.y;
	float posX = startX;
	float posY = startY;
	float velX = launchVelX;
	float velY = launchVelY;
	bool hasLeftGround = false;
	bool clearingLaunchPad = true;
	const float launchPadClearDistance = halfHeight_ * 0.35f;

	auto hasClearedLaunchPad = [&]() {
		switch (springChargeKind_) {
		case SpringChargeKind::Up:
			return posY >= startY + launchPadClearDistance;
		case SpringChargeKind::Down:
			return posY <= startY - launchPadClearDistance;
		case SpringChargeKind::Right:
			return posX >= startX + launchPadClearDistance;
		case SpringChargeKind::Left:
			return posX <= startX - launchPadClearDistance;
		}
		return true;
	};

	outCount = 0;
	outSamples[outCount++] = {startX, startY, 1.0f};

	for (int step = 0; step < 720; ++step) {
		velY -= kGravity;

		posX += velX;
		tileMap_->ResolveCollisionX(posX, posY, halfWidth_, halfHeight_);

		posY += velY;
		bool landed = false;
		if (clearingLaunchPad) {
			if (hasClearedLaunchPad()) {
				clearingLaunchPad = false;
				tileMap_->ResolveCollisionY(posY, posX, halfWidth_, halfHeight_, velY, landed);
			}
		} else {
			tileMap_->ResolveCollisionY(posY, posX, halfWidth_, halfHeight_, velY, landed);
		}

		if (landed) {
			if (hasLeftGround) {
				if (springChargeKind_ == SpringChargeKind::Right || springChargeKind_ == SpringChargeKind::Left) {
					if (outCount < maxSamples) {
						outSamples[outCount++] = {posX, posY, 1.0f};
					}
				}
				break;
			}

			const bool shouldLaunch =
			    (springChargeKind_ == SpringChargeKind::Up && launchVelY > 1.0f) ||
			    (springChargeKind_ == SpringChargeKind::Down && launchVelY < -1.0f) ||
			    (springChargeKind_ == SpringChargeKind::Right && launchVelX > 1.0f) ||
			    (springChargeKind_ == SpringChargeKind::Left && launchVelX < -1.0f);

			if (!shouldLaunch) {
				posX = startX;
				posY = startY;
				velX = launchVelX;
				velY = launchVelY;
				continue;
			}
		} else {
			hasLeftGround = true;
		}

		if (clearingLaunchPad) {
			hasLeftGround = true;
		}

		tileMap_->ClampPositionToMapBounds(posX, posY, halfWidth_, halfHeight_);

		if (hasLeftGround && !clearingLaunchPad) {
			bool outboundEnded = false;
			switch (springChargeKind_) {
			case SpringChargeKind::Up:
				outboundEnded = velY <= 0.0f;
				break;
			case SpringChargeKind::Down:
				outboundEnded = velY >= 0.0f;
				break;
			default:
				break;
			}
			if (outboundEnded) {
				if (outCount < maxSamples) {
					outSamples[outCount++] = {posX, posY, 1.0f};
				}
				break;
			}
		}

		if (hasLeftGround && outCount < maxSamples && step % 3 == 0) {
			outSamples[outCount++] = {posX, posY, 1.0f};
		}
	}

	return outCount >= 2;
}

KamataEngine::Vector3 Player::GetSpringTrajectoryStart() const {
	KamataEngine::Vector3 origin{};
	GetSpringTrajectoryOrigin(origin);
	return origin;
}

void Player::UpdateMovement() {
	if (!tileMap_ || !input_) {
		return;
	}

	KamataEngine::Vector3 pos = worldtransfrom_.translation_;

	if (spikeRespawnCooldown_ > 0) {
		spikeRespawnCooldown_--;
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
			const float visualScale = 0.75f * kPlayerVisualScale;
			worldtransfrom_.scale_ = {tw / kModelExtent * visualScale, th / kModelExtent * visualScale, 1.0f};
			halfWidth_ = tw * 0.375f * kPlayerVisualScale;
			halfHeight_ = th * 0.375f * kPlayerVisualScale;
		}
	}
}

void Player::ResetRotation() {
	worldtransfrom_.rotation_ = {0.0f, 0.0f, 0.0f};
	worldtransfrom_.UpdateMatrix();
}

void Player::ResetVisualScaleFromTileMap() {
	if (!tileMap_) {
		return;
	}

	const float kModelExtent = 8.0f;
	const float tw = tileMap_->GetTileWidth();
	const float th = tileMap_->GetTileHeight();
	if (tw <= 0.0f || th <= 0.0f) {
		return;
	}

	const float visualScale = 0.75f * kPlayerVisualScale;
	worldtransfrom_.scale_ = {tw / kModelExtent * visualScale, th / kModelExtent * visualScale, 1.0f};
	halfWidth_ = tw * 0.375f * kPlayerVisualScale;
	halfHeight_ = th * 0.375f * kPlayerVisualScale;
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

void Player::BeginPortalAbsorption(const KamataEngine::Vector3& portalCenter, PortalAbsorptionStyle style) {
	if (isPortalAbsorbing_) {
		return;
	}

	isPortalAbsorbing_ = true;
	portalAbsorbStyle_ = style;
	portalAbsorbTimer_ = 0.0f;
	portalAbsorbCenter_ = portalCenter;
	portalAbsorbStartPos_ = worldtransfrom_.translation_;
	portalAbsorbStartScale_ = worldtransfrom_.scale_;
	portalAbsorbStartRotZ_ = worldtransfrom_.rotation_.z;
	portalAbsorbSpinZ_ = 0.0f;

	if (portalAbsorbStartScale_.x < 0.01f || portalAbsorbStartScale_.y < 0.01f) {
		ResetVisualScaleFromTileMap();
		portalAbsorbStartScale_ = worldtransfrom_.scale_;
	}

	const float offsetX = portalAbsorbStartPos_.x - portalAbsorbCenter_.x;
	const float offsetY = portalAbsorbStartPos_.y - portalAbsorbCenter_.y;
	const float distance = std::sqrt(offsetX * offsetX + offsetY * offsetY);
	const float minOrbitRadius = halfWidth_ * 0.85f;
	portalAbsorbStartRadius_ = distance > minOrbitRadius ? distance : minOrbitRadius;
	if (distance > 0.5f) {
		portalAbsorbStartAngle_ = std::atan2(offsetY, offsetX);
	} else {
		portalAbsorbStartAngle_ = -1.5707963f;
	}

	velocityX_ = 0.0f;
	velocityY_ = 0.0f;
	onGround_ = false;
	springChargePhase_ = SpringChargePhase::None;
	activeSpringIndex_ = -1;
	isSideSpringFlying_ = false;
}

bool Player::UpdatePortalAbsorptionPlayerSpin(float t, float ease, float oneMinusEase) {
	KamataEngine::Vector3 pos;
	pos.x = portalAbsorbStartPos_.x * oneMinusEase + portalAbsorbCenter_.x * ease;
	pos.y = portalAbsorbStartPos_.y * oneMinusEase + portalAbsorbCenter_.y * ease;
	pos.z = portalAbsorbStartPos_.z * oneMinusEase + portalAbsorbCenter_.z * ease;
	worldtransfrom_.translation_ = pos;

	const float kTwoPi = 6.2831853f;
	const float spinTurns = 5.0f;
	portalAbsorbSpinZ_ = portalAbsorbStartRotZ_ + ease * spinTurns * kTwoPi;
	worldtransfrom_.rotation_.z = portalAbsorbSpinZ_;
	worldtransfrom_.rotation_.y = ease * 1.2f;
	worldtransfrom_.rotation_.x = ease * 0.25f;

	const float scaleFactor = 1.0f - ease * 0.96f;
	const float clampedScale = scaleFactor < 0.04f ? 0.04f : scaleFactor;
	worldtransfrom_.scale_.x = portalAbsorbStartScale_.x * clampedScale;
	worldtransfrom_.scale_.y = portalAbsorbStartScale_.y * clampedScale;
	worldtransfrom_.scale_.z = portalAbsorbStartScale_.z * clampedScale;

	worldtransfrom_.UpdateMatrix();
	return t >= 1.0f;
}

bool Player::UpdatePortalAbsorptionOrbitSpiral(float t, float ease, float oneMinusEase) {
	const float kTwoPi = 6.2831853f;
	const float kHalfPi = 1.5707963f;
	const float orbitTurns = 4.0f;
	const float orbitAngle = portalAbsorbStartAngle_ + ease * orbitTurns * kTwoPi;
	const float radiusEase = oneMinusEase * oneMinusEase;
	const float orbitRadius = portalAbsorbStartRadius_ * radiusEase;

	KamataEngine::Vector3 pos;
	pos.x = portalAbsorbCenter_.x + std::cos(orbitAngle) * orbitRadius;
	pos.y = portalAbsorbCenter_.y + std::sin(orbitAngle) * orbitRadius;
	pos.z = portalAbsorbStartPos_.z * oneMinusEase + portalAbsorbCenter_.z * ease;
	worldtransfrom_.translation_ = pos;

	const float selfSpinTurns = 3.0f;
	worldtransfrom_.rotation_.z = orbitAngle + kHalfPi + ease * selfSpinTurns * kTwoPi;
	worldtransfrom_.rotation_.y = ease * 0.6f;
	worldtransfrom_.rotation_.x = ease * 0.15f;

	const float scaleFactor = 1.0f - ease * 0.96f;
	const float clampedScale = scaleFactor < 0.04f ? 0.04f : scaleFactor;
	worldtransfrom_.scale_.x = portalAbsorbStartScale_.x * clampedScale;
	worldtransfrom_.scale_.y = portalAbsorbStartScale_.y * clampedScale;
	worldtransfrom_.scale_.z = portalAbsorbStartScale_.z * clampedScale;

	worldtransfrom_.UpdateMatrix();
	return t >= 1.0f;
}

bool Player::UpdatePortalAbsorption() {
	if (!isPortalAbsorbing_) {
		return true;
	}

	portalAbsorbTimer_ += 1.0f;
	float t = portalAbsorbTimer_ / kPortalAbsorbDuration;
	if (t > 1.0f) {
		t = 1.0f;
	}

	const float ease = t * t * (3.0f - 2.0f * t);
	const float oneMinusEase = 1.0f - ease;

	bool finished = false;
	if (portalAbsorbStyle_ == PortalAbsorptionStyle::OrbitSpiral) {
		finished = UpdatePortalAbsorptionOrbitSpiral(t, ease, oneMinusEase);
	} else {
		finished = UpdatePortalAbsorptionPlayerSpin(t, ease, oneMinusEase);
	}

	if (finished) {
		isPortalAbsorbing_ = false;
		return true;
	}

	return false;
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