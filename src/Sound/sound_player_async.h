#pragma once

#include "../Concurrent/thread_helper.h"
#include "../Concurrent/sync_queue.h"

#include "sound_player.h"

namespace mana{
namespace sound{

/*! @brief 非同期版サウンドプレイヤー
 *
 *　サウンド処理が専用スレッドで実行される
 *  リクエストは、start_requestで積めるかを確認し
 *  積み終わったらend_reqeust。playでサウンド処理をキックする */
class sound_player_async : public sound_player
{
public:
	enum sound_player_state : uint32_t
	{
		STOP,		//!< リクエスト待機中
		RECEIVE_0,	//!< 0番キューリクエスト処理中
		RECEIVE_1,	//!< 1番キューリクエスト処理中
	};

	typedef concurrent::sync_queue<cmd::sound_player_cmd> cmd_queue;

public:
	sound_player_async():nCurReqIndex_(0),player_(*this),executer_(player_){}
	~sound_player_async(){ fin(); }

	//! @brief プレイヤー初期化。サウンドスレッドを立ち上げる
	/*! @param[in] fIntervalMs サウンド処理の実行間隔
	 * @param[in] nReserveRequestNum 積むリクエストの推定数
	 * @param[in] nAffinity レンダースレッドを動作させるCPUコア番号 */
	bool init_player(uint32_t nIntervalMs=DEFAULT_INTERVAL_MS, uint32_t nReserveRequestNum=DEFAULT_MAX_REQUEST_NUM, uint32_t nAffinity=0)override;

	//! 動作終了。スレッドも終了する
	void fin()override;

	//! @defgroup sound_player_async_request リクエスト処理
	//! @{
	//! @brief request積み処理を開始する
	/*! @param[in] bWait リクエストを積む準備ができてない時、ブロックする
	 *  @return bWaitがfalseの場合、falseが返って来たらコマンドキューが使用中でリクエストが積めない状態 */
	bool start_request(bool bWait=true)override;
	//! @brief requestを積む
	/*! start_request ～ end_request の間でのみ呼ぶこと */
	void request(const cmd::sound_player_cmd& cmd)override;
	//! request積みの終了
	void end_request()override;
	//! @brief 積んだリクエストの処理を開始する
	/*! @param[in] bWait リクエストを処理中だったらブロックするかどうかのフラグ
	 *  @return bWaitがfalseの場合、falseが返って来たらリクエスト処理中 */
	bool play(bool bWait=true)override;
	//! @}

private:
	cmd_queue&	cur_queue(){ return cmdQueue_[nCurReqIndex_]; }
	void		swap_queue(){ nCurReqIndex_^=1; }

private:
	cmd_queue	cmdQueue_[2];
	uint32_t	nCurReqIndex_;

private:
	NON_COPIABLE(sound_player_async);

////////////////////////////
// サウンド処理
private:
	void start_receive(uint32_t Index);
	void end_receive(uint32_t nIndex);

private:
	struct sound_executer
	{
	public:
		sound_executer(sound_player_async& queue);

		void operator()();

		uint32_t state()const{ return nState_.load(std::memory_order_acquire); }
		void	 set_state(uint32_t nState){ nState_.store(nState, std::memory_order_release); }

		bool	is_fin(){ return bFin_.load(std::memory_order_acquire); }
		void	fin(){ bFin_.store(true, std::memory_order_release); }

		void	play(uint32_t nIndex);

	public:
		uint32_t				nIntervalMs_; // 実行されるインターバル(ms)

		std::atomic_uint32_t	nState_;  // プレイヤーの状態
		std::atomic_bool		bFin_;	  // 終了フラグ。フラグ立ってたらプレイヤーを終了する
		ds_sound_player*		pPlayer_; // プレイヤー実体
		sound_player_async&		queue_;

		timer::elapsed_timer	timer_;

	private:
		NON_COPIABLE(sound_executer);
	};

private:
	sound_executer								player_;
	concurrent::thread_helper<sound_executer&>	executer_;
};

} // namespace sound end
} // namespace mana end

/*
// サウンド初期化
	sound::cmd::play_cmd cmdPlay;
	cmdPlay.bLoop_ = true;

	sound::cmd::play_cmd cmdPlay2;
	cmdPlay2.bLoop_ = true;

	sound::sound_player sndPlayer;
	sndPlayer.init();
	if(sndPlayer.start_request())
	{
		sound::cmd::init_cmd cmdInit;
		cmdInit.hWnd_ = wnd_handle();		
		sndPlayer.request(cmdInit);

		sound::cmd::info_add_cmd cmdAdd;
		cmdAdd.sID_ = "TAKUMI";
		cmdAdd.sFilePath_ = "7thmoon.ogg";
		cmdAdd.bStreaming_ = true;
		cmdAdd.callback_ = [&cmdPlay](uint32_t nID){ cmdPlay.nID_ = nID; };
		sndPlayer.request(cmdAdd);

		sound::cmd::info_add_cmd cmdAdd2;
		cmdAdd2.sID_ = "HARUNA";
		cmdAdd2.sFilePath_ = "spirit.wav";
		cmdAdd2.callback_ = [&cmdPlay2](uint32_t nID){ cmdPlay2.nID_ = nID; };
		sndPlayer.request(cmdAdd2);

		sndPlayer.end_request();
		sndPlayer.play();
	}


	// メインループ
	bool s=false;
	bool s2=false;

	while(wnd_msg_peek())
	{
		if(sndPlayer.start_request())
		{
			if(s && cmdPlay.nID_!=0)
			{
				sndPlayer.request(cmdPlay);
				s = false;
			}

			if(s2 && cmdPlay2.nID_!=0)
			{
				sndPlayer.request(cmdPlay2);
				s2 = false;
			}


			sndPlayer.end_request();
		}

		sndPlayer.play();
	}
*/
