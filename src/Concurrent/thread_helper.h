#pragma once

#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace mana{
namespace concurrent{

//! 条件変数向けに使うunique_lockのtypedef
typedef std::unique_lock<std::mutex> cond_mutex_lock;

/*! 
 * @brief スレッドヘルパークラス
 *
 *  ManaFramework内部向けのスレッドクラス
 *  標準のthreadクラスでは出来ない設定を施すために使う。
 *  実行先の論理CPUなどが設定できる。
 *
 *  @tparm Fn スレッドで実行される引数無しファンクター。実体か参照かで型を指定する
 *            thread_helper<Functor> or thread_helper<Functor&>
*/
template<class Fn>
class thread_helper
{
public:
	enum state : uint32_t
	{ 
		NONE,
		ACTIVE,
		FIN,
	};

public:
	//! 実行するファンクターを取る
	thread_helper(Fn proc):proc_(proc),hThread_(INVALID_HANDLE_VALUE),nThreadID_(0){ set_state(state::NONE); /*logger::debugln("thread const " + to_str(state()));*/ }
	~thread_helper(){ join(); }
	
public:
	//! @brief スレッドを起動してファンクターを実行する
	/*! @param[in] affinity スレッドを割り当てる論理CPU番号(0～8とか）
	 *                      0は指定無し扱い */
	bool kick(uint32_t nAffinity=0);

	//! このスレッドが終了するまで待機する
	void join();

	enum state state()const{ return static_cast<enum state>(eState_.load(std::memory_order_acquire)); }

private:
	void set_state(enum state st){ eState_.store(static_cast<uint32_t>(st), std::memory_order_release); /*logger::debugln("thread state " + to_str(state()));*/ }

	void end_thread(){ set_state(state::FIN); }

private:
	Fn proc_; // スレッドで実行するファンクタ

	std::atomic_uint32_t eState_;

	HANDLE	 hThread_;
	uint32_t nThreadID_;

	//! スレッド化する関数
	static uint32_t __stdcall threadProc(void* pArg);
};

///////////////////

template<class Fn>
inline bool thread_helper<Fn>::kick(uint32_t nAffinity=0)
{
	if(state()!=NONE) return false;
	//if(nThreadID_!=0) return false;

	// スレッドを一時停止状態で作成
	hThread_ = reinterpret_cast<HANDLE>(::_beginthreadex(0, 0, &thread_helper<Fn>::threadProc, this, CREATE_SUSPENDED, &nThreadID_));
	if(hThread_==INVALID_HANDLE_VALUE) return false;

	// 動作する論理CPUを指定する
	if(nAffinity>0)
	{
		// 指定された論理CPUが論理CPU数を越えないようにする
		uint32_t nAff = nAffinity - 1;
		nAff %= std::thread::hardware_concurrency();

		if(::SetThreadAffinityMask(hThread_, 1<<nAff)==0)
		{// 設定エラー
			logger::warnln("["+ to_str(nThreadID_) +"]:論理CPU設定(" + to_str(nAffinity) + ")ができませんでした。");
		}
	}
	
	// スレッドを起動
	if(::ResumeThread(hThread_)==-1) return false;

	set_state(state::ACTIVE);

	logger::infoln("[thread_helper]スレッドを起動しました。: " + to_str_s(nThreadID_) + to_str(nAffinity));
	return true;
}

template<class Fn>
inline void thread_helper<Fn>::join()
{
	if(state()==NONE) return;

	// スレッドが終了するまで待機
	::WaitForSingleObject(hThread_, INFINITE);

	// 終了後後始末
	::CloseHandle(hThread_);
	hThread_	= INVALID_HANDLE_VALUE;
	nThreadID_	= 0;
	set_state(state::NONE);
}


////////////////////////////////////
// static
////////////////////////////////////

template<class Fn>
uint32_t __stdcall thread_helper<Fn>::threadProc(void* pArg)
{
	thread_helper<Fn>* pThread = reinterpret_cast<thread_helper<Fn>*>(pArg);
	if(pThread==nullptr) return 1;

	pThread->proc_();

	pThread->end_thread();
	return 0;
}

} // namespace concurrent end
} // namespace mana end
