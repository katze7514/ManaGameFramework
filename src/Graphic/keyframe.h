#pragma once

#include "draw_base.h"

namespace mana{
namespace graphic{

class keyframe : public draw_base
{
public:
	keyframe(uint32_t nReserve=CHILD_RESERVE):draw_base(nReserve),nNextFrame_(0){ eKind_=DRAW_KEYFRAME; }
	virtual ~keyframe();

public:
	keyframe&	set_color(uint8_t a, uint8_t r, uint8_t g, uint8_t b)override;
	keyframe&	set_color(DWORD color)override;
	keyframe&	set_color_mode(color_mode_kind eMode)override;

public:
	uint32_t	next_frame()const{ return nNextFrame_; }
	keyframe&	set_next_frame(uint32_t nFrame){ nNextFrame_=nFrame; return *this; }

	keyframe&	add_shared_id(uint32_t nID){ listSharedID_.emplace_back(nID); return *this; }
	keyframe&	remove_shared_id(uint32_t nID){ listSharedID_.remove(nID); return *this; }
	void		clear_shared_id(){ listSharedID_.clear(); }

public:
	virtual void init_self()override;
			void clear_child(bool bDelete=true)override;

protected:
	//! 切り替えに要するフレーム数。0だと切り替わらない
	uint32_t		nNextFrame_;
	//! 他のフレームと共有してる子ノードID
	list<uint32_t>	listSharedID_;
};

class tweenframe : public keyframe
{
public:
	tweenframe(uint32_t nReserve=CHILD_RESERVE);
	virtual ~tweenframe(){}
	
public:
	const draw::draw_info&	draw_info_start()const{ return easingDrawInfo_[0]; }
	tweenframe&				set_draw_info_start(const draw::draw_info& info){ easingDrawInfo_[0]=info; return *this; }
	const draw::draw_info&	draw_info_end()const{ return easingDrawInfo_[1]; }
	tweenframe&				set_draw_info_end(const draw::draw_info& info){ easingDrawInfo_[1]=info; return *this; }

	float					easing_param()const{ return fEasing_; }
	tweenframe&				set_easing_param(float fEasing){ fEasing_ = clamp(fEasing, -1.0f, 1.0); return *this; }

	uint32_t				cur_step()const{ return nCurStep_; }
	uint32_t				step()const{ return next_frame(); }

protected:
	void init_self()override;
	void exec_self(draw_context& ctx)override;

protected:
	draw::draw_info easingDrawInfo_[2];
	float			fEasing_;

	uint32_t		nCurStep_;
};

} // namespace graphic end
} // namespace mana end
