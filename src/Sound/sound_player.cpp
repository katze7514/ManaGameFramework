#include "../mana_common.h"

#include "ds_sound_player.h"

#include "sound_player_async.h"
#include "sound_player_sync.h"

#include "sound_player.h"

namespace mana{
namespace sound{

sound_player::sound_player():pPlayer_(new_ ds_sound_player())
{
#ifdef MANA_DEBUG
	bReDefine_=false;
#endif
}

sound_player::~sound_player()
{
	safe_delete(pPlayer_);
}

bool sound_player::init_driver(HWND hWnd, uint32_t nReserveSoundNum, bool bHighQuarity, uint32_t nMaxSoundNum)
{
	if(!pPlayer_->init(hWnd,nReserveSoundNum,bHighQuarity)) return false;
	pPlayer_->set_max_sound_buffer_num(nMaxSoundNum);
	return true;
}

optional<uint32_t> sound_player::sound_id(const string& sID)
{
	return pPlayer_->sound_id(sID);
}

void sound_player::request_info_load(const string& sFilePath, const function<void(bool)>& callback, bool bFile)
{
	cmd::info_load_cmd cmd;
	cmd.sFilePath_	= sFilePath;
	cmd.bFile_		= bFile;
	cmd.callback_	= callback;

#ifdef MANA_DEBUG
	cmd.bReDefine_=bReDefine_;
#endif

	request(cmd);
}

void sound_player::request_info_add(const string& sID, const string& sFilePath, bool bStreaming, const function<void (uint32_t)>& callback, float fStreamLoopSec, uint32_t nStreamSec)
{
	cmd::info_add_cmd cmd;
	cmd.sID_			= sID;
	cmd.sFilePath_		= sFilePath;
	cmd.bStreaming_		= bStreaming;
	cmd.fStreamLoopSec_	= fStreamLoopSec;
	cmd.nStreamSec_		= nStreamSec;
	cmd.callback_		= callback;

#ifdef MANA_DEBUG
	cmd.bReDefine_=bReDefine_;
#endif

	request(cmd);
}

void sound_player::request_info_remove(uint32_t nID)
{
	cmd::info_remove_cmd cmd;
	cmd.nID_ = nID;

	request(cmd);
}

void sound_player::request_play_mode(uint32_t nID, const function<void(play_mode)>& callback)
{
	cmd::play_mode_cmd cmd;
	cmd.nID_ = nID;
	cmd.callback_ = callback;

	request(cmd);
}


sound_player_sptr create_sound_player(sound_player_kind eKind)
{
	switch(eKind)
	{
	default:			return std::move(make_shared<sound_player_async>());
	case PLAYER_SYNC:	return std::move(make_shared<sound_player_sync>());
	}
}


} // namespace sound end
} // namespace mana end
