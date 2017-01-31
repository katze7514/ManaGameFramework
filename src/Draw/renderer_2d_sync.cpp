#include "../mana_common.h"

#include "d3d9_renderer_2d.h"

#include "renderer_2d_cmd.h"
#include "renderer_2d_sync.h"

namespace mana{
namespace draw{

renderer_2d_sync::renderer_2d_sync():eRenderResult_(RENDER_SUCCESS){}

renderer_2d_sync::~renderer_2d_sync()
{
#ifdef MANA_RENDER_2D_CMD_COUNT
	logger::infoln("[render_2d_cmd_queue]積まれた最大リクエスト数 : " + to_str(nMaxCmd_));
#endif
	fin();
}

bool renderer_2d_sync::init_render(uint32_t nReserveRequestNum, uint32_t nAffinity)
{
#ifdef MANA_RENDER_2D_CMD_COUNT
	nMaxCmd_=0;
	nCurCmd_=0;
#endif

	eRenderResult_=RENDER_SUCCESS;

	logger::infoln("[render_2d_sync]同期版renderer_2d初期化しました");
	return true;
}

void renderer_2d_sync::fin()
{
	pRenderer_->fin();
}

void renderer_2d_sync::request(const cmd::render_2d_cmd& cmd)
{
#ifdef MANA_RENDER_2D_CMD_COUNT
	++nCurCmd_;
#endif

	// レンダラーにリクエストを積む
	boost::apply_visitor(cmd::render_2d_cmd_exec(*pRenderer_), const_cast<cmd::render_2d_cmd&>(cmd));
}

render_result renderer_2d_sync::render(bool bWait)
{
#ifdef MANA_RENDER_2D_CMD_COUNT
	if(nCurCmd_>nMaxCmd_) nMaxCmd_ = nCurCmd_;
	nCurCmd_=0;
#endif

	switch(eRenderResult_)
	{
	case RENDER_DEVICE_LOST:
	// デバイスロスト処理
		switch(pRenderer_->device_lost())
		{
		case d3d9_renderer_2d::DL_SUCCESS:
			eRenderResult_ = RENDER_SUCCESS;
			if(resetHandler_){ resetHandler_(true); resetHandler_.clear(); }
		break;

		case d3d9_renderer_2d::DL_FATAL:
			eRenderResult_ = RENDER_FATAL;
			if(resetHandler_){ resetHandler_(false); resetHandler_.clear(); }
		break;

		default:
			eRenderResult_ = RENDER_DEVICE_LOST;
		break;
		}
	break;

	case RENDER_SUCCESS:
	// 描画処理
		if(pRenderer_->begin_scene())
		{
			pRenderer_->render();
			pRenderer_->end_scene();
		}

		if(pRenderer_->is_device_lost())
			eRenderResult_ = RENDER_DEVICE_LOST;
		else
			eRenderResult_ = RENDER_SUCCESS;
	break;

	default:
		eRenderResult_ = RENDER_FATAL;
	break;
	}

	return eRenderResult_;
}

} // namespace draw end
} // namespace mana end
