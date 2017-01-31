#pragma once

#include "draw_base.h"

namespace mana{
namespace graphic{

/*! @brief スプライトを1枚描画するノード
 *
 *  親子を作った場合は、親からの相対変換になる
 *  無効なテクスチャを設定した時は、四角ポリゴンが表示される */
class sprite : public draw_base
{
public:
	sprite(uint32_t nReserve=CHILD_RESERVE):draw_base(nReserve),nTexID_(0),bBlend_(false){ eKind_=DRAW_SPRITE; }
	virtual ~sprite(){}

public:
	uint32_t			tex_id()const{ return nTexID_; }
	sprite&				set_tex_id(uint32_t nTexID){ nTexID_=nTexID; return *this; }
	const draw::RECT&	tex_rect()const{ return rect_; }
	sprite&				set_tex_rect(const draw::RECT& rect){ rect_=rect; return *this; }
	bool				is_blend()const{ return bBlend_; }
	sprite&				blend(bool bBlend){ bBlend_ = bBlend; return *this; }

protected:
	virtual void exec_self(draw_context& ctx)override;

protected:
	uint32_t	nTexID_;
	draw::RECT	rect_;
	bool		bBlend_; //!< trueだと強制的にblend描画になる
};

} // namespace graphic end
} // namespace mana end
