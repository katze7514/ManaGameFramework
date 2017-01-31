#include "../mana_common.h"

#include "../Concurrent/lock_helper.h"

#include "text_data.h"
#include "renderer_2d_cmd.h"
#include "renderer_sprite_queue.h"
#include "renderer_text.h"

namespace mana{
namespace draw{

namespace{
inline void assign_code(uint16_t& code, uint8_t byteUP, uint8_t byteDown)
{
	volatile uint8_t* p = reinterpret_cast<uint8_t*>(&code);

	// リトルエンディアン
	p[0] = byteDown;
	p[1] = byteUP;
}

// 文字をコード(sjis)に変換する
inline uint16_t to_sjis_code(const string& s)
{
	if(s.empty()) return 0;

	uint16_t code=0;
	const char* p = s.c_str();

	switch(count_char_sjis(p[0]))
	{
	case 2:
		assign_code(code, p[0],p[1]);
	break;

	case 1:
		assign_code(code, 0, p[0]);
	break;
	}

	return code;
}

} // namespace end

/////////////////////////////////////////
// font_info
#pragma region

void font_info::init( uint32_t nLetterSize, float fLineHeight, bool bAntiAlias, bool bPer, uint32_t nReserveLetterNum)
{
	nLetterSize_	= nLetterSize;
	fLineHeight_	= fLineHeight;
	bAntiAlias_		= bAntiAlias;
	hashLetterInfo_.reserve(nReserveLetterNum);

	if(bAntiAlias_)
		eMode_ = MODE_TEX_COLOR_THR_BLEND;
	else
		eMode_ = MODE_TEX_COLOR_THR;
}

bool font_info::add_letter_info(uint16_t nLetter, const letter_info& info)
{
	auto it = hashLetterInfo_.emplace(nLetter, info);
	return it.second;
}

bool font_info::add_letter_info(const string& sLetter, const letter_info& info)
{
	return add_letter_info(to_sjis_code(sLetter), info);
}

bool font_info::add_letter_info(uint16_t nLetter, uint32_t nTextureID, draw::RECT rect, draw::POS base, DWORD nColor)
{
	letter_info info;
	info.nTextureID_ = nTextureID;
	info.rect_		 = rect;
	info.base_		 = base;
	info.nColor_	 = nColor;

	return add_letter_info(nLetter, info);
}

bool font_info::add_letter_info(const string& sLetter, uint32_t nTextureID, draw::RECT rect, draw::POS base, DWORD nColor)
{
	return add_letter_info(to_sjis_code(sLetter), nTextureID, rect, base, nColor);
}

bool font_info::is_letter_info(uint16_t nLetter)const
{
	auto it = hashLetterInfo_.find(nLetter);
	return it!=hashLetterInfo_.end();
}

bool font_info::is_letter_info(const string& sLetter)const
{
	return is_letter_info(to_sjis_code(sLetter));
}

const font_info::letter_info& font_info::get_letter_info(uint16_t nLetter)const
{
	auto it = hashLetterInfo_.find(nLetter);
	return it->second;
}

const font_info::letter_info& font_info::get_letter_info(const string& sLetter)const
{
	return get_letter_info(to_sjis_code(sLetter));
}

#pragma endregion

/////////////////////////////////////////
// renderer_text
void renderer_text::init(uint32_t nReserveFontNum)
{
	mapFont_.reserve(nReserveFontNum);
}

bool renderer_text::add_font_info(const string_fw& sFontID)
{
	font_info info;
	auto r = mapFont_.emplace(sFontID, info);
	if(!r.second)
	{
		logger::warnln("[renderer_text]フォント情報を登録することができませんでした。: " + sFontID.get());
		return false;
	}

	return true;
}

void renderer_text::remove_font_info(const string_fw& sFontID)
{
	mapFont_.erase(sFontID);
}

font_info* renderer_text::get_font_info(const string_fw& sFontID)
{
	return &mapFont_[sFontID];
}

bool renderer_text::is_font_info(const string_fw& sFontID)const
{
	auto it = mapFont_.find(sFontID);
	return it!=mapFont_.end();
}

bool renderer_text::add_letter_info(const string_fw& sFontID, const string& sLetter, const font_info::letter_info& letterInfo)
{
	if(!is_font_info(sFontID))
	{
		logger::warnln("[renderer_text][add_letter_info]フォント情報が登録されていません。");
		return false;
	}

	font_info* pInfo = get_font_info(sFontID);
	return pInfo->add_letter_info(sLetter, letterInfo);
}

bool renderer_text::add_letter_info(const string_fw& sFontID, const string& sLetter, uint32_t nTextureID, draw::RECT rect, draw::POS base, DWORD nColor)
{
	if(!is_font_info(sFontID))
	{
		logger::warnln("[renderer_text][add_letter_info]フォント情報が登録されていません。");
		return false;
	}

	font_info* pInfo = get_font_info(sFontID);
	return pInfo->add_letter_info(sLetter, nTextureID, rect, base, nColor);
}

///////////////////////////
// draw_text

namespace{
//! @brief nCodeが指す文字のスプライトコマンドを生成する
/*! @param[out] sp 設定先のスプライトコマンド
 *  @param[out] curPos 現在の描画位置。nCodeに応じて変化するので次の文字にそのまま渡すこと 
 *  @param[out] fMaxHeight 改行のための現在の最大高さ */
bool create_sprite_cmd(sprite_param& sp, POS& curPos, float& fMaxHeight, uint32_t count, uint16_t nCode, const font_info& infoFont, DWORD nColor)
{
	if(!infoFont.is_letter_info(nCode))
	{// 文字が無い時はスペースにする。位置をずらすだけ
		if(count==2) 
			curPos.fX += infoFont.letter_pixel_size();
		else 
			curPos.fX += infoFont.letter_pixel_size()/2.0f;

		return false;
	}

	const font_info::letter_info& infoLetter = infoFont.get_letter_info(nCode);

	if(infoLetter.nTextureID_==0)
	{
		logger::warnln("[render_text]\"" + to_str_s(nCode) + "\"に対応するテクスチャが設定されてません。スキップします。");
		return false;
	}

	// スプライトコマンドを生成する
	sp.clear();

	float fWidth  = infoLetter.rect_.width();
	float fHeight = infoLetter.rect_.height();
	if(fMaxHeight<fHeight) fMaxHeight = fHeight;

	// テクスチャID
	sp.nTextureID_ = infoLetter.nTextureID_;
		
	// 位置
	// ベースライン合わせ
	float fX = curPos.fX + infoLetter.base_.fX;
	float fY = curPos.fY - infoLetter.base_.fY;
	sp.set_pos_rect(fX, fY, fX+fWidth, fY+fHeight, curPos.fZ);

	// 次の描画開始位置
	curPos.fX = fX + infoLetter.fAdvance_;

	// UV
	sp.set_uv_rect(infoLetter.rect_.fLeft, infoLetter.rect_.fTop, infoLetter.rect_.fRight, infoLetter.rect_.fBottom);
	// 色
	sp.set_color(0,nColor);
	sp.set_color(1,nColor);
			
	if(((nColor&0xFF000000)>>24)<0xFF)
		sp.mode_ = MODE_TEX_COLOR_THR_BLEND;
	else
		sp.mode_ = infoFont.draw_mode();

	return true;
}

bool is_space(uint8_t c)
{
	return c==' '||c=='\t';
}
}

bool renderer_text::draw_text(renderer_sprite_queue& queue, struct cmd::text_draw_cmd& cmd)
{
	const shared_ptr<text_data>& pTextData		= cmd.pText_;
	const uint32_t				 nCharNum		= cmd.nCharNum_;
	const draw::POS&			 pos			= cmd.pos_;
	const DWORD					 nColor			= cmd.nColor_;
	const uint32_t				 nRenderTarget	= cmd.nRenderTarget_;

	// 描画文字数0なので即座にリターン
	if(nCharNum==0) return true;

	if(!pTextData || pTextData->text().empty())
	{
		logger::warnln("[renderer_text][draw_text]テキストデータが設定されていません。");
		return false;
	}

	if(!is_font_info(pTextData->font_id()))
	{
		logger::warnln("[renderer_text][draw_text]フォント情報が登録されていません。: " + pTextData->font_id().get());
		return false;
	}

	sprite_param sp;
	uint32_t	 nCurCharNum=0;

	// マークアップ処理準備
	bool		bMarkupCmd = false;	 // マークアップ処理中華どうか。[が来るとtrueになり]が来るとfalseになる
	font_info*	pCurFont	= get_font_info(pTextData->font_id());
	DWORD		curColor	= nColor;

	// 先頭から文字に分解しながらも文字情報を取得しつつコマンドを積む
	draw::POS	curPos		= pos;   // 現在の描画位置
	float		fMaxHeight	= 0.0f;  // 一番長い高さ。改行に使う。改行したら0に戻る
	bool		bBreak		= false; // 改行するかどうか

	uint16_t nCode=0;
	const char* pText = pTextData->text().c_str();
	const uint32_t nTextSize = pTextData->text().size();
	uint32_t i=0; // 処理中の文字バイト位置

	while(i<nTextSize && nCurCharNum<nCharNum)
	{
		// 文字のバイト数
		uint32_t count = count_char_sjis(pText[i]);
		switch(count)
		{
		case 1:
		{
			uint8_t c = pText[i++];

			// 制御コードは基本飛ばす
			if((c>=0x01 && c<=0x1f) || c==0x7f)
			{
				// LFは改行扱いになるかもしれない
				if(!pTextData->is_markup() && c=='\n')
					bBreak=true;
				else
					continue;
			}

			if(pTextData->is_markup() && c=='[')
			{// マークアップ開始文字
				bMarkupCmd = true;
			}

			assign_code(nCode, 0, c);
		}
		break;

		case 2:
		{
			// 元々リトルエンディアンで入ってるのでそのまま入れる
			uint8_t first = pText[i++];
			uint8_t second = pText[i++];
			assign_code(nCode, first, second);
		}
		break;

		default:
			 ++i;
			 return true; // nullは終了
		break;
		}

		// マークアップコマンドチェック
#pragma region markup
		if(pTextData->is_markup() && bMarkupCmd)
		{// ] までの範囲を取得する
			uint32_t j = i+1;
			while(j<nTextSize)
			{
				count = count_char_sjis(pText[j]);
				if(count==1)
				{
					if(pText[j]==']') break;
				}
				j+=count;
			}
			
			if(j>=nTextSize)
			{
				logger::warnln("[renderer_text][draw_text]マークアップタグが閉じられていません。");
				return false;
			}

			// i ～ j-1 の範囲が[]の中身
			string markuptag;
			for(uint32_t k=i; k<j; ++k) markuptag += pText[k];
			// iを解析したとこまで進めておく
			i=j+1;

			using namespace boost::xpressive;

			// [r=数値] [font=文字列] [color=FFFFFFFF] のチェック
			smatch what;
			sregex r_tag	 = *_s >> as_xpr('r') >> *_s >> '=' >> *_s >> (s1=+_d) >> *_s;
			sregex font_tag	 = *_s >> as_xpr("font") >> *_s >> '=' >> *_s >> (s1=+_w) >> *_s;
			sregex color_tag = *_s >> as_xpr("color") >> *_s >> '=' >> *_s >> (s1=repeat<8,8>(xdigit)) >> *_s;

			if(regex_match(markuptag, what, r_tag))
			{// 改行タグ
				fMaxHeight = lexical_cast<float>(what.str(1));
				bBreak=true;
			}
			else if(regex_match(markuptag, what, font_tag))
			{// fontタグ
				string_fw m(what.str(1));
				if(is_font_info(m))
					pCurFont = get_font_info(m);
				else
					logger::warnln("[renderer_text][draw_text]指定されたフォントが見つかりません。: " + m.get());
			}
			else if(regex_match(markuptag, what, color_tag))
			{// colorタグ
				curColor = hex_to_uint32(what.str(1));
			}
			else
			{
				logger::warnln("[renderer_text][draw_text]マークアップタグのフォーマットが間違っています。: " + markuptag);
				return false;
			}
			
		}
#pragma endregion
		
		if(bBreak)
		{// 改行チェック
			curPos.fX = pos.fX;
			// Markupじゃない場合の改行はレターサイズからの算出される固定値
			if(!pTextData->is_markup() || fMaxHeight==0.0f)
				fMaxHeight = static_cast<float>(pCurFont->letter_pixel_size())*pCurFont->line_height();

			curPos.fY += fMaxHeight;
			fMaxHeight = 0.0f;
			bBreak = false;
			continue;
		}

		if(bMarkupCmd)
		{
			bMarkupCmd = false;
			continue;
		}

		// スプライトコマンド生成
		if(create_sprite_cmd(sp, curPos, fMaxHeight, count, nCode, *pCurFont, curColor))
		{
			queue.add_cmd(sp, nRenderTarget);
		}

		++nCurCharNum;
	}

	return true;
}

} // namespace draw end
} // namespace mana end
