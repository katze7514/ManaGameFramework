#pragma once

#include "../App/window.h"

namespace mana{

namespace draw{
struct d3d9_device_init;
struct renderer_2d_init;
class  renderer_2d;
} // namespace draw end

namespace graphic{
class text_table;
} // namespace graphic end

namespace sound{
class sound_player;
} // namespace sound end

namespace audio{
class audio_player;
} // namespace audio_player end

namespace input{
class di_driver;
class di_joystick_factory;
class di_keyboard;
class mouse;
} // namespace input end

namespace script{
class xtal_manager;
} // namespace script end

namespace timer{
class fps_timer;
} // namespace timer end

class resource_manager;

/*! @brief ゲームに必要な処理をまとめたウインドウ
 *
 *  2D描画、サウンド、各種入力のオブジェクトを持っている
 *  このクラスを継承してゲームクラスを作る
 */
class game_window : public app::window
{
public:
	game_window();
	virtual ~game_window(){}

public:
	//! ゲームを描画するサイズを指定する。window::createより前に呼んで設定しておくこと
	void set_render_size(uint32_t nRenderWidth, uint32_t nRenderHeight){ nRenderWidth_=nRenderWidth; nRenderHeight_=nRenderHeight; }

	uint32_t render_width()const{ return nRenderWidth_; }
	uint32_t render_height()const{ return nRenderHeight_; }

	//! @brief ウインドウのスタイルを変更する。ウインドウが作成された後に使う
	/*! @param[in] bFullScreen trueならフルスクリーンへ変更。falseならウインドウモードへ変更 */
	void change_window_style(bool bFullScreen);

public:
	const shared_ptr<audio::audio_player>&			audio_player()const{ return pAudioPlayer_; }
	const shared_ptr<audio::audio_player>&			audio_player(){ return pAudioPlayer_; }
	shared_ptr<audio::audio_player>					audio_player_ptr(){ return pAudioPlayer_; }

	const shared_ptr<input::di_joystick_factory>&	joystic_factory(){ return pJoyStickFactory_; }
	const shared_ptr<input::di_keyboard>&			keyboard(){ return pKeyboard_; }
	shared_ptr<input::di_keyboard>&					keyboard_ptr(){ return pKeyboard_; }
	const shared_ptr<input::mouse>&					mouse(){ return pMouse_; }
	shared_ptr<input::mouse>						mouse_ptr(){ return pMouse_; }

	const shared_ptr<resource_manager>&				resource_manager(){ return pResourceManager_; }
	shared_ptr<class resource_manager>				resource_manager_ptr(){ return pResourceManager_; }

	const shared_ptr<script::xtal_manager>&			xtal_manager(){ return pXtalManager_; }
	shared_ptr<script::xtal_manager>				xtal_manager_ptr(){ return pXtalManager_; }

	const shared_ptr<graphic::text_table>&			text_table(){ return pTextTable_; }

	const shared_ptr<mana::timer::fps_timer>&		fps_timer(){ return pFps_; }

public:
	virtual void main() override;
	virtual LRESULT wnd_proc(HWND, UINT, WPARAM, LPARAM) override;

protected:
	virtual void on_pre_window_create(win_info& winfo) override;

	//! 各種ドライバーなどの初期化
	virtual bool game_init();
	//! 各種ドライバーなどの終了処理
	virtual void game_fin();
	//! ゲーム処理のメイン
	virtual void game_main(){}

	//! レンダラーの初期化パラメータを変更する場合、オーバーライドする
	virtual void on_game_init_renderer(draw::d3d9_device_init& device, draw::renderer_2d_init& renderer, uint32_t& nRequestReseve, uint32_t& nCoreNo){}
	//! サウンド/オーディオの初期化パラメータを変更する場合、オーバーライドする
	virtual void on_game_init_sound(uint32_t& nIntervalMs, uint32_t& nRequestReserve, uint32_t& nCoreNo, uint32_t& nGcFrame, uint32_t& nSeGcNum){}

protected:
	void calc_limit_mouse();

protected:
	//! ゲームを描画する領域サイズ
	uint32_t nRenderWidth_, nRenderHeight_;

protected:
	shared_ptr<draw::renderer_2d>			pRenderer_;

	shared_ptr<sound::sound_player>			pSoundPlayer_;
	shared_ptr<audio::audio_player>			pAudioPlayer_;

	shared_ptr<input::di_driver>			pInputDriver_;
	shared_ptr<input::di_joystick_factory>	pJoyStickFactory_;
	shared_ptr<input::di_keyboard>			pKeyboard_;
	shared_ptr<input::mouse>				pMouse_;

	shared_ptr<class resource_manager>		pResourceManager_;
	shared_ptr<script::xtal_manager>		pXtalManager_;

	shared_ptr<graphic::text_table>			pTextTable_;

	shared_ptr<timer::fps_timer>			pFps_;
};


} // namespace mana end
