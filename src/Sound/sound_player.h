#pragma once

#include "sound_util.h"
#include "sound_player_cmd.h"

namespace mana{
namespace sound{

class ds_sound_player;

/*! @brief サウンドプレイヤーのベースクラス
 *
 *  サウンドプレイヤーはこのクラスを継承する。
 *  今の所、DirectSound実装の別スレッド版と同スレッド版の2つを用意している
 *
 *  初期化は、init_driverとinit_playerの両方を呼ぶこと
 */
class sound_player
{
public:
	sound_player();
	virtual ~sound_player();

public:
	//! @brief ドライバー初期化
	/*! @param[in] hWnd 対応するウインドウハンドル
	 *  @param[in] nReserveSoundNum 登録されるサウンドの情報予想値値
	 *  @param[in] bHighQuarity 再生のクオリティを上げるかどうか 
	 *  @param[in] nMaxSoundBufferNum 保持するサウンドバッファ最大数。この数を越えるとサウンドバッファが解放される。
									  0だと解放しない。 */
	bool init_driver(HWND hWnd, uint32_t nReserveSoundNum=DEFAULT_MAX_INFO_NUM, bool bHighQuarity=true, uint32_t nMaxSoundBufferNum=DEFAULT_MAX_BUFFER_NUM);

	//! @brief プレイヤー初期化
	/*! @param[in] fIntervalMs サウンド処理の実行間隔(ms)。非同期版の時のみ有効
	 * @param[in] nReserveRequestNum 積むリクエストの推定数
	 * @param[in] nAffinity レンダースレッドを動作させるCPUコア番号 */
	virtual bool init_player(uint32_t nIntervalMs=DEFAULT_INTERVAL_MS, uint32_t nReserveRequestNum=DEFAULT_MAX_REQUEST_NUM, uint32_t nAffinity=0)=0;
	//! 動作終了。スレッドも終了する
	virtual void fin()=0;

	//! @defgroup sound_player_request リクエスト処理
	//! @{
	//! @brief request積み処理を開始する
	/*! @param[in] bWait リクエストを積む準備ができてない時、ブロックする
	 *  @return bWaitがfalseの場合、falseが返って来たらコマンドキューが使用中でリクエストが積めない状態 */
	virtual bool start_request(bool bWait=true)=0;
	//! @brief requestを積む
	/*! start_request ～ end_request の間でのみ呼ぶこと */
	virtual void request(const cmd::sound_player_cmd& cmd)=0;
	//! request積みの終了
	virtual void end_request()=0;
	//! @brief 積んだリクエストの処理を開始する
	/*! @param[in] bWait リクエストを処理中だったらブロックするかどうかのフラグ
	 *  @return bWaitがfalseの場合、falseが返って来たらリクエスト処理中 */
	virtual bool play(bool bWait=true)=0;
	//! @}

public:
	//! @defgroup sound_player_request_helper リクエストヘルパー
	//! @{
	void request_info_load(const string& sFilePath, const function<void(bool)>& callback=nullptr, bool bFile=true);
	void request_info_add( const string& sID, const string& sFilePath, bool bStreaming, const function<void (uint32_t)>& callback=nullptr, float fStreamLoopSec=0.0f, uint32_t nStreamSec=STREAM_SECONDS);
	void request_info_remove(uint32_t nID);
	void request_play_mode(uint32_t nID, const function<void(play_mode)>& callback);
	void request_play_mode(const string& sID, const function<void(play_mode)>& callback){ auto n = sound_id(sID); if(n) request_play_mode(*n, callback); }
	//! @}

	//! サウンドIDを取得
	optional<uint32_t> sound_id(const string& sID);

protected:
	ds_sound_player* pPlayer_;

#ifdef MANA_DEBUG
public:
	// リクエスト時のリロードフラグ設定
	void request_redefine(bool bReDefine){ bReDefine_=bReDefine; }

private:
	bool bReDefine_;
#endif
};

typedef shared_ptr<sound_player> sound_player_sptr;

//! サウンドプレイヤー種別
enum sound_player_kind
{
	PLAYER_ASYNC,	//! 別スレッド実行
	PLAYER_SYNC,
};

//! サウンドプレイヤー生成関数
extern sound_player_sptr create_sound_player(sound_player_kind eKind=PLAYER_ASYNC);

// リクエストヘルパー。前後でstart_request/end_requestを呼ぶこと

} // namespace sound end
} // namespace mana end

/* プレイヤー初期化 **

	shared_ptr<sound::sound_player> player = create_sound_player();	
	player->init_driver(hWnd);
	player->init_render();
*/
