#pragma once

#include <KamataEngine.h>
#include <functional>
#include <sstream>
#include <string>
#include <unordered_map>

class GameScene;

// データドリブン: enemyPop.csv のコマンド処理をテーブル駆動にする
class SpawnCommandTable {
public:
	using CommandHandler = std::function<void(GameScene&, std::istringstream&)>;

	void Register(const std::string& commandName, CommandHandler handler);
	bool Execute(GameScene& scene, const std::string& commandName, std::istringstream& args) const;

private:
	std::unordered_map<std::string, CommandHandler> handlers_;
};

void RegisterDefaultSpawnCommands(SpawnCommandTable& table);
