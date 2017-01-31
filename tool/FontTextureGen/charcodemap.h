#pragma once

#include <boost/bimap.hpp>

// Unicode SJISコード変換
// コード自体は常に2バイトで扱う
// 1byteコードは、下位8bitに入る
class sijis_unicode_codemap
{
public:
	// 左sjis 右unicode
	typedef boost::bimap<uint16_t, uint16_t> code_map;

	sijis_unicode_codemap();

	// SJISからUnicodeに変換
	uint16_t to_unicode(uint16_t nSjis)const;
	// UnicodeからSJISに変換
	uint16_t to_sjis(uint16_t nUni)const;

	// SJISからUnicodeに変換（1文字だけ入れる）
	uint16_t to_unicode(const string& sSjis)const;
	// UnicodeからSJISに変換（1文字だけ入れる）
	uint16_t to_sjis(const wstring& sUni)const;

	const code_map::left_map&  sjis_to_unicode_map()const{ return mapCode_.left; }
	const code_map::right_map& unicode_to_sjis_map()const{ return mapCode_.right; }

private:
	// Unicode-sjis変換マップ
	code_map mapCode_;
};

extern const sijis_unicode_codemap gCodeMap;