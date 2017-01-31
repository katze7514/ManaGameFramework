#include "../mana_common.h"

#include "../GameApp/game_window.h"
#include "../Draw/renderer_2d.h"
#include "../Graphic/draw_base.h"
#include "../Graphic/draw_context.h"
#include "../Audio/audio_player.h"

#include "actor_context.h"

namespace mana{

const shared_ptr<audio::audio_player>& actor_context::audio_player()
{
	return app()->audio_player();
}

const shared_ptr<audio::audio_player>& actor_context::audio_player()const
{
	return app()->audio_player();
}

shared_ptr<audio::audio_player> actor_context::audio_player_ptr()
{
	return app()->audio_player_ptr();
}


// Graphic
bool actor_context::add_draw_root_child(graphic::draw_base* pDrawBase, uint32_t nPriority)
{
	return draw_root()->add_child(pDrawBase, nPriority);
}

graphic::draw_base* actor_context::remove_draw_root_child(uint32_t nPriority, bool bDelete)
{
	return draw_root()->remove_child(nPriority, bDelete);
}

graphic::draw_base* actor_context::draw_root_child(uint32_t nPriority)
{
	return draw_root()->child(nPriority);
}

bool actor_context::start_draw_request()
{
	return draw_context()->renderer()->start_request();
}

void actor_context::end_draw_request()
{
	draw_context()->renderer()->end_request();
}

// BGM
bool actor_context::play_bgm(const string& sID, uint32_t nFade, audio::change_mode eChangeMode)
{
	return audio_player()->play_bgm(sID, nFade, eChangeMode);
}

void actor_context::stop_bgm(uint32_t nFadeOut)
{
	audio_player()->stop_bgm();
}

void actor_context::pause_bgm()
{
	audio_player()->pause_bgm();
}

void actor_context::set_bgm_volume(uint32_t nPer)
{
	sound::volume vol;
	vol.set_per(nPer);
	audio_player()->set_bgm_volume(vol);
}

void actor_context::fade_bgm_volume(uint32_t nPerStart, uint32_t nPerEnd, int32_t nFadeFrame)
{
	sound::volume start,end;
	start.set_per(nPerStart);
	end.set_per(nPerEnd);
	audio_player()->fade_bgm_volume(start, end, nFadeFrame);
}

void actor_context::fade_bgm_cur_volume(uint32_t nPerEnd, int32_t nFadeFrame)
{
	sound::volume end;
	end.set_per(nPerEnd);
	audio_player()->fade_bgm_volume(end, nFadeFrame);
}

// SE
void actor_context::play_se(const string& sID, bool bLoop, bool bForce)
{
	audio_player()->play_se(sID, bLoop, bForce);
}

void actor_context::stop_se(const string& sID)
{
	audio_player()->stop_se(sID);
}

void actor_context::pause_se(const string& sID)
{
	audio_player()->pause_se(sID);
}

void actor_context::set_se_volume(const string& sID, uint32_t nPer)
{
	sound::volume vol;
	vol.set_per(nPer);
	audio_player()->set_se_volume(sID, vol);
}

void actor_context::fade_se_volume(const string& sID, uint32_t nPerStart, uint32_t nPerEnd, int32_t nFadeFrame)
{
	sound::volume start,end;
	start.set_per(nPerStart);
	end.set_per(nPerEnd);
	audio_player()->fade_se_volume(sID, start, end, nFadeFrame);
}

void actor_context::fade_se_cur_volume(const string& sID, uint32_t nPerEnd, int32_t nFadeFrame)
{
	sound::volume end;
	end.set_per(nPerEnd);
	audio_player()->fade_se_volume(sID, end, nFadeFrame);
}

bool actor_context::is_playing_se(const string& sID)const
{
	return audio_player()->is_playing_se(sID);
}

void actor_context::stop_se_all()
{
	audio_player()->stop_se_all();
}

//////////////////////

// Draw
bool actor_context::add_draw_root_child_xtal(xtal::SmartPtr<graphic::draw_base> pDrawBase, uint32_t nPri)
{
	return add_draw_root_child(pDrawBase.get(), nPri);
}

xtal::SmartPtr<graphic::draw_base> actor_context::remove_draw_root_child_xtal(uint32_t nPri, bool bDel)
{
	return xtal::SmartPtr<graphic::draw_base>(remove_draw_root_child(nPri, bDel));
}

xtal::SmartPtr<graphic::draw_base> actor_context::draw_root_child_xtal(uint32_t nPri)
{
	return xtal::SmartPtr<graphic::draw_base>(draw_root_child(nPri));
}

xtal::SmartPtr<graphic::draw_base> actor_context::draw_root_xtal()
{
	return xtal::SmartPtr<graphic::draw_base>(draw_root()); 
}

// BGM
bool actor_context::play_bgm_xtal(xtal::StringPtr sID, uint32_t nFade, uint32_t nChangeMode)
{
	return play_bgm(string(sID->data(), sID->data_size()), nFade, static_cast<audio::change_mode>(nChangeMode));
}
	
// SE
void actor_context::play_se_xtal(xtal::StringPtr sID, bool bLoop, bool bForce)
{
	return play_se(string(sID->data(), sID->data_size()), bLoop, bForce);
}

void actor_context::stop_se_xtal(xtal::StringPtr sID)
{
	stop_se(string(sID->data(), sID->data_size()));
}

void actor_context::pause_se_xtal(xtal::StringPtr sID)
{
	pause_se(string(sID->data(), sID->data_size()));
}

void actor_context::set_se_volume_xtal(xtal::StringPtr sID, uint32_t nPer)
{
	set_se_volume(string(sID->data(), sID->data_size()), nPer);
}

void actor_context::fade_se_volume_xtal(xtal::StringPtr sID, uint32_t nPerStart, uint32_t nPerEnd, int32_t nFadeFrame)
{
	fade_se_volume(string(sID->data(), sID->data_size()), nPerStart, nPerEnd, nFadeFrame);
}

void actor_context::fade_se_cur_volume_xtal(xtal::StringPtr sID, uint32_t nPerEnd, int32_t nFadeFrame)
{
	fade_se_cur_volume(string(sID->data(), sID->data_size()), nPerEnd, nFadeFrame);
}

bool actor_context::is_playing_se_xtal(xtal::StringPtr sID)const
{
	return is_playing_se(string(sID->data(),sID->data_size()));
}

} // namespace mana end
