#include "../mana_common.h"

#include "../File/file.h"

#include "../Draw/renderer_2d_util.h"
#include "../Draw/renderer_2d_cmd.h"
#include "../Draw/renderer_2d.h"
#include "../Audio/audio_player.h"

#include "text_table.h"
#include "draw_data.h"
#include "draw_context.h"
#include "draw_builder.h"

namespace mana{
namespace graphic{

bool draw_builder::load_draw_info_file(const string& sFilePath, draw_context& context, bool bFile)
{
	if(eLoadState_!=LOAD_FIN && eLoadState_!=LOAD_ERR)
	{
		logger::warnln("[draw_builder]定義ファイルロード中です。");
		return false;
	}

	auto err = [this](const string&& mes){ logger::warnln("[draw_builder]"+mes); eLoadState_=LOAD_ERR; treeLoad_.clear(); return false; };

	if(bFile)
		logger::infoln("[draw_builder]定義ファイルを読み込みます。: " + sFilePath);
	else
		logger::infoln("[draw_builder]定義ファイルを読み込みます。");

	eLoadState_ = LOAD_START;
	nLoadWait_	= 0;
	nLoadFin_.store(0, std::memory_order_release);
	bLoadErr_.store(false, std::memory_order_release);

	std::stringstream ss;
	if(!file::load_file_to_string(ss, sFilePath, bFile))
		return err("定義ファイルが読み込めませんでした。");

	using namespace p_tree;
	xml_parser::read_xml(ss, treeLoad_, xml_parser::no_comments);
	
	optional<ptree&> draw_base_def = treeLoad_.get_child_optional("draw_base_def");
	if(!draw_base_def)
		return err("定義ファイルのフォーマットが間違ってます。: <draw_base_def>が存在しません。");

	uint32_t nTexInfoNum= 1;
	uint32_t nTexNum	= 1;
	uint32_t nFontNum	= 1;

	// texture/font系のロードを仕込む
	for(auto& it : *draw_base_def)
	{
		if(it.first=="texture_def")
		{
			draw::cmd::info_load_cmd cmd;
			cmd.eKind_		= draw::cmd::KIND_TEX;
			cmd.sFilePath_	= it.second.get<string>("<xmlattr>.src","");
			cmd.callback_	= [this](uint32_t result){ if(result==1) ++nLoadFin_; else bLoadErr_.store(true, std::memory_order_release); };

			if(cmd.sFilePath_!="")
			{
				context.renderer()->request(cmd);
				++nTexInfoNum;
			}
			else
			{
				return err("定義ファイルのフォーマットが間違ってます。:" + to_str_s(nTexInfoNum) + "個目の<texture_info>");
			}

			++nLoadWait_;
		}
		else if(it.first=="texture")
		{
			draw::cmd::tex_info_add_cmd cmd;
			cmd.sTextureID_	= it.second.get<string>("<xmlattr>.id","");
			cmd.sFilePath_	= it.second.get<string>("<xmlattr>.src","");
			cmd.sGroup_		= it.second.get<string>("<xmlattr>.group","");
			cmd.callback_	= [this](uint32_t result){ if(result>0) ++nLoadFin_; else bLoadErr_.store(true, std::memory_order_release); };

			if(cmd.sTextureID_!="" && cmd.sFilePath_!="")
			{
				context.renderer()->request(cmd);
				++nTexNum;
			}
			else
			{
				return err("定義ファイルのフォーマットが間違ってます。:" + to_str_s(nTexNum) + "個目のtexture>");
			}

			++nLoadWait_;
		}
		else if(it.first=="font")
		{
			draw::cmd::info_load_cmd cmd;
			cmd.eKind_		= draw::cmd::KIND_FONT;
			cmd.sID_		= it.second.get<string>("<xmlattr>.id","");
			cmd.sFilePath_	= it.second.get<string>("<xmlattr>.src","");
			cmd.callback_	= [this](uint32_t result){ if(result>0) ++nLoadFin_; else bLoadErr_.store(true, std::memory_order_release); };

			if(cmd.sID_!="" && cmd.sFilePath_!="")
			{
				context.renderer()->request(cmd);
				++nFontNum;
			}
			else
			{
				return err("定義ファイルのフォーマットが間違ってます。:" + to_str_s(nFontNum) + "個目の<font>");
			}

			++nLoadWait_;
		}
	}

	if(nLoadWait_>0)
		eLoadState_ = LOAD_TEX_FONT_WAIT;
	else
		eLoadState_ = LOAD_NODE;

	return true;
}

draw_builder::load_result draw_builder::is_fin_load(draw_context& context)
{
	auto success = [this](){ eLoadState_=LOAD_FIN; treeLoad_.clear(); return SUCCESS; };
	auto err	 = [this](){ eLoadState_=LOAD_ERR; treeLoad_.clear(); return FAIL; };

	switch(eLoadState_)
	{
	case LOAD_FIN:
		return SUCCESS;
	break;

	case LOAD_TEX_FONT_WAIT:
		if(bLoadErr_)
		{	return err();	}
		else if(nLoadWait_==nLoadFin_.load(std::memory_order_acquire)) // エラー無くテクスチャ/フォントロード終了
		{	eLoadState_ = LOAD_NODE;	}

		return LOAD;
	break;

	case LOAD_NODE:
		if(load_draw(context))
		{
			//logger::infoln("[draw_builder]定義ファイルの読み込みに成功しました。");
			return success();
		}
		else
		{	return err();	}
	break;

	default:
		return FAIL;
	break;
	}
}

bool draw_builder::load_draw(draw_context& context)
{
	// load_draw_base_info_fileメソッドで存在するのは確認済み
	p_tree::ptree& draw_base_def = treeLoad_.get_child("draw_base_def");

	uint32_t nElementNum=1;
	for(const auto& child : draw_base_def)
	{
		if(child.first=="texture_def"
		|| child.first=="texture"
		|| child.first=="font" ) continue;

		if(!load_draw_switch(child, context))
		{
			logger::warnln("[draw_builder]定義ファイルのフォーマットが間違っています。: "  + to_str_s(nElementNum) + "個目のdraw_base : " + child.first.c_str());
			return false;
		}

		++nElementNum;
	}

	return true;
}

bool draw_builder::load_draw_switch(const p_tree::ptree::value_type& element, draw_context& ctx, draw_base_data* pParent)
{
	draw_base_data*	pData=nullptr;

	if(element.first=="base")
		pData = load_draw_base(element.second, ctx);
	else if(element.first=="label")
		pData = load_draw_label(element.second, ctx);
	else if(element.first=="sprite")
		pData = load_draw_sprite(element.second, ctx);
	else if(element.first=="audio")
		pData = load_draw_audio(element.second, ctx);
	else if(element.first=="message")
		pData = load_draw_message(element.second, ctx);
	else if(element.first=="timeline")
		pData = load_draw_timeline(element.second, ctx);
	else if(element.first=="movieclip")
		pData = load_draw_movieclip(element.second, ctx);

	if(!pData) return false;

	// pParentがいたら子共に、なかったらhashに格納する

	// 共通のエラー処理
	auto err = [&pData](const string&& mes){ logger::warnln("[draw_builder]"+mes); delete pData; return false; };

	// ID系を取得
	optional<string> sID	= element.second.get_optional<string>("<xmlattr>.id");
	optional<string> sName	= element.second.get_optional<string>("<xmlattr>.name");
	optional<string> sDebug	= element.second.get_optional<string>("<xmlattr>.debug");

	if(sName) pData->sName_ = *sName;

#ifdef MANA_DEBUG
	if(sDebug)	 pData->sDebugName_ = *sDebug;
	else if(sID) pData->sDebugName_ = *sID;
#endif

	if(pParent)
	{
		if(sID){ pData->sID_ = *sID; }
		pParent->vecChildren_.emplace_back(pData);
	}
	else
	{
		if(sID)
		{ 
			auto r = hashNode_.emplace(string_fw(*sID),pData); 
			if(!r.second)
			{ 
			#ifdef MANA_DEBUG
				if(bReDefine_)
				{// 再定義フラグがある場合は上書き
					auto& it = r.first->second;
					safe_delete(it);
					it = pData;
					return true;
				}
			#endif

				return err("すでに同IDが登録済みです。: " + *sID);
			}
		}
		else
		{
			return err("IDが設定されていません。");
		}
	}

	return true;
}

//////////////////////////////

namespace{
inline void load_draw_base_pivot(const p_tree::ptree& draw, draw::POS& pivot)
{
	pivot.fX = draw.get<float>("<xmlattr>.x", 0.0f);
	pivot.fY = draw.get<float>("<xmlattr>.y", 0.0f);
}

inline void load_draw_base_draw_info(const p_tree::ptree& draw, draw::draw_info& drawInfo)
{
	drawInfo.pos_.fX = draw.get<float>("<xmlattr>.x", 0.0f);
	drawInfo.pos_.fY = draw.get<float>("<xmlattr>.y", 0.0f);

	drawInfo.scale_.fWidth	= draw.get<float>("<xmlattr>.width",  1.0f);
	drawInfo.scale_.fHeight = draw.get<float>("<xmlattr>.height", 1.0f);

	drawInfo.angle_ = draw.get<float>("<xmlattr>.angle", 0.0f);
	drawInfo.alpha_ = draw.get<uint8_t>("<xmlattr>.alpha", 255);
}

inline void load_draw_base_color(const p_tree::ptree& draw, draw_base_data* pData)
{
	optional<string> color  = draw.get_optional<string>("<xmlattr>.value");
	if(color)
	{// カラー16進数文字列を解釈
		pData->nColor_ = hex_to_uint32(*color);
	}

	optional<string> mode	= draw.get_optional<string>("<xmlatr>.mode");
	if(mode)
	{
		if(*mode=="THR")
			pData->eColorMode_ = COLOR_THR;
		else if(*mode=="BLEND")
			pData->eColorMode_ = COLOR_BLEND;
		else if(*mode=="MUL")
			pData->eColorMode_ = COLOR_MUL;
		else if(*mode=="SCREEN")
			pData->eColorMode_ = COLOR_SCREEN;
		else if(*mode=="NO")
			pData->eColorMode_ = COLOR_NO;
		else
			logger::warnln("[draw_builder]無効なColorModeが指定されています。: " + *mode);
	}
}

} // namespace end

draw_base_data* draw_builder::load_draw_base(const p_tree::ptree& element, draw_context& ctx)
{
	draw_base_data* pData = new_ draw_base_data();

	for(const auto& child : element)
	{
		tribool r = load_draw_common_switch(child, ctx, pData);
		if(indeterminate(r))
		{
			logger::warnln("[draw_builder]baseタグのフォーマットが間違っています。");
			delete pData;
			return nullptr; 
		}
	}

	return pData;
}

label_data*	draw_builder::load_draw_label(const p_tree::ptree& element, draw_context& ctx)
{
	label_data* pData = new_ label_data();

	auto err = [&pData](const string&& mes)->label_data*{ logger::warnln("[draw_builder]labelタグのフォーマットが間違っています。: " + mes); delete pData; return nullptr; };

	bool bTextDataTag=false;

	for(const auto& child : element)
	{
		tribool r = load_draw_common_switch(child, ctx, pData);

		if(indeterminate(r)) return err("");

		if(!r)
		{
			if(child.first=="text_data")
			{
				optional<string> id = child.second.get_optional<string>("<xmlattr>.id");
				if(!id) return err("text_dataタグにidが設定されていません。");

				pData->pTextData_ = ctx.text_table()->text_data(string_fw(*id));
				if(!pData->pTextData_) return err("text_dataがtext_tableに設定されていません。: " + *id);
				bTextDataTag = true;
			}
			
			if(!bTextDataTag)
			{
				if(child.first=="font")
				{
					optional<string> id = child.second.get_optional<string>("<xmlattr>.id");
					if(!id) return err("fontタグにidが設定されていません。");
				
					if(!pData->pTextData_) pData->pTextData_ = make_shared<draw::text_data>();
					pData->pTextData_->set_font_id(string_fw(*id));
				}
				else if(child.first=="text")
				{
					if(!pData->pTextData_) pData->pTextData_ = make_shared<draw::text_data>();

					bool bMarkUp = child.second.get<bool>("<xmlattr>.markup",false);
					pData->pTextData_->markup(bMarkUp);
					pData->pTextData_->set_text(boost::trim_copy(child.second.get_value<string>()));
				}
			}
		}
	}

	// 整合性チェック
	if(!bTextDataTag && pData->pTextData_->font_id().get().empty())
		return err("fontタグが指定されていません。");

	return pData;
}

sprite_data* draw_builder::load_draw_sprite(const p_tree::ptree& element, draw_context& ctx)
{
	sprite_data* pData = new_ sprite_data();

	auto err = [&pData](const string&& mes)->sprite_data*{ logger::warnln("[draw_builder]spriteタグのフォーマットが間違っています。: " + mes); delete pData; return nullptr; };

	for(const auto& child : element)
	{
		tribool r = load_draw_common_switch(child, ctx, pData);

		if(indeterminate(r)) return err("");

		if(!r)
		{
			if(child.first=="tex")
			{
				optional<string> id = child.second.get_optional<string>("<xmlattr>.id");
				if(!id) return err("texタグにidが設定されていません。");

				optional<uint32_t> nID = ctx.renderer()->texture_id(*id);
				if(!nID) return err("textureロードが終わっていません。: " + *id); // 起こりえないはずだが念のため
					
				pData->nTexID_ = *nID;
			}
			else if(child.first=="rect")
			{
				pData->rect_.fLeft		= child.second.get<float>("<xmlattr>.left",0.0f);
				pData->rect_.fTop		= child.second.get<float>("<xmlattr>.top",0.0f);
				pData->rect_.fRight		= child.second.get<float>("<xmlattr>.right",0.0f);
				pData->rect_.fBottom	= child.second.get<float>("<xmlattr>.bottom",0.0f);
			}
		}
	}

	// 整合性チェック
	if(pData->nTexID_==0) return err("texタグが指定されていません。");

	return pData;
}

audio_data* draw_builder::load_draw_audio(const p_tree::ptree& element, draw_context& ctx)
{
	using namespace audio;

	audio_data* pData = new_ audio_data();

	auto err = [&pData](const string&& mes)->audio_data*{ logger::warnln("[draw_builder]audioタグのフォーマットが間違っています。: " + mes); delete pData; return nullptr; };

	string sKind = element.get<string>("<xmlattr>.kind", "");
	if(sKind.empty()) return err("kind属性が指定されていません。");

	if(sKind=="BGM") 
		pData->info_.eAudioKind_ = audio_kind::AUDIO_BGM;
	else if(sKind=="SE") 
		pData->info_.eAudioKind_ = audio_kind::AUDIO_SE;
	else 
		return err("kind属性の指定が間違っています。");

	bool bVol=false;

	for(const auto& child : element)
	{
		tribool r = load_draw_common_switch(child, ctx, pData);

		if(indeterminate(r)) return err("");

		if(child.first=="sound")
		{
			string sID = child.second.get<string>("<xmlattr>.id", "");
			if(sID=="")
			{	pData->info_.nSoundID_ = 0;	}
			else
			{
				pData->info_.nSoundID_ = ctx.audio_player()->sound_id(sID);
			}
		}
		else if(child.first=="play")
		{
			if(bit_test<uint32_t>(pData->info_.nParamFlag_, param_flag::PARAM_STOP))
				return err("playタグとstopが同時に使用されています。");

			pData->info_.nParamFlag_ |= param_flag::PARAM_PLAY;

			string mode = child.second.get<string>("<xmlattr>.change","");
			if(mode=="stop")
				pData->info_.eChangeMode_ = change_mode::CHANGE_STOP;
			else if(mode=="fadeout")
				pData->info_.eChangeMode_ = change_mode::CHANGE_FADEOUT;
			else if(mode=="cross")
				pData->info_.eChangeMode_ = change_mode::CHANGE_CROSS;

			pData->info_.nFadeFrame_	= child.second.get<int32_t>("<xmlattr>.frame",0);

			pData->info_.bLoop_			= child.second.get<bool>("<xmlattr>.loop", false);
			pData->info_.bForce_		= child.second.get<bool>("<xmlattr>.force", false);
		}
		else if(child.first=="stop")
		{
			if(bit_test<uint32_t>(pData->info_.nParamFlag_, param_flag::PARAM_PLAY))
				return err("playタグとstopが同時に使用されています。");

			pData->info_.nParamFlag_	|= param_flag::PARAM_STOP;
			pData->info_.nFadeFrame_	 = child.second.get<int32_t>("<xmlattr>.frame",0);
		}
		else if(child.first=="vol")
		{
			if(bVol)
				return err("volタグとfadeタグが同時に使用されています。");

			bVol = true;

			uint32_t vol = child.second.get<uint32_t>("<xmlattr>.value", 0);
			vol = clamp(vol, 0, 100);
			pData->info_.vol_.set_per(vol);
		}
		else if(child.first=="fade")
		{
			if(bVol)
				return err("volタグとfadeタグが同時に使用されています。");

			bVol = true;

			int32_t nFrame = child.second.get<int32_t>("<xmlattr>.frame", 0);

			optional<uint32_t> nStart = child.second.get_optional<uint32_t>("<xmlattr>.start");
			if(nStart)
			{
				uint32_t vol = clamp(*nStart, 0, 100);
				pData->info_.vol_.set_per(vol);
			}
			else
			{
				nFrame *= -1;
			}

			uint32_t nEnd = child.second.get<uint32_t>("<xmlattr>.end", 0);
			nEnd = clamp(nEnd,0,100);
			pData->info_.volFadeEnd_.set_per(nEnd);
		}
		else if(child.first=="child")
		{
			pData->up_over(draw_base_data::FLAG_CHILDREN);
			if(!load_draw_child(child.second, ctx, pData))
				return err("");
		}
	}

	// エラーチェック

	// サウンドID指定なしが許されるのは、BGM停止の時だけ
	if(pData->info_.nSoundID_==0
	&& !(pData->info_.eAudioKind_==audio_kind::AUDIO_BGM 
		&& bit_test<uint32_t>(pData->info_.nParamFlag_,param_flag::PARAM_STOP))
	)
	{
		return err("サウンドID指定が無いか、間違ったサウンドIDが指定されています。");
	}

	return pData;
}

/////////////////////

message_data* draw_builder::load_draw_message(const p_tree::ptree& element, draw_context& ctx)
{
	message_data* pData = new_ message_data();

	auto err = [&pData](const string&& mes)->message_data*{ logger::warnln("[draw_builder]messageタグのフォーマットが間違っています。: " + mes); delete pData; return nullptr; };

	bool bTextDataTag=false;

	for(const auto& child : element)
	{
		tribool r = load_draw_common_switch(child, ctx, pData);

		if(indeterminate(r)) return err("");

		if(!r)
		{
			if(child.first=="text_data")
			{
				optional<string> id = child.second.get_optional<string>("<xmlattr>.id");
				if(!id) return err("text_dataタグにidが設定されていません。");

				pData->pTextData_ = ctx.text_table()->text_data(string_fw(*id));
				if(!pData->pTextData_) return err("text_dataがtext_tableに設定されていません。: " + *id);
				bTextDataTag = true;
			}
			else if(child.first=="next")
			{
				pData->nNext_ = child.second.get<uint32_t>("<xmlattr>.frame",0);
			}
			else if(child.first=="sound")
			{
				string id = child.second.get<string>("<xmlattr>.id", "");
				if(id.empty()) return err("soundタグのIDが空です。");
				pData->nSoundID_ = ctx.audio_player()->sound_id(id);
			}
			
			if(!bTextDataTag)
			{
				if(child.first=="font")
				{
					optional<string> id = child.second.get_optional<string>("<xmlattr>.id");
					if(!id) return err("fontタグにidが設定されていません。");
				
					if(!pData->pTextData_) pData->pTextData_ = make_shared<draw::text_data>();
					pData->pTextData_->set_font_id(string_fw(*id));
				}
				else if(child.first=="text")
				{
					if(!pData->pTextData_) pData->pTextData_ = make_shared<draw::text_data>();

					bool bMarkUp = child.second.get<bool>("<xmlattr>.markup",false);
					pData->pTextData_->markup(bMarkUp);
					pData->pTextData_->set_text(boost::trim_copy(child.second.get_value<string>()));
				}
			}
		}
	}

	// 整合性チェック
	if(!bTextDataTag && pData->pTextData_->font_id().get().empty())
		return err("fontタグが指定されていません。");

	// 文字数カウント
	if(!pData->pTextData_->text().empty())
	{
		const char* pText = pData->pTextData_->text().c_str();
		pData->nTextCharNum_=0;
		bool bMark=false;
		uint32_t i=0;

		while(i<pData->pTextData_->text().size())
		{
			const uint8_t c = pText[i];
			uint32_t count = draw::count_char_sjis(c);
			i+=count;

			if(count==0) break;
			if(count==1)
			{
				// 制御コードは基本飛ばす
				if((c>=0x01 && c<=0x1f) || c==0x7f)
					continue;

				if(pData->pTextData_->is_markup())
				{
					if(bMark)
					{
						if(c==']')
						{
							bMark=false;
						}
						continue;
					}

					if(c=='[')
					{
						bMark = true;
						continue;
					}
				}
			}
			
			++pData->nTextCharNum_;
		}
	}

	return pData;
}

/////////////////////

timeline_data* draw_builder::load_draw_timeline(const p_tree::ptree& element, draw_context& ctx)
{
	timeline_data* pData = new_ timeline_data();

	auto err = [&pData](const string& mes)->timeline_data*
				{ 
					logger::warnln("[draw_builder]timelineタグのフォーマットが間違っています。: " + mes);
					delete pData;
					return nullptr; 
				};

	for(const auto& child : element)
	{
		if(child.first=="pivot")
		{
			pData->up_over(draw_base_data::FLAG_PIVOT);
			load_draw_base_pivot(child.second, pData->pivot_);
		}
		else if(child.first=="draw")
		{
			pData->up_over(draw_base_data::FLAG_DRAW_INFO);
			load_draw_base_draw_info(child.second, pData->drawInfo_);
		}
		else if(child.first=="keyframe")
		{
			keyframe_data* pFrame = load_draw_keyframe(child.second, ctx);
			if(pFrame) pData->vecChildren_.emplace_back(pFrame);
			else       return err(to_str(pData->vecChildren_.size()) + "個目のframe");
		}
		else if(child.first=="tweenframe")
		{
			tweenframe_data* pFrame = load_draw_tweenframe(child.second, ctx);
			if(pFrame) pData->vecChildren_.emplace_back(pFrame);
			else       return err(to_str(pData->vecChildren_.size()) + "個目のframe");
		}
		else if(child.first=="color")
		{
			pData->up_over(draw_base_data::FLAG_COLOR);
			load_draw_base_color(child.second, pData);
		}
	}

	if(pData->vecChildren_.size()==0)
		return err("frameが一つも指定されていません。");

	pData->up_over(draw_base_data::FLAG_CHILDREN);

	return pData;
}

movieclip_data* draw_builder::load_draw_movieclip(const p_tree::ptree& element, draw_context& ctx)
{
	movieclip_data* pData = new_ movieclip_data();

	auto err = [&pData](const string& mes){ logger::warnln("[draw_builder]movieclipタグのフォーマットが間違っています。: " + mes); delete pData; return nullptr; };

	for(const auto& child : element)
	{
		if(child.first=="pivot")
		{
			pData->up_over(draw_base_data::FLAG_PIVOT);
			load_draw_base_pivot(child.second, pData->pivot_);
		}
		else if(child.first=="draw")
		{
			pData->up_over(draw_base_data::FLAG_DRAW_INFO);
			load_draw_base_draw_info(child.second, pData->drawInfo_);
		}
		else if(child.first=="layer" || child.first=="timeline")
		{
			timeline_data* pTimeline = load_draw_timeline(child.second, ctx);
			if(pTimeline)	pData->vecChildren_.emplace_back(pTimeline);
			else			return err(to_str(pData->vecChildren_.size()) + "個目のレイヤー");
		}
		else if(child.first=="color")
		{
			pData->up_over(draw_base_data::FLAG_COLOR);
			load_draw_base_color(child.second, pData);
		}
	}

	if(pData->vecChildren_.size()==0)
		return err("layerが一つも指定されていません。");

	pData->up_over(draw_base_data::FLAG_CHILDREN);

	return pData;
}

keyframe_data* draw_builder::load_draw_keyframe(const p_tree::ptree& element, draw_context& ctx)
{
	keyframe_data* pFrame = new_ keyframe_data();;

	for(const auto& child : element)
	{
		if(child.first=="next")
		{
			pFrame->nNextFrame_ = child.second.get("<xmlattr>.frame", 0);
		}
		else if(child.first=="pivot")
		{
			pFrame->up_over(draw_base_data::FLAG_PIVOT);
			load_draw_base_pivot(child.second, pFrame->pivot_);
		}
		else if(child.first=="draw")
		{
			pFrame->up_over(draw_base_data::FLAG_DRAW_INFO);
			load_draw_base_draw_info(child.second, pFrame->drawInfo_);
		}
		else if(child.first=="child")
		{
			pFrame->up_over(draw_base_data::FLAG_CHILDREN);
			if(!load_draw_child(child.second, ctx, pFrame))
			{
				logger::warnln("[draw_builder]keyframeタグのフォーマットが間違っています。");
				delete pFrame;
				return nullptr;
			}
		}
		else if(child.first=="color")
		{
			pFrame->up_over(draw_base_data::FLAG_COLOR);
			load_draw_base_color(child.second, pFrame);
		}
	#ifdef MANA_DEBUG
		else if(child.first=="<xmlattr>")
		{
			pFrame->sDebugName_ = child.second.get<string>("debug","");
		}
	#endif
	}

	return pFrame;
}

tweenframe_data* draw_builder::load_draw_tweenframe(const p_tree::ptree& element, draw_context& ctx)
{
	tweenframe_data* pTween = new_ tweenframe_data();

	bool bDraw=false;

	for(const auto& child : element)
	{
		if(child.first=="next")
		{
			pTween->nNextFrame_ = child.second.get("<xmlattr>.frame", 0);
		}
		else if(child.first=="pivot")
		{
			pTween->up_over(draw_base_data::FLAG_PIVOT);
			load_draw_base_pivot(child.second, pTween->pivot_);
		}
		else if(child.first=="draw")
		{
			pTween->up_over(draw_base_data::FLAG_DRAW_INFO);
			if(!bDraw)
			{
				bDraw = true;
				load_draw_base_draw_info(child.second, pTween->drawInfo_);
			}
			else
			{
				load_draw_base_draw_info(child.second, pTween->drawInfoEnd_);
			}
		}
		else if(child.first=="easing")
		{
			pTween->fEasing_ = clamp(child.second.get<float>("<xmlattr>.value",0.0f), -1.0, 1.0);
		}
		else if(child.first=="child")
		{
			pTween->up_over(draw_base_data::FLAG_CHILDREN);
			if(!load_draw_child(child.second, ctx, pTween))
			{
				logger::warnln("[draw_builder]tweenframeタグのフォーマットが間違っています。");
				delete pTween;
				return nullptr;
			}
		}
		else if(child.first=="color")
		{
			pTween->up_over(draw_base_data::FLAG_COLOR);
			load_draw_base_color(child.second, pTween);
		}
	#ifdef MANA_DEBUG
		else if(child.first=="<xmlattr>")
		{
			pTween->sDebugName_ = child.second.get<string>("debug","");
		}
	#endif
	}

	return pTween;
}

///////////////////////

bool draw_builder::load_draw_child(const p_tree::ptree& child, draw_context& ctx, draw_base_data* pData)
{
	for(const auto& e : child)
		if(!load_draw_switch(e, ctx, pData)) return false;

	return true;
}

tribool draw_builder::load_draw_common_switch(const p_tree::ptree::value_type& child, draw_context& ctx, draw_base_data* pData)
{
	if(child.first=="pivot")
	{
		pData->up_over(draw_base_data::FLAG_PIVOT);
		load_draw_base_pivot(child.second, pData->pivot_);
		return true;
	}
	else if(child.first=="draw")
	{
		pData->up_over(draw_base_data::FLAG_DRAW_INFO);
		load_draw_base_draw_info(child.second, pData->drawInfo_);
		return true;
	}
	else if(child.first=="child")
	{
		pData->up_over(draw_base_data::FLAG_CHILDREN);
		if(load_draw_child(child.second, ctx, pData))
		{
			return true;	
		}
		else
		{
			logger::warnln("[draw_builder]childタグ内のフォーマットが間違っています。タグ名が間違っている可能性があります。");
			return indeterminate;
		}
	}
	else if(child.first=="color")
	{
		pData->up_over(draw_base_data::FLAG_COLOR);
		load_draw_base_color(child.second, pData);
		return true;
	}

	return false;
}

} // namespace graphic end
} // namespace mana end
