#include "GameBullet.h"

// 弾が何かに当たったときの共通処理
void MarkBulletDestroyed(GameBullet* bullet) {
	if (bullet == nullptr || bullet->IsDead()) {
		return;
	}
	// 仮想関数のため、PlayerBullet / EnemyBullet それぞれの OnCollision() が呼ばれる
	bullet->OnCollision();
}
