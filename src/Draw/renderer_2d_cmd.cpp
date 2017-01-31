#include "../mana_common.h"

#include "d3d9_renderer_2d.h"
#include "renderer_2d_cmd.h"

namespace mana{
namespace draw{
namespace cmd{

/////////////////////////////////
// リクエスト処理vistor
/////////////////////////////////
void render_2d_cmd_exec::operator()(screen_ctrl_cmd& cmd)const
{
	switch(cmd.eCtrl_)
	{
	case cmd::screen_ctrl_cmd::CTRL_SCEEN_SHOT:
		renderer_.write_screenshot_frame();
	break;

	case cmd::screen_ctrl_cmd::CTRL_CLEAR_COLOR:
		renderer_.set_clear_color(cmd.nColor_);
	break;

	case cmd::screen_ctrl_cmd::CTRL_SCREEN_COLOR:
		renderer_.set_screen_color(cmd.nColor_);
	break;
	}
}

void render_2d_cmd_exec::operator()(info_load_cmd& cmd)const
{
	switch(cmd.eKind_)
	{
	case cmd::KIND_TEX:
	{
	#ifdef MANA_DEBUG
		renderer_.tex_manager().redefine(cmd.bReDefine_);
	#endif

		bool r = renderer_.tex_manager().load_texture_info_file(cmd.sFilePath_, cmd.bFile_);
		if(cmd.callback_) cmd.callback_(r?1:0);

	#ifdef MANA_DEBUG
		renderer_.tex_manager().redefine(false);
	#endif	

	}
	break;

	case cmd::KIND_FONT:
	{
		bool r = renderer_.text_renderer().load_font_info_file(cmd.sID_, renderer_.tex_manager(), cmd.sFilePath_, cmd.bFile_);
		if(cmd.callback_) cmd.callback_(r?1:0);
	}
	break;
	}
}
	
void render_2d_cmd_exec::operator()(info_remove_cmd& cmd)const
{
	switch(cmd.eKind_)
	{
	case cmd::KIND_TEX:
		renderer_.tex_manager().remove_texture_info(boost::get<uint32_t>(cmd.id_));
	break;

	case cmd::KIND_FONT:
		renderer_.text_renderer().remove_font_info(boost::get<string_fw>(cmd.id_));
	break;
	}
}

void render_2d_cmd_exec::operator()(tex_info_add_cmd& cmd)const
{
	auto& texMgr = renderer_.tex_manager();

#ifdef MANA_DEBUG
	texMgr.redefine(cmd.bReDefine_);
#endif

	if(cmd.sFilePath_.empty())
		texMgr.add_texture_info(cmd.sTextureID_, cmd.nWidth_, cmd.nHeight_, static_cast<D3DFORMAT>(cmd.nFormat_), cmd.sGroup_, cmd.release_handler_);
	else
		texMgr.add_texture_info(cmd.sTextureID_, cmd.sFilePath_, cmd.sGroup_);

	if(cmd.callback_)
	{
		auto n = texMgr.texture_id(cmd.sTextureID_);
		if(n) cmd.callback_(*n);
		else  cmd.callback_(0);
	}

#ifdef MANA_DEBUG
	texMgr.redefine(false);
#endif
}

void render_2d_cmd_exec::operator()(tex_group_cmd& cmd)const
{
	auto& texMgr = renderer_.tex_manager();

	switch(cmd.eCtrl_)
	{
	case tex_group_cmd::REMOVE:		texMgr.remove_texture_info_group(cmd.sGroup_);	break;
	case tex_group_cmd::RELEASE:	texMgr.release_texture_group(cmd.sGroup_);		break;
	}
}

void render_2d_cmd_exec::operator()(text_draw_cmd& cmd)const
{
	renderer_.draw_text(cmd);
}

void render_2d_cmd_exec::operator()(cmd::sprite_draw_cmd& cmd)const
{
	sprite_param sp;
	sp.nTextureID_ = cmd.nTexID_;
	sp.set_color(0,cmd.nColor0_);
	sp.set_color(1,cmd.nColor1_);
	sp.mode_ = cmd.mode_;

	if(cmd.nTexID_>0 && cmd.rect_.width()==0 && cmd.rect_.height()==0)
	{// サイズ0だったら、テクスチャサイズで描画する
		const D3DXIMAGE_INFO& img = renderer_.tex_manager().texture_image_info(cmd.nTexID_);
		cmd.rect_.fLeft= 0.f;
		cmd.rect_.fRight = static_cast<float>(img.Width);
		cmd.rect_.fTop = 0.f;
		cmd.rect_.fBottom = static_cast<float>(img.Height);
	}

	// UV。ここでは画像サイズのまま
	sp.set_uv_rect(cmd.rect_.fLeft, cmd.rect_.fTop, cmd.rect_.fRight, cmd.rect_.fBottom);

	// 頂点位置生成
	D3DXVECTOR4 p[4];

	p[0].x = 0.0f;
	p[0].y = 0.0f;
	p[0].z = 0.0f;
	p[0].w = 1.0f;

	p[1].x = cmd.rect_.width();
	p[1].y = 0.0f;
	p[1].z = 0.0f;
	p[1].w = 1.0f;

	p[2].x = 0.0f;
	p[2].y = cmd.rect_.height();
	p[2].z = 0.0f;
	p[2].w = 1.0f;

	p[3].x = p[1].x;
	p[3].y = p[2].y;
	p[3].z = 0.0f;
	p[3].w = 1.0f;

	//logger::traceln("**** start");
	RECT sprite(FLT_MAX,FLT_MAX,FLT_MIN,FLT_MIN);
	for(uint32_t i=0; i<4; ++i)
	{
		D3DXVec4Transform(&p[i],&p[i],&cmd.worldMat_);
		sp.pos_[i].fX = p[i].x;
		sp.pos_[i].fY = p[i].y;
		sp.pos_[i].fZ = cmd.fZ_;

		//logger::traceln(to_str_s(sp.pos_[i].fX)+to_str(sp.pos_[i].fY));

		if(sprite.fLeft >p[i].x) sprite.fLeft=p[i].x;
		if(sprite.fRight<p[i].x) sprite.fRight=p[i].x;

		if(sprite.fTop >p[i].y) sprite.fTop=p[i].y;
		if(sprite.fBottom<p[i].y) sprite.fBottom=p[i].y;
	}
	//logger::traceln("**** end");
	RECT window(0.f,0.f,static_cast<float>(renderer_.render_width()),static_cast<float>(renderer_.render_height()));

	// 4頂点とWindowサイズでAABB判定交差しなかったら描画しない
	if(!(	sprite.fLeft <=window.fRight  && sprite.fTop<=window.fBottom
		 && sprite.fRight>=window.fTop && sprite.fBottom>=window.fTop)) 
		return;

	renderer_.draw_sprite(sp, cmd.nRenderTarget_);
}

void render_2d_cmd_exec::operator()(polygon_draw_cmd& cmd)const
{
	sprite_param sp;
	sp.set_color(0,cmd.nColor0_);
	sp.set_color(1,cmd.nColor1_);
	sp.mode_ = cmd.mode_;

	sp.pos_[0] = cmd.vertex_[0];
	sp.pos_[1] = cmd.vertex_[1];
	sp.pos_[2] = cmd.vertex_[2];

	if(cmd.nVertexNum_==3)
		sp.pos_[3] = cmd.vertex_[0];
	else
		sp.pos_[3] = cmd.vertex_[3];

	renderer_.draw_sprite(sp, cmd.nRenderTarget_);
}

} // namespace cmd end
} // namespace draw end
} // namespace mana end
