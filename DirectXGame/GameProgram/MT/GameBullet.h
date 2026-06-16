#pragma once

#include <KamataEngine.h>



// 弾（自機弾・敵弾）の共通基底クラス

// Template Method Pattern: 更新処理の骨格を共通化し、派生クラスで差分を実装する

class GameBullet {

public:

	virtual ~GameBullet() = default;



	// Template Method: 弾1フレーム分の更新フロー

	void UpdateFrame();



	virtual KamataEngine::Vector3 GetWorldPosition() const = 0;

	virtual bool IsDead() const = 0;

	virtual void OnCollision() = 0;

	virtual float GetCollisionRadius() const = 0;

	virtual const char* GetKindName() const = 0;



protected:

	virtual bool UpdateLifetime() = 0;

	virtual void UpdatePreMovement() {}

	virtual void ApplyMovement() = 0;

	virtual void UpdateTransform() = 0;



	bool isDead_ = false;

};



// ポリモーフィズム: 弾の消滅処理を統一

void MarkBulletDestroyed(GameBullet* bullet);

