#include "GameBalanceAccess.h"

#include "GameBalanceTable.h"

namespace {
const GameBalanceTable* g_balanceTable = nullptr;
} // namespace

namespace GameBalanceAccess {
void SetTable(const GameBalanceTable* table) { g_balanceTable = table; }

const GameBalanceTable& Get() {
	static GameBalanceTable fallback;
	return g_balanceTable != nullptr ? *g_balanceTable : fallback;
}
} // namespace GameBalanceAccess
