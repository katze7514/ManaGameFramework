#include "../mana_common.h"

#include "../Timer/fps_timer.h"
#include "../Draw/renderer_2d.h"
#include "../Sound/sound_player.h"
#include "../Audio/audio_player.h"
#include "../Input/di_driver.h"
#include "../Input/di_joystick.h"
#include "../Input/di_keyboard.h"
#include "../Input/mouse.h"

#include "../Resource/resource_manager.h"
#include "../Script/xtal_manager.h"
#include "../Graphic/text_table.h"

#include "game_window.h"

namespace mana{

game_window::game_window():nRenderWidth_(640),nRenderHeight_(480){}

void game_window::main()
{
	if(game_init())
	{
		game_main();
	}
	else
	{
		logger::fatalln("[game_window]ゲーム処理の初期化に失敗しました。");
	}

	game_fin();
}

LRESULT game_window::wnd_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_SYSCOMMAND:
		switch(wParam & 0xFFF0)
		{
		case SC_MONITORPOWER: // モニターOFF阻止
			return 0;
		break;

		case SC_SCREENSAVE: // スクリーンセイバー起動阻止
			return 1;
		break;
		}
	break;
	}

	if(pMouse_) pMouse_->wnd_proc(hWnd, uMsg, wParam, lParam);

	return app::window::wnd_proc(hWnd, uMsg, wParam, lParam);
}

void game_window::change_window_style(bool bFullScreen)
{
	// 同じ状態だったら何もしない
	if(is_fullscreen()==bFullScreen) return;

	if(bFullScreen)
	{// フルスクリーンへ変更
		nStyle_	 = STYLE_FULLSCREEN;
		nWidth_  = ::GetSystemMetrics(SM_CXSCREEN);
		nHeight_ = ::GetSystemMetrics(SM_CYSCREEN);
	}
	else
	{// ウインドウモードへ変更
		nStyle_  = STYLE_WINDOW;
		nWidth_  = render_width();
		nHeight_ = render_height();
	}

	::SetWindowLong(wnd_handle(), GWL_STYLE, nStyle_);

	// Reset後に呼ばれるハンドラ
	draw::renderer_2d::reset_handler handler = [this,bFullScreen](bool result)
	{// ウインドウサイズと位置を戻す
		if(result)
		{
			int32_t nRealWidth,nRealHeight;
			calc_real_window_size(nRealWidth, nRealHeight);

			int32_t nPosX = (::GetSystemMetrics(SM_CXSCREEN) - nRealWidth) / 2;
			int32_t nPosY = (::GetSystemMetrics(SM_CYSCREEN) - nRealHeight)/ 2;

			::SetWindowPos(wnd_handle(), NULL, nPosX, nPosY, nRealWidth, nRealHeight, SWP_NOZORDER | SWP_FRAMECHANGED);

			// SHOWWINDOWを使ってウインドウを再描画させる必要がある
			::ShowWindow(wnd_handle(), SW_SHOW);

			bFullScreen_ = bFullScreen;

			// カーソル制限仕込み直し
			calc_limit_mouse();
		}
	};

	// レンダラーに通知
	pRenderer_->device_reset(bFullScreen, nWidth_, nHeight_, handler);
}

void game_window::on_pre_window_create(win_info& winfo)
{
	// ゲーム描画領域の方が大きかったら、ウインドウサイズもそっちに合わせる
	if(render_width()>wnd_width())
		nWidth_ = render_width();

	if(render_height()>wnd_height())
		nHeight_ = render_height();

	window::on_pre_window_create(winfo);
}

////////////////////////////////////
// ゲーム処理実体

bool game_window::game_init()
{
	// レンダー初期化
	pRenderer_ = draw::create_renderer_2d();

	draw::d3d9_device_init deviceInit(wnd_handle(), wnd_width(), wnd_height());
	draw::renderer_2d_init rendererInit;
	rendererInit.nBackgroundColor_	= D3DCOLOR_ARGB(255,0,128,0);

	uint32_t nRequestReseve	= 1024;
	uint32_t nCoreNo		= 1;
	on_game_init_renderer(deviceInit, rendererInit, nRequestReseve, nCoreNo);		
	
	// 変更不可の値を強制上書きする
	deviceInit.bWindowMode_			= is_fullscreen() ? FALSE : TRUE;
	deviceInit.hWnd_				= wnd_handle();
	deviceInit.nWidth_				= wnd_width();
	deviceInit.nHeight_				= wnd_height();
	rendererInit.nRenderWidth_		= render_width();
	rendererInit.nRenderHeight_		= render_height();

	if(!pRenderer_->init_driver(deviceInit, rendererInit)
	|| !pRenderer_->init_render(nRequestReseve, nCoreNo))
		return false;

	// サウンド初期化
	pSoundPlayer_ = sound::create_sound_player();

	uint32_t nIntervalMs= 1000/60; // 60fps
	nRequestReseve		= 256;
	nCoreNo				= 2;
	uint32_t nGcFrame	= 601;
	uint32_t nSeGcNum	= 20;
	on_game_init_sound(nIntervalMs, nRequestReseve, nCoreNo, nGcFrame, nSeGcNum);

	if(!pSoundPlayer_->init_driver(wnd_handle())
	|| !pSoundPlayer_->init_player(nIntervalMs, nRequestReseve, nCoreNo))
		return false;

	pAudioPlayer_ = make_shared<audio::audio_player>();
	pAudioPlayer_->init(pSoundPlayer_, nGcFrame, nSeGcNum);

	// 入力初期化
	using namespace mana;
	pInputDriver_ = make_shared<input::di_driver>();
	if(!pInputDriver_->init()) return false;

	pJoyStickFactory_ = make_shared<input::di_joystick_factory>();
	if(!pJoyStickFactory_->init(wnd_handle(), pInputDriver_)) return false;
	
	pKeyboard_  = make_shared<input::di_keyboard>();
	if(!pKeyboard_->init(*pInputDriver_, wnd_handle())) return false;

	pMouse_ = make_shared<input::mouse>();
	if(!pMouse_->init(wnd_handle())) return false;

	// カーソル制限の仕込み
	calc_limit_mouse();

	// リソースマネージャー初期化
	pResourceManager_	= make_shared<mana::resource_manager>();
	pResourceManager_->init();

	// XtalManager初期化
	pXtalManager_		= make_shared<mana::script::xtal_manager>();
	pXtalManager_->init(pResourceManager_);

	// TextTable初期化
	pTextTable_	= make_shared<mana::graphic::text_table>();

	// fpsタイマー初期化
	pFps_		= make_shared<mana::timer::fps_timer>();

	return true;
}

#define safe_fin(x) {if(x) x->fin();}

void game_window::game_fin()
{
	safe_fin(pRenderer_);
	safe_fin(pSoundPlayer_);
	safe_fin(pJoyStickFactory_);
	safe_fin(pKeyboard_);
	safe_fin(pInputDriver_);
	safe_fin(pXtalManager_);
}

void game_window::calc_limit_mouse()
{
	RECT rect;
	rect.left	= static_cast<int32_t>((wnd_width()-render_width())/2.0f);
	rect.top	= static_cast<int32_t>((wnd_height()-render_height())/2.0f);
	rect.right	= rect.left + render_width();
	rect.bottom	= rect.top  + render_height();
	pMouse_->set_cursor_limit_rect(rect);
	pMouse_->cursor_limit(is_fullscreen());
}

} // namespace mana
