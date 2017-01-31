#pragma once

#include <cmath>

namespace mana{
namespace graphic{

//! @brief sinによるイージングステップ計算
/*! 
 *  @param[in] nEdging	イージングパラメータ。-1.0～1.0。負は先詰め、正は後詰め
 *  @param[in] step		0.0～1.0の値を取る */
inline float easing_sin_step(float fEasing, float fStep)
{
	using boost::math::constants::pi;
	return fStep+(sinf(fStep*pi<float>())*fEasing/pi<float>());
}

//! @brief 線形補間関数
/*! @param[in] fStart 開始値
 *  @param[in] fEnd   終了値
 *  @param[in] fStep  ステップ。0.0～1.0 */
inline float lerp(float fStart, float fEnd, float fStep)
{
	fStep = clamp(fStep,0.0f,1.0f);
	return fStart + (fEnd-fStart) * fStep;
}

} // namespace graphic end
} // namespace mana end
