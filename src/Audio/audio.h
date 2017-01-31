#pragma once

#include "../Utility/node.h"
#include "../Sound/sound_util.h"

#include "audio_util.h"

namespace mana{
namespace audio{

class audio_context;

/*! @brief audioクラス。音を操作するクラス
 *
 *  サウンド1つを操作する。
 *  親子関係を作ることで、一括で音量変更・ストップ・ポーズ・再生ができる
 */
class audio : private utility::node<audio>
{
public:
	typedef utility::node<audio> base_type;
	friend base_type;

public:
	enum audio_const
	{
		CHILD_RESERVE=5,
	};

public:
	audio(uint32_t nReserve=CHILD_RESERVE);
	~audio();

	void	init();
	void	exec(audio_context& ctx);

public:
	uint32_t	sound_id()const{ return nSoundID_; }
	//! サウンドIDを設定する
	/*! @param[in] bEventHandler trueにするとsound_envetメソッドが有効になる */
	audio&		set_sound_id(uint32_t nSoundID, bool bSoundEvent=true);

	//! @param[in] bLoop ループ再生するかどうか。indeterminateだと前回の設定を使う 
	//! @param[in] bChildren trueだと子も再生する。子の再生は、子に設定されているループフラグを使う
	void		play(tribool bLoop=indeterminate, bool bChildren=false);
	void		stop(bool bChildren=false);
	void		pause(bool bChildren=false);

	uint32_t	param_flag()const{ return paramFlag_; }
	void		fade_volume(const sound::volume& start, const sound::volume& end, int32_t nFadeFrame);
	void		fade_volume(const sound::volume& end, int32_t nFadeFrame);
	void		set_volume(const sound::volume& vol);
	void		set_speed(float fSpeed);
	void		set_pos(float fSec);

	const sound::volume volume()const{ return vol_;}

	//! ループ再生するかどうか
	bool		is_loop()const{ return bLoop_; }

	//! フェード実行中かどうか
	bool		is_fade()const{ return nFadeFrame_>0; }

	//! 最新のイベント状態。サウンドバッファの状態を取得することにほぼ等価
	sound::play_event	sound_event()const{ return static_cast<sound::play_event>(eEvent_.load(std::memory_order_acquire)); }

	//! 再生状態を取得する
	sound::play_mode	sound_play_mode();	// 値が取得できるまで呼び続ける版
	void				sound_play_mode(const function<void(sound::play_mode)>& handler); // 非同期でハンドラーに結果が返ってくる版

	//! LRU用フラグを確認する
	bool		is_lru_flag()const{ return bLRU_; }
	void		reset_lru_flag(){ bLRU_=false; }

public:
	uint32_t		id()const{ return base_type::id(); }
	const audio*	parent()const{ return base_type::parent(); }
	audio&			set_parent(audio* pParent){ base_type::set_parent(pParent); return *this; }

	//! @defgroup audio_child_ctrl audioの子操作
	//! @{
	virtual bool	add_child(audio* pChild, uint32_t nID){ return base_type::add_child(pChild,nID,this); }
	audio*			remove_child(uint32_t nID, bool bDelete){ return base_type::remove_child(nID, bDelete); }
	const	audio*	child(uint32_t nID)const{ return base_type::child(nID); }
			audio*	child(uint32_t nID){ return base_type::child(nID); }
	void			clear_child(bool bDelete=true){ base_type::clear_child(bDelete); }

	base_type::child_vector&		children(){ return vecChildren_; }
	const base_type::child_vector&	children()const{ return vecChildren_; }

	uint32_t		count_children()const{ return base_type::count_children(); }
	void			shrink_children(){ base_type::shrink_children(); }
	//! @}

public:
	uint32_t				world_param_flag()const{ return worldFlag_; }
	const sound::volume&	world_volue()const{ return worldVol_; }

protected:
	void			exec_parent();
	void			exec_fade();
	void			exec_param_flag(audio_context& ctx);
	void			exec_cmd(audio_context& ctx);
	void			exec_reset();

protected:
	uint32_t			nSoundID_;	//!< 対応するサウンドID

	//! @defgroup auido_param 操作用パラメータ
	//! @{
	sound::play_mode	eCmd_;		//!< 再生モード(MODE_NODEだと何もしない)
	sound::play_mode	ePastCmd_;

	bool				bLoop_;
	uint32_t			paramFlag_;	//!< 自分の変更フラグ

	sound::volume		vol_;		//!< ボリューム
	float				fSpeed_;	//!< スピード
	float				fPos_;		//!< 位置(秒数)

	sound::volume		volFadeBase_;	//!< フェード元ボーリューム
	sound::volume		volFade_;		//!< フェード先ボーリューム
	int32_t				nFadeFrame_;	//!< フェードフレーム
	int32_t				nFadeCounter_;	//!< フェードカウンター

	std::atomic_uint32_t	eEvent_;	//!< 最新のイベント

	std::atomic_uint32_t				eExecPlayMode_;	//!< 状態取得の取得先
	function<void(sound::play_mode)>	playHandler_;	//!< 状態取得のためのハンドラ
	//! @}

	uint32_t			worldFlag_;	//!< 変更フラグまとめ
	sound::volume		worldVol_;	//!< 実際に設定されてるボリューム

	bool				bLRU_;		//!< LRU用フラグ。再生されるとtrueになるが、execのタイミングなので注意

#ifdef MANA_DEBUG
public:
	const string&	debug_name()const{ return base_type::debug_name(); }
	void			set_debug_name(const string& sName){ base_type::set_debug_name(sName); }
#endif
};

} // namespace auido end
} // namespace mana end
