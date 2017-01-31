#pragma once

#include "draw_caps.h"

namespace mana{
namespace draw{

//! create_deviceに渡すパラメータ構造体
struct d3d9_device_init
{
	HWND		hWnd_;					//!< 関連づけるウインドウハンドル
	uint32_t	nWidth_, nHeight_;		//!< バックバッファの幅、高さ
	uint32_t	nColor_;				//!< バックバッファのカラー深度。32/16を指定する。それ以外だとディスプレイと同じ
	BOOL		bWindowMode_;			//!< ウインドウモードかどうか
	bool		bVsync_;				//!< 巣直同期をするか
	uint32_t	nDepth_;				//!< 深度バッファのbit数。32、24、16、15のいずれか。それ以外だと深度バッファを使わない
	bool		bStencil_;				//!< ステンシルバッファを使うかどうか。深度バッファが24、15の時に有効になる
	uint32_t	nMultiSample_;			//!< マルチサンプル数。2～16まで指定できる。それ以外だと無効
	uint32_t	nMultiSampleQuality_;	//!< マルチサンプルのクオリティ。0～3ぐらいまで

	d3d9_device_init(HWND hWnd, uint32_t nWidth, uint32_t nHeight)
		: hWnd_(hWnd),nWidth_(nWidth),nHeight_(nHeight),
			nColor_(0), bWindowMode_(TRUE), bVsync_(false),
			nDepth_(0), bStencil_(false),
			nMultiSample_(0), nMultiSampleQuality_(0){}
};

//! @brief Direct3Dのインターフェイスなどを管理するクラス
class d3d9_driver
{
public:
	enum reset_result
	{
		RESET_SUCCESS,
		RESET_FAIL,
		RESET_FATAL,
	};

public:
	d3d9_driver():pD3d9_(nullptr),pD3d9device_(nullptr),deviceType_(static_cast<D3DDEVTYPE>(0)),nDeviceFlags_(0),
				  nBeginCount_(0),bDeviceLost_(false){}
	~d3d9_driver(){ fin(); }

public:
	//! D3D9オブジェクト生成
	bool create_driver();
	
	//! デバイスの作成
	bool create_device(const d3d9_device_init& initParam);

	void fin();

	bool is_init()const{ return pD3d9_!=nullptr && pD3d9device_!=nullptr; }

	bool begin_scene();
	bool end_scene();

	bool is_device_lost()const{ return bDeviceLost_; }

	reset_result reset_device(bool bForce=false);

public:
	LPDIRECT3D9			driver(){ return pD3d9_; }
	LPDIRECT3DDEVICE9	device(){ return pD3d9device_;}

	const d3d9_draw_caps& draw_caps()const{ return caps_; }

	D3DPRESENT_PARAMETERS& present_params(){ return d3dpp_; }

	HWND		wnd_handle()const{ return d3dpp_.hDeviceWindow; }
	uint32_t	wnd_width()const{ return d3dpp_.BackBufferWidth; }
	uint32_t	wnd_height()const{ return d3dpp_.BackBufferHeight; }
	D3DFORMAT	wnd_format()const{ return d3dpp_.BackBufferFormat;}
	D3DFORMAT	wnd_depth_format()const{ return d3dpp_.AutoDepthStencilFormat; }

	D3DDEVTYPE	device_type()const{ return deviceType_; }
	DWORD		device_flag()const{ return nDeviceFlags_; }

private:
	bool create_device_helper(D3DDEVTYPE type, DWORD nFlags);

private:
	LPDIRECT3D9				pD3d9_;
	LPDIRECT3DDEVICE9		pD3d9device_;

	D3DPRESENT_PARAMETERS	d3dpp_; // デバイスロストの時に再利用する

	D3DDEVTYPE				deviceType_;
	DWORD					nDeviceFlags_;

	uint32_t				nBeginCount_;
	bool					bDeviceLost_;

	d3d9_draw_caps			caps_;

private:
	NON_COPIABLE(d3d9_driver);
};

typedef shared_ptr<d3d9_driver> d3d9_driver_sptr;

} // namespace draw end
} // namespace mana end
