#include "../mana_common.h"

#include "../Draw/renderer_2d.h"

#include "draw_context.h"
#include "sprite.h"

namespace mana{
namespace graphic{

void sprite::exec_self(draw_context& ctx)
{
	draw_base::exec_self(ctx);

	if(is_visible_ctx(ctx))
	{
		draw::cmd::sprite_draw_cmd cmd;
		cmd.nTexID_ = tex_id();
		cmd.rect_	= tex_rect();

		cmd.fZ_		= world_z();
		cmd.nColor0_= world_alpha_color();
		cmd.nColor1_= color();

		// pivot
		if(pivot_x()!=0.f || pivot_y()!=0.f)
		{
			D3DXMATRIX m;
			D3DXMatrixTranslation(&m, -pivot_x(), -pivot_y(), 0.f);
			cmd.worldMat_ = m * world_matrix();
		}
		else
		{
			cmd.worldMat_	= world_matrix();
		}

		if(cmd.nTexID_>0)
		{
			if(world_alpha()<255 || is_blend())
			{
				switch(eColorMode_)
				{
				default:
					cmd.mode_ = draw::MODE_TEX_BLEND;
				break;

				case COLOR_THR:
					cmd.mode_ = draw::MODE_TEX_COLOR_THR_BLEND;
				break;

				case COLOR_BLEND:
					cmd.mode_ = draw::MODE_TEX_COLOR_BLEND_BLEND;
				break;

				case COLOR_MUL:
					cmd.mode_ = draw::MODE_TEX_COLOR_MUL_BLEND;
				break;

				case COLOR_SCREEN:
					cmd.mode_ = draw::MODE_TEX_COLOR_SCREEN_BLEND;
				break;
				}
			}
			else
			{
				switch(eColorMode_)
				{
				default:
					cmd.mode_ = draw::MODE_TEX;
				break;

				case COLOR_THR:
					cmd.mode_ = draw::MODE_TEX_COLOR_THR;
				break;

				case COLOR_BLEND:
					cmd.mode_ = draw::MODE_TEX_COLOR_BLEND;
				break;

				case COLOR_MUL:
					cmd.mode_ = draw::MODE_TEX_COLOR_MUL;
				break;

				case COLOR_SCREEN:
					cmd.mode_ = draw::MODE_TEX_COLOR_SCREEN;
				break;
				}
			}
		}
		else
		{
			if(world_alpha()<255 || is_blend())
				cmd.mode_ = draw::MODE_PLY_COLOR_BLEND;
			else
				cmd.mode_ = draw::MODE_PLY_COLOR;
		}
		
		cmd.nRenderTarget_ = ctx.render_target();
		ctx.renderer()->request(cmd);
	}
}

} // namespace graphic end
} // namespace mana end
