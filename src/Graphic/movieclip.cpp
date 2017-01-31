#include "../mana_common.h"

#include "draw_context.h"
#include "timeline.h"
#include "movieclip.h"

namespace mana{
namespace graphic{

void movieclip::exec(draw_context& ctx)
{
	draw_base::exec(ctx);

	if(!is_pause_ctx(ctx)) ++nTotalFrameCount_;
}

void movieclip::jump_frame(uint32_t nFrame, draw_context& ctx)
{
	for(auto& it : children())
	{
		assert(it->kind()==DRAW_TIMELINE);
		static_cast<timeline*>(it)->jump_frame(nFrame,ctx);
	}
	nTotalFrameCount_ = nFrame;
}

timeline* movieclip::layer(uint32_t nLayer)
{
	if(nLayer>=children().size()) return nullptr;
	return static_cast<timeline*>(child(nLayer));
}

#ifdef MANA_DEBUG
bool movieclip::add_child(draw_base* pChild, uint32_t nID)
{
	assert(pChild->kind()==DRAW_TIMELINE);
	return draw_base::add_child(pChild,nID);
}
#endif

} // namespace graphic end
} // namespace mana end
