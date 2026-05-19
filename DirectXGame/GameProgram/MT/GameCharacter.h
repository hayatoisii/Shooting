#pragma once
#include <KamataEngine.h>

// ゲーム内キャラクター（プレイヤー・敵など）の共通基底クラス
// ポリモーフィズム: GameCharacter* 経由で呼ぶと、実際の型（Player / Enemy）の処理が実行される
class GameCharacter {
public:
	virtual ~GameCharacter() = default;

	// 純粋仮想関数（派生クラスで必ず override する）
	virtual KamataEngine::Vector3 GetWorldPosition() const = 0;
	virtual bool IsDead() const = 0;
	virtual void OnCollision() = 0;
	virtual int GetHp() const = 0;
	virtual int GetMaxHp() const = 0;
	virtual float GetCollisionRadius() const = 0;

	// 種別名（レポート・デバッグ用。実行時に実際の型が分かる）
	virtual const char* GetKindName() const = 0;
};

// ポリモーフィズム: 基底クラスポインタ経由でダメージ処理を統一
void ApplyCollisionDamage(GameCharacter* character);
