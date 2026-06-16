#include "GameBalanceTable.h"

#include <cstdlib>
#include <fstream>
#include <sstream>

bool GameBalanceTable::LoadFromFile(const char* filePath) {
	values_.clear();

	std::ifstream file(filePath);
	if (!file.is_open()) {
		return false;
	}

	std::string line;
	while (std::getline(file, line)) {
		if (line.empty() || line[0] == '#') {
			continue;
		}

		std::istringstream stream(line);
		std::string key;
		std::string valueStr;
		if (!std::getline(stream, key, ',') || !std::getline(stream, valueStr, ',')) {
			continue;
		}

		values_[key] = static_cast<float>(std::atof(valueStr.c_str()));
	}

	return true;
}

float GameBalanceTable::GetFloat(const std::string& key, float defaultValue) const {
	auto it = values_.find(key);
	if (it == values_.end()) {
		return defaultValue;
	}
	return it->second;
}

int GameBalanceTable::GetInt(const std::string& key, int defaultValue) const {
	return static_cast<int>(GetFloat(key, static_cast<float>(defaultValue)));
}

float GameBalanceTable::SampleRange(const std::string& keyMin, const std::string& keyMax) const {
	float minValue = GetFloat(keyMin);
	float maxValue = GetFloat(keyMax);
	if (maxValue < minValue) {
		std::swap(minValue, maxValue);
	}
	float range = maxValue - minValue;
	if (range <= 0.0f) {
		return minValue;
	}
	return minValue + (static_cast<float>(rand()) / RAND_MAX) * range;
}
