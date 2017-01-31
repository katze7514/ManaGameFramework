#pragma once

#include "draw_base.h"

namespace mana{
namespace graphic{

//! ポリゴン描画。三角と四角に対応。色は付くが、テクスチャには未対応
class polygon : public draw_base
{
public:
	enum type : uint32_t
	{
		TRIANGLE	= 3,
		RECTANGLE	= 4,
	};


public:
	polygon(type ePoly=RECTANGLE):ePoly_(ePoly),bBlend_(false){ eKind_=DRAW_POLYGON; }
	virtual ~polygon(){}

public:
	enum type				type()const{ return ePoly_; }
	void					set_type(enum type eType){ ePoly_=eType; }

	const draw::POS_VERTEX&	vertex(uint32_t nIndex)const{ return vertex_[nIndex]; }
	polygon&				set_vertex(uint32_t nIndex, float fX, float fY, float fZ=0.f){ if(nIndex<4){ vertex_[nIndex].fX=fX; vertex_[nIndex].fY=fY; vertex_[nIndex].fZ=fZ; } return *this;}

	bool					is_blend()const{ return bBlend_; }
	polygon&				blend(bool bBlend){ bBlend_=bBlend; return *this; }

protected:
	virtual void exec_self(draw_context& ctx)override;

protected:
	enum type			ePoly_;
	draw::POS_VERTEX	vertex_[4]; // ローカルでOK
	bool				bBlend_;
};

} // namespace graphic end
} // namespace mana end
