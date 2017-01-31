#pragma once

#include "renderer_2d_cmd.h"

namespace mana{
namespace draw{

class d3d9_driver;
class d3d9_renderer_2d;

namespace cmd{
class renderer_2d_cmd;
}

/*! @brief 2Dレンダラーのベースクラス
 *
 *  2Dレンダーはこのクラスを継承する。
 *  今の所、Direct3D9実装の別スレッド版と同スレッド版の2つを用意している
 *
 *  初期化は、init_driverとinit_renderの両方を呼ぶこと
 */
class renderer_2d
{
public:
	typedef function<void(bool)> reset_handler;

public:
	renderer_2d();
	virtual ~renderer_2d();

public:
	//! @defgroup renderer_2d_init 2Dレンダードライバー初期化
	//! @{
	bool init_driver(const shared_ptr<d3d9_driver>& pDriver, renderer_2d_init& renderer, const string& sScreenShotHeader="screenshot/");
	bool init_driver(d3d9_device_init& device, renderer_2d_init& renderer, const string& sScreenShotHeader="screenshot/");
	//! @}

	//! @brief 2Dレンダー初期化
	/*! @param[in] nReserveRequestNum 積むリクエストの推定数
	 *  @param[in] nAffinity レンダースレッドを動作させるCPUコア番号 */
	virtual bool init_render(uint32_t nReserveRequestNum=128, uint32_t nAffinity=0)=0;

	//! 終了処理
	virtual void fin()=0;

	//! @defgroup renderer_2d_request リクエスト処理
	//! @{
	//! @ brief request積み処理を開始する
	virtual bool			start_request(bool bWait=true)=0;
	//! @brief requestを積む
	virtual void			request(const cmd::render_2d_cmd& cmd)=0;
	//! request積みの終了
	virtual void			end_request()=0;
	//! 積んだリクエストの処理を開始する
	virtual render_result	render(bool bWait=true)=0;
	//! @}

	//! @defgroup renderer_2d_util
	//! @{
	optional<uint32_t>		texture_id(const string& sID);
	void					device_reset(bool bFullScreen, int32_t nBackWidth, int32_t nBackHeight, const reset_handler& resetHandler);
	//! @}

	//! デバイスロストしてるかどうか
	virtual bool is_device_lost()const=0;

	//! @defgroup renderer_2d_request_helper リクエストヘルパー
	//! @{
	void	request_screen_shot();
	void	request_screen_color(DWORD nColor);
	void	request_clear_color(DWORD nColor);
	void	request_tex_info_load(const string& sFilePath, const function<void (uint32_t)>& callback=nullptr, bool bFile=true);
	void	request_font_info_load(const string& sID, const string& sFilePath, const function<void (uint32_t)>& callback=nullptr, bool bFile=true);
	void	request_tex_info_remove(uint32_t nID);
	void	request_font_info_remove(const string_fw& sID);
	void	request_tex_info_add(const string& sID, const string& sFilePath, const function<void(uint32_t)>& callback=nullptr, uint32_t nWidth=0, uint32_t nHeight=0, uint32_t nFormat=0);
	void	request_tex_info_remove_group(const string& sGroup);
	void	request_tex_release_group(const string& sGroup);
	//! @}

protected:
	d3d9_renderer_2d* pRenderer_; //!< 描画処理実体

	reset_handler resetHandler_; //!< device_resetが終了した時に呼ばれる

#ifdef MANA_DEBUG
public:
	// 再定義フラグ。trueにするとテクスチャ情報登録などの時に同IDが登録されていた場合、上書きする
	void reqeust_redefine(bool bReload){ bReDefine_=bReDefine_; }

private:
	bool bReDefine_;
#endif
};

typedef shared_ptr<renderer_2d> renderer_2d_sptr;

//! 2Dレンダラー種別
enum renderer_2d_kind
{
	RENDERER_ASYNC,	//!< 別スレッド実行
	RENDERER_SYNC,
};

//! 2Dレンダラー生成関数
extern renderer_2d_sptr create_renderer_2d(renderer_2d_kind eKind=RENDERER_ASYNC);

} // namespace draw end
} // namespace mana end

/* レンダラー初期化 **

	shared_ptr<draw::renderer_2d> renderer = create_renderer_2d();

	d3d9_device_init deviceInit(wnd_handle(), wnd_width(),wnd_height());

	renderer_2d_init rendererInit;
	rendererInit.nRenderWidth_		= 640;
	rendererInit.nRenderHeight_		= 480;
	rendererInit.nBackgroundColor_	= D3DCOLOR_ARGB(255,0,128,0);
	
	renderer->init_driver(deviceInit, rendererInit);
	renderer->init_render();
*/