#include "../mana_common.h"

#include "../Sound/sound_player_cmd.h"
#include "../Sound/sound_player.h"

#include "audio_context.h"
#include "audio.h"

using namespace mana::sound::cmd;

namespace mana{
namespace audio{

audio::audio(uint32_t nReserve):base_type(nReserve),
								nSoundID_(0),eCmd_(sound::MODE_NONE),ePastCmd_(sound::MODE_NONE),
								bLoop_(false),
								nFadeFrame_(0),nFadeCounter_(0),
								eEvent_(sound::EV_END),
								paramFlag_(PARAM_NONE),fSpeed_(1.0f),fPos_(0.0f),
								worldFlag_(PARAM_NONE),
								eExecPlayMode_(sound::MODE_NONE)
{}

audio::~audio()
{
	clear_child();
}

void audio::init()
{
	sound::volume vol;
	vol.set_per(100);
	set_volume(vol);
	set_pos(0.0f);
	set_speed(1.0f);

	for(auto& it: children())
		it->init();
}

audio& audio::set_sound_id(uint32_t nSoundID, bool bSoundEvnet)
{
	nSoundID_ = nSoundID;

	if(bSoundEvnet)
		paramFlag_ |= PARAM_HANDLER;

	return *this;
}

////////////////////////
// audio処理

void audio::exec(audio_context& ctx)
{
	exec_parent();
	exec_fade();
	exec_param_flag(ctx);
	exec_cmd(ctx);

	// 子起動
	for(auto& it : children())
		it->exec(ctx);

	exec_reset();
}

void audio::exec_parent()
{
	if(parent())
	{
		worldFlag_  = parent()->world_param_flag();
		worldVol_	= parent()->worldVol_;
	}
	else
	{// 親がいないならリセット
		worldFlag_	= PARAM_NONE;
		worldVol_.set_per(100);
	}
}

void audio::exec_fade()
{// フェードなどボリューム合成
	if((ePastCmd_==sound::MODE_PLAY || ePastCmd_==sound::MODE_PLAY_LOOP) && is_fade())
	{// フェードあり
		if(nFadeCounter_<=nFadeFrame_)
		{
			int32_t cur = ((volFade_.per()-volFadeBase_.per())*nFadeCounter_)/nFadeFrame_ + volFadeBase_.per();
			vol_.set_per(cur);

			//logger::traceln("[audio]" + to_str_s(volFadeBase_.per()) + to_str_s(volFade_.per()) + to_str_s(cur) + to_str(sound_id()));

			if(nFadeCounter_ >= nFadeFrame_)
			{// フェード終了
				vol_.set_per(volFade_.per());
				paramFlag_ |= PARAM_VOL;

				nFadeFrame_ = 0;
			}
			else if(nFadeCounter_%2)
			{
				paramFlag_ |= PARAM_VOL;
			}
			++nFadeCounter_;
		}
	}
}

void audio::exec_param_flag(audio_context& ctx)
{// フラグ別処理
	if(bit_test<uint32_t>(paramFlag_, PARAM_STOP)
	|| bit_test<uint32_t>(worldFlag_, PARAM_STOP))
	{
		worldFlag_ |= PARAM_STOP;
		eCmd_ = sound::MODE_STOP;
	}
	
	if(bit_test<uint32_t>(paramFlag_, PARAM_PAUSE)
	|| bit_test<uint32_t>(worldFlag_, PARAM_PAUSE))
	{
		worldFlag_ |= PARAM_PAUSE;
		eCmd_ = sound::MODE_PAUSE;
	}

	if(bit_test<uint32_t>(paramFlag_, PARAM_PLAY)
	|| bit_test<uint32_t>(worldFlag_, PARAM_PLAY))
	{
		worldFlag_ |= PARAM_PLAY;
		eCmd_ = sound::MODE_PLAY;
	}

	// ボリュームの合成は必ずやる
	worldVol_.set_per((worldVol_.per()*vol_.per())/100);

	if(bit_test<uint32_t>(paramFlag_, PARAM_VOL)
	|| bit_test<uint32_t>(worldFlag_, PARAM_VOL))
	{
		worldFlag_ |= PARAM_VOL;

		if(nSoundID_>0)
		{
			param_cmd cmd;
			cmd.eKind_	= param_cmd::PARAM_VOL;
			cmd.nID_	= nSoundID_;
			cmd.vol_	= worldVol_;

			ctx.player()->request(cmd);
		}
	}

	if(bit_test<uint32_t>(paramFlag_, PARAM_SPEED))
	{
		if(nSoundID_>0)
		{
			param_cmd cmd;
			cmd.eKind_	= param_cmd::PARAM_SPEED;
			cmd.nID_	= nSoundID_;
			cmd.f_		= fSpeed_;

			ctx.player()->request(cmd);
		}
	}

	if(bit_test<uint32_t>(paramFlag_, PARAM_POS))
	{
		if(nSoundID_>0)
		{
			param_cmd cmd;
			cmd.eKind_	= param_cmd::PARAM_POS;
			cmd.nID_	= nSoundID_;
			cmd.f_		= fPos_;

			ctx.player()->request(cmd);
		}
	}

	if(bit_test<uint32_t>(paramFlag_, PARAM_HANDLER))
	{
		if(nSoundID_>0)
		{
			event_cmd cmd;
			cmd.nID_ = nSoundID_;
			cmd.handler_ = [this](sound::play_event e){ eEvent_.store(e, std::memory_order_release); };

			ctx.player()->request(cmd);
		}
	}
}

void audio::exec_cmd(audio_context& ctx)
{// コマンド別処理
	if(nSoundID_>0)
	{
		switch(eCmd_)
		{
		case sound::MODE_PLAY:
		{
			play_cmd_cmd cmd;
			cmd.eMode_		= eCmd_;
			cmd.nID_		= nSoundID_;
			cmd.bLoop_All_	= bLoop_;

			ctx.player()->request(cmd);

			bLRU_ = true;
		}
		break;

		case sound::MODE_STOP:
		case sound::MODE_PAUSE:
		{
			play_cmd_cmd cmd;
			cmd.eMode_	= eCmd_;
			cmd.nID_	= nSoundID_;
			cmd.bLoop_All_ = false;

			ctx.player()->request(cmd);
		}
		break;

		default:break;
		}

		// 状態取得コマンド
		if(eExecPlayMode_==sound::MODE_REQ)
		{
			play_mode_cmd cmd;
			cmd.nID_		= nSoundID_;
			cmd.callback_	= playHandler_;

			ctx.player()->request(cmd);

			eExecPlayMode_ = sound::MODE_PROC;
		}
	}
}

void audio::exec_reset()
{
	if(eCmd_!=sound::MODE_NONE) ePastCmd_ = eCmd_;
	eCmd_		= sound::MODE_NONE;
	paramFlag_	= PARAM_NONE;
}

////////////////////////
// audio操作

void audio::play(tribool bLoop, bool bChildren)
{
	if(!indeterminate(bLoop))
		bLoop_ = bLoop;
	
	eCmd_ = sound::MODE_PLAY;
	if(bChildren) paramFlag_ |= PARAM_PLAY;
}

void audio::stop(bool bChildren)
{
	eCmd_ = sound::MODE_STOP;
	if(bChildren) paramFlag_ |= PARAM_STOP;
}

void audio::pause(bool bChildren)
{
	eCmd_ = sound::MODE_PAUSE;
	if(bChildren) paramFlag_ |= PARAM_PAUSE;
}

void audio::fade_volume(const sound::volume& start, const sound::volume& end, int32_t nFadeFrame)
{
	vol_ = start;
	fade_volume(end, nFadeFrame);
}

void audio::fade_volume(const sound::volume& end, int32_t nFadeFrame)
{
	if(nFadeFrame<=0) nFadeFrame=1;

	volFadeBase_	= vol_;
	volFade_		= end;
	nFadeFrame_		= nFadeFrame;
	nFadeCounter_	= 0;
	paramFlag_	   |= PARAM_VOL;
}

void audio::set_volume(const sound::volume& vol)
{
	if(vol_.db()!=vol.db())
	{
		nFadeFrame_	= 0;
		vol_		= vol;
		paramFlag_ |= PARAM_VOL;
	}
}

void audio::set_speed(float fSpeed)
{
	if(fSpeed_!=fSpeed)
	{
		paramFlag_ |= PARAM_SPEED;
		fSpeed_ = fSpeed;
	}
}

void audio::set_pos(float fSec)
{
	paramFlag_ |= PARAM_POS;
	fPos_ = fSec;
}

sound::play_mode audio::sound_play_mode()
{
	if(eExecPlayMode_!=sound::MODE_PROC)
	{
		eExecPlayMode_	= sound::MODE_REQ;
		playHandler_	= [this](sound::play_mode m){ eExecPlayMode_.store(m, std::memory_order_release); };
	}

	return static_cast<sound::play_mode>(eExecPlayMode_.load(std::memory_order_acquire));
}

void audio::sound_play_mode(const function<void(sound::play_mode)>& handler)
{
	if(eExecPlayMode_.load(std::memory_order_acquire)!=sound::MODE_PROC)
	{
		eExecPlayMode_.store(sound::MODE_REQ, std::memory_order_release);
		playHandler_	= handler;
	}
}

} // namespace auido end
} // namespace mana end
