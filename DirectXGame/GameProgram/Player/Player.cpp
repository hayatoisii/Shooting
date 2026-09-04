#include "Player.h"
#include "PlayerState.h"
#include "RailCamera.h"
#include <algorithm>
#include <cassert>
#include <cmath>

Player::Player() { state_ = PlayerStateWaiting::Instance(); }

Player::~Player() {
	delete modelRope_;
	delete modelAnchor_;
	ResetBullets();
}

void Player::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& pos) {
	assert(model);
	model_ = model;
	camera_ = camera;
	input_ = KamataEngine::Input::GetInstance();
	modelRope_ = KamataEngine::Model::CreateFromOBJ("cube", true);
	modelAnchor_ = KamataEngine::Model::CreateFromOBJ("tama", true);

	worldtransfrom_.Initialize();
	playerTransform_.Initialize();
	ropeTransform_.Initialize();
	for (int i = 0; i < kMaxAnchorDraw_; ++i) {
		anchorDrawTransforms_[i].Initialize();
	}
	anchorDrawReady_ = true;

	playerTint_.Initialize();
	anchorTint_.Initialize();
	attachedTint_.Initialize();
	playerTint_.SetColor({0.95f, 0.95f, 1.0f, 1.0f});
	anchorTint_.SetColor({0.4f, 0.9f, 1.0f, 1.0f});
	attachedTint_.SetColor({1.0f, 0.9f, 0.15f, 1.0f});
	colorsReady_ = true;

	SetPosition(pos);
	ChangeState(PlayerStateWaiting::Instance());
}

float Player::MakeAnchorY(int index) const {
	// 上寄りに多く出るよう偏らせつつランダム
	unsigned int s1 = static_cast<unsigned int>(index * 1103515245u + 12345u);
	unsigned int s2 = static_cast<unsigned int>((index + 17) * 1664525u + 1013904223u);
	float r1 = static_cast<float>((s1 >> 16) & 0x7fff) / 32767.0f;
	float r2 = static_cast<float>((s2 >> 16) & 0x7fff) / 32767.0f;
	float r = r1 * 0.55f + r2 * 0.45f;
	r = std::sqrt(r); // 高めに寄せる
	return kAnchorYMin_ + r * (kAnchorYMax_ - kAnchorYMin_);
}

void Player::RebuildAnchorListFromStart() {
	anchors_.clear();
	nextAnchorIndex_ = 0;
	nextAnchorZ_ = 0.0f;
	EnsureAnchorsAhead();
}

void Player::EnsureAnchorsAhead() {
	const float needUntil = playerPos_.z + kAnchorGenAhead_;
	while (nextAnchorZ_ <= needUntil) {
		anchors_.push_back({0.0f, MakeAnchorY(nextAnchorIndex_), nextAnchorZ_});
		++nextAnchorIndex_;
		nextAnchorZ_ += kAnchorSpacingZ_;
	}

	const float cullZ = playerPos_.z - kAnchorCullBehind_;
	while (!anchors_.empty() && anchors_.front().z < cullZ) {
		anchors_.erase(anchors_.begin());
		if (attachedAnchorIndex_ >= 0) {
			--attachedAnchorIndex_;
		}
	}
	if (attachedAnchorIndex_ < 0) {
		attachedAnchorIndex_ = -1;
	}
}

int Player::FindNearestAnchorIndex() const {
	int best = -1;
	float bestDistSq = 1.0e30f;
	for (int i = 0; i < static_cast<int>(anchors_.size()); ++i) {
		const KamataEngine::Vector3& a = anchors_[i];
		const float dy = a.y - playerPos_.y;
		const float dz = a.z - playerPos_.z;
		const float d2 = dy * dy + dz * dz;
		if (d2 < bestDistSq) {
			bestDistSq = d2;
			best = i;
		}
	}
	return best;
}

void Player::SetPosition(const KamataEngine::Vector3& position) {
	ropeLength_ = kDefaultRopeLength_;
	angle_ = kStartHangAngle_;
	angularVel_ = 0.0f;
	ropeConnected_ = true;
	isDead_ = false;
	hp_ = kMaxHp_;
	playerVel_ = {0.0f, 0.0f, 0.0f};

	RebuildAnchorListFromStart();
	attachedAnchorIndex_ = 0;
	if (!anchors_.empty()) {
		anchorPos_ = anchors_[0];
	} else {
		anchorPos_ = {position.x, kAnchorBaseY_, 0.0f};
	}

	ApplyPlayerFromAngle();
	SyncTransforms();
}

void Player::ResetStats() {
	hp_ = kMaxHp_;
	isDead_ = false;
	restartRequested_ = false;
	gameOverAnimationTime_ = 0.0f;
	SetPosition({0.0f, kAnchorBaseY_, 0.0f});
	ChangeState(PlayerStateWaiting::Instance());
	spaceWasHeld_ = false;
}

void Player::ResetBullets() {
	for (PlayerBullet* bullet : bullets_) {
		delete bullet;
	}
	bullets_.clear();
}

void Player::RefreshWorldMatrix() { SyncTransforms(); }

void Player::SetParent(const KamataEngine::WorldTransform* parent) { worldtransfrom_.parent_ = parent; }

void Player::SetRailCamera(RailCamera* camera) { railCamera_ = camera; }

void Player::ChangeState(PlayerState* newState) {
	if (newState == nullptr || newState == state_) {
		return;
	}
	state_ = newState;
}

const char* Player::GetStateName() const { return state_ ? state_->GetStateName() : "None"; }

bool Player::IsFlying() const { return state_ == PlayerStateFlying::Instance(); }
bool Player::IsWaiting() const { return state_ == PlayerStateWaiting::Instance(); }
bool Player::IsSwinging() const { return state_ == PlayerStateSwinging::Instance(); }

void Player::Update() {
	EnsureAnchorsAhead();
	if (state_) {
		state_->Update(*this);
	}
	spaceWasHeld_ = IsSpaceHeld();
	SyncTransforms();
}

bool Player::IsSpaceJustPressed() const { return input_ && input_->TriggerKey(DIK_SPACE); }
bool Player::IsSpaceHeld() const { return input_ && input_->PushKey(DIK_SPACE); }
bool Player::WasSpaceReleased() const {
	return spaceWasHeld_ && input_ && !input_->PushKey(DIK_SPACE);
}

void Player::ApplyPlayerFromAngle() {
	playerPos_ = {
	    anchorPos_.x,
	    anchorPos_.y - ropeLength_ * std::cos(angle_),
	    anchorPos_.z + ropeLength_ * std::sin(angle_),
	};
	UpdatePlayerVelocityFromAngle();
}

void Player::UpdatePlayerVelocityFromAngle() {
	playerVel_ = {
	    0.0f,
	    ropeLength_ * angularVel_ * std::sin(angle_),
	    ropeLength_ * angularVel_ * std::cos(angle_),
	};
}

void Player::ClampAngularVelToLinearSpeed() {
	if (ropeLength_ <= 0.001f) {
		return;
	}
	const float maxOmega = kMaxLinearSpeed_ / ropeLength_;
	if (angularVel_ > maxOmega) {
		angularVel_ = maxOmega;
	} else if (angularVel_ < -maxOmega) {
		angularVel_ = -maxOmega;
	}
}

void Player::ComputeAngleFromPositions() {
	const float dy = playerPos_.y - anchorPos_.y;
	const float dz = playerPos_.z - anchorPos_.z;
	ropeLength_ = std::sqrt(dy * dy + dz * dz);
	if (ropeLength_ < 1.0f) {
		ropeLength_ = 1.0f;
	}
	angle_ = std::atan2(dz, -dy);
}

void Player::BeginSwingFromWaiting() {
	angularVel_ = kStartAngularVel_;
	ropeConnected_ = true;
	if (attachedAnchorIndex_ >= 0 && attachedAnchorIndex_ < static_cast<int>(anchors_.size())) {
		anchorPos_ = anchors_[attachedAnchorIndex_];
	}
	ApplyPlayerFromAngle();
	ChangeState(PlayerStateSwinging::Instance());
}

void Player::UpdatePendulum() {
	if (attachedAnchorIndex_ >= 0 && attachedAnchorIndex_ < static_cast<int>(anchors_.size())) {
		anchorPos_ = anchors_[attachedAnchorIndex_];
	}

	const float accel = -(kPendulumGravity_ / ropeLength_) * std::sin(angle_);
	angularVel_ += accel;

	if (input_ && ropeLength_ > 0.001f) {
		float scaleLen = ropeLength_;
		if (scaleLen < kDefaultRopeLength_) {
			scaleLen = kDefaultRopeLength_;
		}
		const float angAccel = kSwingLinearAccel_ / scaleLen;
		if (input_->PushKey(DIK_A)) {
			angularVel_ -= angAccel;
		}
		if (input_->PushKey(DIK_D)) {
			angularVel_ += angAccel;
		}
	}

	angularVel_ *= kPendulumDamping_;
	ClampAngularVelToLinearSpeed();
	angle_ += angularVel_;
	ApplyPlayerFromAngle();
}

void Player::CutRopeAndFly() {
	UpdatePlayerVelocityFromAngle();
	playerVel_.x *= kLaunchSpeedScale_;
	playerVel_.y *= kLaunchSpeedScale_;
	playerVel_.z *= kLaunchSpeedScale_;
	ropeConnected_ = false;
	attachedAnchorIndex_ = -1;
	ChangeState(PlayerStateFlying::Instance());
}

void Player::UpdateFreeFlight() {
	playerVel_.y -= kGravity_;
	playerPos_.x += playerVel_.x;
	playerPos_.y += playerVel_.y;
	playerPos_.z += playerVel_.z;
}

void Player::TryAttachNewAnchor() {
	EnsureAnchorsAhead();
	const int nearest = FindNearestAnchorIndex();
	if (nearest < 0) {
		return;
	}

	attachedAnchorIndex_ = nearest;
	anchorPos_ = anchors_[nearest];
	ComputeAngleFromPositions();

	// いまの速度を「角度が増える方向」の接線に射影（符号を合わせないと逆方向に振れる）
	if (ropeLength_ > 0.001f) {
		const float s = std::sin(angle_);
		const float c = std::cos(angle_);
		const float vAlong = playerVel_.y * s + playerVel_.z * c;
		angularVel_ = vAlong / ropeLength_;
	} else {
		angularVel_ = kAttachKickAngularVel_;
	}
	ClampAngularVelToLinearSpeed();

	ropeConnected_ = true;
	ChangeState(PlayerStateSwinging::Instance());
}

void Player::Fail() { restartRequested_ = true; }

bool Player::ConsumeRestartRequest() {
	if (!restartRequested_) {
		return false;
	}
	restartRequested_ = false;
	return true;
}

void Player::UpdateGameOverAnimation() {
	gameOverAnimationTime_ += 1.0f;
	playerVel_.y -= kGravity_;
	playerPos_.y += playerVel_.y;
	playerPos_.z += playerVel_.z * 0.98f;
}

KamataEngine::Vector3 Player::GetCameraFocusPosition() const { return playerPos_; }

float Player::GetProgressZ() const { return playerPos_.z; }

KamataEngine::Vector3 Player::GetAnchorBallPosition() const {
	if (ropeConnected_ && attachedAnchorIndex_ >= 0 && attachedAnchorIndex_ < static_cast<int>(anchors_.size())) {
		return anchors_[attachedAnchorIndex_];
	}
	const int nearest = FindNearestAnchorIndex();
	if (nearest >= 0) {
		return anchors_[nearest];
	}
	return anchorPos_;
}

bool Player::IsFlyerOnScreen() const { return IsWorldPosOnScreen(playerPos_); }

bool Player::IsAnchorOnScreen() const { return IsWorldPosOnScreen(GetAnchorBallPosition()); }

bool Player::IsWorldPosOnScreen(const KamataEngine::Vector3& worldPos) const {
	if (!camera_) {
		return true;
	}
	const KamataEngine::Matrix4x4& viewMatrix = camera_->matView;
	const KamataEngine::Matrix4x4& projMatrix = camera_->matProjection;

	KamataEngine::Vector3 viewPos;
	viewPos.x = worldPos.x * viewMatrix.m[0][0] + worldPos.y * viewMatrix.m[1][0] + worldPos.z * viewMatrix.m[2][0] + viewMatrix.m[3][0];
	viewPos.y = worldPos.x * viewMatrix.m[0][1] + worldPos.y * viewMatrix.m[1][1] + worldPos.z * viewMatrix.m[2][1] + viewMatrix.m[3][1];
	viewPos.z = worldPos.x * viewMatrix.m[0][2] + worldPos.y * viewMatrix.m[1][2] + worldPos.z * viewMatrix.m[2][2] + viewMatrix.m[3][2];
	if (viewPos.z < 0.0f) {
		return false;
	}

	float clipX = viewPos.x * projMatrix.m[0][0] + viewPos.y * projMatrix.m[1][0] + viewPos.z * projMatrix.m[2][0] + projMatrix.m[3][0];
	float clipY = viewPos.x * projMatrix.m[0][1] + viewPos.y * projMatrix.m[1][1] + viewPos.z * projMatrix.m[2][1] + projMatrix.m[3][1];
	float wClip = viewPos.x * projMatrix.m[0][3] + viewPos.y * projMatrix.m[1][3] + viewPos.z * projMatrix.m[2][3] + projMatrix.m[3][3];
	if (std::abs(wClip) < 0.001f) {
		return false;
	}
	float ndcX = clipX / wClip;
	float ndcY = clipY / wClip;
	const float margin = 1.02f;
	return ndcX > -margin && ndcX < margin && ndcY > -margin && ndcY < margin;
}

void Player::SyncTransforms() {
	playerTransform_.translation_ = playerPos_;
	playerTransform_.scale_ = {kBallScale_, kBallScale_, kBallScale_};
	playerTransform_.rotation_ = {0.0f, 0.0f, 0.0f};
	playerTransform_.UpdateMatrix();

	worldtransfrom_.translation_ = GetCameraFocusPosition();
	worldtransfrom_.UpdateMatrix();
	UpdateRopeTransform();
}

void Player::UpdateRopeTransform() {
	if (!ropeConnected_) {
		ropeTransform_.scale_ = {0.0f, 0.0f, 0.0f};
		ropeTransform_.UpdateMatrix();
		return;
	}
	const float dy = playerPos_.y - anchorPos_.y;
	const float dz = playerPos_.z - anchorPos_.z;
	const float len = std::sqrt(dy * dy + dz * dz);
	if (len < 0.001f) {
		ropeTransform_.scale_ = {0.0f, 0.0f, 0.0f};
		ropeTransform_.UpdateMatrix();
		return;
	}

	float visualLen = len * kRopeVisualLengthScale_;
	if (visualLen < 0.05f) {
		visualLen = 0.05f;
	}
	const float midY = (anchorPos_.y + playerPos_.y) * 0.5f;
	const float midZ = (anchorPos_.z + playerPos_.z) * 0.5f;
	const float pitch = std::atan2(-dy, dz);

	ropeTransform_.translation_ = {anchorPos_.x, midY, midZ};
	ropeTransform_.rotation_ = {pitch, 0.0f, 0.0f};
	ropeTransform_.scale_ = {kRopeThickness_, kRopeThickness_, visualLen};
	ropeTransform_.UpdateMatrix();
}

KamataEngine::Vector3 Player::GetWorldPosition() const { return GetCameraFocusPosition(); }

AABB Player::GetAABB() {
	AABB aabb;
	aabb.min = {playerPos_.x - kWidth / 2.0f, playerPos_.y - kHeight / 2.0f, playerPos_.z - kWidth / 2.0f};
	aabb.max = {playerPos_.x + kWidth / 2.0f, playerPos_.y + kHeight / 2.0f, playerPos_.z + kWidth / 2.0f};
	return aabb;
}

void Player::OnCollision() {}

void Player::Draw() {
	if (!model_ || !camera_ || !anchorDrawReady_) {
		return;
	}

	// 各アンカーに専用 WorldTransform（使い回しだと定数バッファが潰れて消える）
	const int drawCount = (std::min)(static_cast<int>(anchors_.size()), kMaxAnchorDraw_);
	for (int i = 0; i < drawCount; ++i) {
		anchorDrawTransforms_[i].translation_ = anchors_[i];
		anchorDrawTransforms_[i].scale_ = {kAnchorScale_, kAnchorScale_, kAnchorScale_};
		anchorDrawTransforms_[i].rotation_ = {0.0f, 0.0f, 0.0f};
		anchorDrawTransforms_[i].UpdateMatrix();
		if (colorsReady_) {
			if (ropeConnected_ && i == attachedAnchorIndex_) {
				model_->Draw(anchorDrawTransforms_[i], *camera_, &attachedTint_);
			} else {
				model_->Draw(anchorDrawTransforms_[i], *camera_, &anchorTint_);
			}
		} else {
			model_->Draw(anchorDrawTransforms_[i], *camera_);
		}
	}

	if (ropeConnected_ && modelRope_) {
		modelRope_->Draw(ropeTransform_, *camera_);
	}

	if (colorsReady_) {
		model_->Draw(playerTransform_, *camera_, &playerTint_);
	} else {
		model_->Draw(playerTransform_, *camera_);
	}
}
