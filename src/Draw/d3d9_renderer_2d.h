#pragma once

#include "renderer_2d_util.h"
#include "renderer_sprite_queue.h"
#include "renderer_text.h"

#include "d3d9_driver.h"
#include "d3d9_texture_manager.h"

#ifdef MANA_DEBUG
// 1レンダーで使われるスプライト数などをカウントする時は有効にする
#define MANA_D3D9RENDERER2D_RENDER_COUNT
#endif

namespace mana{
namespace draw{

namespace cmd{
struct text_draw_cmd;
} // namespace cmd end

class text_data;

/*! @brief 2D描画に特化した描画システムクラス
 *
 *  X,Yは2Dスクリーン座標指定、Zは0.0～fMaxZの範囲で指定する
 *  パースはつかない、Zに応じた拡縮もしない。Zはただの描画順制御
 *
 *  DirectXを実際に叩く
 *  やりとりは基本的には描画コマンドバッファを通して行う 
 *
 *  テクスチャを登録してからdraw系メソッドを使う
 *　登録したレンダーターゲットは処理順を制御すること。
 *  そうしないと続く処理でテクスチャとして使うことができない
 *
 *  描画領域を指定するのそのサイズで描画される
 *　フルスクリーンモード時のアスペクト比維持などのために使われる
 */
class d3d9_renderer_2d
{
public:
	enum device_lost_result
	{
		DL_PROC,	//!< デバイスロスト処理中
		DL_SUCCESS,	//!< デバイスロストから復帰
		DL_FATAL,	//!< デバイスロストから復帰不可能
	};

public:
	d3d9_renderer_2d();
	~d3d9_renderer_2d();

public:
	//! @brief 生成済みのドライバーを得て初期化と必要なスペックがあるかのチェック
	/*! @param[in] pDriver_ 生成済みのd3d9_driverへのスマートポインタ。Zバッファを使うのでZバッファありで生成している必要がある
	 *  @param[in] param 初期化構造体
	 *  @retrun 各種初期化に失敗したり、ビデオカードが必要なスペックを満たしていなかったらfalseが返る */
	bool init(const d3d9_driver_sptr& pDriver_, renderer_2d_init& param);

	//! @brief ドライバーを生成して、初期化と必要なスペックがあるかのチェック
	/*! @param[in] deviceParam d3d9_driverの初期化に使う構造体 */
	bool init(d3d9_device_init& deviceParam, renderer_2d_init& param);

	//! 終了処理
	void fin();

	//! 描画処理実行
	bool begin_scene();
	bool render();
	bool end_scene();

	//! @brief デバイスリセット
	/*! このメソッドを呼び出すと内部的にデバイスロスト状態になって結果的にResetされる。
	 *  device_lostメソッドを定期的に呼びデバイスロストから復帰したかを確認すること。 */
	void device_reset(bool bFullScreen, int32_t nBackWidth, int32_t nBackHeight);

	//! デバイスロスト
	bool is_device_lost()const;

	//! DL_SUCCESSかDL_FATAL返って来るまで定期的に呼び続けること
	device_lost_result device_lost();

public:
	//! @defgroup 2d_render_scrennshot スクリーンショット操作
	//! @{
	//! スクリーンショットのファイル名の先頭に付ける名前を設定。スクリーンショットは、sScreenShotHeader_フレームカウント.bmp で出力される
	void	set_screenshot_header(const string& sScreenShotHeader){ sScreenShotHeader_ = sScreenShotHeader; }
	//! スクリーンショットを撮るためにフラグを立てる。renderメソッドが呼ばれるとリセットされる
	void	write_screenshot_frame(){ bScreenShot_ = true; }
	//! @}

public:
	//! @defgroup 2d_render_render_tex_ctrl 一時描画対象操作
	//! @{
	void	set_clear_color(D3DCOLOR color){ nRenderTaregetColor_ = color; }
	//! スクリーン全体のカラー。α値をいじることでフェードさせることができる。色がいらない時はαを0にする。即座にバッファに書き込みに行くので注意して使うこと
	void	set_screen_color(D3DCOLOR color);
	//! @}

public:
	//! @defgroup 2d_render_draw_call 描画リクエスト
	//! @{
	bool	add_render_target(const string& sID, uint32_t nPriority, uint32_t nReserveSpriteCmdNum=32);
	bool	add_render_target(uint32_t nID, uint32_t nPriority, uint32_t nReserveSpriteCmdNum=32);
	bool	draw_sprite(const sprite_param& cmd, uint32_t nRenderTarget=cmd::BACK_BUFFER_ID){ return render_cmd_queue().add_cmd(cmd, nRenderTarget); }
	//! @}

public:
	//! @defgroup 2d_render_draw_call_helper 描画リクエストヘルパー
	//! @{
	bool	draw_sprite(const string& sTextureID, POS pos, bool bBlend, uint32_t nRenderTarget=cmd::BACK_BUFFER_ID);
	bool	draw_sprite(const string& sTextureID, POS pos, bool bBlend, SIZE scale, float fRot, POS pivot, uint32_t nRenderTarget=cmd::BACK_BUFFER_ID);
	bool	draw_sprite(const string& sTextureID, RECT rectTex, POS pos, bool bBlend, SIZE scale, float fRot, POS pivot, uint32_t nRenderTarget=cmd::BACK_BUFFER_ID);

	//! DistanceFieldTextureを使った描画
	bool	draw_df(const string& sDFTextureID, uint32_t nColor, POS pos, SIZE scale, float fRot, POS pivot, uint32_t nRenderTarget=cmd::BACK_BUFFER_ID);

	//! テキスト描画
	bool	draw_text(struct cmd::text_draw_cmd& cmd){ return renderText_.draw_text(render_cmd_queue(), cmd); }
	//! @}

public:
	const d3d9_driver_sptr&		driver(){ return pDriver_; }
	const d3d9_driver_sptr&		driver()const{ return pDriver_; }
	d3d9_texture_manager&		tex_manager(){ return texManager_; }
	const d3d9_texture_manager&	tex_manager()const{ return texManager_; }
	renderer_text&				text_renderer(){ return renderText_; }
	const renderer_text&		text_renderer()const{ return renderText_; }
	renderer_sprite_queue&		render_cmd_queue(){	return renderQueue_; }

	uint32_t					render_width()const{ return nRenderWidth_; }
	uint32_t					render_height()const{ return nRenderHeight_; }

private:
	//! 各描画単位でのパラメータ
	struct draw_call_param
	{
		uint32_t	nRendetTarget_; // 描画先のレンダーターゲット
		uint32_t	nTexture_;		// 使用するテクスチャー
		draw_mode	eMode_;			// 描画モード

		uint32_t	nStartVertex_;	// 頂点開始位置 
		uint32_t	nVertexNum_;	// 使用する頂点数
	};

private:
	// ショートカット
	LPDIRECT3DDEVICE9		device(){ return pDriver_->device(); }
	const d3d9_draw_caps&	draw_caps()const{ return pDriver_->draw_caps(); }

private:
	//! @defgroup renderer_2d_init 初期化ヘルパー
	//! @{
	bool init_inner(renderer_2d_init& param);
	bool check_caps();
	bool init_vertex();
	bool init_effect(const string& sPath="");
	bool init_render_target();
	bool init_tex();
	void init_render();
	//! @}

	//! @defgroup renderer_2d_state_helper ステート変更ヘルパー
	//! @{
	void render_state_alpha_test(bool bEnable);
	void render_state_wire_frame(bool bEnable);
	//! @}

	//! @defgroup renderer_2d_render レンダ－実体
	//! @{
	bool render_ready();
	void render_inner();
	//! @}

private:
	//! スプライトコマンドをDrawCallコマンドに変換する
	/*! @return falseだったら、これ以上DrawCallコマンドが積めない。頂点バッファがいっぱいの状態 */ 
	bool sprite_param_to_draw_call_cmd(const sprite_param& cmd, draw_call_param& drawParam, uint32_t& nStartVertex,
									   float* pPos, float* pUV, float* pColor, float* pColor2);

private:
	bool is_device_reset()const{ return bResetDevice_.load(std::memory_order_acquire); }

#ifdef MANA_DEBUG
private:
	//! @defgroup renderer_2d_check 実験用レンダー関数
	//! @{
	void vertex_ready();
	void vertex_draw();

	void tex_ready();
	void tex_draw();

	void tex_color_ready();
	void tex_color_draw();
	//! @}
#endif // MANA_DEBUG

private:
	//! D3D9ドライバーとデバイス
	d3d9_driver_sptr pDriver_;

	//! 頂点宣言
	IDirect3DVertexDeclaration9* pVertexDecl_;

	//! @defgroup d3d9_system_vertexbuf バーテックスバッファ
	/*! Vertexの並びは、左上・右上・左下・右下のＺ並び */
	//! @{
	LPDIRECT3DVERTEXBUFFER9 pPosVertex_;
	LPDIRECT3DVERTEXBUFFER9 pUvVertex_;
	LPDIRECT3DVERTEXBUFFER9 pColorVertex_;
	LPDIRECT3DVERTEXBUFFER9 pColor2Vertex_;
	//! @}

	//! インデックスバッファ
	LPDIRECT3DINDEXBUFFER9	pIndexBuffer_;

	//! エフェクト(シェーダー)
	LPD3DXEFFECT			pEffect_;

	D3DXHANDLE				hTechnique_;
	D3DXHANDLE				hMatPerPixel_;
	D3DXHANDLE				hTex_;

	D3DXMATRIX				matPerPixel_;

	//! 一時描画対象
	LPDIRECT3DVERTEXBUFFER9 pRenderPosVertex_;
	LPDIRECT3DVERTEXBUFFER9 pRenderUvVertex_;
	LPDIRECT3DVERTEXBUFFER9 pRenderColorVertex_;

	LPDIRECT3DTEXTURE9		pRenderTexture_;
	LPDIRECT3DSURFACE9		pRenderTarget_;

	D3DXMATRIX				matRenderPerPixel_;

	D3DCOLOR				nRenderTaregetColor_;		// 一時描画対象のクリアカラー
	D3DCOLOR				nRenderTaregetVertexColor_; // 位置描画対象の頂点カラー。画面全体フェードなどを行う時に使う

	LPDIRECT3DSURFACE9		pBackBuf_;

	//! テクスチャー管理
	d3d9_texture_manager	texManager_;

	//! テキスト描画ヘルパークラス
	renderer_text			renderText_;

	//! 描画コマンドバッファ
	renderer_sprite_queue	renderQueue_;
	vector<draw_call_param>	vecDrawCall_;

	// 描画領域の幅と高さと最大Z
	uint32_t nRenderWidth_;
	uint32_t nRenderHeight_;
	float	 fMaxZ_;

	// ドローコールできるスプライト最大数など
	uint32_t nMaxDrawSpriteNum_;
	uint32_t nMaxDrawCallPrimitiveNum_;

	//! スクリーンショットフラグ
	//! このフラグ立っているフレームはスクショを書き出す
	bool	 bScreenShot_;

	//! スクリーンショットのファイル名ヘッダ
	string	 sScreenShotHeader_;

	//! レンダーカウンター。renderを1回呼ぶと1増える
	uint32_t nRenderCounter_;

private:
	//! デバイスリセットフラグ
	std::atomic_bool bResetDevice_;

	enum device_lost_state
	{
		DEVLOST_NO,
		DEVLOST_RESET_WAIT,
		DEVLOST_FATAL,
	} eDlState_;


#ifdef MANA_D3D9RENDERER2D_RENDER_COUNT
private:
	//　実際の最大数
	// この値を使って、バッファの大きさなどを最適化できるといいんじゃ無いかなー
	uint32_t nMaxDrawSpriteCounter_;
	uint32_t nMaxDrawCallSpriteCounter_;
	uint32_t nMaxDrawCallCounter_; // DrawIndexedPrimitiveを呼んだ回数。vecDrawVertexPos_のreserve値

	uint32_t nMaxTextureVram_; // テクスチャ最大容量(目安)
#endif

private:
	NON_COPIABLE(d3d9_renderer_2d);
};

} // namespace draw end
} // namespace mana end

/*
    draw::d3d9_device_init param(wnd_handle(), width(), height());
	shared_ptr<d3d9_driver> pDriver_ = make_shared<d3d9_driver>();
	pDriver_->create_driver();
	pDriver_->create_device(param);

	renderer_.init(pDriver_);
	renderer_.set_clear_color(D3DCOLOR_ARGB(255,0,128,0));
	renderer_.set_screenshot_header("screenshot/");

	renderer_.add_texture_info("test", "test.png");
 
    while(1)
	{
		renderer_.draw_sprite("test", draw::POS(10.0f,10.0f,100.0f),   false);
		renderer_.draw_sprite("test", draw::POS(400.0f,200.0f,99.0f),  false);
		renderer_.draw_sprite("test", draw::POS(400.0f,200.0f,100.0f), true, draw::SIZE(1.5f,1.5f), 30.0f, draw::POS(64.0f,64.0f));
		renderer_.draw_sprite("test", draw::RECT(10.0f,10.0f, 64.0f, 64.0f), draw::POS(200.0f,50.0f, 10.0f), false, draw::SIZE(0.5f,0.5f), 90.0f, draw::POS(0.0f, 0.0f));

		pDriver_->begin_scene();
		renderer_.render();
		pDriver_->end_scene();
	}

 */
