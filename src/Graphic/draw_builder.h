#pragma once

namespace mana{

namespace graphic{
struct draw_base_data;
struct label_data;
struct sprite_data;
struct audio_data;
struct message_data;
struct keyframe_data;
struct tweenframe_data;
struct timeline_data;
struct movieclip_data;

class label;
class sprite;
class audio_frame;
class message;
class keyframe;
class timeline;
class movieclip;

class draw_context;
class draw_base;

/*! @brief draw_baseビルダー
 *
 *  定義ファイルを読み込み、それに合わせてdraw_baseインスタンスを生成する
 *  TextTableやAudioを使う時は、先にTextTable/AudioPlayerをロード完了させておくこと */
class draw_builder
{
public:
	typedef unordered_map<string_fw, draw_base_data*, string_fw_hash>	draw_hash;

private:
	enum load_state
	{
		LOAD_START,
		LOAD_TEX_FONT_WAIT,
		LOAD_NODE,
		LOAD_FIN,
		LOAD_ERR,
	};

public:
	enum load_result
	{
		FAIL,
		LOAD,
		SUCCESS,
	};

public:
	draw_builder();
	~draw_builder();

public:
	//! ノードを生成して返す。無効なIDだったり、ロード中の場合はnullptrが返る
	draw_base*	create_draw_base(const string_fw& sNodeID);

	//! 生成したノードをT型にキャストして返す
	template<class T>
	T*			create_draw_base_cast(const string_fw& sNodeID){ return static_cast<T*>(create_draw_base(sNodeID)); }

	//! @brief ノード情報ファイルからノード情報を展開する
	/*!	ファイルからデータをロードする。ロードの終了はis_fin_loadメソッドで確認する。
	 *	このメソッド前後で、rendererのstart_request/end_requestを呼んで置くこと
	 *	ロードが終了しているとcreate系メソッドが使える。
	 *
	 *　audio_frameを使う場合は、先にaudio_frameで使うサウンド情報を登録しておくこと
	 **/
	bool		load_draw_info_file(const string& sFilePath, draw_context& context, bool bFile=true);

	//! ロードが終了したかチェックする
	load_result	is_fin_load(draw_context& context);

	//! ノード情報をクリアする。ただし、ファイルロード中の時はクリアしない
	void clear();

protected:
	bool			load_draw(draw_context& context);
	bool			load_draw_switch(const p_tree::ptree::value_type& element, draw_context& ctx, draw_base_data* pParent=nullptr);

	draw_base_data*	load_draw_base(const p_tree::ptree& element, draw_context& ctx);
	label_data*		load_draw_label(const p_tree::ptree& element, draw_context& ctx);
	sprite_data*	load_draw_sprite(const p_tree::ptree& element, draw_context& ctx);
	audio_data*		load_draw_audio(const p_tree::ptree& element, draw_context& ctx);
	message_data*	load_draw_message(const p_tree::ptree& element, draw_context& ctx);
	timeline_data*	load_draw_timeline(const p_tree::ptree& element, draw_context& ctx);
	movieclip_data*	load_draw_movieclip(const p_tree::ptree& element, draw_context& ctx);

	keyframe_data*	 load_draw_keyframe(const p_tree::ptree& element, draw_context& ctx);
	tweenframe_data* load_draw_tweenframe(const p_tree::ptree& element, draw_context& ctx);

	bool			load_draw_child(const p_tree::ptree& child, draw_context& ctx, draw_base_data* pData);

	//! @brief draw系タグが持つ共通のタグの処理を行う
	/*! @retval true			タグが処理された 
	 *  @retval false			タグが処理されなかった
	 *  @retval indeterminate	タグの処理でエラーが起きた */
	tribool			load_draw_common_switch(const p_tree::ptree::value_type& element, draw_context& ctx, draw_base_data* pData);

protected:
	//! @defgroup draw_builder_create create_drawヘルパー
	//! @{
	void			set_draw_base(draw_base* pNode, draw_base_data* pData, bool bChildren=true);
	draw_base*		create_draw_switch(draw_base_data* pData);
	draw_base*		create_draw_from_data(draw_base_data* pData);

	label*			create_label(draw_base_data* pData);
	sprite*			create_sprite(draw_base_data* pData);
	audio_frame*	create_audio(draw_base_data* pData);

	message*		create_message(draw_base_data* pData);

	timeline*		create_timeline(draw_base_data* pData);
	void			create_keyframe_draw(graphic::keyframe* pKeyFrame, keyframe_data* pKeyFrameData, keyframe* pPreKeyFrame, keyframe_data* pPreKeyFrameData);

	movieclip*		create_movieclip(draw_base_data* pData);

	draw_base*		create_draw_inner(draw_base_data* pData);
	//! @}

protected:
	draw_hash	hashNode_;

protected:
	load_state	eLoadState_;
	uint32_t	nLoadWait_;	//!< ロード待ちするテクスチャなどの数

	std::atomic_uint32_t	nLoadFin_;	//!< ロード終了したテクスチャ情報などの数
	std::atomic_bool		bLoadErr_;

	p_tree::ptree treeLoad_; //!< ロード中のプロパティツリー

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
	draw_context ctx;

	draw_builder builder;

	ctx.renderer()->start_request();
	builder.load_draw_info_file("draw_info.xml",ctx);
	ctx.renderer()->end_request();

	while(builder.is_fin_load(ctx)==draw_builder::LOAD);

	draw_base* pNode = builder.create_draw("TEST");
*/

/*
<draw_base_def>
	<texture_def src="" />
	<texture id="" src="" group="" />
	<font id="" src="" />

	<base id="">
		<pivot x="" y="" />
		<draw x="" y="" width="" height="" angle="" alpha="" />
		<color value="" mode="" /> <!-- modeはレイヤー合成。NORMAL/MUL/SCREEN の三種類対応してる -->
		<child>
			<base id="" /> // 指定IDのbaseが読み込まれる。IDの解決はcreate時に行われるのでIDが前方にある必要はない
			<base> // idを指定しなかった場合は空のbaseが生成される
				<draw x="" y="" width="" height="" angle="" alpha="" />
				<color value="" mode="" />
			</base>
		</child>
	</base>

	<lable id="" debug="">
		<draw x="" y="" color="" /> // colorは16進数形式(FFFFFFF)
		<text_data id="" /> // テキストテーブルから引っ張ってくる。これが記述された場合font/textタグは無効になる

		<font id="" />
		<text markup=""></text>
		<child> // ラベル下にもchildは記述できるがworldが崩れるのでラベル下には書かないことを推奨
			<base id="" />
		</child>
	</label>

	<sprite id="">
		<pivot x="" y="" />
		<draw x="" y="" width="" height="" angle="" alpha="" />
		<color value="" mode="" />
		<tex id="" />
		<rect left="" top="" right="" bottom="" />
		<child>
		</child>
	</sprite>

	<audio id="" kind="">
		<sound id="" />
		<play change="" frame="" loop="" force="" />	// BGM・SE用パラメータ. mode: stop/fadeout/cross
		<stop frame="" /> // BGMの時はIDは無視される
		<vol value="" />					// vol
		<fade !start="" end="" frame="" />	// fade vol

		<!-- 以下のタグはdraw_baseとしての互換を保つためでサウンド処理としては
			 何も使っていない -->
		<pivot x="" y="" />
		<draw x="" y="" width="" height="" angle="" color="" />
		<child>
		</child>
	</audio>

	<message id="">
		<next frame="" /> // 1文字を表示するフレーム数
		<sound id="" />   // 1文字表示する時鳴らすSE

		<!-- それ以外はlabelと同じ -->
	</message>

	<timeline id="">
		<pivot x="" y="" />
		<draw x="" y="" width="" height="" angle="" color="" />
		<keyframe debug="">
			<next frame="" />
			<pivot x="" y="" />
			<draw x="" y="" width="" height="" angle="" alpha="" />
			<color value="" mode="" />
			<child>
				<base id="" name=""> // IDを指定しつつdrawタグなどを記述した場合は、記述した部分だけ上書きされる
					<draw_info x="" y="" width="" height="" angle="" color="" />
				</base>
				<audio id="" />
			</child>
		</keyframe>

		<tweenframe>
			<next frame="" />
			<pivot x="" y="" />
			<draw x="" y="" width="" height="" angle="" alpha="" />
			<draw x="" y="" width="" height="" angle="" alpha="" />
			<color value="" mode="" />
			<easing value="1.0" />
			<child>
				<base name=""> // 同一timeline上で、name属性が同じbaseはインスタンスを共有する
					<pos x="" y="" />
					<draw x="" y="" width="" height="" angle="" alpha="" />
				</base>
			</child>
		</tweenframe>
	</timeline>

	<movieclip id="">
		<pivot x="" y="" />
		<draw x="" y="" width="" height="" angle="" alpha="" />
		<color value="" mode="" />
		<layer>
			// timelineタグと中身は一緒
		</layer>
	</movieclip>

</draw_base_def>
*/

