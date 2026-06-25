#include "SceneStateBase.h"
#include "GaneScene.h"

SceneStateStart SceneStateStart::instance_;
SceneStateTransitionToGame SceneStateTransitionToGame::instance_;
SceneStateTransitionFromGame SceneStateTransitionFromGame::instance_;
SceneStateGameIntro SceneStateGameIntro::instance_;
SceneStateGame SceneStateGame::instance_;
SceneStateClear SceneStateClear::instance_;
SceneStateOver SceneStateOver::instance_;

SceneStateStart* SceneStateStart::Instance() { return &instance_; }
SceneStateTransitionToGame* SceneStateTransitionToGame::Instance() { return &instance_; }
SceneStateTransitionFromGame* SceneStateTransitionFromGame::Instance() { return &instance_; }
SceneStateGameIntro* SceneStateGameIntro::Instance() { return &instance_; }
SceneStateGame* SceneStateGame::Instance() { return &instance_; }
SceneStateClear* SceneStateClear::Instance() { return &instance_; }
SceneStateOver* SceneStateOver::Instance() { return &instance_; }

// タイトル画面: Space で遷移開始（状態クラスが次状態を決定）
void SceneStateStart::Update(GameScene& scene) {
	scene.UpdateStateBody_Start();
	if (scene.input_->TriggerKey(DIK_SPACE)) {
		scene.transitionTimer_ = 0.0f;
		scene.gameSceneTimer_ = 0;
		scene.ChangeSceneState(SceneStateTransitionToGame::Instance());
	}
}

// 画面遷移（拡大）→ 縮小へ
void SceneStateTransitionToGame::Update(GameScene& scene) {
	scene.UpdateStateBody_TransitionToGame();
	if (scene.transitionTimer_ >= scene.kTransitionTime) {
		scene.transitionTimer_ = 0.0f;
		scene.ChangeSceneState(SceneStateTransitionFromGame::Instance());
	}
}

// 画面遷移（縮小）→ ゲームイントロへ
void SceneStateTransitionFromGame::Update(GameScene& scene) {
	scene.UpdateStateBody_TransitionFromGame();
	if (scene.transitionTimer_ >= scene.kTransitionTime) {
		scene.gameIntroTimer_ = 0.0f;
		if (scene.player_) {
			scene.player_->SetPosition(scene.playerIntroStartPosition_);
			scene.player_->RefreshWorldMatrix();
		}
		scene.isGameIntroFinished_ = false;
		scene.UpdateEnemyPopCommands();
		scene.ChangeSceneState(SceneStateGameIntro::Instance());
	}
}

// プレイヤー登場演出 → 本編へ
void SceneStateGameIntro::Update(GameScene& scene) {
	scene.UpdateStateBody_GameIntro();

	const float kArrivalThreshold = 0.1f;
	const bool introFinished = scene.gameIntroTimer_ >= scene.kGameIntroDuration_ ||
	                           (scene.player_ && Distance(scene.player_->GetLocalPosition(), scene.playerIntroTargetPosition_) < kArrivalThreshold);

	if (introFinished) {
		if (scene.player_) {
			scene.player_->SetPosition(scene.playerIntroTargetPosition_);
			scene.player_->RefreshWorldMatrix();
		}
		scene.isGameIntroFinished_ = true;
		scene.gameSceneTimer_ = 0;
		if (scene.railCamera_) {
			scene.railCamera_->SetCanMove(true);
		}
		scene.ChangeSceneState(SceneStateGame::Instance());
	}
}

// 本編プレイ
void SceneStateGame::Update(GameScene& scene) {
	scene.UpdateStateBody_Game();
}

// クリア演出
void SceneStateClear::Update(GameScene& scene) {
	scene.UpdateStateBody_Clear();
	if (scene.input_->TriggerKey(DIK_SPACE)) {
		scene.confettiActive_ = false;
		scene.ChangeSceneState(SceneStateStart::Instance());
	}
}

// ゲームオーバー
void SceneStateOver::Update(GameScene& scene) {
	scene.UpdateStateBody_Over();
	if (scene.input_->TriggerKey(DIK_SPACE) || scene.gameOverTimer_ >= 90) {
		scene.ResetToTitle();
		scene.ChangeSceneState(SceneStateStart::Instance());
	}
}
