#include "../mana_common.h"

#include "../Draw/renderer_2d.h"

#include "draw_context.h"
#include "polygon.h"

namespace mana{
namespace graphic{

void polygon::exec_self(draw_context& ctx)
{
	draw_base::exec_self(ctx);

	draw::cmd::polygon_draw_cmd cmd;

	if(is_visible_ctx(ctx))
	{
		// pivot
		if(pivot_x()!=0.f || pivot_y()!=0.f)
		{
			D3DXMATRIX m;
			D3DXMatrixTranslation(&m, -pivot_x(), -pivot_y(), 0.f);
			worldMat_ = m * worldMat_;
		}

		//　グローバル化
		cmd.nVertexNum_ = ePoly_;
		for(uint32_t i=0; i<cmd.nVertexNum_; ++i)
		{
			D3DXVECTOR4 vec(vertex_[i].fX, vertex_[i].fY, 0.f, 1.f);
			D3DXVec4Transform(&vec, &vec, &worldMat_);
			cmd.vertex_[i].fX = vec.x;
			cmd.vertex_[i].fY = vec.y;
		}

		cmd.vertex_[0].fZ = cmd.vertex_[1].fZ = cmd.vertex_[2].fZ = cmd.vertex_[3].fZ = world_z();
		cmd.nColor0_ = world_alpha_color();
		cmd.nColor1_ = color();

		if(world_alpha()<255 || is_blend())
			cmd.mode_ = draw::MODE_PLY_COLOR_BLEND;
		else
			cmd.mode_ = draw::MODE_PLY_COLOR;

		cmd.nRenderTarget_ = ctx.render_target();
		ctx.renderer()->request(cmd);
	}
}

} // namespace graphic end
} // namespace mana end
