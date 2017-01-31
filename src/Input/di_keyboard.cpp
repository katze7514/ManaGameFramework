#include "../mana_common.h"

#include "di_driver.h"
#include "di_keyboard.h"

namespace mana{
namespace input{

bool di_keyboard::init(di_driver& adapter, HWND hWnd, bool bBackGround)
{
	if(pDevice_)
	{
		logger::warnln("[di_keyboard]初期化済みです");
		return false;
	}

	auto driver = adapter.driver();

	HRESULT r = driver->CreateDevice(GUID_SysKeyboard, &pDevice_, NULL);
	if(!check_hresult(r, "[di_keyboard]CreateDevice失敗： ")) return false;

	r = pDevice_->SetDataFormat(&c_dfDIKeyboard);
	if(!check_hresult(r, "[di_keyboard]SetDataformat失敗： ")) return false;

	// CooperativeLevelフラグ決定
	DWORD flags = 0;
	
	if(bBackGround) // ウインドウが非アクティブでも取得される
		flags = DISCL_BACKGROUND | DISCL_NONEXCLUSIVE;
	else
		flags = DISCL_FOREGROUND | DISCL_NONEXCLUSIVE;

	r = pDevice_->SetCooperativeLevel(hWnd, flags);
	if(!check_hresult(r, "[di_keyboard]SetCooperativeLevel失敗： "))
	{
		fin();
		return false;
	}

	const string sFlag = (flags&DISCL_BACKGROUND)>0 ? "BACKGROUND" : "FOREGROUND";
	logger::infoln("[di_keyboard]キーボード用Device作成完了: " + sFlag);

	return true;
}

void di_keyboard::fin()
{
	if(pDevice_ && bAcquire_) pDevice_->Unacquire();
	safe_release(pDevice_);
}

bool di_keyboard::input()
{
	if(!pDevice_)
	{
		logger::warnln("[di_keyboard]キーボードデバイスが初期化されていません。");
		return false;
	}

	HRESULT r;
	// デバイス獲得してなかったら獲得する
	if(!bAcquire_)
	{
		r = pDevice_->Acquire();
		if(FAILED(r))
		{// 獲得できなかたっら次の機会へ
			//logger::warnln("[di_keyboard]Lost: " + to_str(r));
			curbuf_clear();
			return false;
		}

		bAcquire_=true;
	}

	// 入力ガードがあったらここまで
	if(is_guard()) return true;

	// 入力データを取得
	curIndex_ ^= 1;
	r = pDevice_->GetDeviceState(256, &keyBuf_[curIndex_]);

	if(r==DIERR_INPUTLOST)
	{// 獲得権がなかったら1度だけ取得しに行く
		bAcquire_ =false;

		r = pDevice_->Acquire();
		if(FAILED(r))
		{// 獲得できなかたっら次の機会へ
			curbuf_clear();
			return false;
		}
		else
		{
			bAcquire_ = true;
			r = pDevice_->GetDeviceState(256, &keyBuf_[curIndex_]);
			if(FAILED(r))
			{
				curbuf_clear();
				return false;
			}
		}
	}

	return true;
}

bool di_keyboard::is_press(uint32_t nKey)const
{
	return (keyBuf_[curIndex_][nKey] & 0x80)>0;
}

bool di_keyboard::is_push(uint32_t nKey)const
{
	return (keyBuf_[curIndex_][nKey] & 0x80)>0
		&& (keyBuf_[curIndex_^1][nKey] & 0x80)==0;
}

bool di_keyboard::is_release(uint32_t nKey)const
{
	return (keyBuf_[curIndex_][nKey] & 0x80)==0
		&& (keyBuf_[curIndex_^1][nKey] & 0x80)>0;
}

void di_keyboard::guard(bool bGuard)
{
	bGuard_ = bGuard;
	if(bGuard_) buf_clear();
}

} // namespace mana end
} // namespace input end

/*
	mouse_.init(wnd_handle());
	keyboard_.init(input, wnd_handle());

	keyboard_.input();
			
	if(keyboard_.is_press(DIK_A))
		logger::infoln("press   A " + to_str(keyboard_.is_press(DIK_A)));
	if(keyboard_.is_push(DIK_A))
		logger::infoln("push    A " + to_str(keyboard_.is_push(DIK_A)));
	if(keyboard_.is_release(DIK_A))
		logger::infoln("release A " + to_str(keyboard_.is_release(DIK_A)));
*/
