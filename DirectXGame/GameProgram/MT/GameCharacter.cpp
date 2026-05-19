#include "GameCharacter.h"

// 衝突時のダメージ処理（Player / Enemy どちらでも同じ呼び方で処理できる）
void ApplyCollisionDamage(GameCharacter* character) {
	if (character == nullptr || character->IsDead()) {
		return;
	}
	// 仮想関数のため、実際のオブジェクト型の OnCollision() が呼ばれる
	character->OnCollision();
}
