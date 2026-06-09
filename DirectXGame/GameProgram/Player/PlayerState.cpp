#include "PlayerState.h"
#include "Player.h"

PlayerStateNormal   PlayerStateNormal::instance_;
PlayerStateAiming        PlayerStateAiming::instance_;
PlayerStateAimingHeight  PlayerStateAimingHeight::instance_;
PlayerStateGauging       PlayerStateGauging::instance_;
PlayerStateSwing    PlayerStateSwing::instance_;
PlayerStateFlying   PlayerStateFlying::instance_;
PlayerStateRolling  PlayerStateRolling::instance_;
PlayerStateDead     PlayerStateDead::instance_;

PlayerStateNormal*   PlayerStateNormal::Instance()   { return &instance_; }
PlayerStateAiming*       PlayerStateAiming::Instance()       { return &instance_; }
PlayerStateAimingHeight* PlayerStateAimingHeight::Instance() { return &instance_; }
PlayerStateGauging*      PlayerStateGauging::Instance()      { return &instance_; }
PlayerStateSwing*    PlayerStateSwing::Instance()    { return &instance_; }
PlayerStateFlying*   PlayerStateFlying::Instance()   { return &instance_; }
PlayerStateRolling*  PlayerStateRolling::Instance()  { return &instance_; }
PlayerStateDead*     PlayerStateDead::Instance()     { return &instance_; }

// 待機: 重力で地面に止まる。SPACE 第1打 → 照準状態へ
void PlayerStateNormal::Update(Player& player) {
	player.UpdateGolfBall();

	if (player.IsOnGround() && player.IsSpaceJustPressed()) {
		player.BeginAiming();
	}
}

// 照準中: 方向矢印が左右に往復。SPACE 第2打 → 方向確定して高さ照準へ
void PlayerStateAiming::Update(Player& player) {
	player.UpdateGolfBall();   // 地面に留まるよう重力継続
	player.UpdateAimArrow();   // 矢印を左右に動かす

	if (player.IsSpaceJustPressed()) {
		player.LockAimDirection(); // 方向確定
		player.BeginAimingHeight();// 高さ照準へ
	}
}

// 高さ照準中: 矢印がロフト角を往復。SPACE 第3打 → 高さ確定してゲージへ
void PlayerStateAimingHeight::Update(Player& player) {
	player.UpdateGolfBall();   // 地面に留まる
	player.UpdateAimHeight();  // 高さ矢印を動かす

	if (player.IsSpaceJustPressed()) {
		player.LockAimHeight();   // 高さ確定
		player.BeginGauging();    // ゲージへ
	}
}

// ゲージ中: バーが往復。SPACE 第3打 → 現在の打力でスイング開始
void PlayerStateGauging::Update(Player& player) {
	player.UpdateGolfBall();   // 地面に留まるよう重力継続
	player.UpdateGauge();      // バー位置を更新

	if (player.IsSpaceJustPressed()) {
		player.BeginSwing();   // 打力は gaugePower_ を読む
	}
}

// スイング演出: パターが回転してボールを打つ。完了 → 飛翔状態
void PlayerStateSwing::Update(Player& player) {
	const bool swingDone = player.UpdateSwingAnimation();
	if (swingDone) {
		player.LaunchBall();
		player.ChangeState(PlayerStateFlying::Instance());
	}
}

// 飛翔中: 重力・バウンド・摩擦。止まったら待機状態へ
void PlayerStateFlying::Update(Player& player) {
	player.UpdateGolfBall();

	if (player.IsOnGround() && player.IsVelocityNearZero()) {
		player.ChangeState(PlayerStateNormal::Instance());
	}
}

// 旧: 回避（互換用・ゴルフでは使わない）
void PlayerStateRolling::Update(Player& player) {
	player.UpdateBullets();
	const bool rollFinished = player.UpdateRotationRolling();
	if (rollFinished) {
		player.ChangeState(PlayerStateNormal::Instance());
		return;
	}
	player.UpdateHitShake();
	player.FinalizeFrameUpdate();
}

// 死亡
void PlayerStateDead::Update(Player& player) {
	player.UpdateGameOverAnimation();
}
