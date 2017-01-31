#include "../mana_common.h"

#include "ds_driver.h"

namespace mana{
namespace sound{

bool ds_driver::init(HWND hWnd, bool bHighQuarity)
{
	if(pDriver_)
	{
		logger::infoln("[ds_driver]初期化済みです。");
		return true;
	}

	HRESULT r = DirectSoundCreate8(NULL, &pDriver_, NULL);
	if(!check_hresult(r,"[ds_driver]DirectSound8インターフェイスが作成できませんでした。")) return false;

	caps_.dwSize=sizeof(caps_);
	pDriver_->GetCaps(&caps_);

	DWORD flag = DSSCL_NORMAL;

	if(bHighQuarity)
	{// 高品質化するなら、優先協調レベル
		r = pDriver_->SetCooperativeLevel(hWnd, DSSCL_PRIORITY);

		if(check_hresult(r))
		{
			// プライマリバッファ取得
			DSBUFFERDESC dsbd;
			::ZeroMemory(&dsbd, sizeof(dsbd));
			dsbd.dwSize			= sizeof(dsbd);
			dsbd.dwFlags		= DSBCAPS_PRIMARYBUFFER;
			dsbd.dwBufferBytes	= 0;
			dsbd.lpwfxFormat	= NULL;

			pDriver_->CreateSoundBuffer(&dsbd, &pPrimary_, NULL);

		
			// 高品質化。44.1khz 16bit ステレオにする
			WAVEFORMATEX wfx;
			::ZeroMemory(&wfx, sizeof(WAVEFORMATEX)); 
			wfx.cbSize			= sizeof(WAVEFORMATEX);
			wfx.wFormatTag		= WAVE_FORMAT_PCM;
			wfx.nChannels		= 2; 
			wfx.nSamplesPerSec	= 44100; 
			wfx.wBitsPerSample	= 16; 
			wfx.nBlockAlign		= (wfx.wBitsPerSample / 8) * wfx.nChannels;
			wfx.nAvgBytesPerSec	= wfx.nSamplesPerSec * wfx.nBlockAlign;

			r = pPrimary_->SetFormat(&wfx);

			if(check_hresult(r)) 
				flag = DSSCL_PRIORITY;
			else
				logger::warnln("[ds_driver]高品質化できませんでした。");
		}
	}

	if(flag!=DSSCL_PRIORITY)
	{
		r = pDriver_->SetCooperativeLevel(hWnd, DSSCL_NORMAL);
		if(!check_hresult(r,"[ds_driver]SetCooperativeLevelが失敗しました。"))
		{
			fin();
			return false;
		}
	}

	const string sFlag = flag==DSSCL_PRIORITY ? "PRIORITY" : "NORMAL";
	logger::infoln("[ds_driver]DirectSound初期化しました。" + sFlag);

	return true;
}

void ds_driver::fin()
{
	safe_release(pPrimary_);
	safe_release(pDriver_);
}

} // namespace sound end
} // namespace mana end
