#include "PlayerState.h"
#include "Player.h"

PlayerStateNormal PlayerStateNormal::instance_;
PlayerStateRolling PlayerStateRolling::instance_;
PlayerStateDead PlayerStateDead::instance_;

PlayerStateNormal* PlayerStateNormal::Instance() { return &instance_; }
PlayerStateRolling* PlayerStateRolling::Instance() { return &instance_; }
PlayerStateDead* PlayerStateDead::Instance() { return &instance_; }

// 通常状態: WASDで移動
void PlayerStateNormal::Update(Player& player) {
	player.UpdateMovement();
	player.FinalizeFrameUpdate();
}

// 回避状態: ローリング終了時に Normal へ遷移（状態クラスが次状態を決定）
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

// 死亡状態: ゲームオーバー演出のみ行う
void PlayerStateDead::Update(Player& player) {
	player.UpdateGameOverAnimation();
}
