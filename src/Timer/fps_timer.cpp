#include <thread>

#include "fps_timer.h"

using namespace std::chrono;

namespace mana{
namespace timer{

void fps_timer::set_fps(uint32_t nFPS)
{
	nFPS_ = nFPS;
	frameTime_ = microseconds(1000000 / fps());

	nPastIndex_ = 0;
	for(uint32_t i=0; i<60; ++i) aPastTime_[i] = microseconds(0);
}

bool fps_timer::wait_frame()
{
	// フレームの最後
	end_ = high_resolution_clock::now();

	microseconds elapsed = duration_cast<microseconds>(end_ - start_);
	aPastTime_[(nPastIndex_++)&59] = elapsed;

	bool r=true;

	if(fps()>0)
	{
		if(frameTime_ < elapsed) // フレームタイムより経過時間が大きかったらsleepせずに次へ
			r = false;
		else
			std::this_thread::sleep_for(frameTime_ - elapsed);
	}
	
	// フレームの最初
	start_ = high_resolution_clock::now();

	return r;
}

int64_t fps_timer::elapsed_time()
{
	if(nPastIndex_==0) return 0;
	return duration_cast<milliseconds>(aPastTime_[(nPastIndex_-1)&59]).count();
}

uint32_t fps_timer::average_fps()
{
	if(nPastIndex_<60) return 0;

	microseconds ave = microseconds(0);
	for(uint32_t i=0; i<60; ++i)
	{
		if(aPastTime_[i]<frameTime_)
			ave += frameTime_;
		else
			ave += aPastTime_[i];
	}

	ave /= 60;

	return static_cast<uint32_t>(microseconds(1000000).count() / ave.count());
}

} // namespace timer end
} // namespace mana end
