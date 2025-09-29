#pragma once

#include <string>

namespace KamataEngine {

/// <summary>
/// マルチバイト文字列をワイド文字列に変換する
/// </summary>
/// <param name="str">マルチバイト文字列</param>
/// <returns>ワイド文字列</returns>
std::wstring ConvertStringMultiByteToWide(const std::string& str);

} // namespace KamataEngine
