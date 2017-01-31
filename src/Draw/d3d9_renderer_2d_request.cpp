/*
 * d39d_2d_renderクラスのテクスチャ登録や描画リクエストに関わるメソッド実装
 */
#include "../mana_common.h"

#include "renderer_2d_util.h"
#include "d3d9_renderer_2d.h"

namespace mana{
namespace draw{

bool d3d9_renderer_2d::add_render_target(uint32_t nID, uint32_t nPriority, uint32_t nReserveSpriteCmdNum)
{
	if(nID==cmd::BACK_BUFFER_ID) return false;

	if(!tex_manager().is_texture_info(nID))
	{
		logger::warnln("[d3d9_renderer_2d]登録されていないレンダーターゲットです。: " + tex_manager().texture_id(nID));
		return false;
	}

	return render_cmd_queue().add_cmd(nID, nPriority, nReserveSpriteCmdNum);
}


///////////////////////////////////
// 描画リクエストヘルパー
///////////////////////////////////
#pragma warning(disable:4244)
bool d3d9_renderer_2d::add_render_target(const string& sID, uint32_t nPriority, uint32_t nReserveSpriteCmdNum)
{ 
	optional<uint32_t> id = tex_manager().texture_id(sID);
	if(id)
		return add_render_target(*id, nPriority, nReserveSpriteCmdNum); 
	else
		return false;
}

bool d3d9_renderer_2d::draw_sprite(const string& sTextureID, POS pos, bool bBlend, uint32_t nRenderTarget)
{
	sprite_param cmd;

	// Tex
	optional<uint32_t> id = tex_manager().texture_id(sTextureID);
	if(!id) return false; // テクスチャが存在しない
	cmd.nTextureID_ = *id;
	//UV
	const D3DXIMAGE_INFO& img = tex_manager().texture_image_info(sTextureID);
	cmd.set_uv_rect(0, 0, img.Width, img.Height);
	// POS
	cmd.set_pos_rect(pos.fX, pos.fY, pos.fX+img.Width, pos.fY+img.Height, pos.fZ);
	// MODE
	if(bBlend) cmd.mode_ = MODE_TEX_COLOR_BLEND_BLEND;
	else       cmd.mode_ = MODE_TEX_COLOR_BLEND;
	
	return draw_sprite(cmd, nRenderTarget);
}

bool d3d9_renderer_2d::draw_sprite(const string& sTextureID, POS pos, bool bBlend, SIZE scale, float fRot, POS pivot, uint32_t nRenderTarget)
{
	sprite_param cmd;

	// Tex
	optional<uint32_t> id = tex_manager().texture_id(sTextureID);
	if(!id) return false; // テクスチャが存在しない
	cmd.nTextureID_ = *id;
	// UV
	const D3DXIMAGE_INFO& img = tex_manager().texture_image_info(sTextureID);
	cmd.set_uv_rect(0, 0, img.Width, img.Height);
	// POS
	d3d9_transform_world_sprite_pos(cmd.pos_, SIZE(img.Width, img.Height), pos, scale, fRot, pivot); 
	// MODE
	if(bBlend) cmd.mode_ = MODE_TEX_COLOR_BLEND_BLEND;
	else       cmd.mode_ = MODE_TEX_COLOR_BLEND;

	return draw_sprite(cmd, nRenderTarget);
}

bool d3d9_renderer_2d::draw_sprite(const string& sTextureID, RECT rectTex, POS pos, bool bBlend, SIZE scale, float fRot, POS pivot, uint32_t nRenderTarget)
{
	sprite_param cmd;

	// Tex
	optional<uint32_t> id = tex_manager().texture_id(sTextureID);
	if(!id) return false; // テクスチャが存在しない
	cmd.nTextureID_ = *id;
	// UV
	cmd.set_uv_rect(rectTex.fLeft, rectTex.fTop, rectTex.fRight, rectTex.fBottom);
	// POS
	d3d9_transform_world_sprite_pos(cmd.pos_, SIZE(rectTex.fRight-rectTex.fLeft, rectTex.fBottom-rectTex.fTop), pos, scale, fRot, pivot); 
	// MODE
	if(bBlend) cmd.mode_ = MODE_TEX_COLOR_BLEND_BLEND;
	else       cmd.mode_ = MODE_TEX_COLOR_BLEND;

	return draw_sprite(cmd, nRenderTarget);
}

// DistanceFieldTextureで描画する
bool d3d9_renderer_2d::draw_df(const string& sDFTextureID, uint32_t nColor, POS pos, SIZE scale, float fRot, POS pivot, uint32_t nRenderTarget)
{
	sprite_param cmd;

	// Tex
	optional<uint32_t> id = tex_manager().texture_id(sDFTextureID);
	if(!id) return false; // テクスチャが存在しない
	cmd.nTextureID_ = *id;
	// UV
	const D3DXIMAGE_INFO& img = tex_manager().texture_image_info(sDFTextureID);
	cmd.set_uv_rect(0, 0, img.Width, img.Height);
	// POS
	d3d9_transform_world_sprite_pos(cmd.pos_, SIZE(img.Width, img.Height), pos, scale, fRot, pivot); 
	// COLOR
	cmd.set_color(nColor);
	// MODE
	cmd.mode_ = MODE_DF_COLOR;

	return draw_sprite(cmd, nRenderTarget);
}

#pragma warning(default:4244) 

} // namespace mana end
} // bamespace draw end
