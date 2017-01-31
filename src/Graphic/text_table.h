#pragma once

namespace mana{

namespace draw{
class text_data;
} // namespae draw end

namespace graphic{

class draw_context;

/*! @brief テキストテーブル
 *
 *  テキストをID引き出来るようにしたもの
 *  load_fileでロードリクエストをして、is_fin_loadで終了をチェックする
 */
class text_table
{
private:
	typedef unordered_map<string_fw, shared_ptr<draw::text_data>, string_fw_hash> text_map;

public:
	enum load_result
	{
		FAIL,
		LOAD,
		SUCCESS,
	};

private:
	enum load_state
	{
		LOAD_ERR,
		LOAD_START,
		LOAD_FONT,
		LOAD_TEXT,
		LOAD_FIN,
	};

public:
	text_table():eLoadState_(LOAD_FIN)
	{
	#ifdef MANA_DEBUG
		bReDefine_ = false;
	#endif
	}

public:
	bool add_text_data(const string_fw& sID, const string_fw& sFontID, const string& sText, bool bMarkUp=false);
	void remove_text(const string_fw& sID);
	void clear();

	shared_ptr<draw::text_data> text_data(const string_fw& sID);

	bool		load_file(const string& sFilePath, draw_context& ctx, bool bFile=true);
	load_result is_fin_load(draw_context& ctx);

private:
	bool		load_font(draw_context& ctx);
	bool		load_text(draw_context& ctx);

private:
	text_map table_;

	load_state				eLoadState_;
	uint32_t				nLoadWait_;
	std::atomic_uint32_t	nLoadFin_;
	std::atomic_bool		bLoadErr_;
	p_tree::ptree			treeLoad_; //!< ロード中のプロパティツリー

#ifdef MANA_DEBUG
public:
	void redefine(bool bReDefine){ bReDefine_=bReDefine; }

private:
	bool bReDefine_;
#endif
};

} // namespace graphic end
} // namespace mana end

/*
<text_table>
	<fond id="" src="" />

	<text_data id="">
		<fond id="" />
		<text markup="">
		</text>
	</text_data>
</text_table>

 */