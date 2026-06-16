#pragma once

#include <string>
#include <unordered_map>

// データドリブン: ゲームバランス値を CSV から読み込み、コード内の分岐を減らす
class GameBalanceTable {
public:
	bool LoadFromFile(const char* filePath);

	float GetFloat(const std::string& key, float defaultValue = 0.0f) const;
	int GetInt(const std::string& key, int defaultValue = 0) const;

	// [min, max] の範囲からランダムな float を返す（キーは keyMin / keyMax）
	float SampleRange(const std::string& keyMin, const std::string& keyMax) const;

private:
	std::unordered_map<std::string, float> values_;
};
