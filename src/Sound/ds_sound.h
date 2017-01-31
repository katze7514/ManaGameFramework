#pragma once

#include "sound_util.h"
#include "sound_file_reader.h"

namespace mana{
namespace sound{
class ds_driver;

struct sound_cmd
{
	play_mode	eMode_;		//!< 現在の再生モード	
};

/*! @brief サウンドバッファクラス
 *
 *  サウンドを再生したり、サウンドファイル管理など、サウンド再生のための最小単位
 *  wav/ogg vorbis対応。ogg vorbisはストリーム再生のみ可能
 */
class ds_sound
{
public:
	ds_sound();
	~ds_sound(){ fin(); }

	//! 終了処理
	void fin();

	//! @brief 初期化パラメータ登録
	/*! @param[in] sFilename サウンドファイル名
	 *  @param[in] vol 最初のボリューム。STOPするとこのボリュームに戻る
	 *  @param[in] bStreaming ストリーミングするかどうか
	 *  @param[in] bStreamingLoopSec ストリーミングのループする時の先頭位置
	 *  @param[in] nStreamSec ストリーミング時に確保する秒数(sec) */
	void init(const string& sFilename, const volume& vol, bool bStreaming, float fStreamLoopSec=0.0f, uint32_t nStreamSec=STREAM_SECONDS);

	//! @brief バッファを作成し、サウンド再生の準備をする
	/*! @param[in] driver DirectSoundドライバー */
	bool create_buffer(ds_driver& driver);

	//! イベントハンドラ設定
	void set_event_handler(const function<void(play_event)>& handler);

	//!  @brief 定期的に呼ぶことで、ストリーム再生、フェードなどが実行される
	//*! @param[in] nElapsedTime 前にupdateを呼んでからの経過時間(ms)  */
	bool update(uint32_t nElapsedMillSec);

	//! @defgroup sound_sound_cmd サウンド命令。基本的にupdateが呼ばれるタイミングで反映される
	//! @{
	void play(bool bLoop);
	void stop();
	void pause();
	void fade_volume(const volume& fadeVol, uint32_t nFadeFrame);

	// パラメータ系は即座に反映される
	void set_volume(const volume& vol);
	void set_speed(float fSpeed);
	void set_pos(float fSec); //!< 再生位置を秒指定
	//! @}

	//! バッファ作成済みかどうか
	bool is_create_buffer()const{ return pSoundBuffer_!=nullptr; }

	//! 再生中かどうか
	bool is_play()const;

	//! 停止中かどうか。ポーズ中は含まれない
	bool is_stop()const;

	//! 現在の再生状態
	play_mode cur_play_mode()const{ return cur_cmd().eMode_; }

	//! 対応するファイルパス
	const string& filepath()const{ return sFilePath_; }

private:
	void update_volume();
	void update_speed();
	void update_pos();

private:
	//! @defgroup sound_cmd コマンドバッファ操作
	//! @{
	void				init_cmd();
	sound_cmd&			cur_cmd(){ return soundCmd_[cmdIndex_]; }
	const sound_cmd&	cur_cmd()const{ return soundCmd_[cmdIndex_]; }
	sound_cmd&			next_cmd(){ return soundCmd_[cmdIndex_^1]; }
	const sound_cmd&	next_cmd()const{ return soundCmd_[cmdIndex_^1]; }
	void				swap_cmd(){ cmdIndex_^=1; next_cmd().eMode_=MODE_NONE; }
	//! @}

	bool		full_sound_buffer(bool bBegin); // サウンドバッファへの最初のデータ書き込み
	bool		stream_sound_buffer();
	bool		stop_sound_buffer();

private:
	LPDIRECTSOUNDBUFFER8	pSoundBuffer_;
	uint32_t				nSoundBufferSize_;

	string					sFilePath_;
	sound_file_reader		reader_;
	uint32_t				nSoundDataSize_;

	bool					bStreaming_;
	uint32_t				nStreamCheckMs_;	// ストリーム位置をチェックしに行く間隔(ms)
	uint32_t				nStreamLastByte_;	// バッファに書き込んだデータの末尾(byte)
	float					fStreamLoopSec_;	// ストリームの時のみ有効ループする際への戻り先（先頭からの秒数）

	play_event					eEvent_;	//!< 最新のイベント
	function<void(play_event)>	handler_;	//!< 再生状態を伝えるイベントハンドラ

	bool					bLoop_;
	bool					bEndData_;

	sound_cmd				soundCmd_[2];
	uint32_t				cmdIndex_;

	volume					volCur_;	//!< 現在のボリューム
	float					fSpeedCur_;	//!< 現在の速さ
	float					fPosCur_;	//!< 現在の位置(秒)

	bool					bVolUpdate_;
	bool					bSpeedUpdate_;
	bool					bPosUpdate_;

	//! @defgroup ds_sound_fade_param　フェード要パラメータ
	//! @{
	volume					volFadeBase_;		//!< 元ボリューム
	volume					volFade_;			//!< フェード先ボリューム
	uint32_t				nVolFadeFrame_;		//!< フェードフレーム
	uint32_t				nVolFadeCounter_;	//!< フェードカウンタ
	//! @}

	uint32_t				nStreamElapsedTime_; // 最後にバッファに書き込んでからの経過時間

public:
	static void set_enable_sample_rate(uint32_t nMin, uint32_t nMax);

private:
	static uint32_t MIN_SAMPLE_RATE;
	static uint32_t MAX_SAMPLE_RATE;

#ifdef MANA_DEBUG
public:
	void release_force(); // 強制的にSTOPしバッファをリリースする
#endif
};

} // namespace sound end
} // namespace mana end

/*
	ds_driver sd;
	sd.init(wnd_handle());

	ds_sound snd[2];
	volume vol;
	vol.set_per(80);
	snd[0].init("takumi2.ogg", vol, true);
	if(!snd[0].create_buffer(sd))
	{
		logger::fatalln("サウンド作成に失敗しました");
		set_state(state::STOP);
		return;
	}
 */

