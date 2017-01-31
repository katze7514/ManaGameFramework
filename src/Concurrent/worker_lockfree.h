#pragma once

#include "thread_helper.h"

namespace mana{
namespace concurrent{

class future_lockfree;

/*! @brief 別スレッドでワークを実行するクラス（lockfree版）
 *
 *  WorkerThreadパターンの実装
 *　使い方は、worker と一緒
 *  こちらは、ロックフリーキューを使った実装
 */
class worker_lockfree
{
public:
	//! ワーカースレッドで実行されるファンクタ
	struct executer
	{
		executer(worker_lockfree& worker):worker_(worker){}
		executer& operator=(const executer&);

		void operator()();

		worker_lockfree& worker_;
	};
	typedef thread_helper<executer>		worker_thread;
	typedef ptr_vector<worker_thread>	worker_thread_vec;

private:
	//! キューに積まれるクラス
	struct work
	{
		work(){ fin_ = make_shared<future_lockfree>(); }

		function<void(void)>		exec_;	//	ワーク実体
		shared_ptr<future_lockfree>	fin_;	//	ワーク終了チェックに使われる

	private:
		NON_COPIABLE(work);
	};

public:
	worker_lockfree(uint32_t nWorkCapacity);
	~worker_lockfree();

public:
	//! @brief workerを起動する
	/*! @param[in] work_executer_num work実行スレッド数 
	 *  @param[in] aAffinity 実行スレッドの割り当てる論理CPU番号。スレッド数分用意すること */
	void kick(uint32_t nWorkExecuterNum, uint32_t anAffinity[]);

	//! @brief workをキューに積む。キューがいっぱいだったら待機する
	/*! @param[in] job 積む仕事。void(void)のファンクター、bindで束縛した（メンバ）関数、C++ラムダ、を使う
	 *  @retval work 完了チェックができるオブジェクトへのwepk_ptr
	 *               ポインタを取得し、isFinがtrueを返すか、nullptrだったら終了済み
	 *  @retval nullptr リクエスト出来なかった */
	weak_ptr<future_lockfree> request(const function<void(void)>& job);

	bool is_fin()const{ return bFin_.load(std::memory_order_acquire); }
	void set_fin(bool bFin);

private:
	//! @brief workをキューから取得する。キューが空だったら待機する
	/*! @retval nullptr workerが終了状態になっていると返る  */
	work* pop();

	//! workを返却する
	void work_delete(work* pWork);

private:
	//! 終了フラグ
	std::atomic_bool bFin_;

	// Workキュー実体
	lockfree::queue<work*> workQueue_;
	
	// workerスレッド
	worker_thread_vec vecThread_;

	NON_COPIABLE(worker_lockfree);
};

//! requestした時に返されるfuture
class future_lockfree
{
	friend worker_lockfree::executer;

public:
	future_lockfree(){ set_fin(false); }
	bool is_fin()const{ return bFin_.load(std::memory_order_acquire); }

private:
	void set_fin(bool b){ bFin_.store(b, std::memory_order_release); }

private:
	std::atomic_bool bFin_;		// 実行終了フラグ

	NON_COPIABLE(future_lockfree);
};

} // namespace concurrent end
} // namespace mana end

/* 使用例
struct test_work
{
	test_work(int i=0, uint32_t w=0):i_(i),w_(w){}

	int i_;
	uint32_t w_;

	void t()
	{
		//logger::infoln(i_);
		i_+=100;
		::Sleep(w_);
	}
};

template <class worker>
void test_worker(worker& work, uint32_t w_num, uint32_t t_num, uint32_t aff[], uint32_t w)
{
	test_work* test				= new test_work[w_num];
	worker::future_wptr* fut	= new worker::future_wptr[w_num];

	work.kick(t_num, aff);

	auto start = std::chrono::high_resolution_clock::now();

	// ワークリクエスト
	for(uint32_t i=0; i<w_num; ++i)
	{
		test[i].i_=i;
		test[i].i_=w;
		fut[i] = work.request([i,&test](){ test[i].t(); });
		//::Sleep(w);
		// fut[i] = work.request(bind(&test_work::t, &test[i]));
	}

	// 終了確認
	bool bFin=false;
	while(!bFin)
	{
		bool b = true;
		for(uint32_t i=0; i<w_num; ++i)
		{
			worker::future_sptr p = fut[i].lock();
			if(p) // ポインタが取得出来たらisFinで終了チェック
			{
				b &= p->is_fin();
			}
			else
			{// 取得できなかったら、終了していると同じ意味
				b = true;
			}
		}

		bFin = b;
	}

	auto end = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> sec = end - start;
	logger::infoln(sec.count());

	delete[] test;
	delete[] fut;
}

void test_worker_all()
{
	static const int	  num	=	100;
	static const uint32_t t_num	=	4;
				 uint32_t aff[]	=	{0,1,2,3};
	static const uint32_t w		=	50;

{// wokerテスト
	concurrent::worker work(num);
	test_worker(work, num, t_num, aff, w);
}


{// worker_lockfreeテスト
	concurrent::worker_lockfree work(num);
	test_worker(work, num, t_num, aff, w);
}

}

*/
