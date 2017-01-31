#include "../mana_common.h"

#include "d3d9_renderer_2d.h"

#include "renderer_2d_cmd.h"
#include "renderer_2d_async.h"

namespace mana{
namespace draw{

//////////////////////////////////
// 非同期レンダラー
//////////////////////////////////
bool renderer_2d_async::init_render(uint32_t nReserveRequestNum, uint32_t nAffinity)
{
	nCurReqIndex_=0;

	cmdQueue_[0].init(nReserveRequestNum);
	cmdQueue_[1].init(nReserveRequestNum);

#ifdef MANA_SYNC_QUEUE_CMD_COUNT
	cmdQueue_[0].set_mes("render_2d_0");
	cmdQueue_[1].set_mes("render_2d_1");
#endif

	// スレッド起動
	if(!executer_.kick(nAffinity))
	{
		logger::warnln("[renderer_2d_async]初期化済み、もしくはスレッドが起動できませんでした。");
		return false;
	}

	logger::infoln("[render_2d_async]非同期版renderer_2d初期化しました");
	return true;
}

void renderer_2d_async::fin()
{
	renderer_.fin();
}

bool renderer_2d_async::start_request(bool bWait)
{
	// デバイスロスト中はリクエストを受け付けない
	if(is_device_lost()) return false;

	cmd_queue& curQueue = cur_queue();

	while(true)
	{
		if(curQueue.lock(bWait))
		{
			if(!curQueue.queue().empty() && renderer_.state()!=STOP)
			{// リクエスト処理前なのにロック取れてしまったら
			 // ロックを開放して、処理が終わるまで待つ
				curQueue.unlock();
			}
			else
			{
				return true;
			}
		}
		else if(!bWait)
		{
			return false;
		}
	}
}

void renderer_2d_async::request(const cmd::render_2d_cmd& cmd)
{
	cur_queue().push_cmd(cmd);
}

void renderer_2d_async::end_request()
{
	cur_queue().unlock();
}

render_result renderer_2d_async::render(bool bWait)
{
	while(true)
	{
		// レンダーの状態によって処理を分岐
		switch(renderer_.state())
		{
		case STOP:
			if(!cur_queue().queue().empty())
			{
				if(nCurReqIndex_==0)
					renderer_.set_state(RECEIVE_0);
				else
					renderer_.set_state(RECEIVE_1);

				swap_queue();
			}

			return RENDER_SUCCESS;
		break;

		// リクエスト処理中だった
		case RECEIVE_0:
		case RECEIVE_1:
			if(!bWait) return RENDER_PROC;
		break;

		// デバイスロスト対応はメッセージ処理と
		// 同一スレッドでやらないといけない
		case DEVICE_LOST:
			switch(pRenderer_->device_lost())
			{
			case d3d9_renderer_2d::DL_SUCCESS:
				renderer_.set_state(STOP);
				if(resetHandler_){ resetHandler_(true); resetHandler_.clear(); }
				return RENDER_SUCCESS;
			break;

			case d3d9_renderer_2d::DL_FATAL:
				renderer_.set_state(FATAL);
				if(resetHandler_){ resetHandler_(false); resetHandler_.clear(); }
				return RENDER_FATAL;
			break;

			default:
				return RENDER_DEVICE_LOST;
			break;
			}
		break;

		default:
			// デバイスロストとか何かエラー
			return RENDER_FATAL;
		break;
		}
	}
}

//////////////////////////////////////////////
// レンダー処理
//////////////////////////////////////////////
void renderer_2d_async::start_receive(uint32_t nIndex)
{
	while(!cmdQueue_[nIndex].lock(true));
}

void renderer_2d_async::end_receive(uint32_t nIndex)
{
	cmdQueue_[nIndex].unlock();
}

/////////////////////
// レンダースレッド実体
renderer_2d_async::render_executer::render_executer(renderer_2d_async& queue):queue_(queue)
{
	pRenderer_ = queue.pRenderer_;
	set_state(STOP);
	bFin_.store(false, std::memory_order_release);
}

void renderer_2d_async::render_executer::operator()()
{
	logger::initializer_thread log_init_thread("renderer_2d");

	while(!is_fin())
	{
		//logger::traceln(state());
		switch(state())
		{
		case RECEIVE_0:	render(0); break;
		case RECEIVE_1:	render(1); break;
		default: break;
		}
	}
}

// レンダー処理
void renderer_2d_async::render_executer::render(uint32_t nIndex)
{
	// レンダラーにリクエストを積む
	queue_.start_receive(nIndex);
	cmd_queue::queue_type& curQueue = queue_.cmdQueue_[nIndex].queue();

	cmd::render_2d_cmd_exec receiver(*pRenderer_);
	std::for_each(curQueue.begin(), curQueue.end(), boost::apply_visitor(receiver));

	curQueue.clear();
	queue_.end_receive(nIndex);

	// 描画処理
	if(pRenderer_->begin_scene())
	{
		pRenderer_->render();
		pRenderer_->end_scene();
	}

	if(pRenderer_->is_device_lost())
		set_state(DEVICE_LOST);
	else
		set_state(STOP);
}

} // namespace draw end
} // namespace mana end
