#pragma once
#include "Quaternion.h"
#include "MT.h"
#include <3d/Camera.h>
#include <3d/WorldTransform.h>

class Player;

class RailCamera {

public:
	struct Rect {
		float left = 0.0f;
		float right = 1.0f;
		float bottom = 0.0f;
		float top = 1.0f;
	};

	void Initialize(const KamataEngine::Vector3& pos, const KamataEngine::Vector3& rad);
	void Update();

	void SetTarget(Player* target) { target_ = target; }
	const Camera& GetViewProjection() { return camera_; }
	const WorldTransform& GetWorldTransform() { return worldtransfrom_; }

	const KamataEngine::Vector3& GetRotationVelocity() const { return rotationVelocity_; }

	void SetCanMove(bool canMove) { canMove_ = canMove; }

	// ゴルフ用: 固定カメラモード（後方互換のために残す）
	void SetFixedMode(bool fixed) { fixedMode_ = fixed; }
	bool IsFixedMode() const { return fixedMode_; }

	// ゴルフ用: ボール追従カメラモード
	// true にするとボールを後上方から滑らかに追いかける
	void SetGolfChaseMode(bool enabled) { golfChaseMode_ = enabled; }
	bool IsGolfChaseMode() const { return golfChaseMode_; }

	// 飛翔中フラグ（GameScene が毎フレームセット）
	void SetBallFlying(bool flying) { isBallFlying_ = flying; }
	// ゴール位置をセット（GameScene の Initialize 後に呼ぶ）
	void SetGoalPosition(const KamataEngine::Vector3& pos) { goalPosition_ = pos; }
	const KamataEngine::Vector3& GetGoalPosition() const { return goalPosition_; }
	// 現在のヨー角を取得（Player の照準方向に使用）
	float GetCurrentYaw() const { return currentYaw_; }
	// ホイールズーム倍率（1.0=標準, 2.0=最大引き）
	float GetOrbitZoom() const { return orbitZoom_; }

	void Reset();

	void ApplyAimAssist(float ndcX, float ndcY);

	KamataEngine::Matrix4x4 MakeIdentityMatrix();

	void Dodge(float direction);

private:
	Player* target_ = nullptr;

	KamataEngine::WorldTransform worldtransfrom_;

	KamataEngine::Vector3 initialPosition_;
	KamataEngine::Vector3 initialRotationEuler_;
	
	// Playerの移動範囲制限（円状。旧シューティング用・ゴルフは Player 側で制限）
	const float kMaxMoveRadius_ = 2000.0f;

	KamataEngine::Quaternion rotation_;

	KamataEngine::Vector3 rotationVelocity_{0.0f, 0.0f, 0.0f};

	KamataEngine::Vector3 assistAcceleration_ = {0.0f, 0.0f, 0.0f}; // アシストによる加速度

	Camera camera_;

	bool canMove_;

	// 2D固定カメラかどうか
	bool fixedMode_ = false;

	// ゴルフ追従カメラかどうか
	bool golfChaseMode_ = false;

	// ゴルフ追従カメラのオフセット（ボール位置からの相対位置）
	// Y を大きく・Z を長くすることでボールが画面下寄りに映る
	const float kGolfCamOffsetY_ = 20.0f;  // ボールより上（高いほどボールが下に映る）
	const float kGolfCamOffsetZ_ = -36.0f; // ボールより手前（Z-）
	// 追従の滑らかさ（0=瞬時, 1=動かない）
	const float kGolfCamLerpXZ_ = 0.10f;
	const float kGolfCamLerpY_  = 0.06f;

	// ゴール位置（着地後にカメラをゴール方向へ向ける）
	KamataEngine::Vector3 goalPosition_ = {0.0f, 10.0f, 1200.0f};

	// 飛翔中フラグ（外部から毎フレームセット）
	bool isBallFlying_ = false;
	// 飛翔中は進行方向へ、着地後は WASD でボール周りを円運動
	const float kReturnToForwardSpeed_ = 0.07f; // 飛翔開始直後のフォールバック用
	// 着地後: WASD でボール中心に球面軌道（即時追従 + 離したときだけ微慣性）
	const float kOrbitYawSpeed_      = 0.0168f; // 0.021 × 0.8
	const float kOrbitInertiaDecay_  = 0.78f;
	const float kOrbitStopThreshold_ = 0.0004f;
	const float kMouseOrbitSens_     = 0.0012f; // 0.0015 × 0.8
	const float kWheelZoomStep_      = 0.1f;   // ホイール1ノッチあたり
	const float kOrbitZoomMin_       = 1.0f;   // 現在距離（デフォルト）
	const float kOrbitZoomMax_       = 2.0f;   // 最大2倍引き
	float orbitZoom_         = 1.0f;
	const float kGroundLevel_        = 0.0f;
	const float kMinCamClearance_    = 2.5f;
	float orbitYawVelocity_  = 0.0f;
	float orbitElevVelocity_ = 0.0f;
	float currentYaw_      = 0.0f;
	float orbitElevation_  = 0.28f;
	// 1フレーム前のボール位置（進行方向の計算に使用）
	KamataEngine::Vector3 prevBallPos_ = {0.0f, 0.0f, 0.0f};

	bool isDodging_ = false;
	float dodgeTimer_ = 0.0f;
	float dodgeDirection_ = 0.0f;

	// 回避にかかる時間、小さくすると速く回る
	const float kDodgeDuration_ = 30.0f;

};