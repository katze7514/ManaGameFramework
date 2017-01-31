#include "../mana_common.h"

#include "d3d9_driver.h"

namespace mana{
namespace draw{

bool d3d9_driver::create_driver()
{
	// driver初期化
	if(pD3d9_)
	{
		logger::debugln("[d3d9_driver]D3D9 初期化済みです。");
		return true;
	}
	
	pD3d9_ = Direct3DCreate9(D3D_SDK_VERSION);
	if(pD3d9_==nullptr)
	{
		logger::fatalln("[d3d9_driver]D3D9の初期化に失敗しました。");
		return false;
	}

	// driver情報取得
	if(!caps_.check_driver(*this)
	&&  caps_.curdisp().Format==D3DFMT_UNKNOWN)
	{
		logger::warnln("[d3d9_driver]必要な描画情報が取得できませんでした。");
		return false;
	}
	
	logger::infoln("[d3d9_driver]D3D9初期化しました。");
	return true;
}

bool d3d9_driver::create_device(const d3d9_device_init& initParam)
{
	if(!pD3d9_)
	{
		logger::warnln("[d3d9_driver]D3D9が初期化されていません。");
		return false;
	}

	if(pD3d9device_)
	{
		logger::debugln("[d3d9_driver]D3D9 Device初期化済みです。");
		return true;
	}

	ZeroMemory(&d3dpp_, sizeof(d3dpp_));
	d3dpp_.BackBufferWidth				= initParam.nWidth_;
	d3dpp_.BackBufferHeight				= initParam.nHeight_;
	d3dpp_.BackBufferFormat				= caps_.curdisp().Format;
	d3dpp_.BackBufferCount				= 1;
	d3dpp_.SwapEffect					= D3DSWAPEFFECT_DISCARD;
	d3dpp_.hDeviceWindow				= initParam.hWnd_;
	d3dpp_.Windowed						= initParam.bWindowMode_;
	d3dpp_.EnableAutoDepthStencil		= FALSE;
	d3dpp_.AutoDepthStencilFormat		= D3DFMT_UNKNOWN;
	d3dpp_.FullScreen_RefreshRateInHz	= D3DPRESENT_RATE_DEFAULT;
	d3dpp_.PresentationInterval			= D3DPRESENT_INTERVAL_IMMEDIATE;

	// バックバッファカラー
	if(initParam.nColor_==32)
	{
		d3dpp_.BackBufferFormat = D3DFMT_X8R8G8B8;
		logger::infoln("[d3d9_driver]X8R8G8B8フォーマットを選択しました。");
	}
	else if(initParam.nColor_==16)
	{
		if(caps_.is_backbuf_format(D3DDEVTYPE_HAL, D3DFMT_R5G6B5, initParam.bWindowMode_)
		&& caps_.is_backbuf_format(D3DDEVTYPE_REF, D3DFMT_R5G6B5, initParam.bWindowMode_))
		{
			d3dpp_.BackBufferFormat = D3DFMT_R5G6B5;
			logger::infoln("[d3d9_driver]R5G6B5フォーマットを選択しました。");
		}
		else
		{
			d3dpp_.BackBufferFormat = D3DFMT_X1R5G5B5;
			logger::infoln("[d3d9_driver]X1R5G5B5フォーマットを選択しました。");
		}
	}

	// デプスステンシル
	switch(initParam.nDepth_)
	{
	case 32:
		d3dpp_.AutoDepthStencilFormat = D3DFMT_D32;
	break;

	case 24:
		if(initParam.bStencil_)
			d3dpp_.AutoDepthStencilFormat = D3DFMT_D24S8;
		else
			d3dpp_.AutoDepthStencilFormat = D3DFMT_D24X8;
	break;

	case 16:
		d3dpp_.AutoDepthStencilFormat = D3DFMT_D16;
	break;

	case 15:
		if(initParam.bStencil_)
			d3dpp_.AutoDepthStencilFormat = D3DFMT_D15S1;
		else
			d3dpp_.AutoDepthStencilFormat = D3DFMT_D16;	// ステンシル指定がないなら16bit扱い
	break;

	default: break;
	}

	if(d3dpp_.AutoDepthStencilFormat!=D3DFMT_UNKNOWN)
		d3dpp_.EnableAutoDepthStencil = TRUE;

	// 垂直同期
	if(initParam.bVsync_)
		d3dpp_.PresentationInterval	= D3DPRESENT_INTERVAL_ONE;

	// マルチサンプル設定
	if(initParam.nMultiSample_>1)
	{
		uint32_t multisample = initParam.nMultiSample_;
		if(multisample>16) multisample = 16;

		d3dpp_.MultiSampleType		= static_cast<D3DMULTISAMPLE_TYPE>(multisample);
		d3dpp_.MultiSampleQuality	= initParam.nMultiSampleQuality_;
	}

	// デバイスの作成
	if(!create_device_helper(D3DDEVTYPE_HAL, D3DCREATE_HARDWARE_VERTEXPROCESSING))
	{
		if(!create_device_helper(D3DDEVTYPE_HAL, D3DCREATE_SOFTWARE_VERTEXPROCESSING))
		{
			if(!create_device_helper(D3DDEVTYPE_REF, D3DCREATE_HARDWARE_VERTEXPROCESSING))
			{
				if(!create_device_helper(D3DDEVTYPE_REF, D3DCREATE_SOFTWARE_VERTEXPROCESSING))
				{
					logger::fatalln("[d3d9_driver]D3D9 Deviceの初期化に失敗しました。");
					return false;
				}
			}
		}
	}

	// device情報取得
	caps_.check_device(*this);

	nBeginCount_ = 0;

	return true;
}

bool d3d9_driver::create_device_helper(D3DDEVTYPE type, DWORD nFlags)
{
	// リリース時はPUREDEVICEにしようと思ったら、AMDドライバーだと使えないフラグらしい
	//#ifndef MANA_DEBUG	
	//	nFlags |= D3DCREATE_PUREDEVICE;
	//#endif

	if(SUCCEEDED(pD3d9_->CreateDevice(D3DADAPTER_DEFAULT, type, d3dpp_.hDeviceWindow, nFlags, &d3dpp_, &pD3d9device_)))
	{
		deviceType_ = type;
		nDeviceFlags_ = nFlags;

		const string sType  = type==D3DDEVTYPE_HAL ? "TYPE_HAL" : "TYPE_REF";
		const string sFlags = nFlags==D3DCREATE_HARDWARE_VERTEXPROCESSING ? "HARDWARE_VERTEXPROCESSING" : "SOFTWARE_VERTEXPROCESSING";
		logger::infoln("[d3d9_driver]D3D9 Deviceを作成しました。: " + sType + " " + sFlags);

		return true;
	}

	return false;
}

void d3d9_driver::fin()
{
	safe_release(pD3d9device_);
	safe_release(pD3d9_);
}

bool d3d9_driver::begin_scene()
{
	if(!pD3d9device_ || bDeviceLost_) return false;

	if(nBeginCount_==0)
	{
		HRESULT r = pD3d9device_->BeginScene();
		if(!check_hresult(r,"[d3d9_driver]BeginScene fail")) return false;
	}

	++nBeginCount_;
	return true;
}

bool d3d9_driver::end_scene()
{
	if(!pD3d9device_ || nBeginCount_==0) return false;

	--nBeginCount_;
	if(nBeginCount_==0)
	{
		pD3d9device_->EndScene();
		HRESULT r = pD3d9device_->Present(NULL,NULL,NULL,NULL);
		if(!check_hresult(r))
		{
			if(r==D3DERR_DEVICELOST)
			{
				logger::infoln("[d3d9_driver]DeviceLostしました。");
				bDeviceLost_ = true;
			}
			return false;
		}
	}

	return true;
}

d3d9_driver::reset_result d3d9_driver::reset_device(bool bForce)
{
	if(!pD3d9device_) return RESET_FAIL;
	
	HRESULT r = pD3d9device_->TestCooperativeLevel();
	if(r == D3DERR_DEVICENOTRESET || bForce)
	{
		r = pD3d9device_->Reset(&d3dpp_);
		if(!check_hresult(r,"[d3d9_driver]DeviceResetが出来ませんでした。"))
		{
			if(r!=D3DERR_DEVICELOST)
				return RESET_FATAL;
			else
				return RESET_FAIL;
		}

		bDeviceLost_ = false;

		logger::infoln("[d3d9_driver]DeviceResetしました。");
		return RESET_SUCCESS;
	}

	if(SUCCEEDED(r)) return RESET_SUCCESS;

	return RESET_FAIL;
}

} // namespace draw end
} // namespace mana end
