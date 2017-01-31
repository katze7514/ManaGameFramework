#pragma once

#include "../Audio/audio_util.h"

namespace mana{
namespace graphic{
class draw_context;
class draw_base;
} // namespace graphic end

namespace audio{
class audio_player;
} // namespace audio end

class game_window;
class actor;

//! @brief 実質的にゲームシステムのフロントエンド
/*! Scriptからはこのクラスインスタンスを通して色々アクセスする */
class actor_context
{
public:
	actor_context():nPauseFlag_(0),bValid_(true),pDrawRoot_(nullptr),pApp_(nullptr){}

public:
	uint16_t	pause_flag()const{ return nPauseFlag_; }
	void		set_pause_flag(uint16_t nPauseFlag){ nPauseFlag_  = nPauseFlag; }
	void		add_pause_flag(uint16_t nPauseFlag){ nPauseFlag_ |= nPauseFlag; }
	void		remove_pause_flag(uint16_t nPauseFlag){ nPauseFlag_ &= ~nPauseFlag; }

	bool		is_valid()const{ return bValid_; }
	void		valid(bool bValid){ bValid_=bValid; }

public:
	// Graphic
	bool				add_draw_root_child(graphic::draw_base* pDrawBase, uint32_t nPriority);
	graphic::draw_base* remove_draw_root_child(uint32_t nPriority, bool bDelete);
	graphic::draw_base* draw_root_child(uint32_t nPriority);

	bool				start_draw_request();
	void				end_draw_request();

	// BGM
	bool play_bgm(const string& sID="", uint32_t nFade=0, audio::change_mode eChangeMode=audio::CHANGE_CROSS);
	void stop_bgm(uint32_t nFadeOut=0);
	void pause_bgm();

	void set_bgm_volume(uint32_t nPer);
	void fade_bgm_volume(uint32_t nPerStart, uint32_t nPerEnd, int32_t nFadeFrame);
	void fade_bgm_cur_volume(uint32_t nPerEnd, int32_t nFadeFrame);

	// SE
	void play_se(const string& sID, bool bLoop=false, bool bForce=false);
	void stop_se(const string& sID);
	void pause_se(const string& sID);
	void set_se_volume(const string& sID, uint32_t nPer);
	void fade_se_volume(const string& sID, uint32_t nPerStart, uint32_t nPerEnd, int32_t nFadeFrame);
	void fade_se_cur_volume(const string& sID, uint32_t nPerEnd, int32_t nFadeFrame);
	bool is_playing_se(const string& sID)const;
	void stop_se_all();

public:
	const shared_ptr<graphic::draw_context>&	draw_context(){ return pDrawCtx_; }
	actor_context&								set_draw_context(const shared_ptr<graphic::draw_context>& pCtx){ pDrawCtx_=pCtx; return *this; }

	graphic::draw_base*							draw_root(){ return pDrawRoot_; }
	actor_context&								set_draw_root(graphic::draw_base* pDrawRoot){ pDrawRoot_=pDrawRoot; return *this; }

	const shared_ptr<audio::audio_player>&		audio_player();
	const shared_ptr<audio::audio_player>&		audio_player()const;
	shared_ptr<audio::audio_player>				audio_player_ptr();

public:
	const game_window*			app()const{ return pApp_; }
	game_window*				app(){ return pApp_; }
	actor_context&				set_app(game_window* pApp){ pApp_=pApp; return *this; }

protected:
	uint16_t				nPauseFlag_;
	bool					bValid_;		//!< falseだとポーズフラグ関係なしにexecが実行されなくなる

	shared_ptr<graphic::draw_context>	pDrawCtx_;
	graphic::draw_base*					pDrawRoot_;

	game_window*			pApp_;


#ifdef MANA_DEBUG
public:
	graphic::draw_base*	debug_draw_root(){ return pDebugDrawRoot_; }
	void				set_debug_draw_root(graphic::draw_base* pRoot){ pDebugDrawRoot_=pRoot; }

protected:
	graphic::draw_base*	pDebugDrawRoot_;
#endif

	// xtal bind用メソッド
public:
	// Draw
	bool								add_draw_root_child_xtal(xtal::SmartPtr<graphic::draw_base> pDrawBase, uint32_t nPri);
	xtal::SmartPtr<graphic::draw_base>	remove_draw_root_child_xtal(uint32_t nPri, bool bDel);
	xtal::SmartPtr<graphic::draw_base>	draw_root_child_xtal(uint32_t nPri);

	xtal::SmartPtr<graphic::draw_base>	draw_root_xtal();

	// BGM
	bool play_bgm_xtal(xtal::StringPtr sID, uint32_t nFade=0, uint32_t nChangeMode=audio::CHANGE_CROSS);
	
	// SE
	void play_se_xtal(xtal::StringPtr sID, bool bLoop=false, bool bForce=false);
	void stop_se_xtal(xtal::StringPtr sID);
	void pause_se_xtal(xtal::StringPtr sID);
	void set_se_volume_xtal(xtal::StringPtr sID, uint32_t nPer);
	void fade_se_volume_xtal(xtal::StringPtr sID, uint32_t nPerStart, uint32_t nPerEnd, int32_t nFadeFrame);
	void fade_se_cur_volume_xtal(xtal::StringPtr sID, uint32_t nPerEnd, int32_t nFadeFrame);
	bool is_playing_se_xtal(xtal::StringPtr sID)const;
};

} // namespace mana end
