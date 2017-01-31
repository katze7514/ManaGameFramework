#include "../mana_common.h"

#include "../File/file.h"
#include "../Draw/renderer_2d_cmd.h"
#include "../Draw/renderer_2d.h"
#include "../Draw/text_data.h"

#include "draw_context.h"
#include "text_table.h"

namespace mana{
namespace graphic{

bool text_table::add_text_data(const string_fw& sID, const string_fw& sFontID, const string& sText, bool bMarkUp)
{
	shared_ptr<draw::text_data> pTextData = boost::make_shared<draw::text_data>(sFontID, sText, bMarkUp);
	auto r = table_.emplace(sID, pTextData);
	if(!r.second)
	{
	#ifdef MANA_DEBUG
		if(bReDefine_)
		{// リロードフラグが立ってたら上書き
			auto& it = r.first->second;

			it->set_font_id(sFontID);
			it->set_text(sText);
			it->markup(bMarkUp);

			return true;
		}
	#endif

		logger::warnln("[text_table]テキストデータを追加できませんでした。: " + sID.get());
		return false;
	}

	return true;
}

void text_table::remove_text(const string_fw& sID)
{
	table_.erase(sID);
}

void text_table::clear()
{
	table_.clear();
}

shared_ptr<draw::text_data> text_table::text_data(const string_fw& sID)
{
	auto it = table_.find(sID);
	if(it!=table_.end()) return it->second;
	return std::move(shared_ptr<draw::text_data>());
}

bool text_table::load_file(const string& sFilePath, draw_context& ctx, bool bFile)
{
	if(eLoadState_!=LOAD_FIN)
	{
		logger::warnln("[text_table]定義ファイルロード中です。");
		return false;
	}

	if(bFile)
		logger::infoln("[text_table]定義ファイルを読み込みます。: " + sFilePath);
	else
		logger::infoln("[text_table]定義ファイルを読み込みます。");

	std::stringstream ss;
	if(!file::load_file_to_string(ss, sFilePath, bFile))
	{
		eLoadState_=LOAD_ERR;
		logger::warnln("[text_table]定義ファイルが読み込めませんでした。");
		return false;
	}

	treeLoad_.clear();

	using namespace p_tree;
	xml_parser::read_xml(ss, treeLoad_, xml_parser::no_comments);

	return load_font(ctx);
}

text_table::load_result text_table::is_fin_load(draw_context& ctx)
{
	auto success = [this](){ eLoadState_=LOAD_FIN; treeLoad_.clear(); return SUCCESS; };
	auto err	 = [this](){ eLoadState_=LOAD_ERR; treeLoad_.clear(); return FAIL; };

	switch(eLoadState_)
	{
	case LOAD_FIN:
		return SUCCESS;
	break;

	case LOAD_FONT:
		if(bLoadErr_)
		{	return err();	}
		else if(nLoadWait_==nLoadFin_.load(std::memory_order_acquire)) // エラー無くフォントロード終了
		{	eLoadState_ = LOAD_TEXT;	}

		return LOAD;
	break;

	case LOAD_TEXT:
		if(load_text(ctx))
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

bool text_table::load_font(draw_context& ctx)
{
	using namespace p_tree;

	auto err = [this](const string&& mes){ logger::warnln("[text_table]"+mes); eLoadState_=LOAD_ERR; treeLoad_.clear(); return false; };

	eLoadState_ = LOAD_START;
	nLoadWait_	= 0;
	nLoadFin_.store(0, std::memory_order_release);
	bLoadErr_.store(false, std::memory_order_release);

	optional<ptree&> text_table_def = treeLoad_.get_child_optional("text_table");
	if(!text_table_def)
		return err("定義ファイルのフォーマットが間違っています。: <text_table>が存在しません。");

	uint32_t nFontNum=1;

	// fontのロードを仕込む
	for(const auto& it : *text_table_def)
	{
		if(it.first=="font")
		{
			draw::cmd::info_load_cmd cmd;
			cmd.eKind_		= draw::cmd::KIND_FONT;
			cmd.sID_		= it.second.get<string>("<xmlattr>.id","");
			cmd.sFilePath_	= it.second.get<string>("<xmlattr>.src","");
			cmd.callback_	= [this](uint32_t result){ if(result>0) ++nLoadFin_; else bLoadErr_.store(true, std::memory_order_release); };

			if(cmd.sID_!="" && cmd.sFilePath_!="")
			{
				ctx.renderer()->request(cmd);
				++nFontNum;
			}
			else
			{
				return err("定義ファイルのフォーマットが間違っています。: " + to_str_s(nFontNum) + "個目の<font>");
			}

			++nLoadWait_;
		}
	}

	if(nLoadWait_>0)
		eLoadState_ = LOAD_FONT;
	else
		eLoadState_ = LOAD_TEXT;

	return true;
}

bool text_table::load_text(draw_context& ctx)
{
	using namespace p_tree;

	auto err = [](const string&& mes){ logger::warnln("[text_table]text_dataタグのフォーマットが間違っています。: " + mes); return false; };

	auto& def = treeLoad_.get_child("text_table");

	for(const auto& child : def)
	{
		if(child.first=="text_data")
		{
			string_fw sID(child.second.get<string>("<xmlattr>.id",""));
			if(sID=="") return err("text_dataのidが設定されていません。");

			// font
			auto fontID = child.second.get_optional<string>("font.<xmlattr>.id");
			if(!fontID) return err("fontタグにidが設定されていません。");

			// text
			auto text = child.second.get_child_optional("text");
			if(!text) return err("textタグが存在しません。");

			bool bMarkUp = (*text).get<bool>("<xmlattr>.markup",false);

			if(!add_text_data(sID, string_fw(*fontID), boost::trim_copy((*text).get_value<string>()), bMarkUp))
				return false;
		}
	}

	return true;
}

} // namespace graphic end
} // namespace mana end
