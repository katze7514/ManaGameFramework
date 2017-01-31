#pragma once

#include "../Sound/sound_util.h"

namespace mana{
namespace audio{

// Audioクラスのパラメータ操作フラグ
enum param_flag : uint32_t
{
	PARAM_NONE		=	0,
	PARAM_VOL		=	1,
	PARAM_SPEED		=	1<<1,
	PARAM_POS		=	1<<2,
	PARAM_STOP		=	1<<3,
	PARAM_PAUSE		=	1<<4,
	PARAM_PLAY		=	1<<5,
	PARAM_HANDLER	=	1<<6,
};

// BGMの入れ替え指定
enum change_mode : uint32_t
{
	CHANGE_STOP,	//!< 再生中のBGMをすぐに止める
	CHANGE_CROSS,	//!< クロスフェードする
	CHANGE_FADEOUT,	//!< 再生中のBGMをフェードアウトしてから再生を始める
	CHANGE_NONE,
};

// Audioの種類
enum audio_kind{
	AUDIO_BGM,
	AUDIO_SE,
};

// Audioの基本情報
struct audio_info
{
public:
	audio_info():nSoundID_(0),nParamFlag_(0),eAudioKind_(AUDIO_SE),eChangeMode_(CHANGE_NONE),
				 bLoop_(false),bForce_(false),nFadeFrame_(0)
				{}

public:
	uint32_t		nSoundID_;
	uint32_t		nParamFlag_;

	audio_kind		eAudioKind_;
	change_mode		eChangeMode_;

	bool			bLoop_;
	bool			bForce_;
	
	sound::volume	vol_;
	sound::volume	volFadeEnd_;
	int32_t			nFadeFrame_;
};

} // namespace audio end
} // namespace mana end
