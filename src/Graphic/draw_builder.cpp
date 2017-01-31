#include "../mana_common.h"

#include "draw_data.h"
#include "draw_base.h"
#include "label.h"
#include "sprite.h"
#include "audio_frame.h"
#include "message.h"
#include "movieclip.h"
#include "timeline.h"
#include "keyframe.h"

#include "draw_builder.h"

namespace mana{

using namespace graphic;

namespace graphic{

draw_builder::draw_builder():eLoadState_(LOAD_FIN),nLoadWait_(0),nLoadFin_(0)
{ 
	bLoadErr_.store(false, std::memory_order_release); 

#ifdef MANA_DEBUG
	bReDefine_ = false;
#endif
}

draw_builder::~draw_builder()
{
	eLoadState_=LOAD_FIN;
	clear();
}

void draw_builder::clear()
{
	// ロード中だったら保留
	if(eLoadState_==LOAD_START || eLoadState_==LOAD_TEX_FONT_WAIT || eLoadState_==LOAD_NODE) return;

	for(auto& it : hashNode_) safe_delete(it.second);
	hashNode_.clear();

	eLoadState_	= LOAD_FIN;
	nLoadWait_	= 0;
	nLoadFin_.store(0, std::memory_order_release);
}

draw_base* draw_builder::create_draw_base(const string_fw& sNodeID)
{
	if(eLoadState_!=LOAD_FIN) return nullptr;

	draw_hash::iterator it = hashNode_.find(sNodeID);
	if(it==hashNode_.end())
	{
		logger::warnln("[draw_builder]描画ベース情報がありません。: " + sNodeID.get());
		return nullptr;
	}

	draw_base* pNode = create_draw_from_data(it->second);
	if(!pNode)
	{
		logger::warnln("[draw_builder]描画ベースが生成できませんでした。: " + sNodeID.get());
	}

	return pNode;
}

void draw_builder::set_draw_base(draw_base* pNode, draw_base_data* pData, bool bChildren)
{
#ifdef MANA_DEBUG
	pNode->set_debug_name(pData->sDebugName_);
#endif

	if(pData->is_over(draw_base_data::FLAG_DRAW_INFO))
		pNode->set_draw_info(pData->drawInfo_);

	if(pData->is_over(draw_base_data::FLAG_COLOR))
	{
		pNode->set_color(pData->nColor_);
		pNode->set_color_mode(pData->eColorMode_);
	}
	
	if(pData->is_over(draw_base_data::FLAG_PIVOT))
		pNode->set_pivot(pData->pivot_);

	if(!bChildren || !pData->is_over(draw_base_data::FLAG_CHILDREN)) return;
	
	if(pData->vecChildren_.size()>0 && pNode->count_children()>0)
		pNode->clear_child();

	uint32_t nID=0;
	for(auto& it : pData->vecChildren_)
	{
		draw_base* pChild = create_draw_switch(it);
		if(pChild)
		{
			pNode->set_id(nID);
			pNode->add_child(pChild, nID++);
			pChild->init();
		}
	}
	pNode->shrink_children();
}

draw_base* draw_builder::create_draw_switch(draw_base_data* pData)
{
	if(!pData) return nullptr;

	if(pData->sID_.get().empty())
	{
		return create_draw_from_data(pData);
	}
	else
	{
		draw_base* pNode = create_draw_base(pData->sID_);
		set_draw_base(pNode, pData);
		return pNode;
	}
}

draw_base* draw_builder::create_draw_from_data(draw_base_data* pData)
{
	switch(pData->eKind_)
	{
	case DRAW_LABEL:		return create_label(pData);
	case DRAW_SPRITE:		return create_sprite(pData);
	case DRAW_AUDIO_FRAME:	return create_audio(pData);
	case DRAW_MESSAGE:		return create_message(pData);
	case DRAW_TIMELINE:		return create_timeline(pData);
	case DRAW_MOVIECLIP:	return create_movieclip(pData);
	case DRAW_BASE:			return create_draw_inner(pData);
	default:				return nullptr;
	}
}

label* draw_builder::create_label(draw_base_data* pData)
{
	label_data* pLabelData = static_cast<label_data*>(pData);

	label* pLabel = new_ label(false,pLabelData->vecChildren_.size());
	set_draw_base(pLabel, pLabelData);
	pLabel->set_text_data(pLabelData->pTextData_);

	return pLabel;
}

sprite* draw_builder::create_sprite(draw_base_data* pData)
{
	sprite_data* pSpriteData = static_cast<sprite_data*>(pData);
	if(pSpriteData->nTexID_==0)
	{
		logger::warnln("[draw_builder][create_sprite]テクスチャロードが完了してません。");
		return nullptr;
	}

	sprite* pSprite = new_ sprite(pSpriteData->vecChildren_.size());
	set_draw_base(pSprite, pSpriteData);
	pSprite->set_tex_id(pSpriteData->nTexID_);
	pSprite->set_tex_rect(pSpriteData->rect_);

	return pSprite;
}

audio_frame* draw_builder::create_audio(draw_base_data* pData)
{
	audio_data* pAudioData = static_cast<audio_data*>(pData);

	audio_frame* pAudioFrame = new_ audio_frame();
	set_draw_base(pAudioFrame, pAudioData);
	
	pAudioFrame->set_audio_info(pAudioData->info_);

	return pAudioFrame;
}

message* draw_builder::create_message(draw_base_data* pData)
{
	message_data* pMesData = static_cast<message_data*>(pData);

	message* pMes = new_ message(false,pMesData->vecChildren_.size());
	set_draw_base(pMes, pMesData);

	pMes->set_text_data(pMesData->pTextData_);
	pMes->set_text_char_num(pMesData->nTextCharNum_);
	pMes->set_next(pMesData->nNext_);
	pMes->set_sound_id(pMesData->nSoundID_);

	return pMes;
}

namespace{
optional<uint32_t> draw_id_form_name(const vector<draw_base_data*>& vec, const string& sName)
{
	uint32_t nID=0;
	for(auto& it : vec)
	{
		if(it->sName_==sName) return optional<uint32_t>(nID);
		++nID;
	}

	return optional<uint32_t>();
}
} // namespace end

timeline* draw_builder::create_timeline(draw_base_data* pData)
{
	timeline_data* pTimelineData = static_cast<timeline_data*>(pData);

	timeline* pTimeline = new_ timeline(pTimelineData->vecChildren_.size());
	set_draw_base(pTimeline, pTimelineData, false);

	uint32_t		nKeyFrameID		 =	0;
	keyframe*		pPreKeyFrame	 =	nullptr;
	keyframe_data*	pPreKeyFrameData =	nullptr;
	
	// timelineに所属するkeyframe生成
	for(auto& it : pTimelineData->vecChildren_)
	{
		assert(it->eKind_==DRAW_TWEENFRAME || it->eKind_==DRAW_KEYFRAME);

		keyframe*		pKeyFrame		= nullptr;
		keyframe_data*	pKeyFrameData	= nullptr;

		if(it->eKind_==DRAW_TWEENFRAME)
		{// tween
			tweenframe_data*	pTweenData	= static_cast<tweenframe_data*>(it);
			tweenframe*			pTweenFrame = new_ tweenframe(pTweenData->vecChildren_.size());

			pTweenFrame->set_draw_info_start(pTweenData->drawInfo_)
						.set_draw_info_end(pTweenData->drawInfoEnd_)
						.set_easing_param(pTweenData->fEasing_)
						;

			pKeyFrameData	= pTweenData;
			pKeyFrame		= pTweenFrame;
		}
		else
		{// key
			pKeyFrameData	= static_cast<keyframe_data*>(it);
			pKeyFrame		= new_ keyframe(pKeyFrameData->vecChildren_.size());
		}
	
		// tween/key共通設定
		set_draw_base(pKeyFrame, pKeyFrameData, false);
		pKeyFrame->set_next_frame(pKeyFrameData->nNextFrame_);

		// keyframeに所属するdraw_base生成
		create_keyframe_draw(pKeyFrame, pKeyFrameData, pPreKeyFrame, pPreKeyFrameData);

		// keyframeをタイムラインにいれる
		pTimeline->set_id(nKeyFrameID);
		pTimeline->add_child(pKeyFrame, nKeyFrameID++);
		pKeyFrame->init();

		pPreKeyFrame	 = pKeyFrame;
		pPreKeyFrameData = pKeyFrameData;
	}

	pTimeline->shrink_children();
	return pTimeline;
}

void draw_builder::create_keyframe_draw(keyframe* pKeyFrame, keyframe_data* pKeyFrameData, keyframe* pPreKeyFrame, keyframe_data* pPreKeyFrameData)
{
	uint32_t nNodeID=0;
	for(auto& it : pKeyFrameData->vecChildren_)
	{
		draw_base* pNode = nullptr;

		// 共有ノードかどうかをチェック
		if(!it->sName_.get().empty() && pPreKeyFrame && pPreKeyFrameData)
		{
			// 前のフレームからノードを取得する
			optional<uint32_t> nSharedID = draw_id_form_name(pPreKeyFrameData->vecChildren_, it->sName_);
			if(nSharedID)
			{
				pNode = const_cast<draw_base*>(pPreKeyFrame->child(*nSharedID));
				if(pNode) pPreKeyFrame->add_shared_id(*nSharedID); // 一番最後にshareしてるやつが解体責任を負う
			}
		}
			
		// 何も生成されてなかったら新規に作る
		if(!pNode)
		{// 新規
			pNode = create_draw_switch(it);
		}

		if(pNode)
		{
			pKeyFrame->set_id(nNodeID);
			pKeyFrame->add_child(pNode, nNodeID++);
			pNode->init();
		}
	}

	pKeyFrame->shrink_children();
}

movieclip* draw_builder::create_movieclip(draw_base_data* pData)
{
	movieclip_data* pMovieClipData = static_cast<movieclip_data*>(pData);

	movieclip* pMovieClip = new_ movieclip(pMovieClipData->vecChildren_.size());
	set_draw_base(pMovieClip, pMovieClipData);
	return pMovieClip;
}

draw_base* draw_builder::create_draw_inner(draw_base_data* pData)
{
	draw_base* pNode = new_ draw_base(pData->vecChildren_.size());
	set_draw_base(pNode, pData);
	return pNode;
}

} // namespace graphic end
} // namespace mana end
