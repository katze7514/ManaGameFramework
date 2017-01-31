#include "../mana_common.h"

#include "../Sound//sound_player.h"

#include "audio_context.h"
#include "audio.h"

#include "audio_player.h"

namespace mana{
namespace audio{

///////////////////////////////////

audio_player::audio_player():eState_(STATE_NONE), eBgmState_(BGM_NONE),nBgmIndex_(0),nFade_(0),nSeGcNum_(SE_GC_NUM),nSeGcFrame_(SE_GC_FRAME),nSeGcCounter_(0)
{
	eLoadState_.store(NONE, std::memory_order_release);

	pRoot_		=	new_ audio();

	pBgmRoot_	=	new_ audio();
	pRoot_->add_child(pBgmRoot_, 0);

	bgms_[0]	=	new_ audio();
	pBgmRoot_->add_child(bgms_[0], 0);
	bgms_[1]	=	new_ audio();
	pBgmRoot_->add_child(bgms_[1], 1);

	pSeRoot_	=	new_ audio();
	pRoot_->add_child(pSeRoot_, 1);

	pCtx_	=	new_ audio_context();

#ifdef MANA_DEBUG
	bReDefine_ = false;
#endif
}

audio_player::~audio_player()
{
	safe_delete(pRoot_);
	safe_delete(pCtx_);
}

bool audio_player::init(const shared_ptr<sound::sound_player>& pPlayer, uint32_t nSeGcFrame, uint32_t nSeGcNum)
{
	if(!pPlayer)
	{
		logger::fatalln("[audio_player]初期化 sound_playerのポインタが無効です。");
		return false;
	}

	pCtx_->set_player(pPlayer);
	
	eState_ = STATE_INIT;

	nSeGcNum_		= nSeGcNum;
	nSeGcFrame_		= nSeGcFrame;
	nSeGcCounter_	= 0;

	return true;
}

bool audio_player::exec(bool bWait)
{
	exec_bgm();
	if(!pCtx_->player()->start_request(bWait)) return false;
		pRoot_->exec(*pCtx_);
	pCtx_->player()->end_request();
	gc_se();

	return pCtx_->player()->play(bWait);
}

//////////////////////////////////
// Load

bool audio_player::load_sound_file(const string& sFile, bool bFile)
{
	if(state()==STATE_NONE)
	{
		logger::warnln("[audio_player]初期化されていません。");
		return false;
	}
	else if(state()==STATE_LOAD)
	{
		logger::warnln("[audio_player]定義ファイルロード中です。");
		return false;
	}

	if(!pCtx_->player()->start_request())
	{
		logger::warnln("[audio_player]start_requestが実行できませんでした。");
		return false;
	}

	if(bFile)
		logger::infoln("[audio_player]定義ファイルを読み込みます。: " + sFile);
	else
		logger::infoln("[audio_player]定義ファイルを読み込みます。");

#ifdef MANA_DEBUG
	pCtx_->player()->request_redefine(bReDefine_);
#endif

	auto result = [this](bool b){  eLoadState_.store(b ? SUCCESS : FAIL, std::memory_order_release); };
	pCtx_->player()->request_info_load(sFile,result,bFile);

	pCtx_->player()->end_request();

	// ファイルの読み直しなのでIDキャッシュクリア
	cacheID_.clear();

	eLoadState_.store(EXEC, std::memory_order_release);

	eState_ = STATE_LOAD;

	return true;
}

audio_player::load_result audio_player::is_fin_load()
{
	load_result r = static_cast<load_result>(eLoadState_.load(std::memory_order_acquire));
	if(r==SUCCESS || r==FAIL) eState_ = STATE_INIT;
	return r;
}

/////////////////////////////////
// master

void audio_player::set_master_volume(const sound::volume& vol)
{
	pRoot_->set_volume(vol);
}

const sound::volume& audio_player::master_volume()const
{
	return pRoot_->volume();
}

////////////////////////////////
// BGM
void audio_player::exec_bgm()
{
	switch(eBgmState_)
	{
	case BGM_FADE_CHANGE:
		if(!playing_bgm()->is_fade())
		{// 準備済みの曲をフェードインする
			playing_bgm()->stop();
			play_new_bgm(0, nFade_);
			eBgmState_ = BGM_NONE;
		}
	break;

	case BGM_FADE_STOP: // フェードが終わったら止める
		if(!playing_bgm()->is_fade())
		{
			playing_bgm()->stop();
			eBgmState_ = BGM_NONE;
		}
	break;

	case BGM_CROSS_STOP:
		if(!playing_past_bgm()->is_fade())
		{
			playing_past_bgm()->stop();
			eBgmState_ = BGM_NONE;
		}
	break;
	}
}

bool audio_player::play_bgm(uint32_t nID, uint32_t nFade, change_mode eChangeMode)
{
	// 何かしらの処理中は変更不可
	if(eBgmState_!=BGM_NONE) return false;

	audio* pPlayBgm = playing_bgm();

	if(nID==0)
	{// 0の時は現在のBGMのplayを呼ぶだけ。ポーズ解除に使う
		pPlayBgm->play(true);

		return true;
	}

	// 最新のイベントを確認
	switch(pPlayBgm->sound_event())
	{
	case sound::EV_STOP:
	case sound::EV_END:
	// 再生されていないなら、そのまま再生開始
		play_new_bgm(nID, nFade);
	break;

	case sound::EV_PLAY:
	{// 再生中ならchante_modeに合わせて処理

		// 指定したIDのBGMがすでに再生中だったら何もしない
		if(pPlayBgm->sound_id()==nID) return true;

		switch(eChangeMode)
		{
		case CHANGE_STOP:
			// 再生中のを停止
			pPlayBgm->stop();
			// 新しいBGMを再生開始
			play_new_bgm(nID, nFade);
		break;

		case CHANGE_FADEOUT:
		{
			sound::volume end;
			end.set_per(0);
			pPlayBgm->fade_volume(end,nFade);
			nFade_ = nFade;

			// 次に再生するIDだけセットしておく
			if(playing_past_bgm()->sound_id()!=nID)
				playing_past_bgm()->set_sound_id(nID);

			eBgmState_ = BGM_FADE_CHANGE;
		}
		break;

		case CHANGE_CROSS:
			if(nFade==0)
			{// Fadeが0の時は即座入れ替え
				// 再生中のを停止
				pPlayBgm->stop();

				// 新しいBGMを再生開始
				play_new_bgm(nID,0);
			}
			else
			{// 再生中をフェードアウト
				sound::volume end;
				end.set_per(0);
				pPlayBgm->fade_volume(end,nFade);

				// 新しいBGMを再生開始
				play_new_bgm(nID,nFade);

				eBgmState_ = BGM_CROSS_STOP;
			}
		break;

		default: break;
		}
	}
	break;

	case sound::EV_PAUSE:
		if(pPlayBgm->sound_id()==nID)
			pPlayBgm->play(true);
	break;

	default:
	break;
	}

	return true;
}

void audio_player::stop_bgm(uint32_t nFadeOut)
{
	audio* pPlayBgm = playing_bgm();

	switch(pPlayBgm->sound_event())
	{
	case sound::EV_PLAY:
		if(nFadeOut>0)
		{
			sound::volume vol;
			vol.set_per(0);
			pPlayBgm->fade_volume(vol, nFadeOut);
			eBgmState_ = BGM_FADE_STOP;
		}
		else
		{
			pPlayBgm->stop();
		}
	break;

	case sound::EV_PAUSE:
		pPlayBgm->stop();
	break;

	default:
	break;
	}
}

void audio_player::pause_bgm()
{
	audio* pPlayBgm = playing_bgm();
	if(pPlayBgm->sound_event()==sound::EV_PLAY)
	{
		pPlayBgm->pause();
	}
}

void audio_player::set_bgm_volume(const sound::volume& vol)
{
	pBgmRoot_->set_volume(vol);
}

const sound::volume& audio_player::bgm_volume()const
{
	return pBgmRoot_->volume();
}

void audio_player::fade_bgm_volume(const sound::volume& start, const sound::volume& end, int32_t nFadeFrame)
{
	pBgmRoot_->fade_volume(start, end, nFadeFrame);
}

void audio_player::fade_bgm_volume(const sound::volume& end, int32_t nFadeFrame)
{
	pBgmRoot_->fade_volume(end, nFadeFrame);
}

bool audio_player::is_playing_bgm()const
{
	return playing_bgm()->sound_event()==sound::EV_PLAY;
}

void audio_player::play_new_bgm(uint32_t nID, uint32_t nFade)
{
	nBgmIndex_^=1;
	audio* pPlayBgm = playing_bgm();

	if(nID>0 && pPlayBgm->sound_id()!=nID)
		pPlayBgm->set_sound_id(nID);

	pPlayBgm->init();

	if(nFade>0)
	{
		sound::volume start,end;
		start.set_per(0);
		end.set_per(100);
		pPlayBgm->fade_volume(start,end,nFade);
	}
	else
	{
		sound::volume vol;
		vol.set_per(100);
		pPlayBgm->set_volume(vol);
	}
		
	pPlayBgm->play(true);
}

audio* audio_player::playing_bgm()
{
	return bgms_[nBgmIndex_];
}

const audio* audio_player::playing_bgm()const
{
	return bgms_[nBgmIndex_];
}

audio* audio_player::playing_past_bgm()
{
	return bgms_[nBgmIndex_^1];
}

///////////////////////////////
// se
void audio_player::gc_se()
{
	// GCカウント
	if(nSeGcCounter_++ > nSeGcFrame_)
	{
		nSeGcCounter_ = 0;

		// GCをする個数以下だったらここで終了
		if(pSeRoot_->count_children()<nSeGcNum_) return;

		// GC開始
		list<uint32_t> removeChildren;
		// LRUフラグチェック
		for(auto child : pSeRoot_->children())
		{
			if(child->is_lru_flag())
				child->reset_lru_flag();
			else
				removeChildren.push_back(child->id());
		}

		// 削除対象を削除
		for(auto rc : removeChildren)
			pSeRoot_->remove_child(rc, true);
	}
}

void audio_player::play_se(uint32_t nID, bool bLoop, bool bForce)
{
	audio* pSE = pSeRoot_->child(nID);
	if(!pSE)
	{
		pSE = new audio();
		pSE->set_sound_id(nID);
		pSeRoot_->add_child(pSE, nID);
		pSE->init();
	}

	pSE->play(bLoop);

	// 再生強制フラグが立ってるなら再生位置を最初に戻す
	if(bForce) pSE->set_pos(0);
}

void audio_player::stop_se(uint32_t nID)
{
	audio* pSE = pSeRoot_->child(nID);
	if(pSE) pSE->stop();
}

void audio_player::pause_se(uint32_t nID)
{
	audio* pSE = pSeRoot_->child(nID);
	if(pSE) pSE->pause();
}

void audio_player::set_se_volume(uint32_t nID, const sound::volume& vol)
{
	audio* pSE = pSeRoot_->child(nID);
	if(pSE) pSE->set_volume(vol);
}

void audio_player::fade_se_volume(uint32_t nID, const sound::volume& start, const sound::volume& end, int32_t nFadeFrame)
{
	audio* pSE = pSeRoot_->child(nID);
	if(pSE) pSE->fade_volume(start, end, nFadeFrame);
}

void audio_player::fade_se_volume(uint32_t nID, const sound::volume& end, int32_t nFadeFrame)
{
	audio* pSE = pSeRoot_->child(nID);
	if(pSE) pSE->fade_volume(end, nFadeFrame);
}

bool audio_player::is_playing_se(uint32_t nID)const
{
	audio* pSE = pSeRoot_->child(nID);
	if(pSE) return pSE->sound_event()==sound::EV_PLAY;
	return false;
}

void audio_player::stop_se_all()
{
	for(auto child : pSeRoot_->children())
		child->stop();
}

void audio_player::set_se_master_volume(const sound::volume& vol)
{
	pSeRoot_->set_volume(vol);
}

const sound::volume& audio_player::se_master_volume()const
{
	return pSeRoot_->volume();
}

///////////////////////////////
// util

uint32_t audio_player::sound_id(const string& sID)
{
	auto it = cacheID_.find(sID);
	if(it!=cacheID_.end()) return it->second;

	optional<uint32_t> id = pCtx_->player()->sound_id(sID);
	if(id)
	{
		cacheID_.emplace(sID, *id);
		return *id;
	}

	logger::warnln("[audio_player]サウンドIDが登録されていません。: " + sID);
	return 0;
}

} // namespace audio end
} // namespace mana end
