#pragma once

#include <atomic>

namespace mana{
namespace concurrent{

//! 与えられたmutexでスピンロックをし、ロックが取れたらfnを実行。デストラクタでunlockする
/*! @tparam mutex_type mutexの型。try_lockとunlockメソッドを持っていること */
template<class mutex_type>
class spin_lock_invoke
{
public:
	explicit spin_lock_invoke(mutex_type& mtx):mtx_(mtx){}
	~spin_lock_invoke(){ mtx_.unlock(); }

	//! @brief mutexでスピンし、ロックが取れたら実行する
	/*! @param[in] invoke ロックが取れた時に実行するファンクタ。bindなどで作る */
	template<typename T, typename invoke_type>
	T try_and_invoke(invoke_type invoke)
	{
		// ロックが取れるまでスピン
		while(!mtx_.try_lock());
		return invoke();
	}

	template<typename invoke_type>
	void try_and_invoke(invoke_type invoke)
	{
		// ロックが取れるまでスピン
		while(!mtx_.try_lock());
		invoke();
	}

private:
	mutex_type& mtx_;

private:
	NON_COPIABLE(spin_lock_invoke);
};

//! 与えられたatomic_flagでスピンロックを即座にする。デストラクタでclearする
class spin_flag_lock
{
public:
	explicit spin_flag_lock(std::atomic_flag& flag):flag_(flag){ while(flag_.test_and_set(std::memory_order_acquire)); }
	~spin_flag_lock(){ flag_.clear(std::memory_order_release); }

private:
	std::atomic_flag& flag_;

private:
	NON_COPIABLE(spin_flag_lock);
};

} // namespace concurrent end
} // namespace mana end
