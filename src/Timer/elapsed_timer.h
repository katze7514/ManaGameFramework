#pragma once

#include <chrono>

namespace mana{
namespace timer{

//! @brief ある区間の時間を計測するクラス
class elapsed_timer
{
public:
	void start(){ start_ = std::chrono::high_resolution_clock::now(); }
	void end(){ end_ = std::chrono::high_resolution_clock::now(); }

	int64_t elasped_nano()const{ return (std::chrono::duration_cast<std::chrono::nanoseconds>(end_ - start_)).count(); }
	int64_t elasped_micro()const{ return (std::chrono::duration_cast<std::chrono::microseconds>(end_ - start_)).count(); }
	int64_t elasped_mill()const{ return (std::chrono::duration_cast<std::chrono::milliseconds>(end_ - start_)).count(); }

	std::chrono::milliseconds elasped_mill_duration()const{ return std::chrono::duration_cast<std::chrono::milliseconds>(end_ - start_); }

private:
	 std::chrono::high_resolution_clock::time_point start_,end_;
};

} // namespace timer end
} // namespace mana end

/* 使用例

	elapsed_timer t;
	t.start();

	// 計測区間

	t.end();

	logger::infoln(t.elapsed_micro());
*/
