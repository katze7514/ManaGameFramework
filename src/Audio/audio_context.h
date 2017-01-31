#pragma once

#include "audio_util.h"

namespace mana{
namespace sound{
class sound_player;
} // namespace sound end

namespace audio{

class audio_context
{
public:
	const shared_ptr<sound::sound_player>&	player(){ return pPlayer_; }
	void									set_player(const shared_ptr<sound::sound_player>& pPlayer){ pPlayer_=pPlayer; }

protected:
	shared_ptr<sound::sound_player>	pPlayer_;
};

} // namespace audio end
} // namespace mana end
