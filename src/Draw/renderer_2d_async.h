#pragma once

#include "../Concurrent/thread_helper.h"
#include "../Concurrent/sync_queue.h"

#include "renderer_2d_cmd.h"

#include "renderer_2d.h"

namespace mana{
namespace draw{

/*! @brief 非同期版2D描画レンダラー
 *  
 *  描画が専用スレッドで実行される
 *　リクエストは、start_requestでリクエストが積めるかを確認し
 *  積み終わったらend_requestを呼び、renderで描画をキックする
 *  なお、リクエストをキックしない限り、リクエストは処理されない */
class renderer_2d_async : public renderer_2d
{
public:
	enum renderer_state : uint32_t
	{
		STOP,				//!< リクエスト待機中
		RECEIVE_0,			//!< 0番キューリクエスト処理中
		RECEIVE_1,			//!< 1番キューリクエスト処理中
		DEVICE_LOST,		//!< デバイスロストが発生している
		FATAL,				//!< 致命的なエラー発生
	};

	//! renderer_2d_asyncの元で使われるコマンドキュー
	typedef concurrent::sync_queue<cmd::render_2d_cmd> cmd_queue;

public:
	renderer_2d_async():nCurReqIndex_(0),renderer_(*this),executer_(renderer_){}
	~renderer_2d_async(){ fin(); }

	//! @brief レンダラー初期化。レンダースレッドを立ち上げる
	/*! @param[in] nReserveRequestNum 積むリクエストの推定数
	 *  @param[in] nAffinity レンダースレッドを動作させるCPUコア番号 */
	bool init_render(uint32_t nReserveRequestNum=128, uint32_t nAffinity=0)override;

	//! 動作を終了する。スレッドも終了する
	void fin()override;

	//! @defgroup renderer_2d_async_request リクエスト処理
	//! @{
	//! @brief request積み処理を開始する
	/*! @param[in] bWait リクエストを積む準備ができてない時、ブロックする
	 *  @return bWaitがfalseの場合、falseが返って来たらコマンドキューが使用中でリクエストが積めない状態 */
	bool start_request(bool bWait=true)override;
	//! @brief requestを積む
	/*! start_request ～ end_request の間でのみ呼ぶこと */
	void request(const cmd::render_2d_cmd& cmd)override;
	//! request積みの終了
	void end_request()override;
	//! @brief 積んだリクエストの処理を開始する
	/*! @param[in] bWait リクエストを処理中だったらブロックするかどうかのフラグ
	 *  @return bWaitがfalseの場合、falseが返って来たらリクエスト処理中 */
	render_result render(bool bWait=true)override;
	//! @}

	//! デバイスロストしてるかどうか
	bool is_device_lost()const override{ return renderer_.state()==DEVICE_LOST; }

private:
	cmd_queue&	cur_queue(){ return cmdQueue_[nCurReqIndex_]; }
	void		swap_queue(){ nCurReqIndex_^=1; }

private:
	cmd_queue	cmdQueue_[2];	// ダブルバッファ
	uint32_t	nCurReqIndex_;	// 現在リクエストを積むインデックス

private:
	NON_COPIABLE(renderer_2d_async);

////////////////////////////
// レンダー処理
private:
	//! リクエスト処理開始通知
	void	start_receive(uint32_t nIndex);
	//! リクエスト処理終了通知
	void	end_receive(uint32_t nIndex);

private:
	struct render_executer
	{
	public:
		render_executer(renderer_2d_async& queue);

		void operator()();

		uint32_t state()const{ return nState_.load(std::memory_order_acquire); }
		void	 set_state(uint32_t nState){ nState_.store(nState, std::memory_order_release); }

		bool	is_fin(){ return bFin_.load(std::memory_order_acquire); }
		void	fin(){ bFin_.store(true, std::memory_order_release); }

		void	render(uint32_t nIndex);

	public:
		std::atomic_uint32_t	nState_; // レンダラーの状態
		std::atomic_bool		bFin_;	 // 終了フラグ。フラグ立ってたらレンダラーを終了する
		d3d9_renderer_2d*		pRenderer_; // レンダラー実体
		renderer_2d_async&		queue_;

	private:
		NON_COPIABLE(render_executer);
	};

private:
	render_executer								renderer_;
	concurrent::thread_helper<render_executer&>	executer_;
};

} // namespace draw end
} // namespace mana end

/*
	// レンダー初期化
	draw::d3d9_renderer_2d_async renderer;
	renderer.init();

	draw::cmd::text_draw_cmd cmdText;
	cmdText.sText_ = "ニューロダンだよ＾－＾。漢字だってOKさ！\nabcdefghijklmnopqrstuvwxyz";
	cmdText.pos_   = draw::POS(10.f, 200.f);
	
	if(renderer.start_request())
	{
		draw::cmd::d3d9_render_2d_init_cmd cmdInit;
		cmdInit.device_.hWnd_		= wnd_handle();
		cmdInit.device_.nWidth_	= width();
		cmdInit.device_.nHeight_	= height();
		renderer.request(cmdInit);

		draw::cmd::info_load_cmd cmdFont;
		cmdFont.eKind_		= draw::cmd::KIND_FONT;
		cmdFont.sID_		= "RODIN";
		cmdFont.sFilePath_  = "FOT-NewRodinProN-DB_16.xml";
		cmdFont.callback_	= [&cmdText](uint32_t nID){ cmdText.nFontID_=nID; };
		renderer.request(cmdFont);

		renderer.end_request();
		renderer.kick_request();
	}

	// メインループ
	while(wnd_msg_peek())
	{
		if(renderer.start_request())
		{
			if(cmdText.nFontID_!=0)
			{
				renderer.request(cmdText);
			}

			renderer.end_request();
			renderer.kick_request();
		}
	}
 */
