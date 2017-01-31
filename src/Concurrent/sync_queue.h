#pragma once

#include <atomic>

#ifdef MANA_DEBUG
#define MANA_SYNC_QUEUE_CMD_COUNT
#endif

namespace mana{
namespace concurrent{

/*! @brief lock/unlockで同期するキュー
 *
 *  atomic_flag実装なので、lock/unlockの呼び出し順を間違うと
 *  スレッドセーフにならないので注意して使うこと
 *
 *　Queueに積まれた最大数をカウントする時は、
 *  MANA_SYNC_QUEUE_CMD_COUNTをdefineする
 */
template<class T>
class sync_queue
{
public:
	typedef T		  value_type;
	typedef vector<T> queue_type;

public:
	sync_queue();
	~sync_queue();

public:
	void init(uint32_t nReserveCmdNum=128);

	bool lock(bool bWait=true); // lockが取れたらtrueを返す
	void unlock();

	// 以下のメソッドはlock～unlockの間で呼ぶこと
	void		push_cmd(const value_type& cmd);
	void		clear();
	queue_type&	queue(){ return queue_; }

private:
	queue_type		 queue_;
	std::atomic_flag lock_;

#ifdef MANA_SYNC_QUEUE_CMD_COUNT
public:
	void set_mes(const string& sMes){ sMes_=sMes; }

private:
	uint32_t	nMaxCmd_;
	string		sMes_;
#endif

private:
	NON_COPIABLE(sync_queue);
};


////////////////////////
// 実装
template<class T>
inline sync_queue<T>::sync_queue()
{
#ifdef MANA_SYNC_QUEUE_CMD_COUNT
	nMaxCmd_=0;
#endif
}

template<class T>
inline sync_queue<T>::~sync_queue()
{
#ifdef MANA_SYNC_QUEUE_CMD_COUNT
	if(!sMes_.empty()) logger::infoln("[sync_queue]" + sMes_);
	logger::infoln("[sync_queue]積まれた最大リクエスト数 : " + to_str(nMaxCmd_));
#endif
}

template<class T>
inline void sync_queue<T>::init(uint32_t nReserveCmdNum)
{
	queue_.reserve(nReserveCmdNum);
	lock_.clear();
}

template<class T>
inline bool sync_queue<T>::lock(bool bWait)
{
	while(lock_.test_and_set(std::memory_order_acquire))
	{
		if(!bWait) return false;
	}

	return true;
}

template<class T>
inline void sync_queue<T>::unlock()
{
	lock_.clear(std::memory_order_release);
}

template<class T>
inline void sync_queue<T>::push_cmd(const value_type& cmd)
{
	queue_.emplace_back(cmd);

#ifdef MANA_SYNC_QUEUE_CMD_COUNT
	if(queue_.size()>nMaxCmd_) nMaxCmd_=queue_.size();
#endif
}

template<class T>
inline void sync_queue<T>::clear()
{
	queue_.clear();
}


} // namespace cuncurrent end
} // namespace mana end
