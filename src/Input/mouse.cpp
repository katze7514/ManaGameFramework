#include "../mana_common.h"

#include "mouse.h"

namespace mana{
namespace input{

bool mouse::init(HWND hWnd)
{
	hWnd_ = hWnd;
	return true;
}

LRESULT mouse::wnd_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// 入力ガードがあったらメッセージも受け付けない
	if(is_guard()) return 0;

	switch(uMsg)
	{
	case WM_LBUTTONDOWN:
		mouseMsg_.bBtn_[BTN_L]=true;
	break;

	case WM_LBUTTONUP:
		mouseMsg_.bBtn_[BTN_L]=false;
	break;

	case WM_RBUTTONDOWN:
		mouseMsg_.bBtn_[BTN_R]=true;
	break;

	case WM_RBUTTONUP:
		mouseMsg_.bBtn_[BTN_R]=false;
	break;

	case WM_MBUTTONDOWN:
		mouseMsg_.bBtn_[BTN_M]=true;
	break;

	case WM_MBUTTONUP:
		mouseMsg_.bBtn_[BTN_M]=false;
	break;

	case WM_XBUTTONDOWN:
	{
		uint32_t btn=0;
		if((wParam & MK_XBUTTON1)>0)
			btn = BTN_4;
		else if((wParam & MK_XBUTTON2)>0)
			btn = BTN_5;

		if(btn>0)
			mouseMsg_.bBtn_[btn]=true;
	}
	break;

	case WM_XBUTTONUP:
	{
		uint32_t btn=0;
		if((wParam & MK_XBUTTON1)>0)
			btn = BTN_4;

		else if((wParam & MK_XBUTTON2)>0)
			btn = BTN_5;

		if(btn>0)
			mouseMsg_.bBtn_[btn]=false;
	}
	break;

	case WM_MOUSEWHEEL:
		mouseMsg_.nWheelDelta_ = GET_WHEEL_DELTA_WPARAM(wParam);
	break;
	}
	return 0;
}

bool mouse::input()
{
	if(is_guard()) return true;

	curIndex_^=1; // バッファ位置入れ替え

	// ボタン系はメッセージでの状態をコピー
	mouseBuf_[curIndex_] = mouseMsg_;

	mouseMsg_.nWheelDelta_ = 0; // WheelDeltaは0リセットしないと一度UP/DOWNになると維持されてしまう

	// 現在のカーソル位置を取得
	if(::GetCursorPos(&mouseBuf_[curIndex_].pos_)==FALSE)
	{
		::ZeroMemory(&mouseBuf_[curIndex_].pos_, sizeof(POINT));
		logger::warnln("[mouse]カーソル位置を取得できませんでした。");
		return false;
	}

	if(::ScreenToClient(hWnd_, &mouseBuf_[curIndex_].pos_)==FALSE)
	{
		::ZeroMemory(&mouseBuf_[curIndex_].pos_, sizeof(POINT));
		logger::warnln("[mouse]カーソル位置をクライアント座標に変換できませんでした。");
		return false;
	}

	// カーソル範囲制限中の時は強制的にその範囲に収める
	if(is_cursor_limit())
	{
		mouseBuf_[curIndex_].pos_.x = clamp(x_real(), rectCursorLimit_.left, rectCursorLimit_.right);
		mouseBuf_[curIndex_].pos_.y = clamp(y_real(), rectCursorLimit_.top, rectCursorLimit_.bottom);
		::SetCursorPos(x_real(), y_real());
	}

	return true;
}

bool mouse::is_press(uint32_t nBtn) const
{
	return mouseBuf_[curIndex_].bBtn_[nBtn];
}

bool mouse::is_push(uint32_t nBtn) const
{
	return mouseBuf_[curIndex_].bBtn_[nBtn]
		&& mouseBuf_[curIndex_^1].bBtn_[nBtn];
}

bool mouse::is_release(uint32_t nBtn) const
{
	return mouseBuf_[curIndex_].bBtn_[nBtn]
		&& mouseBuf_[curIndex_^1].bBtn_[nBtn];
}

bool mouse::wheel_up()const
{
	return mouseBuf_[curIndex_].nWheelDelta_>0;
}

bool mouse::wheel_down()const
{
	return mouseBuf_[curIndex_].nWheelDelta_<0;
}

int32_t mouse::x() const
{
	if(is_cursor_limit())
		return static_cast<int32_t>(x_real()*fScaleX_);
	else
		return x_real();
}

int32_t mouse::y() const
{
	if(is_cursor_limit())
		return static_cast<int32_t>(y_real()*fScaleY_);
	else
		return y_real();
}

int32_t mouse::x_prev() const
{
	return mouseBuf_[curIndex_^1].pos_.x;
}

int32_t mouse::y_prev() const
{
	return mouseBuf_[curIndex_^1].pos_.y;
}

int32_t mouse::x_real() const
{
	return mouseBuf_[curIndex_].pos_.x;
}

int32_t mouse::y_real() const
{
	return mouseBuf_[curIndex_].pos_.y;
}

int32_t mouse::x_real_prev() const
{
	return mouseBuf_[curIndex_^1].pos_.x;
}

int32_t mouse::y_real_prev() const
{
	return mouseBuf_[curIndex_^1].pos_.y;
}

void mouse::guard(bool bGuard)
{
	bGuard_=bGuard;
	if(bGuard_) buf_clear();
}

} // namespae input end
} // namespae mana end

/*
	mana::input::mouse mouse;
	mouse.init(input, wnd_handle());

	if(mouse_.input())
	{
		logger::infoln("[BTN0 press   " + to_str(mouse_.is_press(0)) + "]");
		logger::infoln("[BTN0 push    " + to_str(mouse_.is_push(0)) + "]");
		logger::infoln("[BTN0 release " + to_str(mouse_.is_release(0)) + "]");
				
		logger::infoln("[x" + to_str(mouse_.x()) + "]");
		logger::infoln("[y" + to_str(mouse_.y()) + "]");
		if(mouse_.wheel_up())
			logger::infoln("[w_u]");

		if(mouse_.wheel_down())
			logger::infoln("[w_d]");
	}
*/
