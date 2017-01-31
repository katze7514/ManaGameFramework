/*
 *　renderer_2d向けの描画コマンド
 *  これらのコマンドをrenderer_2d_queueに積むことで、render_2dを操作する
 *  これによって描画のマルチスレッド化を実現する
 */
#pragma once

#include "renderer_2d_util.h"
#include "d3d9_driver.h"
#include "text_data.h"

#ifdef MANA_DEBUG
#define MANA_RENDER_2D_CMD_COUNT
#endif

namespace mana{
namespace draw{

class d3d9_renderer_2d;
struct tex_info;

namespace cmd{

enum kind
{
	KIND_NONE,
	KIND_TEX,
	KIND_FONT,
};

//! スクリーンに対する操作
//! スクリーンショットや、スクリーンカラーなどを変更する
struct screen_ctrl_cmd
{
public:
	enum ctrl
	{
		CTRL_NONE,
		CTRL_SCEEN_SHOT,
		CTRL_SCREEN_COLOR,
		CTRL_CLEAR_COLOR,
	};

public:
	screen_ctrl_cmd():eCtrl_(CTRL_NONE),nColor_(0xFFFFFFFF){}

public:
	ctrl	eCtrl_;
	DWORD	nColor_;
};

//! テクスチャ/フォント情報ファイルロード
struct info_load_cmd
{
public:
	info_load_cmd():eKind_(KIND_NONE),bFile_(true)
	{
	#ifdef MANA_DEBUG
		bReDefine_ = false;
	#endif
	}

public:
	kind		eKind_;
	string_fw	sID_;	// font id
	string		sFilePath_;
	bool		bFile_;

	function<void (uint32_t)> callback_; // KIND_TEX 0で失敗、1で成功
										 // KIND_FONT フォント数値IDが入ってくる

#ifdef MANA_DEBUG
	bool bReDefine_;
#endif
};

//! テクスチャ/フォント情報削除
struct info_remove_cmd
{
public:
	info_remove_cmd():eKind_(KIND_NONE),id_(0){}

public:
	kind	 eKind_;
	variant<uint32_t, string_fw> id_;
};

//! テクスチャ情報登録
struct tex_info_add_cmd
{
public:
	tex_info_add_cmd():nWidth_(0),nHeight_(0),nFormat_(0)
	{
		release_handler_ = [](const tex_info&){};

	#ifdef MANA_DEBUG
		bReDefine_=false;
	#endif
	}

public:
	string sTextureID_;
	string sFilePath_;	// emptyだとレンダーターゲット

	uint32_t nWidth_;
	uint32_t nHeight_;
	uint32_t nFormat_;

	string	sGroup_;

	// レンダーターゲットがvram_manageされて消える時に呼ばれるハンドラ
	function<void(const tex_info&)> release_handler_;

	function<void (uint32_t)> callback_; // テクスチャ数値IDを返す

#ifdef MANA_DEBUG
	bool bReDefine_; // trueだと強制的に上書きされる
#endif
};

//! グループ操作
struct tex_group_cmd
{
public:
	enum ctrl
	{
		REMOVE,
		RELEASE,
	};

public:
	enum ctrl	eCtrl_;
	string		sGroup_;
};

//! テキスト描画コマンド
struct text_draw_cmd
{
public:
	text_draw_cmd():nCharNum_(UINT_MAX),nColor_(0xFFFFFFFF),nRenderTarget_(cmd::BACK_BUFFER_ID){}

public:
	shared_ptr<text_data>	pText_;
	uint32_t				nCharNum_; // この値までの文字数を描画する
	draw::POS				pos_;
	DWORD					nColor_;

	uint32_t				nRenderTarget_;
};

//! スプライト描画コマンド
struct sprite_draw_cmd
{
public:
	sprite_draw_cmd():nTexID_(0),fZ_(0.0f),nColor0_(D3DCOLOR_ARGB(255,255,255,255)),nColor1_(D3DCOLOR_ARGB(255,255,255,255)),
					  mode_(MODE_TEX_COLOR_BLEND),nRenderTarget_(cmd::BACK_BUFFER_ID)
					  { D3DXMatrixIdentity(&worldMat_); }

public:
	uint32_t	nTexID_;
	draw::RECT	rect_;

	float		fZ_;
	D3DXMATRIX	worldMat_;
	DWORD		nColor0_;
	DWORD		nColor1_;
	draw_mode	mode_;

	uint32_t	nRenderTarget_;
};

//! ポリゴン描画コマンド。実質デバッグ用なのであまり機能はない
struct polygon_draw_cmd
{
public:
	polygon_draw_cmd():nVertexNum_(4),nColor0_(D3DCOLOR_ARGB(255,255,255,255)),nColor1_(D3DCOLOR_ARGB(255,255,255,255)),
					  mode_(MODE_PLY_COLOR),nRenderTarget_(cmd::BACK_BUFFER_ID)
	{
		for(auto& v : vertex_) v.fX = v.fY = v.fZ = 0.f;
	}

public:
	uint32_t			nVertexNum_;
	draw::POS_VERTEX	vertex_[4]; // グローバル変換済み
	DWORD				nColor0_;
	DWORD				nColor1_;
	draw_mode			mode_;

	uint32_t			nRenderTarget_;
};

/////////////////////////
// コマンドvariant
typedef variant<screen_ctrl_cmd,
				tex_info_add_cmd,
				info_load_cmd,
				info_remove_cmd,
				tex_group_cmd,
				text_draw_cmd,
				sprite_draw_cmd,
				polygon_draw_cmd> render_2d_cmd;


/*! @brief render_2d_cmdを実行するvisitor
 *
 *  render_2d_queueに対して、以下のように実行する
 *
 *  d3d9_renderer_2d	renderer;
 *  render_2d_queue		queue;
 *  render_2d_cmd_exec	receiver(renderer);
 *  for_each(queue.queue.begin(), queue.queue.end(), boost::apply_visitor(receiver));
 */
struct render_2d_cmd_exec : public boost::static_visitor<>
{
public:
	render_2d_cmd_exec(d3d9_renderer_2d& renderer):renderer_(renderer){}
	
	void operator()(screen_ctrl_cmd& cmd)const;
	void operator()(info_load_cmd& cmd)const;
	void operator()(info_remove_cmd& cmd)const;
	void operator()(tex_info_add_cmd& cmd)const;
	void operator()(tex_group_cmd& cmd)const;
	void operator()(text_draw_cmd& cmd)const;
	void operator()(sprite_draw_cmd& cmd)const;
	void operator()(polygon_draw_cmd& cmd)const;

private:
	d3d9_renderer_2d& renderer_;

private:
	NON_COPIABLE(render_2d_cmd_exec);
};

} // namespace cmd end

} // namespace draw end
} // namespace mana end
