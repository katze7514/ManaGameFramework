#pragma once

#include "../Utility/id_manger.h"
#include "../File/file.h"

#include "renderer_2d_util.h"

namespace mana{
namespace draw{

namespace cmd{
struct text_draw_cmd;
} // namespace cmd end

class renderer_sprite_queue;
class text_data;

/////////////////////////////////
/*! @brief フォント情報
 *
 *  文字単位の情報を持つ
 *  renderer_textにより、利用されることを前提としてるので
 *  直接は使わないこと
 */
class font_info
{
public:
	//! 1文字当たりの情報
	struct letter_info
	{
		uint32_t	nTextureID_;
		draw::RECT	rect_;		// nTextureID上の左上右下位置
		draw::POS	base_;		// ベースライン
		float		fAdvance_;	// 文字送り

		//! データが格納されてる色成分に255が入っている
		//! 並びはARGB
		DWORD		nColor_;

		letter_info():nTextureID_(0),fAdvance_(0.f),nColor_(0xFFFFFFFF){}
	};

	typedef unordered_map<uint16_t, letter_info> letter_hash;

public:
	font_info():nLetterSize_(0),fLineHeight_(1.5f),bAntiAlias_(false){ eMode_=MODE_TEX_COLOR_THR; }

public:
	void init(uint32_t nLetterSize, float fLineHeight, bool bAntiAlias, bool bPer, uint32_t nReserveLetterNum);

public:
	bool add_letter_info(uint16_t nLetter, const letter_info& info);
	bool add_letter_info(const string& sLetter, const letter_info& info);
	bool add_letter_info(uint16_t nLetter, uint32_t nTextureID, draw::RECT rect, draw::POS base, DWORD nColor=0xFFFFFFFF);
	bool add_letter_info(const string& sLetter, uint32_t nTextureID, draw::RECT rect, draw::POS base,DWORD nColor=0xFFFFFFFF);
	bool is_letter_info(uint16_t nLetter)const;
	bool is_letter_info(const string& sLetter)const;

	const letter_info&	get_letter_info(uint16_t nLetter)const;
	const letter_info&	get_letter_info(const string& sLetter)const;

	uint32_t		letter_pixel_size()const{ return nLetterSize_; }
	float			line_height()const{ return fLineHeight_; }
	bool			is_antialias()const{ return bAntiAlias_; }
	draw::draw_mode	draw_mode()const{ return eMode_; }

	//! @brief フォント情報ファイルを読み込んで文字情報を構築する
	/*! @tparam tex_mgr このフォントテクスチャを管理するテクスチャマネージャー。文字列IDを数値IDに変換するのに使う 
	 *  @param[in] sFilePath 情報ファイルへのパスか、情報文字列そのもの 
	 *  @param[in] bFile trueの時、sFilePathはファイルパス。falseの時、情報文字列 */
	template<class tex_mgr>
	bool load_font_info_file(const string_fw& sFontID, tex_mgr& mgr, const string& sFilePath, bool bFile=true);

private:
	//! 1文字あたりの情報ハッシュ
	letter_hash hashLetterInfo_;

	//! このフォントの文字サイズ。スペースや改行使われる
	uint32_t nLetterSize_;
	//! 改行時の送り量倍率。Markupじゃない時に使われる
	float fLineHeight_;
	//! アンチエイリアス付きかどうか
	bool bAntiAlias_;

	//! このフォントを描画するに使う描画モード
	draw::draw_mode  eMode_;
};

/////////////////////////////////
/*! @brief テキストレンダラー
 *
 *  フォント情報を管理し、与えられたテキストや描画情報に
 *　合わせて、スプライトコマンドを生成する
 *  文字コードはSJISのみ対応
 */
class renderer_text
{
public:
	enum text_align{
		ALIGN_LEFT,
		ALIGN_CENTER,
		ALIGN_RIGHT,
	};

	typedef unordered_map<string_fw, font_info, string_fw_hash>	font_map;

public:
	renderer_text(){}

public:

	//! @brief 初期化
	/*! @param[in] nFontNum 使用するフォント数(予定値) */
	void init(uint32_t nReserveFontNum);

public:
	//! フォント情報領域を確保する。確保した後に文字情報を登録していく
	bool add_font_info(const string_fw& sID);
	void remove_font_info(const string_fw& sID);

	bool is_font_info(const string_fw& sID)const;

	bool add_letter_info(const string_fw& sFontID, const string& sLetter, const font_info::letter_info& letterInfo);
	bool add_letter_info(const string_fw& sFontID, const string& sLetter, uint32_t nTextureID, draw::RECT rect, draw::POS base, DWORD nColor=0xFFFFFFFF);

	template<class tex_mgr>
	bool load_font_info_file(const string_fw& sFontID, tex_mgr& mgr, const string& sFilePath, bool bFile=true);

public:
	//! @defgroup renderer_text_draw テキスト描画メソッド群
	//! @{

	//! @brief 基本のテキスト描画。改行対応
	/*! @param[in] queue 描画コマンドの積み先
	 *  @param[in] cmd 描画情報が入ったテキストコマンド */
	bool draw_text(renderer_sprite_queue& queue, struct cmd::text_draw_cmd& cmd);
	//! @}

private:
	font_info* 	get_font_info(const string_fw& nID);

private:
	//! 管理してるフォント情報マップ
	font_map		mapFont_;

private:
	NON_COPIABLE(renderer_text);
};


////////////////////////////////////////
// template実装

template<class tex_mgr>
bool renderer_text::load_font_info_file(const string_fw& sFontID, tex_mgr& mgr, const string& sFilePath, bool bFile)
{
	if(bFile)
		logger::infoln("[render_text]" + sFontID.get() + "の定義ファイルを読み込みます。: " + sFilePath);
	else
		logger::infoln("[render_text]" + sFontID.get() + "の定義ファイルを読み込みます。");

	if(!is_font_info(sFontID))
	{// 無かったら追加
		if(!add_font_info(sFontID)) return false;
	}

	font_info* pInfo = get_font_info(sFontID);

	bool r = pInfo->load_font_info_file(sFontID, mgr, sFilePath, bFile);
	//if(r) logger::infoln("[render_text]" + sFontID + "の定義ファイルを読み込みに成功しました。: " + sFilePath);
	return r;
}

template<class tex_mgr>
inline bool font_info::load_font_info_file(const string_fw& sFontID, tex_mgr& mgr, const string& sFilePath, bool bFile)
{
	std::stringstream ss;
	if(!file::load_file_to_string(ss, sFilePath, bFile))
	{
		logger::warnln("[font_info]フォント定義ファイルが読み込めませんでした。: " + sFilePath);
		return false;
	}

	// フォント定義XML読み込み
	using namespace p_tree;
	ptree ltree;
	xml_parser::read_xml(ss, ltree, xml_parser::no_comments);

	ptree& font = ltree.get_child("font");

	// フォントパラメータ
	// 文字数
	uint32_t nLetterNum = font.get("<xmlattr>.num",0);
	// ピクセルサイズ
	uint32_t nLetterSize = font.get("<xmlattr>.letter_size",0);
	// 改行倍率
	float fLineHeight = font.get("<xmlattr>.line_height",1.5f);
	// アンチエイリアス
	bool bAntiAlias = font.get("<xmlattr>.antialias",false);
	// チャンネル別
	bool bPer = font.get("<xmlattr>.per",false);

	// フォント情報初期化
	init(nLetterSize, fLineHeight, bAntiAlias, bPer, nLetterNum);

	// 文字
	int32_t count=-1;
	for(auto& let : font)
	{
		// fontの属性は処理済み
		if(let.first=="<xmlattr>") continue;

		if(let.first=="texture")
		{// テクスチャIDとファイルパスをテクスチャマネージャーに登録
			string sID = let.second.get<string>("<xmlattr>.id","");
			string sSrc = let.second.get<string>("<xmlattr>.src","");			
			
			mgr.add_texture_info(sID, sSrc, sFontID);
		}
		else if(let.first=="letter")
		{
			++count;
			letter_info info;

			string sCode = let.second.get<string>("<xmlattr>.code","0");
			if(sCode=="0")
			{
				logger::warnln("[font_info]文字コードが正しく設定されていません : " + to_str_s(count) + sFilePath);
				continue;
			}

			char* endp;
			uint16_t code = static_cast<uint16_t>(::strtol(sCode.c_str(), &endp, 16));
		
			for(auto& letch : let.second)
			{
				if(letch.first=="tex")
				{
					string sID = letch.second.get<string>("<xmlattr>.id","");
					optional<uint32_t> id = mgr.texture_id(sID);
					if(id)
					{
						info.nTextureID_ = *id;
					}
					else
					{
						info.nTextureID_ = 0;
						logger::warnln("[font_info]テクスチャIDが正しく設定されていません : " + to_str_s(count) + sFilePath);
					}
				}
				else if(letch.first=="rect")
				{
					info.rect_.fLeft	= letch.second.get("<xmlattr>.left",-1.f);
					info.rect_.fTop		= letch.second.get("<xmlattr>.top",-1.f);
					info.rect_.fRight	= letch.second.get("<xmlattr>.right",-1.f);
					info.rect_.fBottom	= letch.second.get("<xmlattr>.bottom",-1.f);

					if(info.rect_.fLeft<0 || info.rect_.fTop<0 || info.rect_.fRight<0 || info.rect_.fBottom<0)
						logger::warnln("[font_info]文字範囲が正しく設定されていません : " + to_str_s(count) + sFilePath);
				}
				else if(letch.first=="base")
				{
					info.base_.fX = letch.second.get("<xmlattr>.x",0.f);
					info.base_.fY = letch.second.get("<xmlattr>.y",0.f);
					info.fAdvance_= letch.second.get("<xmlattr>.advance",0.f);
				}
				else if(bPer && letch.first=="channel")
				{
					int32_t nChannel = letch.second.get("<xmlattr>.no",-1);

					if(nChannel<0)
					{
						logger::warnln("[font_info]チャンネル範囲が正しく設定されていません : " + to_str_s(count) + sFilePath);
					}
					else
					{
						switch(nChannel)
						{
						case 1: // blue
							info.nColor_ = 0x0000FF00;
						break;

						case 2: // green
							info.nColor_ = 0x000000FF;
						break;

						case 3: // alpha
							info.nColor_ = 0xFF000000;
						break;

						default: // red
							info.nColor_ = 0x00FF0000;
						break;
						}
					}
				}
			}

			// 文字情報登録
			add_letter_info(code, info);
		}
	}

	return true;
}

} // namespace draw end
} // namespace mana end
