#include "../mana_common.h"

#include "actor_context.h"
#include "actor.h"

namespace mana{

bool actor::init(actor_context& ctx)
{
	if(!init_self(ctx)) return false;
	for(auto& child : children())
		if(!child->init(ctx)) return false;

	return true;
}

void actor::reset(actor_context& ctx)
{
	reset_self(ctx);
	for(auto& child : children())
		child->reset(ctx);
}

void actor::exec(actor_context& ctx)
{
	// ポーズフラグ
	if(!is_exec(ctx)) return;

	exec_self(ctx);

	bool bValid = ctx.is_valid();
	ctx.valid(is_valid() && bValid);

	for(auto& child : children())
		child->exec(ctx);

	ctx.valid(bValid);
}

bool actor::is_exec(actor_context& ctx)
{
	uint32_t nPauseFlag = pause_flag() & ctx.pause_flag();
	if(nPauseFlag>0 || !is_valid() || !ctx.is_valid()) return false;

	return true;
}

} // namespace mana end
