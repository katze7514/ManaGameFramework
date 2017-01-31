#include "../mana_common.h"

#include "d3d9_driver.h"
#include "draw_caps.h"

namespace{
D3DDEVTYPE	aDevType[]	= { D3DDEVTYPE_HAL, D3DDEVTYPE_REF };
BOOL		aWin[]		= { TRUE, FALSE };
} // naemspace end

namespace mana{
namespace draw{

d3d9_draw_caps::d3d9_draw_caps()
{
	curDisp_.Format = D3DFMT_UNKNOWN;
}

////////////////////////////////
// ドライバー系情報調査
////////////////////////////////

bool d3d9_draw_caps::check_driver(d3d9_driver& adapter)
{
	bool success=true;

	success &= check_video_info(adapter);

	if(!check_cur_disp(adapter))
	{// 現在のディスプレイモードが取れなかったら移行は無意味
		return false;
	}

	success &= check_device_type(adapter);
	check_disp_mode(adapter);
	check_adapter_format(adapter);
	check_multisample(adapter);

	return success;
}

bool d3d9_draw_caps::check_video_info(d3d9_driver& adapter)
{
	LPDIRECT3D9 driver = adapter.driver();
	D3DADAPTER_IDENTIFIER9 id;
	if(driver->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &id)!=D3D_OK)
	{
		logger::warnln("[d3d9_draw_caps]ビデオカード情報を取得できませんでした。");
		return false;
	}

	videoInfo_.sName_		= id.Description;
	videoInfo_.nProduct_	= HIWORD(id.DriverVersion.HighPart);
	videoInfo_.nVer_		= LOWORD(id.DriverVersion.HighPart);
	videoInfo_.nSubVer_		= HIWORD(id.DriverVersion.LowPart);
	videoInfo_.nBuild_		= LOWORD(id.DriverVersion.LowPart);
	
	return true;
}

bool d3d9_draw_caps::check_cur_disp(d3d9_driver& adapter)
{
	LPDIRECT3D9 driver = adapter.driver();
	if(driver->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &curDisp_)==D3D_OK)
	{
		return true;
	}
	else
	{
		logger::warnln("[d3d9_draw_caps]現在のディスプレイモードが取得できませんでした");
		return false;
	}
}

bool d3d9_draw_caps::check_device_type(d3d9_driver& adapter)
{
	D3DFORMAT	aFormat[] = { D3DFMT_X8R8G8B8, D3DFMT_A8R8G8B8, D3DFMT_X1R5G5B5, D3DFMT_A1R5G5B5, D3DFMT_R5G6B5 };
	
	for(auto type : aDevType)
		for(auto format : aFormat)
			for(auto win : aWin)
				check_device_type_detail(adapter, type, format, win);

	vecBackbufFormat_.shrink_to_fit();

	return !vecBackbufFormat_.empty();
}

bool d3d9_draw_caps::check_device_type_detail(d3d9_driver& adapter, D3DDEVTYPE type, D3DFORMAT format, BOOL bWindow)
{
	LPDIRECT3D9 driver = adapter.driver();
	if(SUCCEEDED(driver->CheckDeviceType(D3DADAPTER_DEFAULT, type, curDisp_.Format, format, bWindow)))
	{
		vecBackbufFormat_.emplace_back(make_tuple(type, format, bWindow));
		return true;
	}
	else
	{
		return false;
	}
}

void d3d9_draw_caps::check_disp_mode(d3d9_driver& adapter)
{
	check_disp_mode_detail(curDisp_.Format,	adapter);
	vecDispMode_.shrink_to_fit();
}

void d3d9_draw_caps::check_disp_mode_detail(D3DFORMAT format, d3d9_driver& adapter)
{
	LPDIRECT3D9 driver	= adapter.driver();
	uint32_t	count	= driver->GetAdapterModeCount(D3DADAPTER_DEFAULT, format);
	
	 // Countが存在していたら使用可能フォーマット
	if(count>0)	setDispFormat_.insert(format);

	for(uint32_t i=0; i<count; ++i)
	{
		D3DDISPLAYMODE mode;
		if(SUCCEEDED(driver->EnumAdapterModes(D3DADAPTER_DEFAULT, format, i, &mode)))
			vecDispMode_.emplace_back(mode);
	}
}

void d3d9_draw_caps::check_adapter_format(d3d9_driver& adapter)
{
	for(auto type : aDevType)
	{
		// 深度ステンシル
		check_adapter_format_depth(adapter, type, D3DFMT_D16_LOCKABLE);
		check_adapter_format_depth(adapter, type, D3DFMT_D32);
		check_adapter_format_depth(adapter, type, D3DFMT_D15S1);
		check_adapter_format_depth(adapter, type, D3DFMT_D24S8);
		check_adapter_format_depth(adapter, type, D3DFMT_D24X8);
		check_adapter_format_depth(adapter, type, D3DFMT_D24X4S4);
		check_adapter_format_depth(adapter, type, D3DFMT_D32F_LOCKABLE);
		check_adapter_format_depth(adapter, type, D3DFMT_D16);

		// テクスチャ
		check_adapter_format_texture(adapter, type, D3DFMT_R8G8B8);
		check_adapter_format_texture(adapter, type, D3DFMT_A8R8G8B8);
		check_adapter_format_texture(adapter, type, D3DFMT_X8R8G8B8);
		check_adapter_format_texture(adapter, type, D3DFMT_R5G6B5);
		check_adapter_format_texture(adapter, type, D3DFMT_X1R5G5B5);
		check_adapter_format_texture(adapter, type, D3DFMT_A1R5G5B5);
		check_adapter_format_texture(adapter, type, D3DFMT_X1R5G5B5);
		check_adapter_format_texture(adapter, type, D3DFMT_A4R4G4B4);
		check_adapter_format_texture(adapter, type, D3DFMT_R3G3B2);
		check_adapter_format_texture(adapter, type, D3DFMT_A8);
		check_adapter_format_texture(adapter, type, D3DFMT_A8R3G3B2);
		check_adapter_format_texture(adapter, type, D3DFMT_X4R4G4B4);
		check_adapter_format_texture(adapter, type, D3DFMT_A2B10G10R10);
		check_adapter_format_texture(adapter, type, D3DFMT_A8B8G8R8);
		check_adapter_format_texture(adapter, type, D3DFMT_X8B8G8R8);
		check_adapter_format_texture(adapter, type, D3DFMT_G16R16);
		check_adapter_format_texture(adapter, type, D3DFMT_A2R10G10B10);
		check_adapter_format_texture(adapter, type, D3DFMT_A16B16G16R16);
		check_adapter_format_texture(adapter, type, D3DFMT_A8P8);
		check_adapter_format_texture(adapter, type, D3DFMT_P8);
		check_adapter_format_texture(adapter, type, D3DFMT_L8);
		check_adapter_format_texture(adapter, type, D3DFMT_L16);
		check_adapter_format_texture(adapter, type, D3DFMT_A8L8);
		check_adapter_format_texture(adapter, type, D3DFMT_A4L4);

		check_adapter_format_texture(adapter, type, D3DFMT_V8U8);
		check_adapter_format_texture(adapter, type, D3DFMT_Q8W8V8U8);
		check_adapter_format_texture(adapter, type, D3DFMT_V16U16);
		check_adapter_format_texture(adapter, type, D3DFMT_Q16W16V16U16);
		check_adapter_format_texture(adapter, type, D3DFMT_CxV8U8);

		check_adapter_format_texture(adapter, type, D3DFMT_L6V5U5);
		check_adapter_format_texture(adapter, type, D3DFMT_X8L8V8U8);
		check_adapter_format_texture(adapter, type, D3DFMT_A2W10V10U10);

		check_adapter_format_texture(adapter, type, D3DFMT_MULTI2_ARGB8);
		check_adapter_format_texture(adapter, type, D3DFMT_G8R8_G8B8);
		check_adapter_format_texture(adapter, type, D3DFMT_R8G8_B8G8);
		check_adapter_format_texture(adapter, type, D3DFMT_DXT1);
		check_adapter_format_texture(adapter, type, D3DFMT_DXT2);
		check_adapter_format_texture(adapter, type, D3DFMT_DXT3);
		check_adapter_format_texture(adapter, type, D3DFMT_DXT4);
		check_adapter_format_texture(adapter, type, D3DFMT_DXT5);
		check_adapter_format_texture(adapter, type, D3DFMT_UYVY);
		check_adapter_format_texture(adapter, type, D3DFMT_YUY2);

		check_adapter_format_texture(adapter, type, D3DFMT_R16F);
		check_adapter_format_texture(adapter, type, D3DFMT_G16R16F);
		check_adapter_format_texture(adapter, type, D3DFMT_A16B16G16R16F);

		check_adapter_format_texture(adapter, type, D3DFMT_R32F);
		check_adapter_format_texture(adapter, type, D3DFMT_G32R32F);
		check_adapter_format_texture(adapter, type, D3DFMT_A32B32G32R32F);
		
		// レンダーターゲット
		check_adapter_format_render(adapter, type, D3DFMT_R8G8B8);
		check_adapter_format_render(adapter, type, D3DFMT_A8R8G8B8);
		check_adapter_format_render(adapter, type, D3DFMT_X8R8G8B8);
		check_adapter_format_render(adapter, type, D3DFMT_R5G6B5);
		check_adapter_format_render(adapter, type, D3DFMT_X1R5G5B5);
		check_adapter_format_render(adapter, type, D3DFMT_A1R5G5B5);
		check_adapter_format_render(adapter, type, D3DFMT_X1R5G5B5);
		check_adapter_format_render(adapter, type, D3DFMT_A4R4G4B4);
		check_adapter_format_render(adapter, type, D3DFMT_R3G3B2);
		check_adapter_format_render(adapter, type, D3DFMT_A8);
		check_adapter_format_render(adapter, type, D3DFMT_A8R3G3B2);
		check_adapter_format_render(adapter, type, D3DFMT_X4R4G4B4);
		check_adapter_format_render(adapter, type, D3DFMT_A2B10G10R10);
		check_adapter_format_render(adapter, type, D3DFMT_A8B8G8R8);
		check_adapter_format_render(adapter, type, D3DFMT_X8B8G8R8);
		check_adapter_format_render(adapter, type, D3DFMT_G16R16);
		check_adapter_format_render(adapter, type, D3DFMT_A2R10G10B10);
		check_adapter_format_render(adapter, type, D3DFMT_A16B16G16R16);
		check_adapter_format_render(adapter, type, D3DFMT_A8P8);
		check_adapter_format_render(adapter, type, D3DFMT_P8);
		check_adapter_format_render(adapter, type, D3DFMT_L8);
		check_adapter_format_render(adapter, type, D3DFMT_L16);
		check_adapter_format_render(adapter, type, D3DFMT_A8L8);
		check_adapter_format_render(adapter, type, D3DFMT_A4L4);

		check_adapter_format_render(adapter, type, D3DFMT_V8U8);
		check_adapter_format_render(adapter, type, D3DFMT_Q8W8V8U8);
		check_adapter_format_render(adapter, type, D3DFMT_V16U16);
		check_adapter_format_render(adapter, type, D3DFMT_Q16W16V16U16);
		check_adapter_format_render(adapter, type, D3DFMT_CxV8U8);

		check_adapter_format_render(adapter, type, D3DFMT_L6V5U5);
		check_adapter_format_render(adapter, type, D3DFMT_X8L8V8U8);
		check_adapter_format_render(adapter, type, D3DFMT_A2W10V10U10);

		check_adapter_format_render(adapter, type, D3DFMT_MULTI2_ARGB8);
		check_adapter_format_render(adapter, type, D3DFMT_G8R8_G8B8);
		check_adapter_format_render(adapter, type, D3DFMT_R8G8_B8G8);
		check_adapter_format_render(adapter, type, D3DFMT_DXT1);
		check_adapter_format_render(adapter, type, D3DFMT_DXT2);
		check_adapter_format_render(adapter, type, D3DFMT_DXT3);
		check_adapter_format_render(adapter, type, D3DFMT_DXT4);
		check_adapter_format_render(adapter, type, D3DFMT_DXT5);
		check_adapter_format_render(adapter, type, D3DFMT_UYVY);
		check_adapter_format_render(adapter, type, D3DFMT_YUY2);

		check_adapter_format_render(adapter, type, D3DFMT_R16F);
		check_adapter_format_render(adapter, type, D3DFMT_G16R16F);
		check_adapter_format_render(adapter, type, D3DFMT_A16B16G16R16F);

		check_adapter_format_render(adapter, type, D3DFMT_R32F);
		check_adapter_format_render(adapter, type, D3DFMT_G32R32F);
		check_adapter_format_render(adapter, type, D3DFMT_A32B32G32R32F);
	}

	vecDepthStencilFromat_.shrink_to_fit();
	vecTextureFormat_.shrink_to_fit();
	setRenderTargetInfo_.shrink_to_fit();
}

void d3d9_draw_caps::check_adapter_format_depth(d3d9_driver& adapter, D3DDEVTYPE type, D3DFORMAT checkformat)
{
	LPDIRECT3D9 driver = adapter.driver();
	if(SUCCEEDED(driver->CheckDeviceFormat(D3DADAPTER_DEFAULT,    type,             curDisp_.Format,
										   D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, checkformat)))
	{
		vecDepthStencilFromat_.emplace_back(make_tuple(type,checkformat));
	}
}

void d3d9_draw_caps::check_adapter_format_texture(d3d9_driver& adapter, D3DDEVTYPE type, D3DFORMAT checkformat)
{
	LPDIRECT3D9 driver = adapter.driver();
	if(SUCCEEDED(driver->CheckDeviceFormat(D3DADAPTER_DEFAULT, type, curDisp_.Format, 0, D3DRTYPE_TEXTURE, checkformat)))
	{
		vecTextureFormat_.emplace_back(make_tuple(type,checkformat));
	}
}

void d3d9_draw_caps::check_adapter_format_render(d3d9_driver& adapter, D3DDEVTYPE type, D3DFORMAT checkformat)
{
	LPDIRECT3D9 driver = adapter.driver();
	render_target_info info;

	if(FAILED(driver->CheckDeviceFormat(D3DADAPTER_DEFAULT,    type,             curDisp_.Format,
									    D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, checkformat)))
	{// レンダーターゲットとして使えない 
		return;
	}

	// レンダーターゲットとして作成可能
	info.format_ = checkformat;

	if(SUCCEEDED(driver->CheckDeviceFormat(D3DADAPTER_DEFAULT, type, curDisp_.Format,
									       D3DUSAGE_RENDERTARGET|D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING,
										   D3DRTYPE_TEXTURE, checkformat)))
	{
		info.bAlpha_ = true; // αブレンドOK
	}

	if(SUCCEEDED(driver->CheckDeviceFormat(D3DADAPTER_DEFAULT, type, curDisp_.Format,
									       D3DUSAGE_RENDERTARGET|D3DUSAGE_QUERY_FILTER,
										   D3DRTYPE_TEXTURE, checkformat)))
	{
		info.bFilter_ = true; // フィルターOK
	}

	if(SUCCEEDED(driver->CheckDeviceFormat(D3DADAPTER_DEFAULT, type, curDisp_.Format,
									       D3DUSAGE_RENDERTARGET|D3DUSAGE_QUERY_VERTEXTEXTURE,
										   D3DRTYPE_TEXTURE, checkformat)))
	{
		info.bVertexTexture_ = true; // 頂点テクスチャOK
	}

	setRenderTargetInfo_.insert(info);
}

void d3d9_draw_caps::check_multisample(d3d9_driver& adapter)
{
	for(auto type : aDevType)
	{
		for(auto win : aWin)
		{
			// 深度ステンシル
			check_multisample_detail(adapter, type, D3DFMT_D16_LOCKABLE, win);
			check_multisample_detail(adapter, type, D3DFMT_D32, win);
			check_multisample_detail(adapter, type, D3DFMT_D15S1, win);
			check_multisample_detail(adapter, type, D3DFMT_D24S8, win);
			check_multisample_detail(adapter, type, D3DFMT_D24X8, win);
			check_multisample_detail(adapter, type, D3DFMT_D24X4S4, win);
			check_multisample_detail(adapter, type, D3DFMT_D32F_LOCKABLE, win);
			check_multisample_detail(adapter, type, D3DFMT_D16, win);

			// テクスチャ
			check_multisample_detail(adapter, type, D3DFMT_R8G8B8, win);
			check_multisample_detail(adapter, type, D3DFMT_A8R8G8B8, win);
			check_multisample_detail(adapter, type, D3DFMT_X8R8G8B8, win);
			check_multisample_detail(adapter, type, D3DFMT_R5G6B5, win);
			check_multisample_detail(adapter, type, D3DFMT_X1R5G5B5, win);
			check_multisample_detail(adapter, type, D3DFMT_A1R5G5B5, win);
			check_multisample_detail(adapter, type, D3DFMT_X1R5G5B5, win);
			check_multisample_detail(adapter, type, D3DFMT_A4R4G4B4, win);
			check_multisample_detail(adapter, type, D3DFMT_R3G3B2, win);
			check_multisample_detail(adapter, type, D3DFMT_A8, win);
			check_multisample_detail(adapter, type, D3DFMT_A8R3G3B2, win);
			check_multisample_detail(adapter, type, D3DFMT_X4R4G4B4, win);
			check_multisample_detail(adapter, type, D3DFMT_A2B10G10R10, win);
			check_multisample_detail(adapter, type, D3DFMT_A8B8G8R8, win);
			check_multisample_detail(adapter, type, D3DFMT_X8B8G8R8, win);
			check_multisample_detail(adapter, type, D3DFMT_G16R16, win);
			check_multisample_detail(adapter, type, D3DFMT_A2R10G10B10, win);
			check_multisample_detail(adapter, type, D3DFMT_A16B16G16R16, win);
			check_multisample_detail(adapter, type, D3DFMT_A8P8, win);
			check_multisample_detail(adapter, type, D3DFMT_P8, win);
			check_multisample_detail(adapter, type, D3DFMT_L8, win);
			check_multisample_detail(adapter, type, D3DFMT_L16, win);
			check_multisample_detail(adapter, type, D3DFMT_A8L8, win);
			check_multisample_detail(adapter, type, D3DFMT_A4L4, win);

			check_multisample_detail(adapter, type, D3DFMT_V8U8, win);
			check_multisample_detail(adapter, type, D3DFMT_Q8W8V8U8, win);
			check_multisample_detail(adapter, type, D3DFMT_V16U16, win);
			check_multisample_detail(adapter, type, D3DFMT_Q16W16V16U16, win);
			check_multisample_detail(adapter, type, D3DFMT_CxV8U8, win);

			check_multisample_detail(adapter, type, D3DFMT_L6V5U5, win);
			check_multisample_detail(adapter, type, D3DFMT_X8L8V8U8, win);
			check_multisample_detail(adapter, type, D3DFMT_A2W10V10U10, win);

			check_multisample_detail(adapter, type, D3DFMT_MULTI2_ARGB8, win);
			check_multisample_detail(adapter, type, D3DFMT_G8R8_G8B8, win);
			check_multisample_detail(adapter, type, D3DFMT_R8G8_B8G8, win);
			check_multisample_detail(adapter, type, D3DFMT_DXT1, win);
			check_multisample_detail(adapter, type, D3DFMT_DXT2, win);
			check_multisample_detail(adapter, type, D3DFMT_DXT3, win);
			check_multisample_detail(adapter, type, D3DFMT_DXT4, win);
			check_multisample_detail(adapter, type, D3DFMT_DXT5, win);
			check_multisample_detail(adapter, type, D3DFMT_UYVY, win);
			check_multisample_detail(adapter, type, D3DFMT_YUY2, win);

			check_multisample_detail(adapter, type, D3DFMT_R16F, win);
			check_multisample_detail(adapter, type, D3DFMT_G16R16F, win);
			check_multisample_detail(adapter, type, D3DFMT_A16B16G16R16F, win);

			check_multisample_detail(adapter, type, D3DFMT_R32F, win);
			check_multisample_detail(adapter, type, D3DFMT_G32R32F, win);
			check_multisample_detail(adapter, type, D3DFMT_A32B32G32R32F, win);
		}
	}

	vecMultiSampleInfo_.shrink_to_fit();
}

void d3d9_draw_caps::check_multisample_detail(d3d9_driver& adapter, D3DDEVTYPE type, D3DFORMAT checkformat, BOOL bWin)
{
	LPDIRECT3D9 driver = adapter.driver();

	D3DMULTISAMPLE_TYPE aMulti[] = 
	{
		D3DMULTISAMPLE_2_SAMPLES,	D3DMULTISAMPLE_3_SAMPLES,	D3DMULTISAMPLE_4_SAMPLES,	D3DMULTISAMPLE_5_SAMPLES,
		D3DMULTISAMPLE_6_SAMPLES,	D3DMULTISAMPLE_7_SAMPLES,	D3DMULTISAMPLE_8_SAMPLES,	D3DMULTISAMPLE_9_SAMPLES,
		D3DMULTISAMPLE_10_SAMPLES,	D3DMULTISAMPLE_11_SAMPLES,	D3DMULTISAMPLE_12_SAMPLES,	D3DMULTISAMPLE_13_SAMPLES,
		D3DMULTISAMPLE_14_SAMPLES,	D3DMULTISAMPLE_15_SAMPLES,	D3DMULTISAMPLE_16_SAMPLES
	};
	
	DWORD level;
	for(auto multi : aMulti)
	{
		if(SUCCEEDED(driver->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT, type, checkformat, bWin, multi, &level)))
		{
			vecMultiSampleInfo_.emplace_back(make_tuple(type, checkformat, bWin, multi, level-1));
		}
	}
}

////////////////////////////////
// デバイス系情報調査
////////////////////////////////

bool d3d9_draw_caps::check_device(d3d9_driver& adapter)
{
	bool success = true;
	LPDIRECT3DDEVICE9 device = adapter.device();

	// デバイス能力取得
	if(FAILED(device->GetDeviceCaps(&devCap_)))
	{
		logger::warnln("[d3d9_draw_cpas]デバイス能力を取得できませんでした。");
		success=false;
	}

	// テクスチャメモリ量取得
	videoInfo_.nTexRAM_ = device->GetAvailableTextureMem();

	return success;
}

////////////////////////////////
// 情報へのアクセサ
////////////////////////////////

bool d3d9_draw_caps::is_backbuf_format(D3DDEVTYPE type, D3DFORMAT format, BOOL bWin)
{
	return any_of_equal(vecBackbufFormat_, make_tuple(type, format, bWin));
}

bool d3d9_draw_caps::is_disp_format(D3DFORMAT format)
{
	flat_set<D3DFORMAT>::iterator it = setDispFormat_.find(format);
	return it!=setDispFormat_.end();
}

bool d3d9_draw_caps::is_depth_format(D3DDEVTYPE type, D3DFORMAT format)
{
	return any_of_equal(vecDepthStencilFromat_, make_tuple(type, format));
}

bool d3d9_draw_caps::is_textrue_format(D3DDEVTYPE type, D3DFORMAT format)
{
	return any_of_equal(vecTextureFormat_, make_tuple(type, format));
}

optional<DWORD> d3d9_draw_caps::is_multisample(D3DDEVTYPE type, D3DFORMAT format, BOOL bWin, D3DMULTISAMPLE_TYPE multi)
{
	for(auto elem : vecMultiSampleInfo_)
	{
		if(elem.get<0>()==type && elem.get<1>()==format && elem.get<2>()==bWin && elem.get<3>()==multi)
			return optional<DWORD>(elem.get<4>());
	}

	return optional<DWORD>();
}

optional<render_target_info> d3d9_draw_caps::find_render_target(D3DFORMAT format)
{
	flat_set<render_target_info>::iterator it = setRenderTargetInfo_.find(render_target_info(format));
	if(it==setRenderTargetInfo_.end()) return optional<render_target_info>();
	return optional<render_target_info>(*it);
}

} // namespace mana end
} // namespace draw end
