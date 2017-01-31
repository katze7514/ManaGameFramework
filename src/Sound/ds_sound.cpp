#include "../mana_common.h"

#include "ds_driver.h"
#include "ds_sound.h"

namespace mana{
namespace sound{

ds_sound::ds_sound():pSoundBuffer_(nullptr),nSoundBufferSize_(0),nSoundDataSize_(0),
								   bStreaming_(false),nStreamCheckMs_(0),nStreamLastByte_(0),fStreamLoopSec_(0.0f),
								   eEvent_(sound::EV_END),
								   bLoop_(false),bEndData_(false),cmdIndex_(0),
								   fSpeedCur_(1.0f),fPosCur_(0.0f),
								   bVolUpdate_(false),bSpeedUpdate_(false),bPosUpdate_(false),
								   nVolFadeFrame_(0),nVolFadeCounter_(0),nStreamElapsedTime_(0)
{
	init_cmd();

	handler_ = [](play_event eEvent){};
}

void ds_sound::fin()
{
	reader_.fin();
	safe_release(pSoundBuffer_);
}

///////////////////////////////////////
// サウンドバッファ作成
///////////////////////////////////////
void ds_sound::init(const string& sFilename, const volume& vol, bool bStreaming, float fStreamLoopSec, uint32_t nStreamSec)
{
	bStreaming_		= bStreaming;
	fStreamLoopSec_ = fStreamLoopSec;
	bLoop_			= bStreaming_;
	sFilePath_		= sFilename;
	volCur_			= vol;
	nStreamCheckMs_ = nStreamSec*1000 / 2; // チェック間隔はストリーミング秒数の半分

	init_cmd();
}

bool ds_sound::create_buffer(ds_driver& driver)
{
	if(pSoundBuffer_)
	{
		logger::debugln("[ds_sound]サウンドバッファは作成済みです。");
		return true;
	}

	uint32_t nStreamSec = nStreamCheckMs_*2 / 1000;

	// サウンドファイルをロードして準備
	if(!reader_.open(sFilePath_, bStreaming_))
		return false;

	if(reader_.waveformat()->nSamplesPerSec < MIN_SAMPLE_RATE 
	|| reader_.waveformat()->nSamplesPerSec > MAX_SAMPLE_RATE)
	{
		logger::fatalln("[ds_sound]再生できないサンプリングレートです。: " + to_str(reader_.waveformat()->nSamplesPerSec));
		return false;
	}

	// バッファ作成
	LPDIRECTSOUNDBUFFER pBuffer;
	DSBUFFERDESC desc;
	::ZeroMemory(&desc, sizeof(desc));
	desc.dwSize				= sizeof(desc);
	desc.dwFlags			= DSBCAPS_LOCSOFTWARE | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY | DSBCAPS_GETCURRENTPOSITION2;
	desc.lpwfxFormat		= reader_.waveformat();

	if(bStreaming_)
	{
		float fSoundSecs = static_cast<float>(reader_.data_size()) / static_cast<float>(desc.lpwfxFormat->nAvgBytesPerSec);
		if(fSoundSecs<nStreamSec)
		{
			logger::infoln("[ds_sound]ストリーム時間(" + to_str(nStreamSec) + ")より、サウンド時間(" + to_str(fSoundSecs) + ")が短いです。ストリーム時間を短くします。: " + to_str(sFilePath_));
			nStreamSec = static_cast<uint32_t>(fSoundSecs);
		}
	}

	if(bStreaming_)
		desc.dwBufferBytes	= desc.lpwfxFormat->nAvgBytesPerSec * nStreamSec;
	else
		desc.dwBufferBytes	= reader_.data_size() + desc.lpwfxFormat->nBlockAlign; // 1ブロックだけ多くいれて無音を追加する
		
	HRESULT r =driver.driver()->CreateSoundBuffer(&desc, &pBuffer, NULL);
	if(!check_hresult(r,"[ds_sound]サウンドバッファが作成できませんでした。")) return false;

	r = pBuffer->QueryInterface(IID_IDirectSoundBuffer8, reinterpret_cast<void**>(&pSoundBuffer_));
	if(!check_hresult(r,"[ds_sound]DirectSoundBuffer8に変換できませんでした。")) return false;

	nSoundBufferSize_	= desc.dwBufferBytes;
	nSoundDataSize_		= reader_.data_size();

	// 最初のデータ書き込み
	if(!full_sound_buffer(true))
	{
		fin();
		return false;
	}

	if(!bStreaming_)
	{// 一括読み込みの時は、ファイルはもういらないので閉じる
		reader_.close();
	}

	logger::debugln("[ds_sound]サウンドバッファを作成しました。: " + sFilePath_ + " " + to_str_s(desc.dwBufferBytes) + to_str_s(volCur_.per()) + to_str_s(bStreaming_));

	return true;
}

void ds_sound::set_event_handler(const function<void(play_event)>& handler)
{
	handler_(sound::EV_OVERRIDE);
	handler_ = handler;
	handler_(eEvent_); // 新しいハンドラには最新のイベントを送っておく
}

///////////////////////////////////////
// サウンドメイン処理
///////////////////////////////////////
bool ds_sound::update(uint32_t nElapsedMillSec)
{
	HRESULT r;

	// 現在の情報を反映
	update_volume();
	update_speed();
	update_pos();

	// コマンドが積まれてたら処理
	switch(next_cmd().eMode_)
	{
	case MODE_STOP:
		stop_sound_buffer();
	break;

	case MODE_PLAY:
	case MODE_PLAY_LOOP:
	{
		DWORD flag=0;

		if(next_cmd().eMode_==MODE_PLAY_LOOP)
		{
			flag = DSBPLAY_LOOPING;
			bLoop_=true;
		}
		else
		{
			bLoop_=false;

			if(bStreaming_)	flag = DSBPLAY_LOOPING;
		}

		r = pSoundBuffer_->Play(0,0,flag);

		if(r==DSERR_BUFFERLOST)
		{// 再生データを復帰
			r = pSoundBuffer_->Restore();
			if(!check_hresult(r))
			{// リストア出来なかったら、stop扱いにしておく
				next_cmd().eMode_ = MODE_STOP;
			}
			else
			{
				full_sound_buffer(!bStreaming_);
			}
		}
		else
		{
			eEvent_ = sound::EV_PLAY;
			handler_(eEvent_);
		}
	}
	break;

	case MODE_PAUSE:
		if(!is_stop())
		{
			pSoundBuffer_->Stop();
			set_pos(-1.0f);

			eEvent_ = sound::EV_PAUSE;
			handler_(eEvent_);
		}
	break;
	}
	

	// コマンド変更
	if(next_cmd().eMode_!=MODE_NONE) swap_cmd();

	// 再生処理
	if(is_play())
	{
		// Fadeボリューム処理
		if(nVolFadeFrame_>0 && (nVolFadeCounter_<=nVolFadeFrame_))
		{
			float sub = ((volFade_.per() - volFadeBase_.per()) / static_cast<float>(nVolFadeFrame_));
			float volPer = static_cast<float>(volFadeBase_.per()) + sub * nVolFadeCounter_;
			volCur_.set_per(static_cast<uint32_t>(volPer));

			if(nVolFadeCounter_==nVolFadeFrame_)
			{// フェード終了
				pSoundBuffer_->SetVolume(volFade_.db());

				if(volCur_.per()==0)
				{// 音量0になったらSTOP
					volCur_ = volFadeBase_; // 音量は元に戻す
					cur_cmd().eMode_=MODE_STOP;
					stop_sound_buffer();
				}
				else
				{// 0でないなら通常再生へ
					set_volume(volFade_);
					cur_cmd().eMode_ = bLoop_ ? MODE_PLAY_LOOP : MODE_PLAY;
				}
				nVolFadeFrame_ = 0;
			}
			else if(nVolFadeCounter_%2)
			{// 2回に1度設定にする
				pSoundBuffer_->SetVolume(volCur_.db());
			}
			++nVolFadeCounter_;
		}

		// ストリーム処理
		nStreamElapsedTime_ += nElapsedMillSec;
		if(nStreamElapsedTime_ >= nStreamCheckMs_) // チェック時間が来た
		{
			if(bStreaming_)
			{
				stream_sound_buffer();
			}
			else
			{
				DWORD status;
				pSoundBuffer_->GetStatus(&status);
				if(!bit_test<uint32_t>(status,DSBSTATUS_PLAYING)) 
				{
					cur_cmd().eMode_ = MODE_STOP;
					stop_sound_buffer();
				}

				nStreamElapsedTime_=0;
			}
		}
	}

	return true;
}

///////////////////////////////////////
// サウンドコマンド
///////////////////////////////////////
bool ds_sound::is_play()const
{
	return cur_play_mode()==MODE_PLAY
		|| cur_play_mode()==MODE_PLAY_LOOP
		;
}

bool ds_sound::is_stop()const
{
	return cur_play_mode()==MODE_STOP
		|| cur_play_mode()==MODE_NONE
		;
}

void ds_sound::play(bool bLoop)
{
	if(bLoop)
		next_cmd().eMode_	= MODE_PLAY_LOOP;
	else
		next_cmd().eMode_	= MODE_PLAY;
}

void ds_sound::stop()
{
	volume vol;
	vol.set_per(0);
	fade_volume(vol,4);
}

void ds_sound::pause()
{
	next_cmd().eMode_	= MODE_PAUSE;
}

void ds_sound::fade_volume(const volume& fadeVol, uint32_t nFadeFrame)
{
	volFadeBase_		= volCur_;
	volFade_			= fadeVol;
	nVolFadeFrame_		= nFadeFrame;
	nVolFadeCounter_	= 0;
}

void ds_sound::set_volume(const volume& vol)
{
	if(volCur_.db()==vol.db()) return;
	volCur_ = vol;
	bVolUpdate_ = true;
}

void ds_sound::set_speed(float fSpeed)
{
	if(fSpeedCur_==fSpeed) return;
	fSpeedCur_ = fSpeed;
	bSpeedUpdate_ = true;
}

void ds_sound::set_pos(float fSec)
{
	if(fPosCur_==fSec) return;
	fPosCur_ = fSec;
	bPosUpdate_ = true;
}

void ds_sound::update_volume()
{
	if(!bVolUpdate_) return;

	if(is_create_buffer())
		pSoundBuffer_->SetVolume(volCur_.db());

	bVolUpdate_ = false;
}

void ds_sound::update_speed()
{
	if(!bSpeedUpdate_) return;

	DWORD nFreq = clamp(static_cast<DWORD>(reader_.waveformat()->nSamplesPerSec * fSpeedCur_), 
						MIN_SAMPLE_RATE, MAX_SAMPLE_RATE);

	if(is_create_buffer()) pSoundBuffer_->SetFrequency(nFreq);

	bSpeedUpdate_ = false;
}

void ds_sound::update_pos()
{
	if(!bPosUpdate_) return;

	if(is_create_buffer() && fPosCur_>=0.0f)
	{
		if(bStreaming_)
		{
			full_sound_buffer(true);
		}
		else
		{
			DWORD nPos = static_cast<DWORD>(reader_.waveformat()->nAvgBytesPerSec * fPosCur_);

			HRESULT r = 0;

			if(nPos < nSoundBufferSize_)
				r = pSoundBuffer_->SetCurrentPosition(nPos);
		}
	}

	bPosUpdate_ = false;
}

void ds_sound::init_cmd()
{
	for(auto& e : soundCmd_)
	{
		e.eMode_	= MODE_NONE;
	}

	cmdIndex_ = 0;
}

///////////////////////////////////////
// サウンドバッファへの書き込み
///////////////////////////////////////
bool ds_sound::full_sound_buffer(bool bBegin)
{
	bool bOpen=false;
	// ファイルを閉じちゃってたら読み出す
	if(bStreaming_ && !reader_.is_open()
	||!bStreaming_ && reader_.data()==nullptr)
	{
		bOpen=true;
		if(!reader_.open(sFilePath_, bStreaming_))
			return false; // 読み込み失敗したら終了
	}

	// 最初のデータ書き込み
	void* pData;
	DWORD nLen;

	HRESULT r = pSoundBuffer_->Lock(0,0,&pData,&nLen,NULL,NULL,DSBLOCK_ENTIREBUFFER);
	if(r==DSERR_BUFFERLOST)
	{// 一度だけRestoreを試みる
		r = pSoundBuffer_->Restore();
		if(!check_hresult(r,"[ds_sound]リストアできませんでした。")) return false;

		r = pSoundBuffer_->Lock(0,0,&pData,&nLen,NULL,NULL,DSBLOCK_ENTIREBUFFER);
	}
	if(!check_hresult(r,"[ds_sound]ロックできませんでした。")) return false;

	DWORD nWriteSize = nSoundBufferSize_;

	if(bBegin)
	{
		if(fPosCur_>0) reader_.seek_time(fPosCur_);
		else           reader_.seek_time(0.0f);

		// ストリーミングの時に目一杯書くと最初の一回だけ途切れることがあるので
		// 9割書きにしておく
		if(bStreaming_) nWriteSize = static_cast<DWORD>(nWriteSize * 0.9f);
	}

	uint32_t rsize = reader_.read(reinterpret_cast<BYTE*>(pData), nWriteSize, bLoop_, fStreamLoopSec_);

	if(rsize<nWriteSize)
	{// 足りない所は無音(0)で埋めておく
		memset(&reinterpret_cast<BYTE*>(pData)[rsize], 0, nSoundBufferSize_-rsize);
	}
	
	pSoundBuffer_->Unlock(pData, rsize, NULL, 0);

	nStreamElapsedTime_ = 0; // 書き込み経過時間リセット

	bEndData_			= rsize<nWriteSize; // 読み込みが要求サイズより小さいならデータ終了
	nStreamLastByte_	= rsize;

	if(bOpen && !bStreaming_)
	{
		reader_.close();
	}

	if(bBegin) pSoundBuffer_->SetCurrentPosition(0);

	return true;
}

bool ds_sound::stream_sound_buffer()
{
	DWORD plyPos,wrtPos;
	HRESULT r = pSoundBuffer_->GetCurrentPosition(&plyPos, &wrtPos);
	if(!check_hresult(r,"[ds_sound]再生位置が取得できませんでした。")) return false;

	// ループ指定なしで終了フラグ立ってたら終了チェック
	if(bEndData_)
	{
		if(nStreamLastByte_ <= plyPos)
		{// 終了位置過ぎてるので停止
			cur_cmd().eMode_=MODE_STOP;
			return stop_sound_buffer();
		}
	}
	
	// 書き込み範囲を決める
	DWORD writeStart[2]={0,0};
	DWORD writeEnd[2]={0,0};
	DWORD writeSize;

	if(nStreamLastByte_ < plyPos) // l < p
	{
		writeStart[0] = nStreamLastByte_;
		writeEnd[0]   = plyPos;

		if(writeStart[0]<wrtPos) // l < w < p
		{		
			writeStart[0] = wrtPos;
		}
	}
	else // p < l
	{
		writeStart[0]	= nStreamLastByte_;
		writeEnd[0]		= nSoundBufferSize_;

		writeStart[1]	= 0;
		writeEnd[1]		= plyPos;

		if(nStreamLastByte_ < wrtPos) // p < l < w
		{
			writeStart[0] = wrtPos;
		}
		else if(plyPos > wrtPos) // w < p < l
		{
			writeStart[0]   = wrtPos;
			writeEnd[0]		= plyPos;
			writeStart[1]=writeEnd[1]=0;
		}
	}

	// 書き込みサイズ
	DWORD nDataSize[2]={0,0};
	nDataSize[0] = writeEnd[0]-writeStart[0];
	nDataSize[1] = writeEnd[1]-writeStart[1];

	writeSize = nDataSize[0] + nDataSize[1];
	
	// 書き込み
	BYTE* pData[2]={NULL,NULL};
	DWORD nSize[2]={0,0};

	r = pSoundBuffer_->Lock(writeStart[0], writeSize,
							reinterpret_cast<void**>(&pData[0]), &nSize[0],
							reinterpret_cast<void**>(&pData[1]), &nSize[1], 
							0);

	if(!check_hresult(r)) return false; // Lockできないことが正しい位置に書くことを保証するのでエラーを許容する

	nDataSize[0] = reader_.read(pData[0], nSize[0], bLoop_, fStreamLoopSec_);
	if(reader_.is_data_file_end())
	{
		bEndData_=true;
		// 残りを無音0で埋めておく
		memset(&pData[0][nDataSize[0]], 0, nSize[0]-nDataSize[0]);
	}
	else if(pData[1]!=NULL)
	{
		nDataSize[1] = reader_.read(pData[1], nSize[1], bLoop_, fStreamLoopSec_);
		if(reader_.is_data_file_end())
		{
			bEndData_=true;
			// 残りを無音0で埋めておく
			memset(&pData[1][nDataSize[1]], 0, nSize[1]-nDataSize[1]);
		}
	}

	pSoundBuffer_->Unlock(pData[0], nDataSize[0], pData[1], nDataSize[1]);

	// 最後の書き込み位置を設定
	nStreamLastByte_ += nDataSize[0];
	
	if(pData[1]!=NULL)
		nStreamLastByte_  += nDataSize[1];

	if(nStreamLastByte_>nSoundBufferSize_)
		nStreamLastByte_ -= nSoundBufferSize_;

	nStreamElapsedTime_ = 0; // 書き込み時間リセット

	return true;
}

bool ds_sound::stop_sound_buffer()
{
	pSoundBuffer_->Stop();

	set_pos(0.0f);
	bEndData_ = false;

	eEvent_ = sound::EV_STOP;
	handler_(eEvent_);

	return true;
}


uint32_t ds_sound::MIN_SAMPLE_RATE=0;
uint32_t ds_sound::MAX_SAMPLE_RATE=UINT_MAX;

void ds_sound::set_enable_sample_rate(uint32_t nMin, uint32_t nMax)
{
	MIN_SAMPLE_RATE=nMin;
	MAX_SAMPLE_RATE=nMax;
}

#ifdef MANA_DEBUG
void ds_sound::release_force()
{
	if(pSoundBuffer_)
	{
		pSoundBuffer_->Stop();
		fin();
		nSoundBufferSize_  = 0;
		nSoundDataSize_	   = 0;
	}
}
#endif

} // namespace sound end
} // namespace mana end
