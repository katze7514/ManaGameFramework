/*
 *  renderer_2d系で使うユーティリティー群
 */
#pragma once

namespace mana{
namespace draw{

extern const uint32_t	DEFAULT_MAX_SPRITE_NUM;
extern const float		DEFAULT_MAX_Z;

namespace cmd{
//! renderer_2dが予約してるテクスチャID
enum reserve_tex_id : uint32_t
{
	BACK_BUFFER_ID,	//!< バックバッファ
	RESERVE_TEX_ID_END,
};
} // namespace cmd

//! 描画モード
enum draw_mode : uint32_t
{
	MODE_PLY,					//!< ポリゴンのみ
	MODE_PLY_COLOR,				//!< ポリゴンのみ、カラーあり
	MODE_PLY_COLOR_BLEND,		//!< ポリゴンのみ、カラーあり、αブレンドあり
	MODE_TEX,					//!< テクスチャあり、αブレンドなし
	MODE_TEX_BLEND,				//!< テクスチャあり、αブレンドあり
	MODE_TEX_COLOR1,			//!< テクスチャあり、カラーを1つだけ取り通常α合成をする
	MODE_TEX_COLOR_THR,			//!< テクスチャあり、カラースルー、αブレンドなし
	MODE_TEX_COLOR_THR_BLEND,	//!< テクスチャあり、カラースルー、αブレンドあり
	MODE_TEX_COLOR_BLEND,		//!< テクスチャあり、カラーブレンド、αブレンドなし
	MODE_TEX_COLOR_BLEND_BLEND,	//!< テクスチャあり、カラーブレンド、αブレンドあり
	MODE_TEX_COLOR_MUL,			//!< テクスチャあり、カラー乗算、αブレンドなし
	MODE_TEX_COLOR_MUL_BLEND,	//!< テクスチャあり、カラー乗算、αブレンドあり
	MODE_TEX_COLOR_SCREEN,		//!< テクスチャあり、カラースクリーン、αブレンドなし
	MODE_TEX_COLOR_SCREEN_BLEND,//!< テクスチャあり、カラースクリーン、αブレンドあり
	MODE_DF_COLOR,				//!< ディスタンスフィールドテクスチャあり、デフューズカラーで描画される、αブレンドなし
	MODE_DF_COLOR_BLEND,		//!< ディスタンスフィールドテクスチャあり、デフューズカラーで描画される、αブレンドあり
	MODE_NONE,					//!< 無効値
};

//! テクスチャを使う描画モードかどうか
extern inline bool is_mode_use_texture(draw_mode eMode);

//! アルファブレンドを使う描画モードどうか
extern inline bool is_mode_use_blend(draw_mode eMode);

////////////////////////
//! renderer_2d 初期化構造体
////////////////////////
struct renderer_2d_init
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

	string		sShaderPath_;				//!< Shader/Effectファイルが格納されてるフォルダへのパス。末尾に\を忘れないように

	renderer_2d_init():fMaxZ_(DEFAULT_MAX_Z),nRenderWidth_(0),nRenderHeight_(0),
						nMaxDrawSpriteNum_(DEFAULT_MAX_SPRITE_NUM),nMaxDrawCallPrimitiveNum_(DEFAULT_MAX_SPRITE_NUM),
						nBackgroundColor_(0xFF000000),
						nReserveTextureNum_(128),nTextureMaxVRAM_(0),
						nReserveFontNum_(16){}
};

////////////////////////
//! renderer_2d のrenderメソッドの戻り値
////////////////////////
enum render_result
{
	RENDER_PROC,		//!< レンダー実行中
	RENDER_SUCCESS,		//!< 正常動作中
	RENDER_DEVICE_LOST,	//!< デバイスロスト発生
	RENDER_FATAL,		//!< 致命的なエラーが発生。描画続行不可能
};


////////////////////////
// 頂点バッファとやりとりするためのPOD構造体
////////////////////////
struct POS_VERTEX
{
	float fX,fY,fZ;
};

struct UV_VERTEX
{
	float fU,fV;
};

struct COLOR_VERTEX
{
	DWORD nColor;
};

////////////////////////
// Draw系メソッドなどで使う
////////////////////////
struct POS
{
	float fX,fY,fZ;
	POS(float x=0.0f, float y=0.0f, float z=0.0f):fX(x),fY(y),fZ(z){}
};

struct RECT
{
	float fLeft,fTop,fRight,fBottom;

	RECT(float l=0.0f, float t=0.0f, float r=0.0f, float b=0.0f):fLeft(l),fTop(t),fRight(r),fBottom(b){}
	float width()const{ return fRight-fLeft; }
	float height()const{ return fBottom-fTop; }
};

struct SIZE
{
	float fWidth,fHeight;
	SIZE(float w=1.0f, float h=1.0f):fWidth(w),fHeight(h){}
};

struct draw_info
{
public:
	draw_info():angle_(0.f),alpha_(255){}

	uint8_t		alpha()const{ return alpha_; }
	draw_info&	set_alpha(uint8_t alpha){ alpha_=alpha; return *this; }

	POS		pos_;
	SIZE	scale_;	
	float	angle_;	//!< 0～360
	uint8_t	alpha_;
};

////////////////////////
// 関数
////////////////////////

//! スプライト座標をローカル空間で変形して親空間の座標位置に変換する
extern inline void d3d9_transform_world_sprite_pos(POS_VERTEX* verPos, const SIZE& size, const POS& pos, const SIZE& scale, float fRot, const POS& pivot);

//! @brief 矩形情報から頂点に座標を書き込む
/*! @param[in] pStartVertex 書き込むバッファの先頭ポインタ */
extern inline void write_pos(float* pStartVertex, float fLeft, float fTop, float fRight, float fBottom, float fZ);

//! @brief 矩形情報から頂点にUV座標を書き込む
/*! @param[in] pStartVertex 書き込むバッファの先頭ポインタ */
extern inline void write_uv(float* pStartVertex, float fLeft, float fTop, float fRight, float fBottom);

//! @brief 矩形情報から頂点にカラーを書き込む
/*! @param[in] pStartVertex 書き込むバッファの先頭ポインタ */
extern inline void write_color(DWORD* pStartVertex, DWORD nColor);

extern inline void write_color(DWORD* pStartVertex, DWORD nLTColor, DWORD nRTColor, DWORD nLBColor, DWORD nRBColor);

//! @brief 矩形情報から頂点に座標を書き込む
/*! @param[in] pStartVertex 書き込むバッファの先頭ポインタ
 *  @return 次に書き込む先頭位置 */
extern inline float* write_pos_vertex(float* pStartVertex, float fLeft, float fTop, float fRight, float fBottom, float fZ);

//! @brief 矩形情報から頂点にUV座標を書き込む
/*! @param[in] pStartVertex 書き込むバッファの先頭ポインタ
 *  @return 次に書き込む先頭位置 */
extern inline float* write_uv_vertex(float* pStartVertex, float fLeft, float fTop, float fRight, float fBottom);

//! @brief 矩形情報から頂点にカラーを書き込む
/*! @param[in] pStartVertex 書き込むバッファの先頭ポインタ
 *  @return 次に書き込む先頭位置 */
extern inline DWORD* write_color_vertex(DWORD* pStartVertex, DWORD nColor);

extern inline DWORD* write_color_vertex(DWORD* pStartVertex, DWORD nLTColor, DWORD nRTColor, DWORD nLBColor, DWORD nRBColor);

//! c(sjis)が何バイト文字かを返す
inline uint32_t count_char_sjis(uint8_t c)
{
	if(c==0) return 0; // NULL
	// 2バイト範囲
	if((c>=0x81 && c<=0x9f) || (c>=0xe0 && c<=0xfc)) return 2;
	// それ以外は1バイト
	return 1;
}

} // namespace mana end
} // namespace draw end
