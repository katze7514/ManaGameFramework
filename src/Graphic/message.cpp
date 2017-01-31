#include "../mana_common.h"

#include "../Draw/renderer_2d_cmd.h"
#include "../Draw/renderer_2d.h"

#include "../Audio/audio_player.h"

#include "draw_context.h"
#include "message.h"

namespace mana{
namespace graphic{

message::message(bool bCreate, uint32_t nReserve):label(bCreate, nReserve),nCurCharNum_(0),nWait_(0),nTextCharNum_(0),nNext_(0),nSoundID_(0)
{
	eKind_ = DRAW_MESSAGE;
}

void message::init_self()
{
	set_cur_char_num(0);
	nWait_=0;
	pause(false);
}

void message::exec_self(draw_context& ctx)
{
	draw_base::exec_self(ctx);

	if(!is_visible_ctx(ctx) || !text_data()) return;

	bool bSound=false;

	if(!is_pause_ctx(ctx))
	{// フレームを進める
		if(next()==0)
		{// 0だったら瞬間表示
			nCurCharNum_ = nTextCharNum_;
		}
		else if(++nWait_>=next())
		{// wait越えたら1文字表示
			++nCurCharNum_;
			nWait_=0;
			bSound=true;
		}

		// すべての文字を表示する状態になったら文字送り処理を止める
		if(cur_char_num()>=text_char_num())
		{
			set_cur_char_num(text_char_num());
			pause(true);
		}
	}

	if(cur_char_num()==0) return;

	draw::cmd::text_draw_cmd cmd;

	cmd.pText_		= text_data();
	cmd.nCharNum_	= cur_char_num();
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

	if(sound_id()>0 && bSound)
		ctx.audio_player()->play_se(sound_id(),false,true);
}

} // namespace graphic end
} // namespace mana end
