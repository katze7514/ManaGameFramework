/*!
 *	@brief ちょっとしたことに使う便利な関数たちを定義する
 */
#pragma once

#include <cstdio>
#include <boost/lexical_cast.hpp>

#include "../Debug/logger.h"

namespace mana{
namespace utility{

// _から始まる関数書くの気持ち悪いからというだけ
#define snprintf _snprintf_s;

//! lexical_castの文字列変換への名前ショートカット
template<typename T>
inline string to_str(const T& val)
{
	return std::move(boost::lexical_cast<string>(val));
}

//! lexical_castの文字列変換への名前ショートカット。spaceを付与する
template<typename T>
inline string to_str_s(const T& val)
{
	return std::move(boost::lexical_cast<string>(val) + " ");
}

//! boolを文字列にする特殊化
template<>
inline string to_str(const bool& b)
{
	if(b) return "true";
	return "false";
}

//! boolを文字列にする特殊化。space付き
template<>
inline string to_str_s(const bool& b)
{
	if(b) return "true ";
	return "false ";
}

//! bits中の立っているbitを数える
inline uint32_t bit_count(uint32_t bits)
{
	// by ハッカーのたのしみ
    bits = (bits & 0x55555555) + (bits >> 1 & 0x55555555);
    bits = (bits & 0x33333333) + (bits >> 2 & 0x33333333);
    bits = (bits & 0x0f0f0f0f) + (bits >> 4 & 0x0f0f0f0f);
    bits = (bits & 0x00ff00ff) + (bits >> 8 & 0x00ff00ff);
    return (bits & 0x0000ffff) + (bits >>16 & 0x0000ffff);
}

//! flagのbitが立っているかをtestする
template<typename T>
inline bool bit_test(T flag, T bit)
{
	return (flag & bit)>0;
}

//! flagにbitを追加した値を返す
template<typename T>
inline T bit_add(T flag, T bit)
{
	return flag | bit;
}

//! flagからbitを削除した値を返す
template<typename T>
inline T bit_remove(T flag, T bit)
{
	return flag & (~bit);
}

//! HREUSLTを返すWinAPI用の戻り値チェッカー。失敗してた場合、エラーログを吐く
inline bool check_hresult(HRESULT r, const string& sErr="")
{
	if(FAILED(r))
	{
		if(sErr != "") mana::debug::logger::fatalln(sErr + " " + to_str(r & 0x0000FFFF)); // HRESULTは下位16bitがエラーコード
		return false;
	}
	return true;
}

//! HREUSLTを返すWinAPI用の戻り値チェッカー。失敗してた場合、ハンドラを呼ぶ
inline bool check_hresult(HRESULT r, function<void(HRESULT)>& err)
{
	if(FAILED(r))
	{
		if(err) err(r & 0x0000FFFF); // HRESULTは下位16bitがエラーコード
		return false;
	}
	return true;
}

//! 16進数文字列を数値に変換する
inline uint32_t hex_to_uint32(const string& sHex)
{
	if(sHex.empty()) return 0;

	char* endp;
	return ::strtoul(sHex.c_str(), &endp, 16);
}

} // namespace utility end
} // namespace mana end
