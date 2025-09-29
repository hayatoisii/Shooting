#pragma once

namespace KamataEngine {

/// <summary>
/// 3次元ベクトル
/// </summary>

	struct Vector3 final {
	float x, y, z;

	// 内積を計算するメンバー関数
	float dot(const Vector3& other) const { return x * other.x + y * other.y + z * other.z; }

	// スカラー倍を行うメンバー関数
	Vector3 scale(float scalar) const { return {x * scalar, y * scalar, z * scalar}; }

	// 演算子オーバーロード
	Vector3 operator+(const Vector3& other) const { return {x + other.x, y + other.y, z + other.z}; }

	Vector3 operator-(const Vector3& other) const { return {x - other.x, y - other.y, z - other.z}; }

	Vector3 operator/(float scalar) const { return {x / scalar, y / scalar, z / scalar}; }
	Vector3 operator-(float scalar) const { return {x - scalar, y - scalar, z - scalar}; }
	Vector3 operator+(float scalar) const { return {x + scalar, y + scalar, z + scalar}; }
	// ベクトルの長さの二乗を計算するメンバー関数
	float lengthSquared() const { return x * x + y * y + z * z; }

	// += 演算子のオーバーロード
	Vector3& operator+=(const Vector3& other) {
		this->x += other.x;
		this->y += other.y;
		this->z += other.z;
		return *this;
	}

	Vector3 operator*(float scalar) const { return {x * scalar, y * scalar, z * scalar}; }
};

} // namespace KamataEngine
