#include "PlayerState.h"
#include "Player.h"

PlayerStateWaiting PlayerStateWaiting::instance_;
PlayerStateSwinging PlayerStateSwinging::instance_;
PlayerStateFlying PlayerStateFlying::instance_;
PlayerStateDead PlayerStateDead::instance_;
PlayerStateNormal PlayerStateNormal::instance_;
PlayerStateAiming PlayerStateAiming::instance_;
PlayerStateAimingHeight PlayerStateAimingHeight::instance_;
PlayerStateGauging PlayerStateGauging::instance_;
PlayerStateSwing PlayerStateSwing::instance_;
PlayerStateRolling PlayerStateRolling::instance_;

PlayerStateWaiting* PlayerStateWaiting::Instance() { return &instance_; }
PlayerStateSwinging* PlayerStateSwinging::Instance() { return &instance_; }
PlayerStateFlying* PlayerStateFlying::Instance() { return &instance_; }
PlayerStateDead* PlayerStateDead::Instance() { return &instance_; }
PlayerStateNormal* PlayerStateNormal::Instance() { return &instance_; }
PlayerStateAiming* PlayerStateAiming::Instance() { return &instance_; }
PlayerStateAimingHeight* PlayerStateAimingHeight::Instance() { return &instance_; }
PlayerStateGauging* PlayerStateGauging::Instance() { return &instance_; }
PlayerStateSwing* PlayerStateSwing::Instance() { return &instance_; }
PlayerStateRolling* PlayerStateRolling::Instance() { return &instance_; }

void PlayerStateWaiting::Update(Player& player) {
	// 開始直後から常時振り子
	player.BeginSwingFromWaiting();
}

void PlayerStateSwinging::Update(Player& player) {
	player.UpdatePendulum();
	// SPACE 一回押しで糸を切って前進
	if (player.IsSpaceJustPressed()) {
		player.CutRopeAndFly();
	}
}

void PlayerStateFlying::Update(Player& player) {
	player.UpdateFreeFlight();

	// 画面外でも再接続可（ゲームオーバー条件はリングすり抜け）
	if (player.IsSpaceJustPressed()) {
		player.TryAttachNewAnchor();
	}
}

void PlayerStateDead::Update(Player& player) {
	player.UpdateGameOverAnimation();
}

void PlayerStateNormal::Update(Player& player) { player.ChangeState(PlayerStateWaiting::Instance()); }
void PlayerStateAiming::Update(Player& player) { (void)player; }
void PlayerStateAimingHeight::Update(Player& player) { (void)player; }
void PlayerStateGauging::Update(Player& player) { (void)player; }
void PlayerStateSwing::Update(Player& player) { (void)player; }
void PlayerStateRolling::Update(Player& player) { (void)player; }
