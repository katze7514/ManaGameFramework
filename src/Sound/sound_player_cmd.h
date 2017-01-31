#pragma once

#include "sound_util.h"

namespace mana{
namespace sound{

class ds_sound_player;

namespace cmd{

// サウンド情報ロード
struct info_load_cmd
{
public:
	info_load_cmd():bFile_(true)
	{
	#ifdef MANA_DEBUG
		bReDefine_ = false;
	#endif
	}

	string	sFilePath_;
	bool	bFile_;

	function<void (bool)> callback_; // 成否

#ifdef MANA_DEBUG
	bool bReDefine_;
#endif
};

// サウンド情報追加
struct info_add_cmd
{
public:
	info_add_cmd():bStreaming_(false),fStreamLoopSec_(0.0f),nStreamSec_(STREAM_SECONDS)
	{
	#ifdef MANA_DEBUG
		bReDefine_ = false;
	#endif
	}

public:
	string		sID_;
	string		sFilePath_;
	bool		bStreaming_;
	float		fStreamLoopSec_;
	uint32_t	nStreamSec_;

	function<void (uint32_t)> callback_; // サウンドID

#ifdef MANA_DEBUG
	bool bReDefine_;
#endif
};

// サウンド情報削除
struct info_remove_cmd
{
public:
	info_remove_cmd():nID_(0){}

	uint32_t nID_;
};

// サウンドイベントハンドラ設定
struct event_cmd
{
public:
	event_cmd():nID_(0){}

	uint32_t nID_;
	function<void(play_event)> handler_;
};

// 再生・停止・一時停止
struct play_cmd_cmd
{
public:
	play_cmd_cmd():eMode_(MODE_NONE),nID_(0),bLoop_All_(false){}

public:
	play_mode	eMode_; //!< MODE_PLAY,MODE_STOP,MODE_PAUSE のいずれか
	uint32_t	nID_;
	bool		bLoop_All_; 
};

// ボリュームフェード
struct volume_fade_cmd
{
public:
	volume_fade_cmd():nID_(0),nFrame_(0){}

public:
	uint32_t nID_;
	volume	 vol_;
	uint32_t nFrame_;
};

// 各種パラメタ変更
struct param_cmd
{
public:
	enum kind
	{
		PARAM_NONE,
		PARAM_DEFAULT_VOL,
		PARAM_VOL,
		PARAM_SPEED,
		PARAM_POS,
	};

public:
	param_cmd():eKind_(PARAM_NONE), nID_(0), f_(0.f){}

public:
	kind	 eKind_;
	uint32_t nID_;
	volume	 vol_;
	float	 f_;
};

// 状態チェック
struct play_mode_cmd
{
public:
	play_mode_cmd():nID_(0){}

public:
	uint32_t	nID_;

	function<void(play_mode)> callback_; // 結果
};

/////////////////////////
// コマンドvariant
typedef variant<info_load_cmd,
				info_add_cmd,
				info_remove_cmd,
				event_cmd,
				play_cmd_cmd,
				volume_fade_cmd,
				param_cmd,
				play_mode_cmd> sound_player_cmd;

//! @brief sound_player_cmdを実行するvisitor
struct sound_player_exec : public boost::static_visitor<>
{
public:
	sound_player_exec(ds_sound_player& player):player_(player){}

	void operator()(info_load_cmd& cmd)const;
	void operator()(info_add_cmd& cmd)const;
	void operator()(info_remove_cmd& cmd)const;
	void operator()(event_cmd& cmd)const;
	void operator()(play_cmd_cmd& cmd)const;
	void operator()(volume_fade_cmd& cmd)const;
	void operator()(param_cmd& cmd)const;
	void operator()(play_mode_cmd& cmd)const;

private:
	ds_sound_player& player_;

private:
	NON_COPIABLE(sound_player_exec);
};

} // namespace cmd end

} // namespace sound end
} // namespace mana end
