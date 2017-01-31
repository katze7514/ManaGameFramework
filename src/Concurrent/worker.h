#pragma once

#include "thread_helper.h"

namespace mana{
namespace concurrent{

//! requestした時にj返されるfuture
class future;

/*! 
 * @brief 別スレッドでワークを実行するクラス
 *
 *  WorkerThreadパターンの実装。
 *  workキューを一つと、実行スレッドを1つ以上持つ。
 *  workerで動かすworkは引数なしファンクターか関数ポインタ
 *  requestで仕事を積む。仕事終了はrequestの戻り値のfuture::is_finを使う
 */
class worker
{
public:
	//! ワーカースレッドで実行されるファンクター
	struct executer
	{
		executer(worker& worker):worker_(worker){}
		executer& operator=(const executer&);

		void operator()();

		worker& worker_;
	};
	typedef thread_helper<executer>			worker_thread;
	typedef ptr_vector<worker_thread>		worker_thread_vec;
	
private:
	//! キューに積まれるクラス
	struct work
	{
		work(){ fin_ = make_shared<future>(); }

		function<void(void)>	exec_;	//	ワーク実体
		shared_ptr<future>		fin_;	//	ワーク終了チェックに使われる

	private:
		NON_COPIABLE(work);
	};

public:
	worker(uint32_t nWorkCapacity);
	~worker();

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
	weak_ptr<future> request(const function<void(void)>& job);

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
	circular_buffer<work*> workQueue_;
	
	// キューへのpush/popロック
	std::mutex				mutex_;
	std::condition_variable empty_, full_;

	// workerスレッド
	worker_thread_vec vecThread_;

	NON_COPIABLE(worker);
};	

//! requestした時にj返されるfuture
class future
{
	friend struct worker::executer;

public:
	future(){ set_fin(false); }
	bool is_fin()const{ return bFin_.load(std::memory_order_acquire); }

private:
	void set_fin(bool b){ bFin_.store(b, std::memory_order_release); }

private:
	std::atomic_bool bFin_;	// 実行終了フラグ

	NON_COPIABLE(future);
};



} // namespace concurrent end
} // namespace mana end


// 使用例などは、worker_lockfree.h を参照
