#include "../mana_common.h"

#include "../Concurrent/lock_helper.h"

#include "ds_sound_player.h"

namespace mana{
namespace sound{

////////////////////////////////////
// 初期化と終了
///////////////////////////////////
bool ds_sound_player::init(HWND hWnd, uint32_t nReserveSoundNum, bool bHighQuarity)
{
	if(driver_.driver())
	{
		logger::debugln("[ds_sound_player]初期化済みです");
		return true;
	}

	if(!driver_.init(hWnd,bHighQuarity))
		return false;

	ds_sound::set_enable_sample_rate(driver_.caps().dwMinSecondarySampleRate, driver_.caps().dwMaxSecondarySampleRate);

	hashSound_.reserve(nReserveSoundNum);
	soundIdMgr_.init(nReserveSoundNum,1,"");

	// 無音バッファを作成し再生を始める
	create_silent_sound(bHighQuarity);

	logger::debugln("[ds_sound_player]初期化しました。");

	return true;
}

void ds_sound_player::fin()
{
	hashSound_.clear();
	safe_release(pSilentBuffer_);

	driver_.fin();
}

////////////////////////////////////
// アップデート
///////////////////////////////////
bool ds_sound_player::update(uint32_t nElapsedMillSec)
{
	if(!driver_.driver()) return false;

	// logger::traceln("[ds_sound_player] elapsed : " + to_str(nElapsedMillSec));
	list<ds_sound*>::iterator it = activeSound_.begin();
	while(it!=activeSound_.end())
	{
		(*it)->update(nElapsedMillSec);

		if((*it)->is_stop()) // 再生が終わったらリストから削除
			it = activeSound_.erase(it);
		else
			++it;
	}

	return true;
}

void ds_sound_player::set_default_volume(const volume& def)
{
	defaultVolume_ = def;
}

////////////////////////////////////
// サウンド情報
///////////////////////////////////
bool ds_sound_player::add_sound_info(const string& sID, const string& sFilePath, bool bStreaming, float fStreamLoopSec, uint32_t nStreamSec)
{
	uint32_t nID = soundIdMgr_.assign_id(sID);

	auto r = hashSound_.emplace(nID, sound_info());

	if(!r.second)
	{
	#ifdef MANA_DEBUG
		if(bReDefine_)
		{// リロード時は強制リリースして上書きする
			r.first->second.snd_.release_force();
		}
		else
		{
	#endif

		soundIdMgr_.erase_id(nID);

		if(r.first->second.snd_.filepath()==sFilePath)
		{
			logger::debugln("[ds_sound_player]サウンド情報は登録済みです。 : " + sID );
			return true;
		}
		else
		{
			logger::warnln("[ds_sound_player]サウンド情報を追加できませんでした。:" + sID + " " + sFilePath);
			return false;
		}

	#ifdef MANA_DEBUG
		}
	#endif
	}

	sound_info& info = r.first->second;
	info.snd_.init(sFilePath, defaultVolume_, bStreaming, fStreamLoopSec, nStreamSec);
	info.it_ = listSound_.end();

	return true;
}

void ds_sound_player::remove_sound_info(uint32_t nID)
{
	sound_hash::iterator it = hashSound_.find(nID);
	if(it!=hashSound_.end())
	{
		soundIdMgr_.erase_id(nID);
		
		if(it->second.it_!=listSound_.end())
			listSound_.erase(it->second.it_);

		hashSound_.erase(it);
	}
}

bool ds_sound_player::is_sound_info(uint32_t nID)const
{
	sound_hash::const_iterator it = hashSound_.find(nID);
	return it!=hashSound_.end();
}

bool ds_sound_player::load_sound_info_file(const string& sFilePath, bool bFile)
{/*
  *	<sound_def>
  *		<sound !id="" src="" stream="" !stream_loop_sec="" !strem_sec="" />
  *	</sound_def>
  */

	if(bFile)
		logger::infoln("[ds_sound_player]サウンド定義ファイルを読み込みます。: " + sFilePath);
	else
		logger::infoln("[ds_sound_player]サウンド定義ファイルを読み込みます。");

	std::stringstream ss;
	if(!file::load_file_to_string(ss, sFilePath, bFile))
	{
		logger::warnln("[ds_sound_player]サウンド情報ファイルが読み込めませんでした。");
		return false;
	}

	using namespace p_tree;
	ptree stree;
	xml_parser::read_xml(ss, stree, xml_parser::no_comments);
	
	optional<ptree&> sound_def = stree.get_child_optional("sound_def");
	if(!sound_def)
	{
		logger::warnln("[ds_sound_player]サウンド情報ファイルのフォーマットが間違ってます。： <sound_def>が存在しません。");
		return false;
	}

	uint32_t nSoundNum=1;
	for(auto& it : *sound_def)
	{
		if(it.first=="sound")
		{
			string			sID				= it.second.get<string>("<xmlattr>.id","");
			string			sSrc			= it.second.get<string>("<xmlattr>.src","");
			optional<bool>	bStream			= it.second.get_optional<bool>("<xmlattr>.stream");
			float			fStreamLoopSec	= it.second.get<float>("<xmlattr>.stream_loop_sec",0.0f);
			uint32_t		nStreamSec		= it.second.get<uint32_t>("<xmlattr>.stream_sec",STREAM_SECONDS);

			if(!(sSrc.empty() || !bStream))
			{
				if(sID.empty()) sID = sSrc; // IDが空の時はsrcをIDにする

				add_sound_info(sID, sSrc, *bStream, fStreamLoopSec, nStreamSec);
			}
			else
			{
				logger::warnln("[ds_sound_player]" + to_str_s(nSoundNum) + "個目のサウンドタグのフォーマットが間違っています");
				return false;
			}

			++nSoundNum;
		}
	}

	return true;
}


////////////////////////////////////
// サウンド命令
///////////////////////////////////
void ds_sound_player::set_event_handler(uint32_t nID, const function<void(play_event)>& handler)
{
	ds_sound* pSnd = sound(nID);
	if(pSnd==nullptr) return;
	
	pSnd->set_event_handler(handler);
}

void ds_sound_player::play(uint32_t nID, bool bLoop)
{
	ds_sound* pSnd = sound(nID);
	if(pSnd==nullptr) return;
	
	pSnd->play(bLoop);

	if(!pSnd->is_play()) activeSound_.emplace_back(pSnd);
}

void ds_sound_player::stop(uint32_t nID)
{
	ds_sound* pSnd = sound(nID);
	if(pSnd==nullptr) return;
	
	if(pSnd->is_play()) pSnd->stop();
}

void ds_sound_player::pause(uint32_t nID)
{
	ds_sound* pSnd = sound(nID);
	if(pSnd==nullptr) return;
	
	if(pSnd->is_play()) pSnd->pause();
}

void ds_sound_player::fade_volume(uint32_t nID, const volume& fadeVol, uint32_t nFadeFrame)
{
	ds_sound* pSnd = sound(nID);
	if(pSnd==nullptr) return;
	
	if(pSnd->is_play()) pSnd->fade_volume(fadeVol,nFadeFrame);
}

void ds_sound_player::set_volume(uint32_t nID, const volume& vol)
{
	ds_sound* pSnd = sound(nID);
	if(pSnd==nullptr) return;
	
	pSnd->set_volume(vol);
}

void ds_sound_player::set_speed(uint32_t nID, float fSpeed)
{
	ds_sound* pSnd = sound(nID);
	if(pSnd==nullptr) return;

	pSnd->set_speed(fSpeed);
}

void ds_sound_player::set_pos(uint32_t nID, float fSec)
{
	ds_sound* pSnd = sound(nID);
	if(pSnd==nullptr) return;

	pSnd->set_pos(fSec);
}

play_mode ds_sound_player::sound_play_mode(uint32_t nID)const
{
	auto it = hashSound_.find(nID);
	if(it==hashSound_.end())
	{
		logger::warnln("[ds_sound_player]指定されたサウンドの情報がありません。: " + nID);
		return MODE_ERR;
	}

	return it->second.snd_.cur_play_mode();
}

void ds_sound_player::stop_all()
{
	for(list<ds_sound*>::iterator it = activeSound_.begin(); it!=activeSound_.end(); ++it)
		(*it)->stop();
}

void ds_sound_player::pause_all()
{
	for(list<ds_sound*>::iterator it = activeSound_.begin(); it!=activeSound_.end(); ++it)
		(*it)->pause();
}

////////////////////////////////////
// ヘルパー
////////////////////////////////////
bool ds_sound_player::create_sound_buffer(const string& sID)
{
	return sound(sID)!=nullptr;
}

ds_sound* ds_sound_player::sound(uint32_t nID)
{
	sound_hash::iterator it = hashSound_.find(nID);
	if(it==hashSound_.end())
	{
		logger::warnln("[ds_sound_player]指定されたサウンドの情報がありません。: " + sound_id(nID));
		return nullptr;
	}

	sound_info& info = it->second;
	ds_sound& snd = info.snd_;

	// バッファ作って無かったら作成
	if(!snd.is_create_buffer())
	{
		if(!snd.create_buffer(driver_))
			return nullptr;
	}
	else
	{
		listSound_.erase(info.it_);
	}

	listSound_.emplace_back(nID);
	info.it_ = --listSound_.end();

	limit_sound();

	return &(it->second.snd_);
}

void ds_sound_player::limit_sound()
{
	list<uint32_t>::iterator it = listSound_.begin();
	while(listSound_.size()>=nMaxSoundBufferNum_
	   && it!=listSound_.end())
	{
		sound_hash::iterator sit = hashSound_.find(*it);
		if(sit!=hashSound_.end())
		{
			sound_info& info = sit->second;
			if(info.snd_.is_stop())
			{
				info.snd_.fin();
				it = listSound_.erase(info.it_);
				continue;
			}
		}
		
		++it;
	}
}

void ds_sound_player::create_silent_sound(bool bHighQuarity)
{
	LPDIRECTSOUNDBUFFER pBuffer;

	WAVEFORMATEX wvFmt;
	wvFmt.wFormatTag	= WAVE_FORMAT_PCM;
	wvFmt.cbSize		= sizeof(wvFmt);
	wvFmt.nChannels		= 2;

	if(bHighQuarity)
	{
		wvFmt.nSamplesPerSec = 44100;
		wvFmt.wBitsPerSample = 16;
	}
	else
	{
		wvFmt.nSamplesPerSec = 22050;
		wvFmt.wBitsPerSample = 8;
	}

	wvFmt.nBlockAlign		= wvFmt.nChannels * wvFmt.wBitsPerSample / 8;
	wvFmt.nAvgBytesPerSec	= wvFmt.nSamplesPerSec * wvFmt.nBlockAlign;

	DSBUFFERDESC desc;
	::ZeroMemory(&desc, sizeof(desc));
	desc.dwSize			= sizeof(desc);
	desc.dwFlags		= DSBCAPS_LOCSOFTWARE;
	desc.lpwfxFormat	= &wvFmt;
	desc.dwBufferBytes	= wvFmt.nAvgBytesPerSec * 1; // 1秒の長さ
		
	HRESULT r =driver_.driver()->CreateSoundBuffer(&desc, &pBuffer, NULL);
	if(!check_hresult(r,"[ds_sound_player]無音サウンドが作成できませんでした。")) return;

	r = pBuffer->QueryInterface(IID_IDirectSoundBuffer8, reinterpret_cast<void**>(&pSilentBuffer_));
	if(!check_hresult(r,"[ds_sound_player]無音サウンドバッファをDirectSoundBuffer8に変換できませんでした。")) return;

	void* pData;
	DWORD nLen;
	r = pSilentBuffer_->Lock(0,0,&pData,&nLen,NULL,NULL,DSBLOCK_ENTIREBUFFER);
	if(r==DSERR_BUFFERLOST)
	{
		pSilentBuffer_->Restore();
		if(!check_hresult(r)) return;
		r = pSilentBuffer_->Lock(0,0,&pData,&nLen,NULL,NULL,DSBLOCK_ENTIREBUFFER);
	}
	if(!check_hresult(r,"[ds_sound_player]無音サウンドバッファをロックできませんでした。")) return;

	// 無音で埋める
	memset(pData, 0, nLen);

	pSilentBuffer_->Unlock(pData, nLen, NULL, 0);

	pSilentBuffer_->Play(0,0,DSBPLAY_LOOPING);
}

} // namespace sound end
} // namespace mana end
