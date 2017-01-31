#pragma once

#include <chrono>

namespace mana{
namespace timer{

/*! @brief FPSタイマー
 *
 *	FPSを管理するタイマー
 *　FPS維持のためのSleepを行ったり、過去の1フレームあたりの処理時間などを管理している
 *
 *  0fps設定にすると経過時間の管理のみで、sleepしない
 */
class fps_timer
{
public:
	fps_timer(uint32_t nFPS=60){ set_fps(nFPS); }

	uint32_t	fps()const{ return nFPS_; }
	void		set_fps(uint32_t nFPS);

	//! @brief 設定された状態に合わせて、適切にsleepを入れる
	/*! @return falseの場合、1フレームで使える時間を越えた */
	bool		wait_frame();

	//! 1フレーム前の経過時間(ms)
	int64_t		elapsed_time();

	//! 経過フレーム数(wait_frameを読んだ回数)
	uint32_t	past_frame()const{ return nPastIndex_; }

	//! FPSアベレージ。60フレーム以後から有効になる
	uint32_t	average_fps();

private:
	//! 維持するFPS
	uint32_t					nFPS_;
	//! 1フレームあたりの最大時間
	std::chrono::microseconds	frameTime_;

	//! フレームの最初と最後の時間
	std::chrono::high_resolution_clock::time_point start_,end_;

	//! 経過時間をいれるべき位置
	uint32_t nPastIndex_;
	//! 過去の経過時間
	std::chrono::microseconds aPastTime_[60];
};


} // namespace timer end
} // namespace mana end
