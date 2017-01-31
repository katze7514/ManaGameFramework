#pragma once

#include "../Audio/audio_util.h"

#include "draw_base.h"

namespace mana{
namespace graphic{

class draw_context;

/*! @brief draw_baseとしてサウンドを扱うクラス
 *
 *　それによってtimelineクラス入れられるので、アニメと音を同期実行できる
 *  sound_id と auido_kindを設定してから使うこと
 *
 *  world計算はしないので、子に何かを入れないこと
 */
class audio_frame : public draw_base
{
public:
	audio_frame():bExec_(false){}
	virtual ~audio_frame(){}

public:
	void			set_audio_info(const audio::audio_info& info){ info_=info; }

public:
	uint32_t		sound_id()const{ return info_.nSoundID_; }
	audio_frame&	set_sound_id(uint32_t nSoundID){ info_.nSoundID_=nSoundID; }

	void			set_audio_kind(audio::audio_kind eKind){ info_.eAudioKind_ = eKind; }

public:
	// play系はAudioKindを上書きするので注意
	void	play_bgm(int32_t nFadeFrame=0, audio::change_mode eChangeMode=audio::change_mode::CHANGE_CROSS);
	void	play_se(bool bLoop=false, bool bForce=false);

	void	stop(int32_t nFadeFrame);
	void	set_volume(const sound::volume& vol);
	void	fade_volume(const sound::volume& start, const sound::volume& end, int32_t nFadeFrame);
	void	fade_volume(const sound::volume& end, int32_t nFadeFrame);

public:
	virtual bool	add_child(draw_base* pChild, uint32_t nID)override;

protected:
	virtual void init_self()override;
	virtual void exec_self(draw_context& ctx)override;

protected:
	//! 対象のサウンドID
	audio::audio_info	info_;
	bool				bExec_; //!< 実行フラグ。trueだったら実行済みなのでリクエストは送らない
};

} // namespace graphic end
} // namespace mana end
