#include "../mana_common.h"

#include "ds_sound_player.h"
#include "sound_player_async.h"

namespace mana{
namespace sound{

bool sound_player_async::init_player(uint32_t nIntervalMs, uint32_t nReserveRequestNum, uint32_t nAffinity)
{
	nCurReqIndex_=0;
	cmdQueue_[0].init(nReserveRequestNum);
	cmdQueue_[1].init(nReserveRequestNum);

#ifdef MANA_SYNC_QUEUE_CMD_COUNT
	cmdQueue_[0].set_mes("sound_player_0");
	cmdQueue_[1].set_mes("sound_player_1");
#endif

	player_.nIntervalMs_ = nIntervalMs;
	if(!executer_.kick(nAffinity))
	{
		logger::warnln("[sound_player_async]初期化済み、もしくはスレッドが起動できませんでした。");
		return false;
	}

	logger::infoln("[sound_player_async]非同期版sound_player初期化しました");
	return true;
}

void sound_player_async::fin()
{
	player_.fin();
}

bool sound_player_async::start_request(bool bWait)
{
	cmd_queue& curQueue = cur_queue();

	while(true)
	{
		if(curQueue.lock(bWait))
		{
			return true;
		}
		else if(!bWait)
		{
			return false;
		}
	}
}

void sound_player_async::request(const cmd::sound_player_cmd& cmd)
{
	cur_queue().push_cmd(cmd);
}

void sound_player_async::end_request()
{
	cur_queue().unlock();
}

bool sound_player_async::play(bool bWait)
{
	while(true)
	{
		// レンダーの状態によって処理を分岐
		switch(player_.state())
		{
		case STOP:
			if(!cur_queue().queue().empty())
			{
				if(nCurReqIndex_==0)
					player_.set_state(RECEIVE_0);
				else
					player_.set_state(RECEIVE_1);

				swap_queue();
			}

			return true;
		break;

		// リクエスト処理中だった
		case RECEIVE_0:
		case RECEIVE_1:
			if(!bWait) return false;
		break;

		default:
			// 何かエラー
			return false;
		break;
		}
	}
}

//////////////////////////////////////////////
// サウンド処理
//////////////////////////////////////////////
void sound_player_async::start_receive(uint32_t nIndex)
{
	while(!cmdQueue_[nIndex].lock(true));
}

void sound_player_async::end_receive(uint32_t nIndex)
{
	cmdQueue_[nIndex].unlock();
}

/////////////////////
// サウンドスレッド実体
sound_player_async::sound_executer::sound_executer(sound_player_async& queue):nIntervalMs_(1000/30),queue_(queue)
{
	pPlayer_ = queue.pPlayer_;
	set_state(STOP); 
	bFin_.store(false, std::memory_order_release);
}

void sound_player_async::sound_executer::operator()()
{
	using std::this_thread::sleep_for;
	using namespace std::chrono;

	logger::initializer_thread log_init_thread("sound_player");

	timer_.start();

	while(!is_fin())
	{
		switch(state())
		{
		case RECEIVE_0:	play(0); break;
		case RECEIVE_1: play(1); break;
		default: break;
		}

		timer_.end();

		// 何も無くてもupdateは呼び続ける
		pPlayer_->update(static_cast<uint32_t>(timer_.elasped_mill()));

		if(state()!=STOP) set_state(STOP);
		auto elapsed = timer_.elasped_mill_duration();

		timer_.start();

		if(milliseconds(nIntervalMs_)>elapsed)
			sleep_for(milliseconds(nIntervalMs_) - elapsed);
	}
}

void sound_player_async::sound_executer::play(uint32_t nIndex)
{
	queue_.start_receive(nIndex);
	cmd_queue::queue_type& curQueue = queue_.cmdQueue_[nIndex].queue();

	cmd::sound_player_exec receiver(*pPlayer_);
	std::for_each(curQueue.begin(), curQueue.end(), boost::apply_visitor(receiver));

	curQueue.clear();
	queue_.end_receive(nIndex);
}

} // namespace sound end
} // namesppace mana end
