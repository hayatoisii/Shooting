#pragma once

namespace KamataEngine {

/// <summary>
/// 4x4行列
/// </summary>
struct Matrix4x4 final {
	float m[4][4];

	// *= 演算子のオーバーロード
	Matrix4x4& operator*=(const Matrix4x4& rhs) {
		*this = *this * rhs;
		return *this;
	}

	// 行列の掛け算のオーバーロード
	Matrix4x4 operator*(const Matrix4x4& rhs) const {
		Matrix4x4 result = {}; // 初期化
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				result.m[i][j] = 0.0f;
				for (int k = 0; k < 4; ++k) {
					result.m[i][j] += m[i][k] * rhs.m[k][j];
				}
			}
		}
		return result;
	}
};

} // namespace KamataEngine