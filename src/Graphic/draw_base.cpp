#include "../mana_common.h"

#include "draw_context.h"
#include "draw_base.h"

namespace mana{
namespace graphic{

draw_base::draw_base(uint32_t nReserve):base_type(nReserve),eKind_(DRAW_BASE),nColor_(D3DCOLOR_ARGB(0,255,255,255)),eColorMode_(COLOR_NO),bVisible_(true),bPause_(false)
{
	//handler_ = [](draw_base* pNode, draw_base_event_id id){};
}

draw_base::~draw_base()
{
	clear_child();
}

bool draw_base::is_visible_ctx(draw_context& ctx)const
{
	return is_visible() && ctx.is_visible();
}

bool draw_base::is_pause_ctx(draw_context& ctx)const
{
	return is_pause() || ctx.is_pause();
}

void draw_base::init()
{
	init_self();
	for(auto& it: children())
		it->init();
}

void draw_base::exec(draw_context& ctx)
{
	//if(!context.is_pause()) handler_(this, EV_START_FRAME);

	// 自分の動き
	exec_self(ctx);

	bool bVisible	= ctx.is_visible();
	ctx.visible(is_visible_ctx(ctx));
	
	bool bPause		= ctx.is_pause();
	ctx.pause(is_pause_ctx(ctx));

	// 子の動き
	for(auto& it: children())
		it->exec(ctx);

	//if(!context.is_pause()) handler_(this, EV_END_FRAME);

	ctx.pause(bPause);
	ctx.visible(bVisible);
}

void draw_base::exec_self(draw_context& ctx)
{
	calc_world(ctx);
	//if(context.is_exec()) handler_(this, EV_EXEC_FRAME);
}

void draw_base::calc_world(draw_context& ctx)
{
	D3DXMATRIX m;

	// 初期化
	if(parent())
	{
		worldMat_	 = parent()->world_matrix();
		nWorldAlpha_ = parent()->world_alpha();
	}
	else
	{
		D3DXMatrixIdentity(&worldMat_);
		nWorldAlpha_ = 255;
	}

	// 行列
	// 右から掛けていく

	// pos
	if(x()!=0.f || y()!=0.f)
	{
		D3DXMatrixTranslation(&m, x(), y(), 0.f);
		worldMat_ = m * worldMat_;
	}

	// rot
	if(angle()!=0.f)
	{
		D3DXMatrixRotationZ(&m, angle() * boost::math::constants::pi<float>()/180.0f);
		worldMat_ = m * worldMat_;
	}

	// scale
	if(width()!=1.f || height()!=1.f)
	{
		D3DXMatrixScaling(&m, width(), height(), 1.0f);
		worldMat_ = m * worldMat_;
	}

	// カラー合成
	if(alpha()<255)
		nWorldAlpha_ = static_cast<uint8_t>((static_cast<float>(nWorldAlpha_) / 255.0f) * (static_cast<float>(alpha()) / 255.0f) * 255.0f);

	// Z
	ctx.add_total_z(-0.1f);
	fWorldZ_ = ctx.total_z();
}

} // namespace graphic_base end
} // namespace mana end
