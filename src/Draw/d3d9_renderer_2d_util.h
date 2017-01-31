#pragma once

#include "renderer_2d_util.h"

namespace mana{
namespace draw{

//! d3d9_renderer_2d 初期化構造
struct d3d9_renderer_2d_init
{
	float		fMaxZ_;						//!< Zの最大値。デフォルトは100.0f
	uint32_t	nRenderWidth_;				//!< 描画領域の幅。ウインドウと描画領域サイズが違う時に指定する。0だとウインドウサイズと同じになる
	uint32_t	nRenderHeight_;				//!< 描画領域の高さ。ウインドウと描画領域サイズが違う時に指定する。0だとウインドウサイズと同じになる
	uint32_t	nMaxDrawSpriteNum_;			//!< 1レンダーで使う最大スプライト数。×4の頂点を確保する
	uint32_t	nMaxDrawCallPrimitiveNum_;	//!< 1回のDrawIndexedPrimitiveで描画できるスプライト数。この数に基づいてインデックスバッファが確保される

	DWORD		nBackgroundColor_;			//!< 背景カラー

	uint32_t	nReserveTextureNum_;		//!< 使用するテクスチャ枚数の推定値(越えても大丈夫)
	uint32_t	nTextureMaxVRAM_;			//!< テクスチャ容量制限(byte)。0だと制限なし

	uint32_t	nReserveFontNum_;			//!< 使用するフォントの推定数

	d3d9_renderer_2d_init():fMaxZ_(DEFAULT_MAX_Z),nRenderWidth_(0),nRenderHeight_(0),
							nMaxDrawSpriteNum_(DEFAULT_MAX_SPRITE_NUM),nMaxDrawCallPrimitiveNum_(DEFAULT_MAX_SPRITE_NUM),
							nBackgroundColor_(0xFF000000),
							nReserveTextureNum_(10),nTextureMaxVRAM_(0),
							nReserveFontNum_(5){}
};

} // namespace draw end
} // namespace mana end
