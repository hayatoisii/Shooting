#include "Quaternion.h" // (パスは環境に合わせてください)
#include <cassert>
#include <cmath>

namespace KamataEngine {

// --- コンストラクタ ---
Quaternion::Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

// --- 単項演算子 ---
Quaternion Quaternion::operator-() const { return Quaternion(-x, -y, -z, -w); }

// --- 代入演算子 ---
Quaternion& Quaternion::operator+=(const Quaternion& rhs) {
	x += rhs.x;
	y += rhs.y;
	z += rhs.z;
	w += rhs.w;
	return *this;
}

Quaternion& Quaternion::operator-=(const Quaternion& rhs) {
	x -= rhs.x;
	y -= rhs.y;
	z -= rhs.z;
	w -= rhs.w;
	return *this;
}

Quaternion& Quaternion::operator*=(float scalar) {
	x *= scalar;
	y *= scalar;
	z *= scalar;
	w *= scalar;
	return *this;
}

Quaternion& Quaternion::operator/=(float scalar) {
	// ゼロ除算チェック (assertはデバッグビルドでのみ有効)
	assert(scalar != 0.0f && "Quaternion division by zero!");
	x /= scalar;
	y /= scalar;
	z /= scalar;
	w /= scalar;
	return *this;
}

Quaternion& Quaternion::operator*=(const Quaternion& rhs) {
	// 乗算結果を一時変数に格納してから代入
	Quaternion result = *this * rhs;
	*this = result;
	return *this;
}

// --- 静的メンバ関数 ---

Quaternion Quaternion::Identity() { return Quaternion(0.0f, 0.0f, 0.0f, 1.0f); }

Quaternion Quaternion::MakeRotateAxisAngle(const Vector3& axis, float angle) {
	float halfAngle = angle * 0.5f;
	float sinHalfAngle = std::sin(halfAngle);

	// 軸ベクトルを正規化
	// (KamataEngine::Vector3 に Normalize が静的関数として存在することを前提)
	Vector3 normalizedAxis = axis.Normalize();

	Quaternion result;
	result.x = normalizedAxis.x * sinHalfAngle;
	result.y = normalizedAxis.y * sinHalfAngle;
	result.z = normalizedAxis.z * sinHalfAngle;
	result.w = std::cos(halfAngle);
	return result;
}

Matrix4x4 Quaternion::MakeMatrix(const Quaternion& q) {
	// クォータニオンは正規化(Normalize)されていることを前提とします

	float xx = q.x * q.x;
	float yy = q.y * q.y;
	float zz = q.z * q.z;
	float xy = q.x * q.y;
	float xz = q.x * q.z;
	float yz = q.y * q.z;
	float wx = q.w * q.x;
	float wy = q.w * q.y;
	float wz = q.w * q.z;

	Matrix4x4 result = {}; // ゼロ初期化

	result.m[0][0] = 1.0f - 2.0f * (yy + zz);
	result.m[0][1] = 2.0f * (xy + wz);
	result.m[0][2] = 2.0f * (xz - wy);
	result.m[0][3] = 0.0f;

	result.m[1][0] = 2.0f * (xy - wz);
	result.m[1][1] = 1.0f - 2.0f * (xx + zz);
	result.m[1][2] = 2.0f * (yz + wx);
	result.m[1][3] = 0.0f;

	result.m[2][0] = 2.0f * (xz + wy);
	result.m[2][1] = 2.0f * (yz - wx);
	result.m[2][2] = 1.0f - 2.0f * (xx + yy);
	result.m[2][3] = 0.0f;

	result.m[3][0] = 0.0f;
	result.m[3][1] = 0.0f;
	result.m[3][2] = 0.0f;
	result.m[3][3] = 1.0f;

	return result;
}

Quaternion Quaternion::Normalize(const Quaternion& q) {
	float len = Length(q);
	if (len != 0.0f) {
		// 長さで割って正規化
		return q / len;
	}
	// ゼロクォータニオンの場合は単位クォータニオンを返す（エラー回避）
	return Identity();
}

float Quaternion::Norm(const Quaternion& q) {
	// (w*w + x*x + y*y + z*z)
	return (q.x * q.x) + (q.y * q.y) + (q.z * q.z) + (q.w * q.w);
}

float Quaternion::Length(const Quaternion& q) {
	// Normの平方根
	return std::sqrt(Norm(q));
}

Quaternion Quaternion::Conjugate(const Quaternion& q) {
	// 虚数部(x, y, z)の符号を反転
	return Quaternion(-q.x, -q.y, -q.z, q.w);
}

Quaternion Quaternion::Inverse(const Quaternion& q) {
	float norm = Norm(q);
	if (norm == 0.0f) {
		// ゼロクォータニオンには逆元がない
		assert(!"Inverse of zero quaternion is undefined!");
		return Identity(); // エラー回避
	}

	// 逆クォータニオン = 共役 / ノルム
	float invNorm = 1.0f / norm;
	return Conjugate(q) * invNorm;
}

Quaternion Quaternion::Slerp(const Quaternion& q0, const Quaternion& q1, float t) {
	float dot = q0.x * q1.x + q0.y * q1.y + q0.z * q1.z + q0.w * q1.w;
	Quaternion q1_ = q1;

	// ドット積がマイナスの場合、最短経路を通るようにq1を反転
	// (q と -q は同じ回転を表すが、補間経路が異なる)
	if (dot < 0.0f) {
		q1_ = -q1_;
		dot = -dot;
	}

	// ほぼ同一のクォータニオンの場合 (角度が非常に近い)
	// ゼロ除算を避けるため、線形補間(Lerp)して正規化する
	if (dot > 0.9995f) {
		Quaternion result = q0 + (q1_ - q0) * t;
		return Normalize(result);
	}

	// 球面線形補間
	float theta_0 = std::acos(dot); // q0とq1の間の角度
	float theta = theta_0 * t;      // 補間角度
	float sin_theta = std::sin(theta);
	float sin_theta_0 = std::sin(theta_0);

	if (sin_theta_0 == 0.0f) {
		// (dot > 0.9995f のチェックでカバーされるはずだが、念のため)
		return q0;
	}

	float scale0 = std::cos(theta) - dot * sin_theta / sin_theta_0;
	float scale1 = sin_theta / sin_theta_0;

	return (q0 * scale0) + (q1_ * scale1);
}

Vector3 Quaternion::RotateVector(const Vector3& vector, const Quaternion& q) {
	// v' = q * v * q^-1 (qは正規化されている前提)
	// v (Vector3) を純粋クォータニオン (w=0) として扱う
	Quaternion vQuat(vector.x, vector.y, vector.z, 0.0f);

	// q^-1 (逆クォータニオン) は、正規化クォータニオンの場合 q* (共役) と同じ
	Quaternion qConj = Conjugate(q);

	Quaternion resultQuat = q * vQuat * qConj;

	// 結果の虚数部(x, y, z)が回転後のベクトル
	return Vector3(resultQuat.x, resultQuat.y, resultQuat.z);
}

// --- 二項演算子 (グローバル) ---

Quaternion operator+(const Quaternion& q1, const Quaternion& q2) { return Quaternion(q1.x + q2.x, q1.y + q2.y, q1.z + q2.z, q1.w + q2.w); }

Quaternion operator-(const Quaternion& q1, const Quaternion& q2) { return Quaternion(q1.x - q2.x, q1.y - q2.y, q1.z - q2.z, q1.w - q2.w); }

Quaternion operator*(const Quaternion& q, float scalar) { return Quaternion(q.x * scalar, q.y * scalar, q.z * scalar, q.w * scalar); }

Quaternion operator*(float scalar, const Quaternion& q) {
	// 交換法則
	return q * scalar;
}

Quaternion operator/(const Quaternion& q, float scalar) {
	assert(scalar != 0.0f && "Quaternion division by zero!");
	return Quaternion(q.x / scalar, q.y / scalar, q.z / scalar, q.w / scalar);
}

Quaternion operator*(const Quaternion& q1, const Quaternion& q2) {
	// (w1w2 - x1x2 - y1y2 - z1z2) +
	// (w1x2 + x1w2 + y1z2 - z1y2)i +
	// (w1y2 - x1z2 + y1w2 + z1x2)j +
	// (w1z2 + x1y2 - y1x2 + z1w2)k

	float resultW = (q1.w * q2.w) - (q1.x * q2.x) - (q1.y * q2.y) - (q1.z * q2.z);
	float resultX = (q1.w * q2.x) + (q1.x * q2.w) + (q1.y * q2.z) - (q1.z * q2.y);
	float resultY = (q1.w * q2.y) - (q1.x * q2.z) + (q1.y * q2.w) + (q1.z * q2.x);
	float resultZ = (q1.w * q2.z) + (q1.x * q2.y) - (q1.y * q2.x) + (q1.z * q2.w);

	return Quaternion(resultX, resultY, resultZ, resultW);
}

} // namespace KamataEngine