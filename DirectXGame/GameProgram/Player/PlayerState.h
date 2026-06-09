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

// ゴルフ: 待機（地面に止まっていて SPACE 第1打で照準開始）
class PlayerStateNormal : public PlayerState {
public:
	static PlayerStateNormal* Instance();
	void Update(Player& player) override;
	const char* GetStateName() const override { return "Normal"; }
private:
	PlayerStateNormal() = default;
	static PlayerStateNormal instance_;
};

// ゴルフ: 照準中（矢印が左右に往復して SPACE 第2打で方向確定 → ゲージへ）
class PlayerStateAiming : public PlayerState {
public:
	static PlayerStateAiming* Instance();
	void Update(Player& player) override;
	const char* GetStateName() const override { return "Aiming"; }
private:
	PlayerStateAiming() = default;
	static PlayerStateAiming instance_;
};

// ゴルフ: 高さ照準中（矢印が上下にロフト角を往復。SPACE 第3打で確定）
class PlayerStateAimingHeight : public PlayerState {
public:
	static PlayerStateAimingHeight* Instance();
	void Update(Player& player) override;
	const char* GetStateName() const override { return "AimingHeight"; }
private:
	PlayerStateAimingHeight() = default;
	static PlayerStateAimingHeight instance_;
};

// ゴルフ: ゲージ中（バーが往復して SPACE 第4打で打力決定）
class PlayerStateGauging : public PlayerState {
public:
	static PlayerStateGauging* Instance();
	void Update(Player& player) override;
	const char* GetStateName() const override { return "Gauging"; }
private:
	PlayerStateGauging() = default;
	static PlayerStateGauging instance_;
};

// ゴルフ: スイング演出中（パターが回転してボールを打つ）
class PlayerStateSwing : public PlayerState {
public:
	static PlayerStateSwing* Instance();
	void Update(Player& player) override;
	const char* GetStateName() const override { return "Swing"; }
private:
	PlayerStateSwing() = default;
	static PlayerStateSwing instance_;
};

// ゴルフ: 飛翔中（Z+上方向に飛んでバウンドして転がる）
class PlayerStateFlying : public PlayerState {
public:
	static PlayerStateFlying* Instance();
	void Update(Player& player) override;
	const char* GetStateName() const override { return "Flying"; }
private:
	PlayerStateFlying() = default;
	static PlayerStateFlying instance_;
};

// 旧: 回避（互換維持）
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
