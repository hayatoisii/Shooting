#pragma once

class Player;

// プレイヤー行動状態の基底クラス（State Pattern）
// ポイント: Player が状態オブジェクトを持ち、仮想関数で振る舞いを切り替える
class PlayerState {
public:
	virtual ~PlayerState() = default;

	// 毎フレームの更新（状態ごとに処理が異なる）
	virtual void Update(Player& player) = 0;

	// 状態名（デバッグ・レポート用）
	virtual const char* GetStateName() const = 0;

	// 回避中か（当たり判定の無敵判定に使用）
	virtual bool IsRolling() const { return false; }
};

// --- 各状態クラス（シングルトン） ---

// 通常飛行
class PlayerStateNormal : public PlayerState {
public:
	static PlayerStateNormal* Instance();
	void Update(Player& player) override;
	const char* GetStateName() const override { return "Normal"; }

private:
	PlayerStateNormal() = default;
	static PlayerStateNormal instance_;
};

// 回避（ローリング）
class PlayerStateRolling : public PlayerState {
public:
	static PlayerStateRolling* Instance();
	void Update(Player& player) override;
	const char* GetStateName() const override { return "Rolling"; }
	bool IsRolling() const override { return true; }

private:
	PlayerStateRolling() = default;
	static PlayerStateRolling instance_;
};

// 死亡（ゲームオーバー演出）
class PlayerStateDead : public PlayerState {
public:
	static PlayerStateDead* Instance();
	void Update(Player& player) override;
	const char* GetStateName() const override { return "Dead"; }

private:
	PlayerStateDead() = default;
	static PlayerStateDead instance_;
};
