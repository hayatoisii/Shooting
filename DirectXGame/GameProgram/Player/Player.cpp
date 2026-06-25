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

	if (tileMap_) {
		tileMap_->ClampPositionToMapBounds(worldtransfrom_.translation_.x, worldtransfrom_.translation_.y, halfWidth_, halfHeight_);
	}
}

void Player::ResetStats() {
	hp_ = kMaxHp_;
	isDead_ = false;
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

void Player::UpdateMovement() {
	if (!tileMap_ || !input_) {
		return;
	}

	float moveX = 0.0f;
	if (input_->PushKey(DIK_A)) {
		moveX -= kMoveSpeed;
	}
	if (input_->PushKey(DIK_D)) {
		moveX += kMoveSpeed;
	}

	if (input_->PushKey(DIK_W) && onGround_) {
		velocityY_ = kJumpSpeed;
		onGround_ = false;
	}

	velocityY_ -= kGravity;

	KamataEngine::Vector3 pos = worldtransfrom_.translation_;

	if (trampolineSprings_) {
		for (TrampolineSpring& spring : *trampolineSprings_) {
			spring.TryBounce(pos.x, pos.y, halfWidth_, halfHeight_, velocityX_, velocityY_, onGround_);
		}
	}

	pos.x += moveX + velocityX_;
	tileMap_->ResolveCollisionX(pos.x, pos.y, halfWidth_, halfHeight_);

	const float prevY = pos.y;
	pos.y += velocityY_;
	bool landed = false;
	tileMap_->ResolveCollisionY(pos.y, pos.x, halfWidth_, halfHeight_, velocityY_, landed);
	if (landed) {
		velocityY_ = 0.0f;
		onGround_ = true;
	} else if (pos.y != prevY + velocityY_) {
		velocityY_ = 0.0f;
		onGround_ = false;
	} else {
		onGround_ = false;
	}

	// 空中は横速度を維持してふわっと飛ばす。着地後だけゆっくり減速
	if (onGround_) {
		if (std::abs(velocityX_) < 0.1f) {
			velocityX_ = 0.0f;
		} else {
			velocityX_ *= kVelocityXDamping;
		}
	}

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