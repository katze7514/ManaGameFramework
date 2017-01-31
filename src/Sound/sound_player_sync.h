#pragma once

#include "sound_player.h"

namespace mana{
namespace sound{

/*! @brief 同期版サウンドプレイヤー
 *
 * 非同期版と合わせるために冗長になっている所がある
 * playは毎フレーム呼ぶこと */
class sound_player_sync : public sound_player
{
public:
	sound_player_sync(){}

	//! @brief サウンド初期化
	//*!@param[in] nIntervalMs 同期版の場合は使われない値 */
	bool init_player(uint32_t nIntervalMs=DEFAULT_INTERVAL_MS, uint32_t nReserveRequestNum=DEFAULT_MAX_REQUEST_NUM, uint32_t nAffinity=0)override;

	//! 動作終了。スレッドも終了する
	void fin()override;

	//! @defgroup sound_player_sync_request リクエスト処理
	//! @{
	//! request積み処理を開始する
	bool start_request(bool bWait=true)override{ return true; }
	//! @brief requestを積む
	void request(const cmd::sound_player_cmd& cmd)override;
	//! request積みの終了
	void end_request()override{}
	//! 積んだリクエストの処理を開始する
	bool play(bool bWait=true)override;
	//! @}

private:
	timer::elapsed_timer timer_;

private:
	NON_COPIABLE(sound_player_sync);
};

} // namespace sound end
} // namespace mana end
