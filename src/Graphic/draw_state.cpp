#include "draw_context.h"
#include "timeline.h"
#include "movieclip.h"

#include "draw_state.h"

namespace mana{
namespace graphic{

void draw_state::exec_self(draw_context& ctx)
{
	// 変更処理
	if(bChange_ && !is_pause_ctx(ctx))
	{
		change_state_inner(ctx);
		bChange_=false;
	}
	
	bool bVisible	= ctx.is_visible();
	ctx.visible(is_visible_ctx(ctx));

	bool bPause		= ctx.is_pause();
	ctx.pause(is_pause_ctx(ctx));

	draw_base::exec_self(ctx);
	if(pCurDrawBase_) pCurDrawBase_->exec(ctx);

	ctx.visible(bVisible);
	ctx.pause(bPause);
}

//////////////////////////////////

draw_state&	draw_state::set_color(uint8_t a, uint8_t r, uint8_t g, uint8_t b)
{
	draw_base::set_color(a,r,g,b);
	if(cur_state()) cur_state()->set_color(a,r,g,b);
	return *this;
}

draw_state&	draw_state::set_color(DWORD color)
{
	draw_base::set_color(color);
	if(cur_state()) cur_state()->set_color(color);
	return *this;
}

draw_state&	draw_state::set_color_mode(color_mode_kind eMode)
{
	draw_base::set_color_mode(eMode);
	if(cur_state()) cur_state()->set_color_mode(eMode);
	return *this;
}


//////////////////////////////////

bool draw_state::add_state(const string_fw& sID, draw_base* pDrawBase)
{
	auto r = mapDrawState_.emplace(sID, pDrawBase);
	if(!r.second)
	{
		//logger::warnln("[draw_state]すでにStateが存在しています。: " + sID.get());
		return false;
	}

	r.first->second->set_parent(this);
	return true;
}

void draw_state::remove_state(const string_fw& sID)
{
	auto it = mapDrawState_.find(sID);
	if(it==mapDrawState_.end()) return;

	safe_delete(it->second);
	mapDrawState_.erase(it);
}

void draw_state::clear_state()
{
	pCurDrawBase_ = nullptr;

	for(auto& state : mapDrawState_)
		delete state.second;

	mapDrawState_.clear();
}

bool draw_state::is_exist_state(const string_fw& sID)
{
	return mapDrawState_.find(sID)!=mapDrawState_.end();
}

graphic::draw_base* draw_state::state(const string_fw& sID)
{
	auto it = mapDrawState_.find(sID);
	if(it==mapDrawState_.end())
	{
		logger::warnln("[draw_state]指定されたstateは登録されていません。: " + sID.get());
		return nullptr;
	}
	return it->second;
}

bool draw_state::change_state_inner(draw_context& ctx)
{
	// 同じアニメへの変更だったら何もせずにリターン
	if((sCurID_==sChangeID_ && bSame_) || sChangeID_=="") return true;

	auto pChange = state(sChangeID_);
	if(!pChange) return false;

	sCurID_ = sChangeID_;

	if(bSame_)
	{
		if(pCurDrawBase_)
		{
			switch(pCurDrawBase_->kind())
			{
			case DRAW_MOVIECLIP:
				nFrame_ = static_cast<movieclip*>(pCurDrawBase_)->total_frame_count();
			break;

			case DRAW_TIMELINE:
				nFrame_ = static_cast<timeline*>(pCurDrawBase_)->total_frame_count();
			break;

			default:
				nFrame_=1;
			break;
			}
		}
	}

	if(bInit_) pChange->init();

	switch(pChange->kind())
	{
	case DRAW_MOVIECLIP:
		static_cast<movieclip*>(pChange)->jump_frame(nFrame_,ctx);
	break;

	case DRAW_TIMELINE:
		static_cast<timeline*>(pChange)->jump_frame(nFrame_,ctx);
	break;

	default:
	break;
	}

	pCurDrawBase_ = pChange;

	// 変わった時に子に伝わらないデータを反映しておく
	pCurDrawBase_->set_color(color());

	if(color_mode()!=COLOR_NO)
		pCurDrawBase_->set_color_mode(color_mode());

	return true;
}

} // namespace graphic end
} // namespace mana end
