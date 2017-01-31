#include "../mana_common.h"

#include "graphic_fun.h"
#include "draw_context.h"
#include "keyframe.h"

namespace mana{
namespace graphic{

////////////////////////////////////
// keyframe
////////////////////////////////////
keyframe::~keyframe(){ clear_child(); }

void keyframe::clear_child(bool bDelete)
{
	for(auto it : listSharedID_)
		remove_child(it,false);

	clear_shared_id();
	draw_base::clear_child(bDelete);
}

void keyframe::init_self()
{
	// 共有ノードがあるかもしれないので、毎回親を張り直す
	for(auto& pChild : children())
		pChild->set_parent(this);
}

keyframe& keyframe::set_color(uint8_t a, uint8_t r, uint8_t g, uint8_t b)
{
	draw_base::set_color(a,r,g,b);
	for(auto& pChild : children())
		pChild->set_color(a,r,g,b);

	return *this;
}

keyframe& keyframe::set_color(DWORD color)
{
	draw_base::set_color(color);
	for(auto& pChild : children())
		pChild->set_color(color);

	return *this;
}

keyframe& keyframe::set_color_mode(color_mode_kind eMode)
{
	draw_base::set_color_mode(eMode);
	for(auto& pChild : children())
		pChild->set_color_mode(eMode);

	return *this;
}

////////////////////////////////////
// tween
////////////////////////////////////
tweenframe::tweenframe(uint32_t nReserve):keyframe(nReserve),fEasing_(0.0f),nCurStep_(0)
{ 
	eKind_ = DRAW_TWEENFRAME;
}

void tweenframe::init_self()
{
	drawInfo_	= easingDrawInfo_[0];
	nCurStep_	= 0;

	keyframe::init_self();
}

void tweenframe::exec_self(draw_context& ctx)
{
	const float fEasingStep = easing_sin_step(easing_param(), static_cast<float>(cur_step())/static_cast<float>(step()));

	set_x(lerp(draw_info_start().pos_.fX, draw_info_end().pos_.fX, fEasingStep));
	set_y(lerp(draw_info_start().pos_.fY, draw_info_end().pos_.fY, fEasingStep));

	set_angle(lerp(draw_info_start().angle_, draw_info_end().angle_, fEasingStep));

	set_width(lerp(draw_info_start().scale_.fWidth, draw_info_end().scale_.fWidth, fEasingStep));
	set_height(lerp(draw_info_start().scale_.fHeight, draw_info_end().scale_.fHeight, fEasingStep));

	set_alpha(static_cast<uint8_t>(lerp(draw_info_start().alpha(), draw_info_end().alpha(), fEasingStep)));

	draw_base::exec_self(ctx);

	if(!is_pause_ctx(ctx) && cur_step()<step()) ++nCurStep_;
}

} // namespace graphic end
} // namespace mana end
