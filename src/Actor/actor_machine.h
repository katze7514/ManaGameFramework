#pragma once

#include "actor.h"

namespace mana{

class actor_context;
class actor_factory;

/*! @brief アクターの切り替えを行うマシン
 *
 *  いわゆるシーンマネージャーみたいなもの
 *  シーンに相当するactorを生成するactor_factoryを設定して使う
 *
 *  常に一つのActorを実行し、call/returnが出来る
 *  actorの切り替えはexecのタイミングに行われる 
 *
 *  このクラスによって管理されるactorは、
 *    factoryから生成された時にinitが呼ばれ
 *    キャッシュから取得された時はresetが呼ばれる
 */
class actor_machine : public actor
{
protected:
	enum cmd{
		CMD_CALL,
		CMD_RETURN,
		CMD_RETURN_DELETE,
		CMD_NONE,
	};

	typedef tuple<cmd, uint32_t> cmd_tuple;

public:
	actor_machine():nextCmd_(CMD_NONE,0),pCurrentActor_(nullptr){}
	virtual ~actor_machine();

public:
	virtual bool init(actor_context& ctx)override;
	virtual void reset(actor_context& ctx)override;
	virtual void exec(actor_context& ctx)override;

public:
	void set_actor_factory(const shared_ptr<actor_factory>& pFct){ pFct_ = pFct; }

public:
	//! 現在実行中のActorをスタックに積んで新しいActorを実行する
	void call_actor(uint32_t nID){ nextCmd_.get<0>()=CMD_CALL; nextCmd_.get<1>()=nID; }

	//! @brief 現在実行中のActorをスタックからpopし、一つ前のActorを実行する
	/*! @param[in] bCache falseだと、popしたActorはキャッシュされず、即座にdeleteされる */
	void return_actor(bool bCache=true){ nextCmd_.get<0>()=(bCache ? CMD_RETURN : CMD_RETURN_DELETE); nextCmd_.get<1>()=0; }

	//! 実行するActorが存在している
	bool is_exec_actor()const{ return pCurrentActor_!=nullptr; }

	//! キャッシュとしてをactorを作る
	void cache_create_actor(uint32_t nID, actor_context& ctx);
	void cache_destory_actor(uint32_t nID);
	void clear_cache();

public:
	// actor_machineでは、通常の子Actor操作は禁止なので何も動作しないようにしておく
	bool			add_child(actor* pChild, uint32_t nID)final{ return false; }
	actor*			remove_child(uint32_t nID, bool bDelete)final{ return nullptr; }
	const actor*	child(uint32_t nID)const final{ return nullptr; }
	void			sort_child(bool bChild=false)final{}

protected:
	void	push_actor(uint32_t nID, actor_context& ctx);
	void	pop_actor();

	actor*	create_actor_factory(uint32_t nID, actor_context& ctx);

protected:
	cmd_tuple					nextCmd_;

	actor*						pCurrentActor_;

	flat_map<uint32_t, actor*>	cacheActor_;
	shared_ptr<actor_factory>	pFct_;
};

//! @brief actor_machineに設定するファクトリーのインターフェイスクラス
class actor_factory
{
public:
	virtual ~actor_factory(){}

public:
	virtual actor*	create_actor(uint32_t nID)=0;
};


} // namespace mana end
