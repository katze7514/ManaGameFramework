#include "../mana_common.h"

#include "draw_context.h"
#include "movieclip.h"
#include "keyframe.h"
#include "timeline.h"

namespace mana{
namespace graphic{

timeline::timeline(uint32_t nReserve):draw_base(nReserve),nCurKeyFrameNo_(0),pCurKeyFrameNode_(nullptr),nTotalFrameCount_(0),nChangeFrameCount_(0)
{ 
	eKind_=DRAW_TIMELINE;
}

void timeline::exec(draw_context& ctx)
{
	//if(context.is_exec()) handler_(this, EV_START_FRAME);

	draw_base::exec_self(ctx);

	bool bVisible	= ctx.is_visible();
	ctx.visible(is_visible_ctx(ctx));

	bool bPause		= ctx.is_pause();
	ctx.pause(is_pause_ctx(ctx));

	exec_frame(ctx);

	//if(context.is_exec()) handler_(this, EV_END_FRAME);

	ctx.visible(bVisible);
	ctx.pause(bPause);
}

timeline& timeline::set_color(uint8_t a, uint8_t r, uint8_t g, uint8_t b)
{
	draw_base::set_color(a,r,g,b);
	if(pCurKeyFrameNode_) pCurKeyFrameNode_->set_color(a,r,g,b);
	return *this;
}

timeline& timeline::set_color(DWORD color)
{
	draw_base::set_color(color);
	if(pCurKeyFrameNode_) pCurKeyFrameNode_->set_color(color);
	return *this;
}

timeline& timeline::set_color_mode(color_mode_kind eMode)
{
	draw_base::set_color_mode(eMode);
	if(pCurKeyFrameNode_) pCurKeyFrameNode_->set_color_mode(eMode);
	return *this;
}

void timeline::exec_frame(draw_context& ctx)
{
	if(!is_pause_ctx(ctx) && nTotalFrameCount_>=nChangeFrameCount_)
		change_keyframe();

	if(pCurKeyFrameNode_)
		pCurKeyFrameNode_->exec(ctx);

	if(!is_pause_ctx(ctx)) ++nTotalFrameCount_;
}

void timeline::init_self()
{
	nCurKeyFrameNo_		= 0;
	pCurKeyFrameNode_	= nullptr;
	bPause_				= false;

	nTotalFrameCount_	= 0;
	nChangeFrameCount_	= 0;
}

void timeline::change_keyframe()
{
	if(nCurKeyFrameNo_ < count_children())
	{
		assert(children()[nCurKeyFrameNo_]->kind()==DRAW_KEYFRAME || children()[nCurKeyFrameNo_]->kind()==DRAW_TWEENFRAME);

		pCurKeyFrameNode_ = static_cast<keyframe*>(children()[nCurKeyFrameNo_]);
		++nCurKeyFrameNo_;

		//pCurKeyFrameNode_->init();

		if(pCurKeyFrameNode_->next_frame() > 0)
			nChangeFrameCount_ += pCurKeyFrameNode_->next_frame();
		else
			bPause_ = true;
	}
	else
	{// 実行するフレーム番号が設定を越えたらループ
		init_self();
		change_keyframe();
	}

	if(pCurKeyFrameNode_)
	{
		pCurKeyFrameNode_->set_color(color());
		pCurKeyFrameNode_->set_color_mode(color_mode());
	}
}

void timeline::next_keyframe()
{
	if(cur_keyframe_no()<count_children()-1)
	{
		++nCurKeyFrameNo_;
		change_keyframe();
	}
}

void timeline::next_keyframe(draw_context& ctx)
{
	if(cur_keyframe_no()<count_children()-1)
		jump_keyframe(cur_keyframe_no()+1, ctx);
}

void timeline::prev_keyframe()
{
	if(cur_keyframe_no()>2)
	{
		--nCurKeyFrameNo_;
		change_keyframe();
	}
}

void timeline::prev_keyframe(draw_context& ctx)
{
	if(cur_keyframe_no()>2)
		jump_keyframe(cur_keyframe_no()-1, ctx);
}

void timeline::jump_keyframe(uint32_t nKeyFrame, draw_context& ctx)
{
	bool bPauseCtx	= ctx.is_pause();
	ctx.pause(false);
	bool bVisible	= ctx.is_visible();
	ctx.visible(false);

	bool bPause = is_pause();
	pause(false);

	init_self();
	while(cur_keyframe_no()<count_children() && cur_keyframe_no()!=nKeyFrame && !is_pause())
		exec_frame(ctx);

	pause(bPause);

	ctx.pause(bPauseCtx);
	ctx.visible(bVisible);
}

void timeline::jump_frame(uint32_t nFrame, draw_context& ctx)
{
	if(nTotalFrameCount_==nFrame) return;

	bool bPauseCtx	= ctx.is_pause();
	ctx.pause(false);
	bool bVisible	= ctx.is_visible();
	ctx.visible(false);
	
	bool bPause = is_pause();
	pause(false);

	init_self();
	while(cur_keyframe_no()<count_children() && nTotalFrameCount_!=nFrame && !is_pause())
		exec_frame(ctx);

	pause(bPause);

	ctx.pause(bPauseCtx);
	ctx.visible(bVisible);
}

#ifdef MANA_DEBUG
bool timeline::add_child(draw_base* pChild, uint32_t nID)
{
	assert(pChild->kind()==DRAW_KEYFRAME || pChild->kind()==DRAW_TWEENFRAME);
	return draw_base::add_child(pChild,nID);
}
#endif

} // namespace graphic end
} // namespace mana end
