#pragma once

#include "../Concurrent/thread_helper.h"

namespace mana{
namespace app{

/*!
 *  @brief ウインドウ一つを担う基本クラス
 *  
 *  このクラスのインスタンスを作るとウインドウが作られる。
 *  作られたウインドウは別スレッドで動作する。
 *  ウインドウの動作は、mainメソッドをオーバーライドする。
 */
class window
{
public:
	enum window_const : DWORD
	{
		STYLE_WINDOW	 = WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, //!< ウインドウモード時の規定のスタイル
		STYLE_FULLSCREEN = WS_POPUP,											 //!< フルスクリーンモード時の規定のスタイル
	};

	enum state
	{
		NONE,	//!< 初期化前
		INIT,	//!< 初期化中
		ACTIVE,	//!< 動作中
		STOP,   //!< 終了処理中
		FIN,	//!< 動作終了
	};

	//! ウインドウ作成の際に使う情報まとめ
	struct win_info
	{
		WNDCLASSEX	wc_;				//!< ウインドウクラス
		
		win_info()
		{
			::ZeroMemory(&wc_, sizeof(WNDCLASSEX));
			wc_.cbSize	= sizeof(WNDCLASSEX);
		}
	};

public:
	window();
	virtual ~window(){ wait_fin(); }

public:
	enum state	state()const{ return static_cast<enum state>(eState_.load(std::memory_order_acquire)); }

	//! @brief ウインドウ動作スレッドを作成する
	/*! @param[in] sCaption ウインドウのキャプション
	 *  @param[in] bFullScreen フルスクリーンモードでウインドウを作るかどうか
	 *                         フルスクリーンの時は画面サイズでウインドウが作られるので、続く幅・高さは無視される
	 *  @param[in] nWidth ウインドウ幅
	 *  @param[in] nHeight ウインドウ高さ
	 *  @param[in] nAffinity 動作スレッドが動く論理CPU番号。0～8とか。
	 *  　　　　　　　　　　0は指定なし扱い。論理CPU数を越えてる時はmodを取る
	 *  @retval true  成功 
	 *  @retval false 失敗 */
	bool		create(const string& sCaption, bool bFullScreen=false, uint32_t nWidth=640, uint32_t nHeight=480, uint32_t nAffinity=0);

	//! @brief このメソッドを呼んだスレッドは、ウインドウが動作終了まで待機する
	/*! 初期化前の時は待機しない */ 
	void		wait_fin();

public:
	//! 非スレッドセーフなので、ACTIVE以外で呼ぶ時は不正な値の可能性がある
	HWND		wnd_handle()const{ return hWnd_; }

	//! ウインドウの幅
	uint32_t	wnd_width()const{ return nWidth_; }
	//! ウインドウの高さ
	uint32_t	wnd_height()const{ return nHeight_; }
	//! 現在フルスクリーンモードかどうか
	bool		is_fullscreen()const{ return bFullScreen_; }

	//! threadProc_で実行されるファンクター
	uint32_t	operator()();

protected:
	void set_state(enum state st){ eState_.store(st, std::memory_order_release); }

	//! WM_CLOSEが呼ばれるとtrueが返るようになる
	bool is_close_mes()const{ return bCloseMes_; }
	//! WM_CLOSEフラグを下ろす
	void reset_close_mes(){ bCloseMes_=false; }
	//! ウインドウがアクティブかどうか
	bool is_active()const{ return bActive_; }

	//! @retval false ウインドウ動作終了。mainを直ちに終わらすこと
	bool is_valid()const{ return bValid_; }

	//! @brief ウインドウを作成するメソッド
	/*! @retval true  成功 
		@retval false 失敗 */
	bool create_window();

	//! @brief ウインドウ作成前に呼ばれる
	/*! 細かい調整をしたい時は、オーバーライドする。
	 *  インスタンスハンドルとウインドウプロシージャは規定ものに置き換えられるので
	 *  書き換えても意味はない
	 *
	 *  @param[out] winfo ウインドウ情報を入れる */
	virtual void on_pre_window_create(win_info& winfo);

	//! @brief ウインドウの動作
	/*! オーバーライドすることで動作を規定する */
	virtual void main();

	//! @defgroup msg_loop メッセージ処理ループ
	/*! falseが返ったらmainを終了させること */
	//!@{
	bool wnd_msg_wait();
	bool wnd_msg_peek();
	//!@}

	//! @brief ウインドウ固有のウインドウプロシージャ
	/*!  WndProcShareから呼び出される。
	 *   WM_CLOSE/WM_DESTROYの処理をいれているので、オーバーライドしても
	 *   最後にwindow::WndProcを呼ぶのを推奨 */
	virtual LRESULT wnd_proc(HWND, UINT, WPARAM, LPARAM);

protected:
	//! ウインドウの表示/非表示
	void show_window(bool bShow);
	//! ウインドウの実際のサイズを取得する
	bool calc_real_window_size(int32_t& nRealWidth, int32_t& nRealHeight);

protected:
	//! @defgroup win_helper 各種ヘルパー
	//!@{
	void active_window() { bValid_ = true;  set_state(state::ACTIVE); }
	void end_window()	 { bValid_ = false; set_state(state::FIN); }
	bool is_create()const{ return state()!=state::NONE && state()!=state::FIN; }
	//!@}

protected:
	HWND		hWnd_;

	string		sCaption_;
	uint32_t	nWidth_, nHeight_;
	bool		bFullScreen_;

	BOOL		bMenu_;
	DWORD		nStyle_;
	DWORD		nExStyle_;

	//! @brief 内部用のウインドウ状態
	bool bValid_;

private:
	concurrent::thread_helper<window&> threadProc_;

	HACCEL	hAccel_;
	bool	bCloseMes_;
	bool	bActive_;

	//! 外部公開用ウインドウの状態
	std::atomic_int32_t eState_;

	//! @brief 全ウインドウ共通ウインドウプロシージャ
	/*! ここから個別のプロシージャに分岐する */
	static LRESULT CALLBACK WndProcShare(HWND, UINT, WPARAM, LPARAM);
};

} // namespace app end
} // namespace mana end
