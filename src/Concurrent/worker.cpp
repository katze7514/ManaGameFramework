#include "../mana_common.h"

#include "worker.h"

namespace mana{
namespace concurrent{

worker::worker(uint32_t nWorkCapacity)
{
	set_fin(false);

	// Workキュー初期化
	if(nWorkCapacity==0) nWorkCapacity=1;
	workQueue_.set_capacity(nWorkCapacity);
}


worker::~worker()
{
	set_fin(true);
}

void worker::kick(uint32_t nWorkExecuterNum, uint32_t anAffinity[])
{
	// Workスレッド起動
	if(nWorkExecuterNum==0) nWorkExecuterNum=1;
	vecThread_.reserve(nWorkExecuterNum);

	for(uint32_t i=0; i<nWorkExecuterNum; ++i)
		vecThread_.push_back(new_ worker_thread(executer(*this)));

	int i=0;
	worker_thread_vec::iterator it;
	for(it = vecThread_.begin(); it!=vecThread_.end(); ++it)
		it->kick(anAffinity[i++]);
}

weak_ptr<future> worker::request(const function<void(void)>& job)
{
	if(is_fin()) return weak_ptr<future>();

	cond_mutex_lock lock(mutex_);

	while(workQueue_.full())
	{
		if(is_fin()) return weak_ptr<future>();
		full_.wait(lock);
	}

	if(is_fin()) return weak_ptr<future>();

	work* pWork = new_ work();
	if(pWork==nullptr) return weak_ptr<future>();

	pWork->exec_  = job;
	auto r = weak_ptr<future>(pWork->fin_);

	workQueue_.push_back(pWork);

	empty_.notify_all(); // キューに積まれたことを通知

	return r;
}

worker::work* worker::pop()
{
	if(is_fin()) return nullptr;

	cond_mutex_lock lock(mutex_);

	while(workQueue_.empty())
	{
		if(is_fin()) return nullptr;
		empty_.wait(lock);
	}

	if(is_fin()) return nullptr;

	work* r = workQueue_.front();
	workQueue_.pop_front();
	full_.notify_all(); // キューが空いたことを通知
	return r;
}

void worker::work_delete(work* pWork)
{
	delete pWork;
}

void worker::set_fin(bool bFin)
{ 
	bFin_.store(bFin, std::memory_order_release);

	if(is_fin())
	{// 終了フラグが立ったら待機スレッドを呼び起こす
		cond_mutex_lock lock(mutex_);
		empty_.notify_all();
		full_.notify_all();
	}
}

void worker::executer::operator()()
{
	while(!worker_.is_fin())
	{
		work* pWork = worker_.pop();
		if(pWork)
		{
			pWork->exec_();
			pWork->fin_->set_fin(true);
			worker_.work_delete(pWork);
		}

	}
}

} // namespace concurrent end
} // namespace mana end
