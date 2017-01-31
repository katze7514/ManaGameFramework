#include "mana_common.h"

#include "GameApp/game_window.h"

#include "File/file.h"
#include "Resource/resource_file.h"
#include "Resource/resource_manager.h"

#include "Input/di_keyboard.h"

#include "Draw/renderer_2d.h"
#include "Graphic//text_table.h"
#include "Graphic/draw_builder.h"
#include "Graphic/draw_context.h"
#include "Graphic/draw_base.h"

#include "Audio/audio_player.h"

#include "Script/xtal_manager.h"

#include "Actor/actor_context.h"
#include "Actor/actor.h"

#include "check_scene.h"

mana::actor* check_scene_factory::create_actor(uint32_t nID)
{
	switch(nID)
	{
	case 0: return new_ check_scene();
	case 1: return new_ check_xtal_scene();
	}
	return nullptr;
}

///////////////////////

#pragma region base_scene
bool check_scene::init_self(mana::actor_context& ctx)
{
	pDrawBuilder_ = make_shared<mana::graphic::draw_builder>();
	set_state(INIT);
	return true;
}

void check_scene::reset_self(mana::actor_context& ctx)
{
}

void check_scene::exec_self(mana::actor_context& ctx)
{
	check_context& sceneCtx = static_cast<check_context&>(ctx);

	switch(state())
	{
	case INIT:
		if(ctx.app()->resource_manager()->request(string_fw("TEXT_TABLE"))==mana::resource_manager::FAIL
		|| ctx.app()->resource_manager()->request(string_fw("AUDIO"))==mana::resource_manager::FAIL
		|| ctx.app()->resource_manager()->request(string_fw("DRAW_BASE"))==mana::resource_manager::FAIL)
			set_state(END);
		else
			set_state(LOAD_FILE);
	break;

	case LOAD_FILE:
	{
		auto t = ctx.app()->resource_manager()->resource(string_fw("TEXT_TABLE"));
		auto a = ctx.app()->resource_manager()->resource(string_fw("AUDIO"));
		

		if(t && a)
		{
			if((*t)->state()==mana::resource_file::FAIL
			|| (*a)->state()==mana::resource_file::FAIL)
			{
				logger::fatalln("[check_scene]各種定義ファイルの読み込みができませんでした。");
				set_state(END);
			}

			ctx.audio_player()->load_sound_file( reinterpret_cast<char*>((*a)->buf().get()), false);

			if(ctx.draw_context()->renderer()->start_request())
			{
				ctx.draw_context()->text_table()->load_file(reinterpret_cast<char*>((*t)->buf().get()), *ctx.draw_context(), false);
				ctx.draw_context()->renderer()->end_request();
				set_state(LOAD_SOUND);
			}
		}
		else
		{
			logger::traceln("[check_scene]ファイルロード中...");
		}
	}
	break;

	case LOAD_SOUND:
		if(ctx.audio_player()->is_fin_load()==mana::audio::audio_player::SUCCESS
		&& ctx.draw_context()->text_table()->is_fin_load(*ctx.draw_context())==mana::graphic::text_table::SUCCESS)
		{
			auto d = ctx.app()->resource_manager()->resource(string_fw("DRAW_BASE"));
			if(d)
			{
				if((*d)->state()==mana::resource_file::FAIL)
				{
					logger::fatalln("[check_scene]各種定義ファイルの読み込みができませんでした。");
					set_state(END);
				}
				else if(ctx.draw_context()->renderer()->start_request())
				{
					pDrawBuilder_->load_draw_info_file(reinterpret_cast<char*>((*d)->buf().get()), *ctx.draw_context(), false);
					ctx.draw_context()->renderer()->end_request();
					set_state(LOAD_DRAW);
				}
			}
		}
	break;

	case LOAD_DRAW:
		switch(pDrawBuilder_->is_fin_load(*ctx.draw_context()))
		{
		case mana::graphic::draw_builder::SUCCESS:
		{
			mana::graphic::draw_base* pDraw = pDrawBuilder_->create_draw_base(string_fw("mikoto"));
			if(pDraw)
			{
				ctx.draw_root()->add_child(pDraw,0);
				pDraw->init();
				set_state(EXEC);
			}
			else
			{
				set_state(END);
			}
		}
		break;

		case mana::graphic::draw_builder::FAIL:
			set_state(END);
		break;
		}
		
	break;

	case EXEC:
		if(sceneCtx.keyboard()->is_release(DIK_R))
		{
			ctx.draw_root()->child(0)->init();
		}

		if(sceneCtx.keyboard()->is_release(DIK_ESCAPE))
			set_state(END); 
	break;

	case END:
		static_cast<mana::actor_machine*>(parent())->return_actor();
	break;
	}
}
#pragma endregion

///////////////////////

#pragma region xtal_scene
bool check_xtal_scene::init_self(mana::actor_context& ctx)
{
	set_state(REQUEST);
	return true;
}

void check_xtal_scene::exec_self(mana::actor_context& ctx)
{
	//exec_simple(static_cast<check_context&>(ctx));
	exec_reload(static_cast<check_context&>(ctx));
}

void check_xtal_scene::exec_simple(check_context& ctx)
{
	switch(state())
	{
	case REQUEST:
		pXtalCode_ = ctx.app()->xtal_manager()->create_code(string_fw("CHECK"));
		if(pXtalCode_)
		{
			pXtalCode_->set_resource(string_fw("XTAL_CHECK"));
			if(pXtalCode_->compile())
				set_state(LOAD_WAIT);
			else
				set_state(END);
		}
		else
		{
			set_state(END);
		}
	break;


	case LOAD_WAIT:
		switch(pXtalCode_->state())
		{
		case mana::script::xtal_code::OK:
			set_state(EXEC);
		break;

		case mana::script::xtal_code::ERR:
			if(ctx.keyboard()->is_release(DIK_L))
				set_state(REQUEST);
		break;
		}
	break;

	case EXEC:
		pXtalCode_->code()->member(Xid(foo))->call();
		mana::script::check_xtal_result();

		if(ctx.keyboard()->is_release(DIK_ESCAPE))
			set_state(END);
	break;

	case END:
		static_cast<mana::actor_machine*>(parent())->return_actor();
	break;
	}
}

void check_xtal_scene::exec_reload(check_context& ctx)
{
	using mana::script::xtal_code;

	switch(state())
	{
	case REQUEST:
	{
		auto pCode = ctx.app()->xtal_manager()->create_code(string_fw("RELOAD_1"));
		pCode->set_resource(string_fw("XTAL_RELOAD_1"))
				.add_reload_id(string_fw("RELOAD_2"))
				.add_reload_id(string_fw("RELOAD_3"));
		bool r = pCode->compile();

		pCode = ctx.app()->xtal_manager()->create_code(string_fw("RELOAD_2"));
		pCode->set_resource(string_fw("XTAL_RELOAD_2"));
		r &= pCode->compile();

		pCode = ctx.app()->xtal_manager()->create_code(string_fw("RELOAD_3"));
		pCode->set_resource(string_fw("XTAL_RELOAD_3"));
		r &= pCode->compile();

		if(r)
			set_state(LOAD_WAIT);
		else
			set_state(END);
	}
	break;

	case LOAD_WAIT:
	{
		auto pCode1 = ctx.app()->xtal_manager()->code(string_fw("RELOAD_1"));
		auto pCode2 = ctx.app()->xtal_manager()->code(string_fw("RELOAD_2"));
		auto pCode3 = ctx.app()->xtal_manager()->code(string_fw("RELOAD_3"));

		if(pCode1->state()	== xtal_code::OK
		&& pCode2->state()	== xtal_code::OK
		&& pCode3->state()	== xtal_code::OK)
			set_state(EXEC);
	}
	break;

	case EXEC:
	{
		auto pCode1 = ctx.app()->xtal_manager()->code(string_fw("RELOAD_1"));
		auto pCode2 = ctx.app()->xtal_manager()->code(string_fw("RELOAD_2"));
		auto pCode3 = ctx.app()->xtal_manager()->code(string_fw("RELOAD_3"));

		pCode1->code()->member(Xid(foo))->call();
		pCode2->code()->member(Xid(foo))->call();
		pCode3->code()->member(Xid(foo))->call();

		if(ctx.keyboard()->is_release(DIK_ESCAPE))
		{
			set_state(END);
		}
		else if(ctx.keyboard()->is_release(DIK_L))
		{
			pCode1->reload();
			set_state(LOAD_WAIT);
		}
	}
	break;

	case END:
		static_cast<mana::actor_machine*>(parent())->return_actor();
	break;
	}
}
#pragma endregion