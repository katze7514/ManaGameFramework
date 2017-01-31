#pragma once

namespace mana{
namespace input{

/*! @brief 物理マウス入力を取得する 
 *
 *	ウインドウズメッセージを使った実装なので、
 *	これを使うウインドウのウインドウプロシージャから、wnd_procを呼び出すこと
 *
 *  ウインドウや描画領域の外にカーソルが出ないように制限を掛けることもできる
 */
class mouse
{
public:
	enum btn : uint32_t
	{
		BTN_L,	//!< 左
		BTN_R,	//!< 右
		BTN_M,	//!< 中央
		BTN_4,	//!< WIN4
		BTN_5,	//!< WIN5
		BTN_END,
	};

	struct mouse_data
	{
		POINT	pos_;
		bool	bBtn_[BTN_END];
		int32_t nWheelDelta_;

		void reset()
		{
			pos_.x = pos_.y = 0;
			bBtn_[BTN_L]=bBtn_[BTN_R]=bBtn_[BTN_M]=bBtn_[BTN_4]=bBtn_[BTN_5]=false;
			nWheelDelta_ = 0;
		}
	};

public:
	mouse():curIndex_(0),bGuard_(false),bCursorLimit_(false),fScaleX_(1.0f),fScaleY_(1.0f){ buf_clear(); mouseMsg_.reset(); ::ZeroMemory(&rectCursorLimit_, sizeof(rectCursorLimit_)); }
	~mouse(){ buf_clear(); mouseMsg_.reset(); }

public:
	//! 初期化
	bool init(HWND hWnd);

public:
	void cursor_limit(bool bLimit){ bCursorLimit_ = bLimit; }
	bool is_cursor_limit()const{ return bCursorLimit_; }

	const RECT& cursol_limit_rect(){ return rectCursorLimit_; }
	void		set_cursor_limit_rect(RECT rect){ rectCursorLimit_=rect; }
	
	void set_cursor_scale(float fX, float fY){ fScaleX_ = fX; fScaleY_ = fY; }

public:
	//! メッセージプロシージャ
	LRESULT wnd_proc(HWND, UINT, WPARAM, LPARAM);

	//! マウス入力を取得する
	bool input();

	//! ボタンが押されている
	bool is_press(uint32_t nBtn)const;
	//! ボタンを押した瞬間
	bool is_push(uint32_t nBtn)const;
	//! ボタンを離した瞬間
	bool is_release(uint32_t nBtn)const;

	//! ホイールアップ
	bool wheel_up()const;
	//! ホイールダウン
	bool wheel_down()const;

	//! @defgroup mouse_cursor_pos フルスクリーン時などゲーム画面サイズ拡縮も考慮にいれた座標が取得される
	/*! 基本的にこのメソッドを使えば良い */
	//! @{
	//! マウスカーソルのX位置（クライアント座標系）
	int32_t	x()const;
	//! マウスカーソルのY位置（クライアント座標系）
	int32_t y()const;

	//! マウスカーソルの一つ前のX位置（クライアント座標系）
	int32_t	x_prev()const;
	//! マウスカーソルの一つ前のY位置（クライアント座標系）
	int32_t y_prev()const;
	//! @}

	//! @defgroup mouse_cursor_pos_real APIで取得された生座標が取得できる
	//! @{
	//! マウスカーソルの実X位置（クライアント座標系）
	int32_t	x_real()const;
	//! マウスカーソルの実Y位置（クライアント座標系）
	int32_t y_real()const;

	//! マウスカーソルの一つ前の実X位置（クライアント座標系）
	int32_t	x_real_prev()const;
	//! マウスカーソルの一つ前の実Y位置（クライアント座標系）
	int32_t y_real_prev()const;
	//! @}

	// 入力ガード
	void guard(bool bGuard);
	bool is_guard()const{ return bGuard_; }

	//! バッファをクリアする
	void buf_clear(){ curIndex_=0; mouseMsg_.reset(); for(auto d : mouseBuf_) d.reset(); }

	//! 現在の入力情報をクリアする
	void curbuf_clear(){ mouseBuf_[curIndex_].reset(); }

private:
	//! 対応するウインドウのハンドル
	HWND hWnd_;

	//! マウスの入力バッファ
	mouse_data	mouseBuf_[2];
	//! 現在のバッファ位置
	uint32_t	curIndex_;

	//! ウインドウメッセージによる状態
	mouse_data	mouseMsg_;

	//! @brief 入力ガードフラグ
	/*! trueになすると入力バッファがクリアされ、inputを呼んでも更新されない
	 *  つまり、入力されてない扱いになる */
	bool bGuard_;

	//! カーソル制限有効かどうか
	bool bCursorLimit_;

	//! カーソル制限範囲(ウインドウサイズに対する範囲)
	RECT rectCursorLimit_;

	//! カーソル位置の倍率(この係数が掛けられたサイズが返る)
	float fScaleX_, fScaleY_;
};

typedef shared_ptr<mouse> mouse_ptr;

} // namespace input end
} // namespace mana end
