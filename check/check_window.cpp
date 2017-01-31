#include "mana_common.h"

#include "Timer/fps_timer.h"

#include "Input/di_driver.h"
#include "Input/di_joystick.h"
#include "Input/di_keyboard.h"
#include "Input/mouse.h"

#include "Resource/resource_manager.h"

#include "Actor/actor_context.h"
#include "Actor/actor.h"
#include "Actor/actor_machine.h"

#include "Draw/renderer_2d.h"
#include "Graphic/draw_context.h"
#include "Graphic/draw_base.h"
#include "Graphic/draw_builder.h"

#include "Sound/sound_player.h"
#include "Audio/audio_player.h"

#include "Script/xtal_manager.h"

#include "check_scene.h"

#include "check_window.h"

using namespace mana;

void check_window::game_main()
{
	// リソース
	if(!pResourceManager_->load_resource_info("resource_manager.xml"))
	{
		logger::fatalln("[check_window]リソース定義ファイルが読み込めませんでした。");
		return;
	}

	// 描画
	shared_ptr<graphic::draw_context> pDrawCtx = make_shared<graphic::draw_context>();
	pDrawCtx->set_renderer(pRenderer_);
	pDrawCtx->set_audio_player(pAudioPlayer_);
	pDrawCtx->set_text_table(pTextTable_);

	graphic::draw_base* pDrawRoot = new_ graphic::draw_base();

	// Actor
	check_context* pActorCtx = new_ check_context();
	pActorCtx->set_draw_context(pDrawCtx);
	pActorCtx->set_draw_root(pDrawRoot);
	pActorCtx->set_app(this);
	pActorCtx->set_keyboard(pKeyboard_);

	actor_machine* pActorRoot = new_ actor_machine();
	pActorRoot->set_actor_factory(make_shared<check_scene_factory>());
	pActorRoot->call_actor(0);

#ifdef MANA_DEBUG
	pActorRoot->set_debug_name("Main_ActorMachine");
#endif

	// fps
	timer::fps_timer fps(30);

	// メインループ
	while(wnd_msg_peek())
	{
		// WM_CLOSEが呼ばれた
		if(is_close_mes()) break;

		pKeyboard_->input();

		// フルスクリーン切り替え
		if((pKeyboard_->is_press(DIK_LALT) || pKeyboard_->is_press(DIK_RALT))
			&& pKeyboard_->is_push(DIK_RETURN))
			change_window_style(!is_fullscreen());
		
		// Actor
		pActorRoot->exec(*pActorCtx);
		if(!pActorRoot->is_exec_actor()) break;

		// 描画
		if(pRenderer_->start_request())
		{
			pDrawCtx->set_total_z(draw::DEFAULT_MAX_Z);
			pDrawRoot->exec(*pDrawCtx);
			pRenderer_->end_request();
		}

		if(pRenderer_->render()==draw::RENDER_FATAL)
		{
			logger::fatalln("[game_window]renderer_2dに致命的なエラーが発生しました。");
			break;
		}

		// サウンド実行
		pAudioPlayer_->exec();

		// XtalGC
		pXtalManager_->exec();		

		// fps調整
		fps.wait_frame();
	}

	safe_delete(pActorRoot);
	safe_delete(pActorCtx);

	safe_delete(pDrawRoot);
	pDrawCtx.reset();
}