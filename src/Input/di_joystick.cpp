#include "../mana_common.h"

#include "di_driver.h"
#include "di_joystick.h"

namespace mana{
namespace input{

//////////////////////////////////////
// di_joystick_enum
/////////////////////////////////////
bool di_joystick_enum::init(di_driver& adapter)
{
	vecJoyStickInfo_.clear();
	auto pDriver = adapter.driver();

	HRESULT r = pDriver->EnumDevices(DI8DEVCLASS_GAMECTRL, &di_joystick_enum::on_enum_device, this, DIEDFL_ATTACHEDONLY);
	if(FAILED(r))
	{
		logger::warnln("[di_joystick_enum]使用可能なジョイスティックを検索できませんでした。");
		return false;
	}

	vecJoyStickInfo_.shrink_to_fit();
	return true;
}

device_inst_opt di_joystick_enum::joystick_info(uint32_t nJoyNo)const
{
	if(vecJoyStickInfo_.empty() || nJoyNo>=enabled_joystick_num())
	{
		logger::warnln("[di_joystick_enum]使用可能なジョイスティック番号ではありません。：" + to_str(nJoyNo));
		return device_inst_opt();
	}

	return device_inst_opt(vecJoyStickInfo_.at(nJoyNo));
}

BOOL CALLBACK di_joystick_enum::on_enum_device(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
	di_joystick_enum* pEnum = reinterpret_cast<di_joystick_enum*>(pvRef);

	DIDEVICEINSTANCE di = *lpddi;
	pEnum->vecJoyStickInfo_.emplace_back(di);

	//logger::traceln(di.tszProductName);

	return DIENUM_CONTINUE;
}

//////////////////////////////////////
// di_joystick
/////////////////////////////////////
const int32_t di_joystick::MAX_AXIS_VALUE =  1000;

bool di_joystick::init(const DIDEVICEINSTANCE& joyins, di_driver& adapter, HWND hWnd, bool bBackGround)
{
	if(pDevice_)
	{
		logger::infoln("[di_joystick]初期化済みです");
		return false;
	}

	auto driver = adapter.driver();

	HRESULT r = driver->CreateDevice(joyins.guidInstance, &pDevice_, NULL);
	if(!check_hresult(r, "[di_joystick]CreateDevice失敗： ")) return false;

	r = pDevice_->SetDataFormat(&c_dfDIJoystick);
	if(!check_hresult(r, "[di_joystick]SetDataformat失敗： "))
	{
		fin();
		return false;
	}

	// CooperativeLevelフラグ決定
	DWORD flags = 0;
	
	if(bBackGround) // ウインドウが非アクティブでも取得される
		flags = DISCL_BACKGROUND | DISCL_NONEXCLUSIVE;
	else
		flags = DISCL_FOREGROUND | DISCL_NONEXCLUSIVE;

	r = pDevice_->SetCooperativeLevel(hWnd, flags);
	if(!check_hresult(r, "[di_joystick]SetCooperativeLevel失敗： "))
	{
		fin();
		return false;
	}

	// 軸の設定

	// 絶対軸モードに設定
	DIPROPDWORD diprop;
	diprop.diph.dwSize			= sizeof(DIPROPDWORD);
	diprop.diph.dwHeaderSize	= sizeof(DIPROPHEADER);
	diprop.diph.dwHow			= DIPH_DEVICE;
	diprop.diph.dwObj			= 0;
	diprop.dwData				= DIPROPAXISMODE_ABS;

	r = pDevice_->SetProperty(DIPROP_AXISMODE, &diprop.diph);
	if(!check_hresult(r, "[di_joystick]軸モードを絶対軸に設定できませんでした："))
	{
		fin();
		return false;
	}

	// 軸の範囲を設定
	tuple<HRESULT, LPDIRECTINPUTDEVICE8> axisResult(DI_OK, pDevice_);
	pDevice_->EnumObjects(&di_joystick::on_enum_axis, reinterpret_cast<void*>(&axisResult), DIDFT_AXIS);

	if(!check_hresult(axisResult.get<0>()))
	{
		fin();
		return false;
	}

	const string sFlag = (flags&DISCL_BACKGROUND)>0 ? "BACKGROUND" : "FOREGROUND";
	logger::infoln("[di_joystick]ジョイスティック用Device作成完了: " + sFlag);

	return true;
}

void di_joystick::fin()
{
	if(pDevice_ && bAcquire_) pDevice_->Unacquire();
	safe_release(pDevice_);
}

BOOL CALLBACK di_joystick::on_enum_axis(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
	tuple<HRESULT, LPDIRECTINPUTDEVICE8>* pAxisResult = reinterpret_cast<tuple<HRESULT, LPDIRECTINPUTDEVICE8>*>(pvRef);

	DIPROPRANGE diprg;
	diprg.diph.dwSize       = sizeof(DIPROPRANGE); 
	diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER); 
	diprg.diph.dwHow        = DIPH_BYID; 
	diprg.diph.dwObj        = lpddoi->dwType;
	diprg.lMin              = -MAX_AXIS_VALUE;
	diprg.lMax              =  MAX_AXIS_VALUE;

	pAxisResult->get<0>() = pAxisResult->get<1>()->SetProperty(DIPROP_RANGE, &diprg.diph);

	if(!check_hresult(pAxisResult->get<0>(), "[di_joystick]" + string(lpddoi->tszName) + "の範囲を設定できませんでした："))
		return DIENUM_STOP;

	return DIENUM_CONTINUE;
}

bool di_joystick::input()
{
	if(!pDevice_)
	{
		logger::warnln("[di_joystick]ジョイスティックが初期化されていません。");
		return false;
	}

	HRESULT r;
	// デバイス獲得してなかったら獲得する
	if(!bAcquire_)
	{
		r = pDevice_->Acquire();
		if(FAILED(r))
		{// 獲得できなかたっら次の機会へ
			//logger::warnln("[di_joystick]Lost: " + to_str(r));
			curbuf_clear();
			return false;
		}

		bAcquire_=true;
	}

	// 入力ガードが有効だったらPollしない
	if(is_guard()) return true;

	// 入力データを取得
	nCurIndex_ ^= 1;

	// ポーリング
	r = pDevice_->Poll();
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
			// 改めてPoll
			r = pDevice_->Poll();
			if(FAILED(r))
			{
				curbuf_clear();
				return false;
			}
		}
	}

	// データ取得
	r = pDevice_->GetDeviceState(sizeof(DIJOYSTATE), &joyBuf_[nCurIndex_]);
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
			r = pDevice_->GetDeviceState(sizeof(DIJOYSTATE), &joyBuf_[nCurIndex_]);
			if(FAILED(r))
			{
				curbuf_clear();
				return false;
			}
		}
	}

	return true;
}

void di_joystick::guard(bool bGuard)
{
	bGuard_ = bGuard;
	if(bGuard_) buf_clear();
}

//////////////////////////////
// 軸
/////////////////////////////
namespace{
// スレッショルドの中にあるかをチェック
inline bool is_thr_inner(int32_t n, uint32_t nThreshould)
{
	int32_t nThr = static_cast<int32_t>(nThreshould);
	return -nThr<=n && n<=nThr;
}

inline float calc_axis_ratio(int32_t n, uint32_t nThreshould)
{
	float r = static_cast<float>(::abs(n)-nThreshould)/static_cast<float>(di_joystick::MAX_AXIS_VALUE-nThreshould);
	return n<0 ? -r : r;
}
} // namespace end

float di_joystick::x()const
{
	int32_t nX = joyBuf_[nCurIndex_].lX;

	if(is_thr_inner(nX, nX_Threshold_)) return 0.f;
	return calc_axis_ratio(nX, nX_Threshold_);
}

float di_joystick::y()const
{
	int32_t nY = joyBuf_[nCurIndex_].lY;

	if(is_thr_inner(nY, nY_Threshold_)) return 0.f;
	return calc_axis_ratio(nY, nY_Threshold_);
}

float di_joystick::z()const
{
	int32_t nZ = joyBuf_[nCurIndex_].lZ;
	
	if(is_thr_inner(nZ, nZ_Threshold_)) return 0.f;
	return calc_axis_ratio(nZ, nZ_Threshold_);
}

float di_joystick::x_prev()const
{
	int32_t nX = joyBuf_[nCurIndex_^1].lX;

	if(is_thr_inner(nX, nX_Threshold_)) return 0.f;
	return calc_axis_ratio(nX, nX_Threshold_);
}

float di_joystick::y_prev()const
{
	int32_t nY = joyBuf_[nCurIndex_^1].lY;

	if(is_thr_inner(nY, nY_Threshold_)) return 0.f;
	return calc_axis_ratio(nY, nY_Threshold_);
}

float di_joystick::z_prev()const
{
	int32_t nZ = joyBuf_[nCurIndex_^1].lZ;
	
	if(is_thr_inner(nZ, nZ_Threshold_)) return 0.f;
	return calc_axis_ratio(nZ, nZ_Threshold_);
}

//////////////////////////////
// 回転
/////////////////////////////
float di_joystick::x_rot()const
{
	int32_t nXrot = joyBuf_[nCurIndex_].lRx;

	if(is_thr_inner(nXrot, nXrot_Threshold_)) return 0.f;
	return calc_axis_ratio(nXrot, nXrot_Threshold_);
}

float di_joystick::y_rot()const
{
	int32_t nYrot = joyBuf_[nCurIndex_].lRy;

	if(is_thr_inner(nYrot, nYrot_Threshold_)) return 0.f;
	return calc_axis_ratio(nYrot, nYrot_Threshold_);
}

float di_joystick::z_rot()const
{
	int32_t nZrot = joyBuf_[nCurIndex_].lRz;

	if(is_thr_inner(nZrot, nZrot_Threshold_)) return 0.f;
	return calc_axis_ratio(nZrot, nZrot_Threshold_);
}

float di_joystick::x_rot_prev()const
{
	int32_t nXrot = joyBuf_[nCurIndex_^1].lRx;

	if(is_thr_inner(nXrot, nXrot_Threshold_)) return 0.f;
	return calc_axis_ratio(nXrot, nXrot_Threshold_);
}

float di_joystick::y_rot_prev()const
{
	int32_t nYrot = joyBuf_[nCurIndex_^1].lRy;

	if(is_thr_inner(nYrot, nYrot_Threshold_)) return 0.f;
	return calc_axis_ratio(nYrot, nYrot_Threshold_);
}

float di_joystick::z_rot_prev()const
{
	int32_t nZrot = joyBuf_[nCurIndex_^1].lRz;

	if(is_thr_inner(nZrot, nZrot_Threshold_)) return 0.f;
	return calc_axis_ratio(nZrot, nZrot_Threshold_);
}

//////////////////////////////
// POV
/////////////////////////////
int32_t di_joystick::pov()const
{
	return joyBuf_[nCurIndex_].rgdwPOV[0];
}

int32_t di_joystick::pov_prev()const
{
	return joyBuf_[nCurIndex_^1].rgdwPOV[0];
}

bool di_joystick::is_pov_press(enum arrow eArrow)const
{
	if(is_ad_conv() 
	&& is_axis_press_inner(swap_pov_axis(eArrow), nCurIndex_))
		return true;

	return is_pov_press_inner(eArrow, nCurIndex_);
}

bool di_joystick::is_pov_push(enum arrow eArrow)const
{
	if( is_ad_conv()
	&&  is_axis_press_inner(swap_pov_axis(eArrow), nCurIndex_)
	&& !is_axis_press_inner(swap_pov_axis(eArrow), nCurIndex_^1))
		return true;

	return is_pov_press_inner(eArrow, nCurIndex_)
		&& !is_pov_press_inner(eArrow, nCurIndex_^1);
}

bool di_joystick::is_pov_release(enum arrow eArrow)const
{
	if( is_ad_conv()
	&& !is_axis_press_inner(swap_pov_axis(eArrow), nCurIndex_)
	&&  is_axis_press_inner(swap_pov_axis(eArrow), nCurIndex_^1))
		return true;

	return !is_pov_press_inner(eArrow, nCurIndex_)
		&&  is_pov_press_inner(eArrow, nCurIndex_^1);
}

bool di_joystick::is_pov_press_inner(enum arrow eArrow, uint32_t nIndex)const
{
	DWORD pov = joyBuf_[nIndex].rgdwPOV[0];

	// 斜め入力対応
	switch(eArrow)
	{
	case POV_NONE:	return LOWORD(pov)==0xFFFF;
	case POV_UP:	return pov==0     || pov==4500  || pov==31500;
	case POV_RIGHT:	return pov==9000  || pov==4500  || pov==13500;
	case POV_DOWN:	return pov==18000 || pov==13500 || pov==22500;
	case POV_LEFT:	return pov==27000 || pov==22500 || pov==31500;
	default:		return false;
	}
}

bool di_joystick::is_axis_press(enum arrow eArrow)const
{
	if(is_ad_conv() 
	&& is_pov_press_inner(swap_pov_axis(eArrow), nCurIndex_))
		return true;

	return is_axis_press_inner(eArrow, nCurIndex_);
}

bool di_joystick::is_axis_push(enum arrow eArrow)const
{
	if( is_ad_conv()
	&&  is_pov_press_inner(swap_pov_axis(eArrow), nCurIndex_)
	&& !is_pov_press_inner(swap_pov_axis(eArrow), nCurIndex_^1))
		return true;

	return  is_axis_press_inner(eArrow, nCurIndex_)
		&& !is_axis_press_inner(eArrow, nCurIndex_^1);
}

bool di_joystick::is_axis_release(enum arrow eArrow)const
{
	if( is_ad_conv()
	&& !is_pov_press_inner(swap_pov_axis(eArrow), nCurIndex_)
	&&  is_pov_press_inner(swap_pov_axis(eArrow), nCurIndex_^1))
		return true;

	return !is_axis_press_inner(eArrow, nCurIndex_)
		&&  is_axis_press_inner(eArrow, nCurIndex_^1);
}


bool di_joystick::is_axis_press_inner(enum arrow eArrow, uint32_t nIndex)const
{
	bool bCur = (nIndex==nCurIndex_);
	switch (eArrow)
	{
	case AXIS_NONE:	return (bCur ? x() : x_prev())==0.f && (bCur ? y() : y_prev())==0.f;
	case AXIS_UP:	return (bCur ? y() : y_prev())<0.f;
	case AXIS_RIGHT:return (bCur ? x() : x_prev())>0.f;
	case AXIS_DOWN:	return (bCur ? y() : y_prev())>0.f;
	case AXIS_LEFT:	return (bCur ? x() : x_prev())<0.f;
	default:		return false;
	}
}

enum di_joystick::arrow di_joystick::swap_pov_axis(enum arrow eArrow)const
{
	switch(eArrow)
	{
	case AXIS_NONE: return POV_NONE;
	case AXIS_UP:	return POV_UP;
	case AXIS_RIGHT:return POV_RIGHT;
	case AXIS_DOWN:	return POV_DOWN;
	case AXIS_LEFT:	return POV_LEFT;
	case POV_NONE:  return AXIS_NONE;
	case POV_UP:	return AXIS_UP;
	case POV_RIGHT: return AXIS_RIGHT;
	case POV_DOWN:  return AXIS_DOWN;
	case POV_LEFT:  return AXIS_LEFT;
	default:		return ARROW_END;
	}
}

//////////////////////////////
// ボタン
/////////////////////////////
bool di_joystick::is_press(uint32_t nBtn)const
{
	if(nBtn>=ARROW_END)	return false;
	if(nBtn>=AXIS_NONE) return is_axis_press(static_cast<enum arrow>(nBtn));
	if(nBtn>=POV_NONE)	return is_pov_press(static_cast<enum arrow>(nBtn));

	return (joyBuf_[nCurIndex_].rgbButtons[nBtn] & 0x80)>0;
}

bool di_joystick::is_push(uint32_t nBtn)const
{
	if(nBtn>=ARROW_END)	return false; 
	if(nBtn>=AXIS_NONE) return is_axis_push(static_cast<enum arrow>(nBtn));
	if(nBtn>=POV_NONE)	return is_pov_push(static_cast<enum arrow>(nBtn));

	return (joyBuf_[nCurIndex_].rgbButtons[nBtn] & 0x80)>0
		&& (joyBuf_[nCurIndex_^1].rgbButtons[nBtn] & 0x80)==0;
}

bool di_joystick::is_release(uint32_t nBtn)const
{
	if(nBtn>=ARROW_END)	return false;
	if(nBtn>=AXIS_NONE) return is_axis_release(static_cast<enum arrow>(nBtn));
	if(nBtn>=POV_NONE)	return is_pov_release(static_cast<enum arrow>(nBtn));

	return (joyBuf_[nCurIndex_].rgbButtons[nBtn] & 0x80)==0
		&& (joyBuf_[nCurIndex_^1].rgbButtons[nBtn] & 0x80)>0;
}

//////////////////////////////////////
// joystick_factory
/////////////////////////////////////
bool di_joystick_factory::init(HWND hWnd, const shared_ptr<di_driver>& pDriver, bool bBackGround)
{
	hWnd_		= hWnd;
	pAdapter_	= pDriver;
	bBackGround_= bBackGround;

	bool r = joyEnum_.init(*pAdapter_);
	if(r) mapJoy_.reserve(joyEnum_.enabled_joystick_num());
	return r;
}

void di_joystick_factory::fin()
{
	mapJoy_.clear();
}

di_joystick_ptr di_joystick_factory::joystick(uint32_t nNo)
{
	joy_map::iterator it = mapJoy_.find(nNo);
	if(it!=mapJoy_.end()) return it->second;

	device_inst_opt info = joyEnum_.joystick_info(nNo);
	if(!info)
	{
		logger::warnln("[joystic_manager]joystickが存在しません。:" + to_str(nNo));
		return std::move(di_joystick_ptr());
	}
	
	di_joystick_ptr pJoy = make_shared<di_joystick>();
	if(!pJoy->init(*info, *pAdapter_, hWnd_, bBackGround_))
		return std::move(di_joystick_ptr());
	
	auto r = mapJoy_.emplace(joy_map::value_type(nNo, pJoy));
	if(!r.second) return std::move(di_joystick_ptr());

	return r.first->second;
}

} // namespace input end
} // namespace mana end
