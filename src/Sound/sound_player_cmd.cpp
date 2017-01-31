#include "../mana_common.h"

#include "ds_sound_player.h"
#include "sound_player_cmd.h"

namespace mana{
namespace sound{
namespace cmd{

/////////////////////////////////
// リクエスト処理vistor
/////////////////////////////////
void sound_player_exec::operator()(info_load_cmd& cmd)const
{
#ifdef MANA_DEBUG
	player_.redefine(cmd.bReDefine_);
#endif

	bool r = player_.load_sound_info_file(cmd.sFilePath_, cmd.bFile_);
	if(cmd.callback_) cmd.callback_(r);

#ifdef MANA_DEBUG
	player_.redefine(false);
#endif
}

void sound_player_exec::operator()(info_add_cmd& cmd)const
{
#ifdef MANA_DEBUG
	player_.redefine(cmd.bReDefine_);
#endif

	bool r = player_.add_sound_info(cmd.sID_, cmd.sFilePath_, cmd.bStreaming_, cmd.fStreamLoopSec_, cmd.nStreamSec_);
	if(cmd.callback_)
	{
		uint32_t nID=0;
		if(r)
		{
			optional<uint32_t> id = player_.sound_id(cmd.sID_);
			if(id) nID=*id;
		}

		cmd.callback_(nID);
	}

#ifdef MANA_DEBUG
	player_.redefine(false);
#endif
}

void sound_player_exec::operator()(info_remove_cmd& cmd)const
{
	player_.remove_sound_info(cmd.nID_);
}

void sound_player_exec::operator()(event_cmd& cmd)const
{
	player_.set_event_handler(cmd.nID_, cmd.handler_);
}

void sound_player_exec::operator()(play_cmd_cmd& cmd)const
{
	switch(cmd.eMode_)
	{
	case MODE_PLAY:	
		player_.play(cmd.nID_, cmd.bLoop_All_);
	break;

	case MODE_STOP:
		if(cmd.bLoop_All_)
			player_.stop_all(); 
		else
			player_.stop(cmd.nID_);
	break;

	case MODE_PAUSE:
		if(cmd.bLoop_All_)
			player_.pause_all(); 
		else
			player_.pause(cmd.nID_);
	break;
	}
}

void sound_player_exec::operator()(volume_fade_cmd& cmd)const
{
	player_.fade_volume(cmd.nID_,cmd.vol_,cmd.nFrame_);
}

void sound_player_exec::operator()(param_cmd& cmd)const
{
	switch(cmd.eKind_)
	{
	case param_cmd::PARAM_DEFAULT_VOL:
		player_.set_default_volume(cmd.vol_);
	break;

	case param_cmd::PARAM_VOL:
		player_.set_volume(cmd.nID_, cmd.vol_);
	break;

	case param_cmd::PARAM_SPEED:
		player_.set_speed(cmd.nID_, cmd.f_);
	break;

	case param_cmd::PARAM_POS:
		player_.set_pos(cmd.nID_, cmd.f_);
	break;
	}
}

void sound_player_exec::operator()(play_mode_cmd& cmd)const
{
	if(cmd.callback_)
		cmd.callback_(player_.sound_play_mode(cmd.nID_));
}

} // naemspace cmd end
} // namespace sound end
} // namespace mana end
