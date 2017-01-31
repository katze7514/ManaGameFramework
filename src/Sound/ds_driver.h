#pragma once

namespace mana{
namespace sound{

class ds_driver
{
public:
	ds_driver():pDriver_(nullptr),pPrimary_(nullptr){}
	~ds_driver(){ fin(); }

	//! @brief DirectSoundの初期化
	/*! @param[in] bHighQuarity trueにするとプライマリバッファの再生フォーマットを高品質化する。
	 *                          プライマリバッファが取得できなかった場合は無効。 */
	bool init(HWND hWnd, bool bHighQuarity=true);
	void fin();

	LPDIRECTSOUND8		driver(){ return pDriver_; }
	LPDIRECTSOUNDBUFFER	primary_buffer(){ return pPrimary_; }

	const DSCAPS&		caps()const{ return caps_; }

private:
	LPDIRECTSOUND8		pDriver_;
	LPDIRECTSOUNDBUFFER	pPrimary_;

	DSCAPS				caps_;
};

} // namespace sound end
} // namespace mana end
