#include "../mana_common.h"

#include "d3d9_renderer_2d.h"

#include "renderer_2d_cmd.h"
#include "renderer_2d_async.h"
#include "renderer_2d_sync.h"

#include "renderer_2d.h"

namespace mana{
namespace draw{
renderer_2d::renderer_2d():pRenderer_(new_ d3d9_renderer_2d())
{
#ifdef MANA_DEBUG
	bReDefine_ = false;
#endif
}

renderer_2d::~renderer_2d()
{
	safe_delete(pRenderer_);
}

bool renderer_2d::init_driver(const shared_ptr<d3d9_driver>& pDriver, renderer_2d_init& renderer, const string& sScreenShotHeader)
{
	if(!pRenderer_->init(pDriver, renderer)) return false;
	pRenderer_->set_clear_color(renderer.nBackgroundColor_);
	pRenderer_->set_screenshot_header(sScreenShotHeader);
	return true;
}

bool renderer_2d::init_driver(d3d9_device_init& device, renderer_2d_init& renderer, const string& sScreenShotHeader)
{
	if(!pRenderer_->init(device, renderer)) return false;
	pRenderer_->set_clear_color(renderer.nBackgroundColor_);
	pRenderer_->set_screenshot_header(sScreenShotHeader);
	return true;
}

optional<uint32_t> renderer_2d::texture_id(const string& sID)
{
	return pRenderer_->tex_manager().texture_id(sID);
}

void renderer_2d::device_reset(bool bFullScreen, int32_t nBackWidth, int32_t nBackHeight, const renderer_2d::reset_handler& resetHandler)
{
	resetHandler_ = resetHandler;
	pRenderer_->device_reset(bFullScreen, nBackWidth, nBackHeight);
}

///////////////////////////////
// 2Dレンダラーリクエストヘルパー
void renderer_2d::request_screen_shot()
{
	cmd::screen_ctrl_cmd cmd;
	cmd.eCtrl_	= cmd::screen_ctrl_cmd::CTRL_SCEEN_SHOT;

	request(cmd);
}


void renderer_2d::request_screen_color(DWORD nColor)
{
	cmd::screen_ctrl_cmd cmd;
	cmd.eCtrl_	= cmd::screen_ctrl_cmd::CTRL_SCREEN_COLOR;
	cmd.nColor_	= nColor;

	request(cmd);
}

void renderer_2d::request_clear_color(DWORD nColor)
{
	cmd::screen_ctrl_cmd cmd;
	cmd.eCtrl_	= cmd::screen_ctrl_cmd::CTRL_CLEAR_COLOR;
	cmd.nColor_	= nColor;

	request(cmd);
}

void renderer_2d::request_tex_info_load(const string& sFilePath, const function<void (uint32_t)>& callback, bool bFile)
{
	cmd::info_load_cmd cmd;
	cmd.eKind_		= cmd::kind::KIND_TEX;
	cmd.sFilePath_	= sFilePath;
	cmd.bFile_		= bFile;
	cmd.callback_	= callback;

#ifdef MANA_DEBUG
	cmd.bReDefine_ = bReDefine_;
#endif

	request(cmd);
}

void renderer_2d::request_font_info_load(const string& sID, const string& sFilePath, const function<void (uint32_t)>& callback, bool bFile)
{
	cmd::info_load_cmd cmd;
	cmd.eKind_		= cmd::kind::KIND_FONT;
	cmd.sID_		= sID;
	cmd.sFilePath_	= sFilePath;
	cmd.bFile_		= bFile;
	cmd.callback_	= callback;

	request(cmd);
}

void renderer_2d::request_tex_info_remove(uint32_t nID)
{
	cmd::info_remove_cmd cmd;
	cmd.eKind_	= cmd::kind::KIND_TEX;
	cmd.id_	= nID;

	request(cmd);
}

void renderer_2d::request_font_info_remove(const string_fw& sID)
{
	cmd::info_remove_cmd cmd;
	cmd.eKind_	= cmd::kind::KIND_FONT;
	cmd.id_	= sID;

	request(cmd);
}

void renderer_2d::request_tex_info_add(const string& sID, const string& sFilePath, const function<void(uint32_t)>& callback, uint32_t nWidth, uint32_t nHeight, uint32_t nFormat)
{
	cmd::tex_info_add_cmd cmd;
	cmd.sTextureID_	= sID;
	cmd.sFilePath_	= sFilePath;
	cmd.nWidth_		= nWidth;
	cmd.nHeight_	= nHeight;
	cmd.nFormat_	= nFormat;
	cmd.callback_	= callback;

#ifdef MANA_DEBUG
	cmd.bReDefine_ = bReDefine_;
#endif

	request(cmd);
}

void renderer_2d::request_tex_info_remove_group(const string& sGroup)
{
	cmd::tex_group_cmd cmd;
	cmd.eCtrl_	= cmd::tex_group_cmd::REMOVE;
	cmd.sGroup_	= sGroup;

	request(cmd);
}

void renderer_2d::request_tex_release_group(const string& sGroup)
{
	cmd::tex_group_cmd cmd;
	cmd.eCtrl_	= cmd::tex_group_cmd::RELEASE;
	cmd.sGroup_	= sGroup;

	request(cmd);
}

///////////////////////////////
// 2Dレンダラー生成関数
renderer_2d_sptr create_renderer_2d(renderer_2d_kind eKind)
{
	switch(eKind)
	{
	case RENDERER_SYNC: return std::move(make_shared<renderer_2d_sync>());
	default:			return std::move(make_shared<renderer_2d_async>());
	}
}

} // namespace draw end
} // namespace mana end
