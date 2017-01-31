#include "../mana_common.h"

#include "worker_lockfree.h"

namespace mana{
namespace concurrent{

worker_lockfree::worker_lockfree(uint32_t nWorkCapacity):workQueue_(nWorkCapacity)
{
	set_fin(false);
}

worker_lockfree::~worker_lockfree()
{
	set_fin(true);
}

void worker_lockfree::kick(uint32_t nWorkExecuterNum, uint32_t anAffinity[])
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

weak_ptr<future_lockfree> worker_lockfree::request(const function<void(void)>& job)
{
	if(is_fin()) return weak_ptr<future_lockfree>();

	work* pWork = new_ work();
	if(pWork==nullptr)	return weak_ptr<future_lockfree>();
	
	pWork->exec_ = job;
	auto r = weak_ptr<future_lockfree>(pWork->fin_);

	int counter=0;
	while(!workQueue_.bounded_push(pWork))
	{ 
		++counter; 
		if(counter>100)
		{
			if(is_fin())
			{
				delete pWork;
				return weak_ptr<future_lockfree>(); 
			}
			else
			{
				counter=0;
			}
		}
	}

	return r;
}


worker_lockfree::work* worker_lockfree::pop()
{
	if(is_fin()) return nullptr;

	int counter=0;

	work* pWork = nullptr;
	while(!workQueue_.pop(pWork))
	{ 
		++counter; 
		if(counter>1000)
		{
			if(is_fin())
				return nullptr; 
			else
				counter=0;
		}
	}
	
	return pWork;
}

void worker_lockfree::work_delete(work* pWork)
{
	delete pWork;
}

void worker_lockfree::set_fin(bool bFin)
{ 
	bFin_.store(bFin, std::memory_order_release);
}

void worker_lockfree::executer::operator()()
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
