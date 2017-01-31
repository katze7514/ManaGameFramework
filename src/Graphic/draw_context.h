#pragma once

#include "../Draw/renderer_2d_util.h"

namespace mana{
namespace draw{
class renderer_2d;
} // namespace draw end

namespace audio{
class audio_player;
} // namespace audio end

namespace graphic{
class text_table;

class draw_context
{
public:
	draw_context():bPause_(false),nRenderTarget_(draw::cmd::BACK_BUFFER_ID),bVisible_(true),fTotalZ_(0.f){}

public:
	bool			is_pause()const{ return bPause_; }
	void			pause(bool bPause){ bPause_=bPause; }

	bool			is_visible()const{ return bVisible_; }
	void			visible(bool bVisible){ bVisible_=bVisible; }

	float			total_z()const{ return fTotalZ_; }
	void			set_total_z(float fZ){ fTotalZ_ = fZ; }
	void			add_total_z(float fZ){ fTotalZ_ += fZ; if(fTotalZ_<0) fTotalZ_=0; }
	
	uint32_t		render_target()const{ return nRenderTarget_; }
	void			set_render_target(uint32_t nRenderTarget){ nRenderTarget_ = nRenderTarget; }

public:
	const shared_ptr<class text_table>&		text_table(){ return pTextTable_; }
	draw_context&							set_text_table(const shared_ptr<class text_table>& pTextTable){ pTextTable_=pTextTable; return *this; }

	const shared_ptr<draw::renderer_2d>&	renderer(){ return pRenderer_; }
	draw_context&							set_renderer(const shared_ptr<draw::renderer_2d>& pRenderer){ pRenderer_ = pRenderer; return *this; }

	const shared_ptr<audio::audio_player>&	audio_player(){ return pAudioPlayer_; }
	draw_context&							set_audio_player(const shared_ptr<audio::audio_player>& pAudioPlayer){ pAudioPlayer_ = pAudioPlayer; return *this; }

protected:
	bool		bPause_; //!< trueだと、イベントハンドラが呼ばれなくなり、timelineのフレーム進行なども止まる

	uint32_t	nRenderTarget_;
	bool		bVisible_; //!< falseだとrenderコマンドが生成されず、非表示になる
	float		fTotalZ_; //!< 現在のZ

	shared_ptr<class text_table>	pTextTable_;

	shared_ptr<draw::renderer_2d>	pRenderer_;
	shared_ptr<audio::audio_player>	pAudioPlayer_;
};

} // namespace graphic end
} // namespace mana end
