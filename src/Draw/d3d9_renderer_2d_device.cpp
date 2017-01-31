#include "../mana_common.h"

#include "d3d9_renderer_2d.h"

namespace mana{
namespace draw{

void d3d9_renderer_2d::device_reset(bool bFullScreen, int32_t nBackWidth, int32_t nBackHeight)
{
	D3DPRESENT_PARAMETERS& param = driver()->present_params();
	param.Windowed			= bFullScreen ? FALSE : TRUE;
	param.BackBufferWidth	= nBackWidth;
	param.BackBufferHeight	= nBackHeight;

	// リセットフラグ立てておく
	bResetDevice_.store(true, std::memory_order_release);
}

bool d3d9_renderer_2d::is_device_lost()const
{
	if(!driver()) return false;
	return is_device_reset() || driver()->is_device_lost();
}

d3d9_renderer_2d::device_lost_result d3d9_renderer_2d::device_lost()
{
	switch(eDlState_)
	{
	default:
	{// 本当にデバイスロストしてるか改めて確認
		HRESULT r = device()->TestCooperativeLevel();
		if(FAILED(r) || is_device_reset()) // 失敗したらデバイスロスト中
		{// 一端、いろいろ解放
			safe_release(pVertexDecl_)
			safe_release(pPosVertex_);
			safe_release(pUvVertex_);
			safe_release(pColorVertex_);
			safe_release(pColor2Vertex_);
			safe_release(pIndexBuffer_);

			HRESULT r = pEffect_->OnLostDevice();
			if(!check_hresult(r, "[d3d9_render_2d]EffectのOnLostDeviceが失敗しました。")) return DL_FATAL;

			safe_release(pRenderPosVertex_);
			safe_release(pRenderUvVertex_);
			safe_release(pRenderColorVertex_);
			safe_release(pRenderTarget_);
			safe_release(pRenderTexture_);

			tex_manager().device_lost();

			safe_release(pBackBuf_);

			eDlState_ = DEVLOST_RESET_WAIT;
			return DL_PROC;
		}

		return DL_SUCCESS;
	}
	break;

	case DEVLOST_RESET_WAIT: 
		switch(driver()->reset_device(is_device_reset()))
		{
		case d3d9_driver::RESET_SUCCESS:
		{// 再構築

			if(!init_vertex() 
			|| !init_render_target())
				return DL_FATAL;

			HRESULT r = pEffect_->OnResetDevice();
			if(!check_hresult(r, "[d3d9_render_2d]EffectのOnResetDeviceが失敗しました。")) return DL_FATAL;

			hTechnique_		= pEffect_->GetTechniqueByName("base");
			hMatPerPixel_	= pEffect_->GetParameterByName(NULL,"matPerPixel");
			hTex_			= pEffect_->GetParameterByName(NULL,"tex");

			init_render();

			eDlState_ = DEVLOST_NO;
			bResetDevice_.store(false, std::memory_order_release);
		}
		return DL_SUCCESS;

		case d3d9_driver::RESET_FATAL:
			eDlState_ = DEVLOST_FATAL;
			bResetDevice_.store(false, std::memory_order_release);
		return DL_FATAL;

		default: return DL_PROC;
		}
	break;

	case DEVLOST_FATAL:
		return DL_FATAL;
	break;
	}
}

} // namespace draw end
} // namespace mana end
