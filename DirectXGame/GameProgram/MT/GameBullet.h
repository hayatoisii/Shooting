#pragma once
#include <KamataEngine.h>

// 弾（自機弾・敵弾）の共通基底クラス
// ポリモーフィズム: GameBullet* 経由で弾の種類に依存しない処理ができる
class GameBullet {
public:
	virtual ~GameBullet() = default;

	virtual KamataEngine::Vector3 GetWorldPosition() const = 0;
	virtual bool IsDead() const = 0;
	virtual void OnCollision() = 0;
	virtual float GetCollisionRadius() const = 0;

	virtual const char* GetKindName() const = 0;
};

// ポリモーフィズム: 弾の消滅処理を統一
void MarkBulletDestroyed(GameBullet* bullet);
