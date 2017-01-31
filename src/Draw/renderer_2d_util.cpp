#include "../mana_common.h"

#include "renderer_2d_util.h"

namespace mana{
namespace draw{

const uint32_t DEFAULT_MAX_SPRITE_NUM	= 512;
const float    DEFAULT_MAX_Z			= 100.0f;

bool is_mode_use_texture(draw_mode eMode)
{
	return eMode==MODE_TEX				|| eMode==MODE_TEX_BLEND 
		|| eMode==MODE_TEX_COLOR_THR	|| eMode==MODE_TEX_COLOR_THR_BLEND
		|| eMode==MODE_TEX_COLOR_MUL	|| eMode==MODE_TEX_COLOR_MUL_BLEND
		|| eMode==MODE_TEX_COLOR_SCREEN	|| eMode==MODE_TEX_COLOR_SCREEN_BLEND
		|| eMode==MODE_DF_COLOR			|| eMode==MODE_DF_COLOR_BLEND
		;
}

bool is_mode_use_blend(draw_mode eMode)
{
	return eMode==MODE_PLY_COLOR_BLEND 
		|| eMode==MODE_TEX_BLEND || eMode==MODE_TEX_COLOR_THR_BLEND
		|| eMode==MODE_TEX_COLOR_MUL_BLEND || eMode==MODE_TEX_COLOR_SCREEN_BLEND
		|| eMode==MODE_DF_COLOR_BLEND
		;
}

void d3d9_transform_world_sprite_pos(POS_VERTEX* verPos, const SIZE& size, const POS& pos, const SIZE& scale, float fRot, const POS& pivot)
{
	// ローカル座標
	D3DXVECTOR4 p[4];
	p[0].x = 0.0f;
	p[0].y = 0.0f;
	p[0].z = 0.0f;
	p[0].w = 1.0f;

	p[1].x = size.fWidth;
	p[1].y = 0.0f;
	p[1].z = 0.0f;
	p[1].w = 1.0f;

	p[2].x = 0.0f;
	p[2].y = size.fHeight;
	p[2].z = 0.0f;
	p[2].w = 1.0f;

	p[3].x = p[1].x;
	p[3].y = p[2].y;
	p[3].z = 0.0f;
	p[3].w = 1.0f;

	D3DXMATRIX m;

	// pivot
	D3DXMatrixTranslation(&m, -pivot.fX, -pivot.fY, 0.f);
	D3DXVec4Transform(&p[0],&p[0],&m);
	D3DXVec4Transform(&p[1],&p[1],&m);
	D3DXVec4Transform(&p[2],&p[2],&m);
	D3DXVec4Transform(&p[3],&p[3],&m);

	// scale
	D3DXMatrixScaling(&m, scale.fWidth, scale.fHeight, 1.0f);
	D3DXVec4Transform(&p[0],&p[0],&m);
	D3DXVec4Transform(&p[1],&p[1],&m);
	D3DXVec4Transform(&p[2],&p[2],&m);
	D3DXVec4Transform(&p[3],&p[3],&m);

	// rot
	D3DXMatrixRotationZ(&m, fRot*boost::math::constants::pi<float>()/180.0f);
	D3DXVec4Transform(&p[0],&p[0],&m);
	D3DXVec4Transform(&p[1],&p[1],&m);
	D3DXVec4Transform(&p[2],&p[2],&m);
	D3DXVec4Transform(&p[3],&p[3],&m);


	// pos
	D3DXMatrixTranslation(&m, pos.fX, pos.fY, 0.f);
	D3DXVec4Transform(&p[0],&p[0],&m);
	D3DXVec4Transform(&p[1],&p[1],&m);
	D3DXVec4Transform(&p[2],&p[2],&m);
	D3DXVec4Transform(&p[3],&p[3],&m);

	// 計算済み位置をコマンドに設定
	verPos[0].fX = p[0].x;
	verPos[0].fY = p[0].y;
	verPos[0].fZ = pos.fZ;
	
	verPos[1].fX = p[1].x;
	verPos[1].fY = p[1].y;
	verPos[1].fZ = pos.fZ;

	verPos[2].fX = p[2].x;
	verPos[2].fY = p[2].y;
	verPos[2].fZ = pos.fZ;

	verPos[3].fX = p[3].x;
	verPos[3].fY = p[3].y;
	verPos[3].fZ = pos.fZ;
}

void write_pos(float* pStartVertex, float fLeft, float fTop, float fRight, float fBottom, float fZ)
{
	// 左上
	pStartVertex[0] = fLeft;
	pStartVertex[1] = fTop;
	pStartVertex[2] = fZ;

	// 右上
	pStartVertex[3] = fRight;
	pStartVertex[4] = fTop;
	pStartVertex[5] = fZ;

	// 左下
	pStartVertex[6] = fLeft;
	pStartVertex[7] = fBottom;
	pStartVertex[8] = fZ;

	// 右下
	pStartVertex[9] = fRight;
	pStartVertex[10]= fBottom;
	pStartVertex[11]= fZ;
}

void write_uv(float* pStartVertex, float fLeft, float fTop, float fRight, float fBottom)
{
	// 左上
	pStartVertex[0] = fLeft;
	pStartVertex[1] = fTop;

	// 右上
	pStartVertex[2] = fRight;
	pStartVertex[3] = fTop;

	// 左下
	pStartVertex[4] = fLeft;
	pStartVertex[5] = fBottom;

	// 右下
	pStartVertex[6] = fRight;
	pStartVertex[7]= fBottom;
}

void write_color(DWORD* pStartVertex, DWORD nColor)
{
	pStartVertex[0] = nColor;
	pStartVertex[1] = nColor;
	pStartVertex[2] = nColor;
	pStartVertex[3] = nColor;
}

void write_color(DWORD* pStartVertex, DWORD nLTColor, DWORD nRTColor, DWORD nLBColor, DWORD nRBColor)
{
	pStartVertex[0] = nLTColor;
	pStartVertex[1] = nRTColor;
	pStartVertex[2] = nLBColor;
	pStartVertex[3] = nRBColor;
}

float* write_pos_vertex(float* pStartVertex, float fLeft, float fTop, float fRight, float fBottom, float fZ)
{
	write_pos(pStartVertex, fLeft, fTop, fRight, fBottom, fZ);
	return &pStartVertex[12];
}

float* write_uv_vertex(float* pStartVertex, float fLeft, float fTop, float fRight, float fBottom)
{
	write_uv(pStartVertex, fLeft, fTop, fRight, fBottom);
	return &pStartVertex[8];
}

DWORD* write_color_vertex(DWORD* pStartVertex, DWORD nColor)
{
	write_color(pStartVertex, nColor);
	return &pStartVertex[4];
}

DWORD* write_color_vertex(DWORD* pStartVertex, DWORD nLTColor, DWORD nRTColor, DWORD nLBColor, DWORD nRBColor)
{
	write_color(pStartVertex, nLTColor, nRTColor, nLBColor, nRBColor);
	return &pStartVertex[4];
}

} // namespace draw end
} // namespace mana end
