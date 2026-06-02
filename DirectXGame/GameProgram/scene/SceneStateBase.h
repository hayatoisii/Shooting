#pragma once

class GameScene;

// シーン状態の種別（旧 SceneState enum に相当）
enum class SceneStateKind {
	Start,
	TransitionToGame,
	TransitionFromGame,
	GameIntro,
	Game,
	Clear,
	Over,
};

// ゲームシーン状態の基底クラス（State Pattern）
class SceneStateBase {
public:
	virtual ~SceneStateBase() = default;

	// 毎フレームの更新
	virtual void Update(GameScene& scene) = 0;

	virtual SceneStateKind GetKind() const = 0;
};

// --- 各シーン状態（シングルトン） ---

class SceneStateStart : public SceneStateBase {
public:
	static SceneStateStart* Instance();
	void Update(GameScene& scene) override;
	SceneStateKind GetKind() const override { return SceneStateKind::Start; }

private:
	SceneStateStart() = default;
	static SceneStateStart instance_;
};

class SceneStateTransitionToGame : public SceneStateBase {
public:
	static SceneStateTransitionToGame* Instance();
	void Update(GameScene& scene) override;
	SceneStateKind GetKind() const override { return SceneStateKind::TransitionToGame; }

private:
	SceneStateTransitionToGame() = default;
	static SceneStateTransitionToGame instance_;
};

class SceneStateTransitionFromGame : public SceneStateBase {
public:
	static SceneStateTransitionFromGame* Instance();
	void Update(GameScene& scene) override;
	SceneStateKind GetKind() const override { return SceneStateKind::TransitionFromGame; }

private:
	SceneStateTransitionFromGame() = default;
	static SceneStateTransitionFromGame instance_;
};

class SceneStateGameIntro : public SceneStateBase {
public:
	static SceneStateGameIntro* Instance();
	void Update(GameScene& scene) override;
	SceneStateKind GetKind() const override { return SceneStateKind::GameIntro; }

private:
	SceneStateGameIntro() = default;
	static SceneStateGameIntro instance_;
};

class SceneStateGame : public SceneStateBase {
public:
	static SceneStateGame* Instance();
	void Update(GameScene& scene) override;
	SceneStateKind GetKind() const override { return SceneStateKind::Game; }

private:
	SceneStateGame() = default;
	static SceneStateGame instance_;
};

class SceneStateClear : public SceneStateBase {
public:
	static SceneStateClear* Instance();
	void Update(GameScene& scene) override;
	SceneStateKind GetKind() const override { return SceneStateKind::Clear; }

private:
	SceneStateClear() = default;
	static SceneStateClear instance_;
};

class SceneStateOver : public SceneStateBase {
public:
	static SceneStateOver* Instance();
	void Update(GameScene& scene) override;
	SceneStateKind GetKind() const override { return SceneStateKind::Over; }

private:
	SceneStateOver() = default;
	static SceneStateOver instance_;
};
