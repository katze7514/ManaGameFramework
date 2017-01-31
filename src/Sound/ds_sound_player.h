#pragma once

#include "../Utility/id_manger.h"

#include "sound_util.h"
#include "ds_sound.h"
#include "ds_driver.h"

namespace mana{
namespace sound{

/*! @brief サウンドプレイヤー
 *
 *　1ショット再生、ストリーム再生対応
 *  wav/ogg vorbisが使える。ogg vorbisはストリームのみ対応
 *
 *  サウンド情報を登録して、サウンド命令メソッドを呼ぶと再生などが
 *  行われる。
 *
 *  最大サウンド数を指定すると、サウンドバッファ作成数をできるだけ
 *  その範囲内で収めようとする。ただし、再生中のサウンドを止めてまで
 *  削除はしないので、再生状況によっては最大サウンド数を超えることも
 *  ある。
 */
class ds_sound_player
{
public:
	struct sound_info
	{
		ds_sound	snd_;	//!< サウンド実体
		//string		sID_;  //!< 文字列ID

		list<uint32_t>::iterator it_; // 管理リストの対応iterator
	};

	typedef unordered_map<uint32_t, sound_info>	sound_hash;
	typedef utility::id_manager<string>			sound_id_manager;

public:
	ds_sound_player():pSilentBuffer_(nullptr),nMaxSoundBufferNum_(DEFAULT_MAX_BUFFER_NUM)
	{
	#ifdef MANA_DEBUG
		bReDefine_=false;
	#endif // MANA_DEBUG
	}
	~ds_sound_player(){ fin(); }

	//! @brief 初期化
	/*! @param[in] hWnd 対応するウインドウハンドル
	 *  @param[in] nReserveSoundNum 登録するサウンド情報数の予想値
	 *  @param[in] bHighQuarity 再生のクオリティを上げるかどうか */
	bool init(HWND hWnd, uint32_t nReserveSoundNum=DEFAULT_MAX_INFO_NUM, bool bHighQuarity=true);

	//! 終了処理
	void fin();

	//! @brief 定期的呼ぶことで、サウンド処理が回る
	/*! @param[in] nElapsedTime 前にupdateを呼んでからの経過時間(ms) */
	bool update(uint32_t nElapsedMillSec);

	//! デフォルトボリュームの設定
	void set_default_volume(const volume& base);

	//! 保持しておく最大サウンドバッファ数。この値を越えるとバッファが解放される
	void set_max_sound_buffer_num(uint32_t nMax){ if(nMax>0) nMaxSoundBufferNum_=nMax; }

	//! サウンド情報追加
	bool add_sound_info(const string& sID, const string& sFilePath, bool bStreaming, float fStreamLoopSec=0.0f, uint32_t nStreamSec=STREAM_SECONDS);

	//! サウンド情報削除。実体があった場合は解放する
	void remove_sound_info(uint32_t nID);
	void remove_sound_info(const string& sID){ auto n = sound_id(sID); if(n) remove_sound_info(*n); }

	//! サウンド情報が登録されてるかどうか
	bool is_sound_info(uint32_t nID)const;
	bool is_sound_info(const string& sID){ auto n = sound_id(sID); if(n) return is_sound_info(*n); else return false; }

	//! サウンドID変換
	optional<uint32_t>	sound_id(const string& sID){ return soundIdMgr_.id(sID); }
	const string&		sound_id(uint32_t nID){ return soundIdMgr_.id(nID); }

	//! @brief サウンド情報ファイルを読み込んで、サウンド情報を登録する
	/*! @param[in] sFilePath サウンド情報ファイルへのパス。もしくはサウンド情報文字列 
	 *  @param[in] bFile sFilePathの種別を表す。trueならファイルへのパス、falseなら文字列 */
	bool load_sound_info_file(const string& sFilePath, bool bFile=true);

	//! @defgroup sound_player_sound_cmd サウンド命令
	//! @{
	void set_event_handler(uint32_t nID, const function<void(play_event)>& handler);
	void set_event_handler(const string& sID, const function<void(play_event)>& handler){ auto n= sound_id(sID); if(n) set_event_handler(*n,handler); }

	void play(uint32_t nID, bool bLoop);
	void play(const string& sID, bool bLoop){ auto n = sound_id(sID); if(n) play(*n, bLoop); }
	void stop(uint32_t nID);
	void stop(const string& sID){ auto n = sound_id(sID); if(n) stop(*n); }
	void pause(uint32_t nID);
	void pause(const string& sID){ auto n = sound_id(sID); if(n) pause(*n); }

	void fade_volume(uint32_t nID, const volume& fadeVol, uint32_t nFadeFrame);
	void fade_volume(const string& sID, const volume& fadeVol, uint32_t nFadeFrame){ auto n = sound_id(sID); if(n) fade_volume(*n, fadeVol, nFadeFrame); }

	void set_volume(uint32_t nID, const volume& vol);
	void set_volume(const string& sID, const volume& vol){ auto n = sound_id(sID); if(n) set_volume(*n,vol); }
	void set_speed(uint32_t nID, float fSpeed);
	void set_speed(const string& sID, float fSpeed){ auto n = sound_id(sID); if(n) set_speed(*n,fSpeed); }
	void set_pos(uint32_t nID, float fSec);
	void set_pos(const string& sID, float fSec){ auto n = sound_id(sID); if(n) set_pos(*n,fSec); }

	play_mode sound_play_mode(uint32_t nID)const;
	play_mode sound_play_mode(const string& sID){ auto n = sound_id(sID); if(n) return sound_play_mode(*n); else return MODE_NONE; }
	
	void stop_all();  //!< 再生中のサウンドをすべてstopする
	void pause_all(); //!< 再生中のサウンドをすべてpauseする
	//! @}

	//! サウンドバッファを作る。プリロード的に使う
	bool create_sound_buffer(const string& sID);

private:
	//! サウンドを取得する。サウンドバッファが作られてなかったら作る
	ds_sound*	sound(uint32_t nID);
	ds_sound*	sound(const string& sID){ auto n = sound_id(sID); if(n) return sound(*n); else return false; }

	//! サウンド数をnMaxSoundNum_以下に留める
	void		limit_sound();

	//! 無音のサウンドを作り再生する
	void		create_silent_sound(bool bHighQuarity);

private:
	//! サウンドドライバー
	ds_driver			driver_;

	//! デフォルトボリューム。ds_soundを作成する際に渡される音量
	volume				defaultVolume_;

	//! サウンドハッシュ
	sound_hash			hashSound_;
	//! 数値IDとサウンドID対応テーブル
	sound_id_manager	soundIdMgr_;

	//! 有効なサウンドリスト。再生中のサウンドが入る。(一時)停止すると外れる
	list<ds_sound*>		activeSound_;

	//! サウンド利用の管理IDリスト(LRU）。数値IDを入れる。前にあるほど古い
	//! このリストに無いサウンド情報は、バッファ未作成
	list<uint32_t>		listSound_;
	// 保持しておく最大サウンドバッファ数
	uint32_t			nMaxSoundBufferNum_;

	//! 停止時ノイズ対策用の無音バッファ
	LPDIRECTSOUNDBUFFER8 pSilentBuffer_;

#ifdef MANA_DEBUG
public:
	void redefine(bool bReDefine){ bReDefine_=bReDefine; }

private:
	bool bReDefine_;
#endif // MANA_DEBUG

};

} // namespace sound end
} // namespace mana end

/*
	sound::ds_sound_player sndPlayer;
	sndPlayer.init(wnd_handle());
	sound::volume vol;
	vol.set_per(70);
	sndPlayer.set_master_volume(vol);

	sndPlayer.add_sound_info("TAKUMI", "takumi2.ogg", true);
	sndPlayer.play("TAKUMI",true);

	imer::elapsed_timer st;
	st.start();
	while(1)
	{
		st.end();
		sndPlayer.update(st.elasped_mill());

		st.start();
	}
 */

/*
<sound_def>
	<sound id="" src="" stream="" stream_sec="" />

</sound_def>

 */
