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
		scene.currentStage_ = 1;
		if (scene.player_) {
			scene.player_->ResetStats();
			scene.player_->SetPosition(scene.playerIntroStartPosition_);
			scene.player_->RefreshWorldMatrix();
		}
		scene.isGameIntroFinished_ = false;
		scene.LoadStage(1);
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
		scene.debug10ElapsedSec_ = 0.0f;
		// ゴルフ: ゲーム開始時のボールZ座標を記録（飛距離カウンターの基準点）
		if (scene.player_) {
			scene.ballStartZ_ = scene.player_->GetWorldPosition().z;
			scene.score_ = 0;
			scene.UpdateScoreSprites();
		}
		scene.ChangeSceneState(SceneStateGame::Instance());
	}
}

// 本編プレイ
void SceneStateGame::Update(GameScene& scene) {
	scene.UpdateStateBody_Game();

	// デバッグ: 指定秒数でタイトルへ（状態クラスが遷移を決定）
	if (scene.debug10 && scene.isGameIntroFinished_) {
		const float kDeltaSec = 1.0f / 60.0f;
		scene.debug10ElapsedSec_ += kDeltaSec;
		if (scene.debug10ElapsedSec_ >= scene.kDebug10Seconds) {
			scene.ResetToTitle();
			scene.ChangeSceneState(SceneStateStart::Instance());
		}
	}
}

// クリア演出
void SceneStateClear::Update(GameScene& scene) {
	scene.UpdateStateBody_Clear();
	if (scene.input_->TriggerKey(DIK_SPACE)) {
		scene.confettiActive_ = false;
		if (scene.currentStage_ < scene.kMaxStage_) {
			scene.AdvanceToNextStage();
			scene.ChangeSceneState(SceneStateGameIntro::Instance());
		} else {
			scene.ResetToTitle();
			scene.ChangeSceneState(SceneStateStart::Instance());
		}
	}
}

// ゲームオーバー
void SceneStateOver::Update(GameScene& scene) {
	scene.UpdateStateBody_Over();
	if (scene.input_->TriggerKey(DIK_SPACE)) {
		scene.ResetToTitle();
		scene.ChangeSceneState(SceneStateStart::Instance());
	}
}
