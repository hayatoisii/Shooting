#include "SpawnCommandTable.h"

#include "GaneScene.h"

void SpawnCommandTable::Register(const std::string& commandName, CommandHandler handler) {
	handlers_[commandName] = std::move(handler);
}

bool SpawnCommandTable::Execute(GameScene& scene, const std::string& commandName, std::istringstream& args) const {
	auto it = handlers_.find(commandName);
	if (it == handlers_.end()) {
		return false;
	}
	it->second(scene, args);
	return true;
}

void RegisterDefaultSpawnCommands(SpawnCommandTable& table) {
	table.Register("POP", [](GameScene& scene, std::istringstream& args) {
		std::string word;
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;

		if (std::getline(args, word, ',')) {
			x = static_cast<float>(std::atof(word.c_str()));
		}
		if (std::getline(args, word, ',')) {
			y = static_cast<float>(std::atof(word.c_str()));
		}
		if (std::getline(args, word, ',')) {
			z = static_cast<float>(std::atof(word.c_str()));
		}

		scene.EnemySpawn(KamataEngine::Vector3(x, y, z));
	});

	// WAIT はデータ上のコマンドとして登録（現状は一括スポーンのためパラメータのみ消費）
	table.Register("WAIT", [](GameScene& scene, std::istringstream& args) {
		(void)scene;
		std::string word;
		while (std::getline(args, word, ',')) {
		}
	});
}
