#pragma once

#include <KamataEngine.h>
#include <array>
#include <cstddef>

// データドリブン: コンフェティの色パターン（switch 分岐の代わりにテーブル参照）
struct ConfettiColorPattern {
	float fixedR;
	float fixedG;
	float fixedB;
	int varyingAxis; // 0=R, 1=G, 2=B
};

inline constexpr std::array<ConfettiColorPattern, 6> kConfettiColorPatterns = {{
    {1.0f, 0.0f, 0.0f, 1}, // 黄系: R=1, G=rand
    {0.0f, 1.0f, 0.0f, 0}, // 赤系: G=1, R=rand
    {0.0f, 1.0f, 0.0f, 2}, // シアン系: G=1, B=rand
    {0.0f, 0.0f, 1.0f, 1}, // 青系: B=1, G=rand
    {0.0f, 0.0f, 1.0f, 0}, // マゼンタ系: B=1, R=rand
    {1.0f, 0.0f, 0.0f, 2}, // 黄系: R=1, B=rand
}};

inline KamataEngine::Vector4 MakeConfettiColor(const ConfettiColorPattern& pattern, float randomValue) {
	KamataEngine::Vector4 color = {pattern.fixedR, pattern.fixedG, pattern.fixedB, 1.0f};
	switch (pattern.varyingAxis) {
	case 0:
		color.x = randomValue;
		break;
	case 1:
		color.y = randomValue;
		break;
	default:
		color.z = randomValue;
		break;
	}
	return color;
}

inline const ConfettiColorPattern& PickConfettiColorPattern(int patternIndex) {
	size_t index = static_cast<size_t>(patternIndex) % kConfettiColorPatterns.size();
	return kConfettiColorPatterns[index];
}
