#pragma once
#include "KamataEngine.h"
#include <cmath>

namespace KamataEngine {

struct Matrix4x4; // 前方宣言 (Matrix.hで定義されている場合)
struct Vector3;   // 前方宣言 (Vector.hで定義されている場合)

// クォータニオン
struct Quaternion {
	float x;
	float y;
	float z;
	float w;

	// --- コンストラクタ ---
	Quaternion(float x = 0.0f, float y = 0.0f, float z = 0.0f, float w = 1.0f);

	// --- 単項演算子 ---
	Quaternion operator-() const; // 符号反転

	// --- 代入演算子 ---
	Quaternion& operator+=(const Quaternion& rhs);
	Quaternion& operator-=(const Quaternion& rhs);
	Quaternion& operator*=(float scalar);
	Quaternion& operator/=(float scalar);
	Quaternion& operator*=(const Quaternion& rhs); // クォータニオン同士の乗算

	// --- 静的メンバ関数 (ユーティリティ) ---

	// 単位クォータニオン (w=1, x=y=z=0)
	static Quaternion Identity();

	// 任意軸(axis)周りに(angle)ラジアン回転するクォータニオンを生成
	static Quaternion MakeRotateAxisAngle(const Vector3& axis, float angle);

	// クォータニオンから回転行列への変換
	static Matrix4x4 MakeMatrix(const Quaternion& q);

	// 正規化 (長さを1にする)
	static Quaternion Normalize(const Quaternion& q);

	// ノルム（長さ）の2乗
	static float Norm(const Quaternion& q);

	// ノルム（長さ）
	static float Length(const Quaternion& q);

	// 共役クォータニオン
	static Quaternion Conjugate(const Quaternion& q);

	// 逆クォータニオン
	static Quaternion Inverse(const Quaternion& q);

	// 球面線形補間 (Slerp)
	static Quaternion Slerp(const Quaternion& q0, const Quaternion& q1, float t);

	// ベクトルをクォータニオンで回転させる
	static Vector3 RotateVector(const Vector3& vector, const Quaternion& q);
};

// --- 二項演算子 (グローバル関数) ---

// クォータニオンの加算
Quaternion operator+(const Quaternion& q1, const Quaternion& q2);
// クォータニオンの減算
Quaternion operator-(const Quaternion& q1, const Quaternion& q2);
// クォータニオンのスカラー倍
Quaternion operator*(const Quaternion& q, float scalar);
Quaternion operator*(float scalar, const Quaternion& q);
// クォータニオンのスカラー除算
Quaternion operator/(const Quaternion& q, float scalar);
// クォータニオン同士の乗算
Quaternion operator*(const Quaternion& q1, const Quaternion& q2);

} // namespace KamataEngine