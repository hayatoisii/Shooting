#pragma once

class GameBalanceTable;

// データドリブン: 弾・敵など複数箇所から参照するバランス表へのアクセス
namespace GameBalanceAccess {
void SetTable(const GameBalanceTable* table);
const GameBalanceTable& Get();
} // namespace GameBalanceAccess
