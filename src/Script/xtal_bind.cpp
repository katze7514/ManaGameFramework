/*! ManaFrameworkのクラスや関数のbind */

#include "../Audio/audio_util.h"

#include "../Actor/actor_context.h"
#include "../Actor/actor.h"

#include "../Graphic/draw_base.h"

//////////////////

XTAL_PREBIND(mana::actor_context)
{
	Xdef_ctor0();
}

XTAL_BIND(mana::actor_context)
{
	Xpublic();

	Xdef_method(pause_flag);
	Xdef_method(set_pause_flag);
	Xdef_method(add_pause_flag);
	Xdef_method(remove_pause_flag);

	Xdef_method(is_valid);
	Xdef_method(valid);

	Xdef_method_alias(add_draw_root_child, &mana::actor_context::add_draw_root_child_xtal);
	Xdef_method_alias(remove_draw_root_child, &mana::actor_context::remove_draw_root_child_xtal);
	Xdef_method_alias(draw_root_child, &mana::actor_context::draw_root_child_xtal);

	Xdef_method_alias(draw_root, &mana::actor_context::draw_root_xtal);

	Xdef_method_alias(play_bgm, &mana::actor_context::play_bgm_xtal);
	Xparam(nFade,0); 
	Xparam(nChangeMode,mana::audio::CHANGE_CROSS);

	Xdef_method(stop_bgm);
	Xparam(nFadeOut,0);

	Xdef_method(pause_bgm);
	Xdef_method(set_bgm_volume);
	Xdef_method(fade_bgm_volume);
	Xdef_method(fade_bgm_cur_volume);

	Xdef_method_alias(play_se, &mana::actor_context::play_se_xtal);
	Xparam(bLoop,false);
	Xparam(bForce,false);

	Xdef_method_alias(stop_se, &mana::actor_context::stop_se_xtal);
	Xdef_method_alias(pause_se, &mana::actor_context::pause_se_xtal);
	Xdef_method_alias(set_se_volume, &mana::actor_context::set_se_volume);
	Xdef_method_alias(fade_se_volume, &mana::actor_context::fade_se_volume_xtal);
	Xdef_method_alias(fade_se_cur_volume, &mana::actor_context::fade_se_cur_volume_xtal);
	Xdef_method_alias(is_playing_se, &mana::actor_context::is_playing_se_xtal);
	Xdef_method(stop_se_all);
}

//////////////////

XTAL_PREBIND(mana::actor)
{
	Xdef_ctor0();
}

XTAL_BIND(mana::actor)
{
	Xpublic();

	Xdef_method(is_valid);
	Xdef_method(valid);
	Xdef_method(type);
	Xdef_method(set_type);
	Xdef_method(attr);
	Xdef_method(set_attr);
	Xdef_method(state);
	Xdef_method(set_state);
	Xdef_method(id);
	Xdef_method(set_id);
	Xdef_method(priority);
	Xdef_method(set_priority);

	Xdef_method_alias(parent,		&mana::actor::parent_xtal);
	Xdef_method_alias(add_child,	&mana::actor::add_child_xtal);
	Xdef_method_alias(remove_child, &mana::actor::remove_child_xtal);
	Xdef_method_alias(child,		&mana::actor::child_xtal);
	Xdef_method(count_children);
	Xdef_method(shrink_children);
}

//////////////////

XTAL_PREBIND(mana::graphic::draw_base)
{
	Xdef_ctor0();
}

XTAL_BIND(mana::graphic::draw_base)
{
	Xpublic();
	Xdef_method(kind);
	Xdef_method(is_visible);
	Xdef_method(visible);
	Xdef_method(is_pause);
	Xdef_method(pause);

	using mana::graphic::draw_base;

	Xdef_method(x);
	Xdef_method_alias(set_x,		&draw_base::set_x_xtal);
	Xdef_method(y);
	Xdef_method_alias(set_y,		&draw_base::set_y_xtal);
	Xdef_method(z);
	Xdef_method_alias(set_z,		&draw_base::set_z_xtal);

	Xdef_method_alias(set_scale,	&draw_base::set_scale_xtal);
	Xdef_method(width);
	Xdef_method_alias(set_width,	&draw_base::set_width_xtal);
	Xdef_method(height);
	Xdef_method_alias(set_height,	&draw_base::set_height_xtal);

	Xdef_method(angle);
	Xdef_method_alias(set_angle,	&draw_base::set_angle_xtal);

	Xdef_method_alias(set_pivot,	&draw_base::set_pivot_xtal);
	Xdef_method(pivot_x);
	Xdef_method_alias(set_pivot_x,	&draw_base::set_pivot_x_xtal);
	Xdef_method(pivot_y);
	Xdef_method_alias(set_pivot_y,	&draw_base::set_pivot_y_xtal);

	Xdef_method(color);
	Xdef_method_alias(set_color,	&draw_base::set_color_xtal);

	Xdef_method(alpha);
	Xdef_method_alias(set_alpha,	&draw_base::set_alpha_xtal);

	Xdef_method(id);
	Xdef_method_alias(set_id,		&draw_base::set_id_xtal);
	Xdef_method(priority);
	Xdef_method_alias(set_priority,	&draw_base::set_priority_xtal);

	Xdef_method_alias(parent,		&draw_base::parent_xtal);
	Xdef_method_alias(add_child,	&draw_base::add_child_xtal);
	Xdef_method_alias(remove_child, &draw_base::remove_child_xtal);
	Xdef_method_alias(child,		&draw_base::child_xtal);
}

