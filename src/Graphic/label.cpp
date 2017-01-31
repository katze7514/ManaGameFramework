#include "../mana_common.h"

#include "../Draw/renderer_2d.h"

#include "draw_context.h"
#include "label.h"

namespace mana{
namespace graphic{

label& label::set_text(const string& sText, bool bMarkUp)
{
	if(text_data())
	{
		text_data()->set_text(sText);
		text_data()->markup(bMarkUp);
	}
	return *this;
}

label& label::set_font_id(const string_fw& sFont)
{
	if(text_data())
		text_data()->set_font_id(sFont);

	return *this;
}

void label::exec_self(draw_context& ctx)
{
	draw_base::exec_self(ctx);

	if(is_visible_ctx(ctx) && text_data() && !text_data()->text().empty())
	{
		draw::cmd::text_draw_cmd cmd;

		cmd.pText_		= text_data();
		cmd.nColor_		= (color()&0x00FFFFFF)|static_cast<uint32_t>(alpha())<<24;

		// 描画開始位置計算
		D3DXVECTOR4 p;
		p.x = p.y = p.z = 0.0f;
		p.w = 1.0f;
		D3DXVec4Transform(&p,&p,&worldMat_);

		cmd.pos_.fX = p.x;
		cmd.pos_.fY = p.y;
		cmd.pos_.fZ = world_z();

		cmd.nRenderTarget_ = ctx.render_target();
		ctx.renderer()->request(cmd);
	}
}

} // namespace graphic end
} // namespace mana end
