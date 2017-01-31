#pragma once

#include "../Audio/audio_util.h"
#include "../Draw/renderer_2d_util.h"
#include "../Draw/text_data.h"
#include "draw_util.h"

namespace mana{
namespace graphic{

struct draw_base_data
{
public:
	enum over_flag : uint8_t
	{
		FLAG_DRAW_INFO	= 1,
		FLAG_PIVOT		= 1<<1,
		FLAG_COLOR		= 1<<2,
		FLAG_CHILDREN	= 1<<3,
	};

public:
	draw_base_data():eKind_(DRAW_BASE),nColor_(D3DCOLOR_ARGB(255,255,255,255)),eColorMode_(COLOR_MUL),nOverFlag_(0){}
	virtual ~draw_base_data(){ for(auto& it : vecChildren_){ delete it; } vecChildren_.clear(); }

	bool	is_over(over_flag eFlag)const{ return bit_test<uint8_t>(nOverFlag_, eFlag);}
	void	up_over(over_flag eFlag){ nOverFlag_|=eFlag; }

public:
	draw_base_kind	eKind_;
	string_fw		sID_;	// IDを設定すると、すでに登録済みのデータを取りに行く

	string_fw		sName_; // タイムライン内で使うと前のフレームにある同名draw_baseは同じdraw_baseを指す

	draw::draw_info		drawInfo_;
	draw::POS			pivot_;

	DWORD				nColor_;
	color_mode_kind		eColorMode_;

	vector<draw_base_data*>	vecChildren_;

	uint8_t				nOverFlag_; // ビットが立っている所は上書きする

#ifdef MANA_DEBUG
	string				sDebugName_;
#endif
};

/////////////////////////////
// 基本ノードデータ
struct label_data : public draw_base_data
{
public:
	label_data(){ eKind_=DRAW_LABEL; }

public:
	shared_ptr<draw::text_data>	pTextData_;
};

struct sprite_data : public draw_base_data
{
public:
	sprite_data():nTexID_(0){ eKind_=DRAW_SPRITE; }

public:
	uint32_t	nTexID_;
	draw::RECT	rect_;
};

struct audio_data : public draw_base_data
{
public:
	audio_data(){ eKind_=DRAW_AUDIO_FRAME; }

public:
	audio::audio_info info_;	
};

/////////////////////////////
// Message
struct message_data : public label_data
{
public:
	message_data():nTextCharNum_(0),nNext_(0),nSoundID_(0){ eKind_=DRAW_MESSAGE; }

public:
	uint32_t nTextCharNum_;
	uint32_t nNext_;
	uint32_t nSoundID_;
};


/////////////////////////////
// MovieClip系ノードデータ

struct keyframe_data : public draw_base_data
{
public:
	keyframe_data():nNextFrame_(0){ eKind_=DRAW_KEYFRAME; }

public:
	uint32_t			nNextFrame_;
};

struct tweenframe_data : public keyframe_data
{
public:
	tweenframe_data():fEasing_(0){ eKind_=DRAW_TWEENFRAME; }

public:
	draw::draw_info		drawInfoEnd_;
	float				fEasing_;
};

struct timeline_data : public draw_base_data
{
public:
	timeline_data(){ eKind_=DRAW_TIMELINE; }
};

struct movieclip_data : public draw_base_data
{
public:
	movieclip_data(){ eKind_=DRAW_MOVIECLIP; }
};

} // namespace graphic end
} // namespace mana end
