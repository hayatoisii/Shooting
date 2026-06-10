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

// 待機: 重力で地面に止まる。停止したら自動で照準状態へ（SPACE 不要）
void PlayerStateNormal::Update(Player& player) {
	player.UpdateGolfBall();

	// 地面に接地＆ほぼ停止したら即座に照準開始（毎回 SPACE を押す手間を省く）
	if (player.IsOnGround() && player.IsVelocityNearZero()) {
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
// SPACE を押すと空中でボールを停止して方向・高さを打ち直せる
void PlayerStateFlying::Update(Player& player) {
	player.UpdateGolfBall();

	// 空中で SPACE → 残り回数があれば静止して再照準
	if (player.IsSpaceJustPressed() && player.GetAirShotsRemaining() > 0) {
		player.DecrementAirShots();
		player.SetAirAiming(true);
		player.BeginAiming();
		return;
	}

	if (player.IsOnGround() && player.IsVelocityNearZero()) {
		player.ResetAirShots(); // 着地したら回数リセット
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
