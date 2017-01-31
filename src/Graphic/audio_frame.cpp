#include "../mana_common.h"
#include "draw_context.h"

#include "../Audio/audio_player.h"

#include "audio_frame.h"

namespace mana{

using namespace audio;

namespace graphic{

void audio_frame::init_self()
{
	bExec_ = false;
}

void audio_frame::exec_self(draw_context& ctx)
{
	if(bExec_ || info_.nSoundID_==0 || is_pause_ctx(ctx) || is_visible_ctx(ctx)) return;
	
	auto ap = ctx.audio_player();

	// ボリューム系
	if(bit_test<uint32_t>(info_.nParamFlag_, param_flag::PARAM_VOL))
	{
		if(info_.nFadeFrame_>0)
		{// FadeFraemが正数だとstart～end
			if(info_.eAudioKind_==audio_kind::AUDIO_BGM)
				ap->fade_bgm_volume(info_.vol_, info_.volFadeEnd_, info_.nFadeFrame_);
			else
				ap->fade_se_volume(info_.nSoundID_, info_.vol_, info_.volFadeEnd_, info_.nFadeFrame_);
		}
		else if(info_.nFadeFrame_<0)
		{// FadeFraemが負数だとendのみ
			if(info_.eAudioKind_==audio_kind::AUDIO_BGM)
				ap->fade_bgm_volume(info_.vol_, -info_.nFadeFrame_);
			else
				ap->fade_se_volume(info_.nSoundID_, info_.vol_, -info_.nFadeFrame_);
		}
		else
		{// FadeFraemが0だとset_volのみ
			if(info_.eAudioKind_==audio_kind::AUDIO_BGM)
				ap->set_bgm_volume(info_.vol_);
			else
				ap->set_se_volume(info_.nSoundID_, info_.vol_);
		}
	}

	// 実行
	if(bit_test<uint32_t>(info_.nParamFlag_, param_flag::PARAM_PLAY))
	{
		if(info_.eAudioKind_==audio_kind::AUDIO_BGM)
			ap->play_bgm(info_.nSoundID_, info_.nFadeFrame_, info_.eChangeMode_);
		else
			ap->play_se(info_.nSoundID_, info_.bLoop_, info_.bForce_);
	}
	else if(bit_test<uint32_t>(info_.nParamFlag_, param_flag::PARAM_STOP))
	{
		if(info_.eAudioKind_==audio_kind::AUDIO_BGM)
			ap->stop_bgm(info_.nFadeFrame_);
		else
			ap->stop_se(info_.nSoundID_);
	}

	bExec_ = true;
}

//////////////////////

bool audio_frame::add_child(draw_base* pChild, uint32_t nID)
{
	// audio_frame以外は子供にできない
	if(!pChild || pChild->kind()!=DRAW_AUDIO_FRAME) return false;

	return draw_base::add_child(pChild, nID);
}

//////////////////////

void audio_frame::play_bgm(int32_t nFadeFrame, change_mode eChangeMode)
{
	info_.eAudioKind_	= audio_kind::AUDIO_BGM;
	info_.nParamFlag_  |= param_flag::PARAM_PLAY;

	info_.nFadeFrame_	= nFadeFrame;
	info_.eChangeMode_	= eChangeMode;
}

void audio_frame::play_se(bool bLoop, bool bForce)
{
	info_.eAudioKind_	= audio_kind::AUDIO_SE;
	info_.nParamFlag_  |= param_flag::PARAM_PLAY;

	info_.bLoop_	= bLoop;
	info_.bForce_	= bForce;
}

void audio_frame::stop(int32_t nFadeFrame)
{
	info_.nParamFlag_  |= param_flag::PARAM_STOP;
	info_.nFadeFrame_	= nFadeFrame;
}

void audio_frame::set_volume(const sound::volume& vol)
{
	info_.nParamFlag_  |= param_flag::PARAM_VOL;
	info_.vol_			= vol;
	info_.nFadeFrame_	= 0;
}

void audio_frame::fade_volume(const sound::volume& start, const sound::volume& end, int32_t nFadeFrame)
{
	info_.nParamFlag_  |= param_flag::PARAM_VOL;
	info_.vol_			= start;
	info_.volFadeEnd_	= end;
	info_.nFadeFrame_	= nFadeFrame;
}

void audio_frame::fade_volume(const sound::volume& end, int32_t nFadeFrame)
{
	info_.nParamFlag_  |= param_flag::PARAM_VOL;
	info_.volFadeEnd_	= end;
	info_.nFadeFrame_	= -nFadeFrame;
}


} // namespace graphic end
} // namespace mana end
