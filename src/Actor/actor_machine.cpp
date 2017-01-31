#include "../mana_common.h"

#include "actor_machine.h"

namespace mana{

actor_machine::~actor_machine()
{
	pCurrentActor_=nullptr;
	clear_cache();
}

bool actor_machine::init(actor_context& ctx)
{
	nextCmd_ = make_tuple(CMD_NONE, 0);
	pCurrentActor_=nullptr;
	return true;
}

void actor_machine::reset(actor_context& ctx)
{
	init(ctx);
	clear_cache();
	clear_child();
}

void actor_machine::exec(actor_context& ctx)
{
	if(!is_exec(ctx)) return;

	switch(nextCmd_.get<0>())
	{
	case CMD_CALL:
		push_actor(nextCmd_.get<1>(), ctx);
		nextCmd_.get<0>() = CMD_NONE;
	break;

	case CMD_RETURN:
		if(children().size()>0)
		{
			actor* pActor = pCurrentActor_;
			cacheActor_.emplace(pActor->id(), pActor);

			pop_actor();
		}
		nextCmd_.get<0>() = CMD_NONE;
	break;

	case CMD_RETURN_DELETE:
		if(children().size()>0)
		{
			actor* pActor = pCurrentActor_;
			delete pActor;

			pop_actor();
		}
		nextCmd_.get<0>() = CMD_NONE;
	break;

	default: break;
	}

	if(is_exec_actor()) pCurrentActor_->exec(ctx);
}

/////////////////////////////////

void actor_machine::cache_create_actor(uint32_t nID, actor_context& ctx)
{
	// すでにキャッシュ上に存在してたら生成しない
	auto it = cacheActor_.find(nID);
	if(it!=cacheActor_.end()) return;

	for(auto it : children())
	{// すでにスタック上に存在してたら生成しない
		if(it->id()==nID) return;
	}

	actor* pActor = create_actor_factory(nID, ctx);
	if(pActor) cacheActor_.emplace(nID, pActor);
}

void actor_machine::cache_destory_actor(uint32_t nID)
{
	auto it = cacheActor_.find(nID);
	if(it==cacheActor_.end()) return;

	delete it->second;
	cacheActor_.erase(it);
}

void actor_machine::clear_cache()
{
	for(auto& it : cacheActor_)
		delete it.second;

	cacheActor_.clear();
}

//////////////////////////////////

actor* actor_machine::create_actor_factory(uint32_t nID, actor_context& ctx)
{
	if(pFct_)
	{
		actor* pActor = pFct_->create_actor(nID);

		if(pActor)
		{
			pActor->set_parent(this);
			pActor->set_id(nID);
			pActor->init(ctx);
		}

		return pActor;
	}

	return nullptr;
}

void actor_machine::push_actor(uint32_t nID, actor_context& ctx)
{
	actor* pActor=nullptr;

	auto it = cacheActor_.find(nID);
	if(it!=cacheActor_.end())
	{
		pActor = it->second;
		cacheActor_.erase(it);

		pActor->reset(ctx);
	}
	else
	{
		pActor = create_actor_factory(nID,ctx);
	}
	
	if(pActor)
	{
		pCurrentActor_ = pActor;
		children().emplace_back(pCurrentActor_);
	}

#ifdef MANA_DEBUG
	logger::debugln("[actor_machine][" + debug_name() + "] push_actor : " + to_str(nID));
#endif
}

void actor_machine::pop_actor()
{
	children().pop_back();

	if(children().size()>0)
		pCurrentActor_ = *(children().rbegin());
	else
		pCurrentActor_ = nullptr;

#ifdef MANA_DEBUG
	logger::debugln("[actor_machine][" + debug_name() + "] pop_actor");
#endif
}


} // namespace mana end
