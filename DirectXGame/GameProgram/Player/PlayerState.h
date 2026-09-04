#pragma once

class Player;

// プレイヤー行動状態の基底クラス（State Pattern）
class PlayerState {
public:
	virtual ~PlayerState() = default;
	virtual void Update(Player& player) = 0;
	virtual const char* GetStateName() const = 0;
	virtual bool IsRolling() const { return false; }
};

// 開始待ち: 糸でぶら下がり、SPACE で振り子開始
class PlayerStateWaiting : public PlayerState {
public:
	static PlayerStateWaiting* Instance();
	void Update(Player& player) override;
	const char* GetStateName() const override { return "Waiting"; }

private:
	PlayerStateWaiting() = default;
	static PlayerStateWaiting instance_;
};

// 振り子中: SPACE 押し続け、離すと糸を切って飛翔
class PlayerStateSwinging : public PlayerState {
public:
	static PlayerStateSwinging* Instance();
	void Update(Player& player) override;
	const char* GetStateName() const override { return "Swinging"; }

private:
	PlayerStateSwinging() = default;
	static PlayerStateSwinging instance_;
};

// 糸切り後の自由飛翔: 画面内で SPACE 押下で新アンカー接続
class PlayerStateFlying : public PlayerState {
public:
	static PlayerStateFlying* Instance();
	void Update(Player& player) override;
	const char* GetStateName() const override { return "Flying"; }

private:
	PlayerStateFlying() = default;
	static PlayerStateFlying instance_;
};

// 失敗（画面外ミスなど）
class PlayerStateDead : public PlayerState {
public:
	static PlayerStateDead* Instance();
	void Update(Player& player) override;
	const char* GetStateName() const override { return "Dead"; }

private:
	PlayerStateDead() = default;
	static PlayerStateDead instance_;
};

// --- 旧ゴルフ状態（互換スタブ。遷移しない） ---
class PlayerStateNormal : public PlayerState {
public:
	static PlayerStateNormal* Instance();
	void Update(Player& player) override;
	const char* GetStateName() const override { return "Normal"; }

private:
	PlayerStateNormal() = default;
	static PlayerStateNormal instance_;
};
class PlayerStateAiming : public PlayerState {
public:
	static PlayerStateAiming* Instance();
	void Update(Player& player) override;
	const char* GetStateName() const override { return "Aiming"; }

private:
	PlayerStateAiming() = default;
	static PlayerStateAiming instance_;
};
class PlayerStateAimingHeight : public PlayerState {
public:
	static PlayerStateAimingHeight* Instance();
	void Update(Player& player) override;
	const char* GetStateName() const override { return "AimingHeight"; }

private:
	PlayerStateAimingHeight() = default;
	static PlayerStateAimingHeight instance_;
};
class PlayerStateGauging : public PlayerState {
public:
	static PlayerStateGauging* Instance();
	void Update(Player& player) override;
	const char* GetStateName() const override { return "Gauging"; }

private:
	PlayerStateGauging() = default;
	static PlayerStateGauging instance_;
};
class PlayerStateSwing : public PlayerState {
public:
	static PlayerStateSwing* Instance();
	void Update(Player& player) override;
	const char* GetStateName() const override { return "Swing"; }

private:
	PlayerStateSwing() = default;
	static PlayerStateSwing instance_;
};
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
