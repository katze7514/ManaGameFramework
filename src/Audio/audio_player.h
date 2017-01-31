#pragma once

#include "audio_util.h"

namespace mana{

namespace sound{
class sound_player;
class volume;
} // namespace sound end

namespace audio{

class audio;
class audio_context;


/*! @brief audioプレイヤー。音データや再生状態を管理するフロントエンドクラス
 *
 *  sound_playerの呼び出しも担う。
 *  このクラスを介して音を扱うと良い */
class audio_player
{
public:
	enum player_const{
		SE_GC_NUM	= 20,	//!< SEオブジェクトGCをかけ始めるSEオブジェクト数
		SE_GC_FRAME	= 601,	//!< SEオブジェクトのGC間隔(フレーム数)
	};

	enum player_state{
		STATE_INIT,	//!< 初期化済み
		STATE_LOAD,	//!< サウンド定義ファイルロード中
		STATE_NONE,
	};

	enum load_result : uint32_t{
		FAIL,		//!< ロード失敗
		EXEC,		//!< ロード中
		SUCCESS,	//!< ロード成功
		NONE,		//!< 何もなし
	};

private:
	enum bgm_state{
		BGM_FADE_CHANGE,	//!< フェードチェンジ
		BGM_FADE_STOP,		//!< フェードストップ
		BGM_CROSS_STOP,		//!< クロスストップ
		BGM_NONE,
	};

public:
	audio_player();
	~audio_player();

	//! @brief 初期化
	/*! @param[in] pPlaeyr 初期済みのsound_playerインスタンスを渡すこと */
	bool init(const shared_ptr<sound::sound_player>& pPlayer, uint32_t nGcFrame=SE_GC_FRAME, uint32_t nSeGcNum=SE_GC_NUM);

	//! @brief オーディオプレイヤー実行。毎フレーム呼ぶこと
	bool exec(bool bWait=true);

	//! @brief このプレイヤーで管理するサウンドデータの定義ファイルを読み込む
	/*! 読み込みは別スレッドで行われるので、読み込みが終了したかどうかは,
	 *  is_fin_loadで確認する。 */ 
	bool		load_sound_file(const string& sFile, bool bFile=true);
	load_result	is_fin_load();

	player_state state()const{ return eState_; }

public:
	//! 全体のボリューム設定
	void set_master_volume(const sound::volume& vol);
	//! 全体ボリューム取得
	const sound::volume& master_volume()const;


public:
	//! @brief BGMを再生する
	/*! 再生中のBGMがある場合、BGMの入れ替えを行う。またポーズ中だったらポーズを解除する
	 * 
	 *  @param[in] sID BGM ID
	 *  @param[in] nFadeIn フェードインする時間(フレーム数)
	 *  @param[in] eChangeMode BGMの入れ替え方  *
	 *  @return falseだとBGM処理(入れ替えなど)中なので変更できないことを示す */
	bool play_bgm(const string& sID="", uint32_t nFade=0, change_mode eChangeMode=CHANGE_CROSS){ return play_bgm(sound_id(sID), nFade, eChangeMode); }
	bool play_bgm(uint32_t nID, uint32_t nFade=0, change_mode eChangeMode=CHANGE_CROSS);
	//! @brief 再生中のBGMを停止する
	/*! @param[in] nFadeOut フェードアウト時間(フレーム数) */
	void stop_bgm(uint32_t nFadeOut=0);
	//! @brief 再生中のBGMを一時停止する
	void pause_bgm();

	//! bgmのボリュームを変更する
	void set_bgm_volume(const sound::volume& vol);
	//! bgmのボリュームをフェードする
	void fade_bgm_volume(const sound::volume& start, const sound::volume& end, int32_t nFadeFrame);
	void fade_bgm_volume(const sound::volume& end, int32_t nFadeFrame);
	//! bgmのボリュームを取得する
	const sound::volume& bgm_volume()const;

	//! bgmが再生中かどうか
	bool is_playing_bgm()const;

public:
	//! @brief SEを再生する
	/*! @param[in] sID SE ID
	 *  @param[in] bLoop ループ再生するならtrue
	 *  @param[in] bForce trueだったら、すでに再生中だったら最初から再生し直す
	 *					  flaseだったら、すでに再生中だったら再生されない */
	void play_se(const string& sID, bool bLoop=false, bool bForce=false){ play_se(sound_id(sID), bLoop, bForce); }
	void play_se(uint32_t nID, bool bLoop=false, bool bForce=false);
	//! 再生中のSEを止める
	void stop_se(const string& sID){ stop_se(sound_id(sID)); }
	void stop_se(uint32_t nID);
	//! 再生中のSEを一時停止する
	void pause_se(const string& sID){ pause_se(sound_id(sID)); }
	void pause_se(uint32_t nID);
	//! 指定したSEのボリュームを変更する
	void set_se_volume(const string& sID, const sound::volume& vol){ set_se_volume(sound_id(sID), vol); }
	void set_se_volume(uint32_t nID, const sound::volume& vol);
	//! 指定したSEのボリュームをフェードする
	void fade_se_volume(const string& sID, const sound::volume& start, const sound::volume& end, int32_t nFadeFrame){ fade_se_volume(sound_id(sID), start, end, nFadeFrame);  }
	void fade_se_volume(uint32_t nID, const sound::volume& start, const sound::volume& end, int32_t nFadeFrame);
	void fade_se_volume(const string& sID, const sound::volume& end, int32_t nFadeFrame){ fade_se_volume(sound_id(sID), end, nFadeFrame);  }
	void fade_se_volume(uint32_t nID, const sound::volume& end, int32_t nFadeFrame);

	//! SEが再生中かどうか
	bool is_playing_se(const string& sID){ return is_playing_se(sound_id(sID)); }
	bool is_playing_se(uint32_t nID)const;

	//! 再生しているすべてのSEを停止する
	void stop_se_all();

	//! SE全体のボリュームを変更する
	void set_se_master_volume(const sound::volume& vol);
	//! SE全体のボリュームを取得する
	const sound::volume& se_master_volume()const;

public:
	//! 文字列IDを数値IDに変換する
	uint32_t sound_id(const string& sID);


private:
	//! bgmの処理
	void			exec_bgm();
	//! 現在再生してるBGMインスタンス
		  audio*	playing_bgm();
	const audio*	playing_bgm()const;
	//! 一つ前に再生していたBGMインスタンス
	audio*			playing_past_bgm();
	//! @brief bgmを新しく再生する
	/*! BGMインデックスを入れ替える。nIDが0の時はサウンドIDをセットしない */
	void			play_new_bgm(uint32_t nID, uint32_t nFade);

	//! SEのGC処理
	void			gc_se();

private:
	player_state			eState_;
	std::atomic_uint32_t	eLoadState_;

	// コンテキスト
	audio_context* pCtx_;

	// サウンドIDと数値IDのキャッシュ
	unordered_map<string, uint32_t>	cacheID_;

	// Audioルート
	audio* pRoot_;

	/////////////////
	// BGM
	bgm_state eBgmState_;

	// bgmルート
	audio* pBgmRoot_;
	// bgmクロスフェード用に2つ用意
	array<audio*,2> bgms_;
	// 操作対象のBGMのインデックス
	uint32_t nBgmIndex_;
	// フェード時間
	uint32_t nFade_;

	////////////////
	// se
	// seルート
	audio* pSeRoot_;

	// se gc
	uint32_t nSeGcNum_;		//!< GCをかけ始めるオブジェクト数
	uint32_t nSeGcFrame_;	//!< GC間隔
	uint32_t nSeGcCounter_;

#ifdef MANA_DEBUG
public:
	void redefine(bool bReDefine){ bReDefine_=bReDefine; }

private:
	bool bReDefine_;
#endif
};

} // namespace audio end
} // namespace mana end

/*
// サウンド
shared_ptr<audio_player> pAudioPlayer = make_shared<audio_player>();
pAudioPlayer->init(pSoundPlayer);

pAudioPlayer_->load_sound_file("audio_player.xml");
bool bAudioLoad=false;

if(!bAudioLoad)
{
	switch(pAudioPlayer_->is_fin_load())
	{
	case audio::audio_player::SUCCESS:
		bAudioLoad = true;
	break;

	case audio::audio_player::FAIL:
		logger::fatalln("[game_window]audio定義ファイルの読み込みに失敗しました");
		goto END;
	break;
	}

}
else
{
	bool bPlay;
	if(pKeyboard_->is_push(DIK_T))
	{
		bPlay = pAudioPlayer_->play_bgm("TAKUMI", 60);
		logger::debugln("TAKUMI Play! "+ to_str(bPlay));
	}

	if(pKeyboard_->is_push(DIK_H))
	{
		bPlay = pAudioPlayer_->play_bgm("HARUNA", 0, audio::CHANGE_STOP);
		logger::debugln("HARUNA Play! "+ to_str(bPlay));
	}

	if(pKeyboard_->is_push(DIK_E))
	{
		bPlay = pAudioPlayer_->play_bgm("ENEMY", 30, audio::CHANGE_FADEOUT);
		logger::debugln("ENEMY Play! " + to_str(bPlay));
	}

	if(pKeyboard_->is_push(DIK_K))
	{
		bPlay = pAudioPlayer_->play_bgm("KU", 90);
		logger::debugln("KU Play! " + to_str(bPlay));
	}

	if(pKeyboard_->is_push(DIK_Z))
	{
		pAudioPlayer_->stop_bgm(30);
		logger::debugln("BGM Stop!");
	}

	if(pKeyboard_->is_push(DIK_X))
	{
		pAudioPlayer_->pause_bgm();
		logger::debugln("BGM Pause!");
	}


	if(pKeyboard_->is_push(DIK_S))
	{
		pAudioPlayer_->play_se("SPIRIT",false,true);
		logger::debugln("SE Play!");
	}

}

// サウンド実行
pAudioPlayer_->exec();

*/
