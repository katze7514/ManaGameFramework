#pragma once

#include "draw_base.h"

namespace mana{
namespace graphic{

class timeline;

/*! @brief ムービークリップノード
 *
 *  Flashのムービークリップを模したもの。
 *  実質的にはtimelineノードを一括管理するために使う
 */
class movieclip : public draw_base
{
public:
	movieclip(uint32_t nReserve=CHILD_RESERVE):draw_base(nReserve),nTotalFrameCount_(0){ eKind_ = DRAW_MOVIECLIP; }
	virtual ~movieclip(){}

public:
	void		exec(draw_context& ctx)override;

public:
	uint32_t	total_frame_count()const{ return nTotalFrameCount_; }
	
public:
	void		jump_frame(uint32_t nFrame, draw_context& ctx);

public:
	timeline*	layer(uint32_t nLayer);

#ifdef MANA_DEBUG
public:
	bool	add_child(draw_base* pChild, uint32_t nID)override;
#endif

protected:
	virtual void init_self(){ nTotalFrameCount_=0; }

protected:
	uint32_t nTotalFrameCount_;
};

} // namespace graphic end
} // namespace mana end
