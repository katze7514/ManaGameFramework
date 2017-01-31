#include "../mana_common.h"

#include "ds_sound_player.h"
#include "sound_player_sync.h"

namespace mana{
namespace sound{

bool sound_player_sync::init_player(uint32_t nIntervalMs, uint32_t nReserveRequestNum, uint32_t nAffinity)
{
	timer_.start();
	return true;
}

void sound_player_sync::fin()
{
	if(pPlayer_) pPlayer_->fin();
}

void sound_player_sync::request(const cmd::sound_player_cmd& cmd)
{
	boost::apply_visitor(cmd::sound_player_exec(*pPlayer_), const_cast<cmd::sound_player_cmd&>(cmd));
}

bool sound_player_sync::play(bool bWait)
{
	timer_.end();

	pPlayer_->update(static_cast<uint32_t>(timer_.elasped_mill()));

	timer_.start();
	return true;
}

} // namespace sound end
} // namespace mana end
