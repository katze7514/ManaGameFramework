#pragma once

#include "draw_base.h"

namespace mana{
namespace graphic{

/*! @brief stateとして登録したdraw_baseを排他的に切り替えるクラス
 *
 *	切り替える時に、実行開始フレーム(キーフレームでは無い)を指定することができる。
 *  jump_frameが呼ばれる。対応してないdraw_baseの時は無視される
 *
 *  実際に変更されるのは、execが実行された時
 *
 *  stateと子供は別にしてあるので子供をぶら下げることもできる
 */
class draw_state : public mana::graphic::draw_base
{
public:
	draw_state():pCurDrawBase_(nullptr),bChange_(false),bSame_(false),nFrame_(0),bInit_(true){}
	virtual ~draw_state(){ clear_state(); }

public:
	const string_fw&	cur_state_id()const{ return sCurID_; }
	draw_base*			cur_state(){ return pCurDrawBase_; }
	const draw_base*	cur_state()const{ return pCurDrawBase_; }

public:
	// stateにも伝わるようにしたい時はこのメソッドを使う
	draw_state&			set_color(uint8_t a, uint8_t r, uint8_t g, uint8_t b)override;
	draw_state&			set_color(DWORD color)override;
	draw_state&			set_color_mode(color_mode_kind eMode)override;

public:
	bool				add_state(const string_fw& sID, draw_base* pDrawBase);
	void				remove_state(const string_fw& sID);
	void				clear_state();
	bool				is_exist_state(const string_fw& sID);

	mana::graphic::draw_base* state(const string_fw& sID);

public:
	//! @brief state変更指示
	/*! @param[in] nID 変更先のID
	 *  @param[in] bSame 変える時に現在と同じフレーム番号にするかどうか 
	 *  @param[in] bInit 変える時にinitを呼ぶかどうか */
	void change_state(const string_fw& sID, bool bSame, bool bInit=true){ bChange_=true; sChangeID_=sID; bSame_=bSame; nFrame_=0; bInit_=bInit; }

	//! @brief state変更指示
	/*! @param[in] nID 変更先のID
	 *  @param[in] nFrame 変更したDrawBaseを指定したフレームまで進める
	 *  @param[in] bInit 変える時にinitを呼ぶかどうか */
	void change_state(const string_fw& sID, uint32_t nFrame, bool bInit=true){ bChange_=true; sChangeID_=sID; bSame_=false; nFrame_=nFrame; bInit_=bInit; }

protected:
	virtual void exec_self(draw_context& ctx)override;

	//! 実際にstateを変更する
	bool change_state_inner(draw_context& ctx);

protected:
	string_fw					sCurID_;
	mana::graphic::draw_base*	pCurDrawBase_;

	bool						bChange_;
	string_fw					sChangeID_;
	bool						bSame_;
	uint32_t					nFrame_;
	bool						bInit_;

	flat_map<string_fw, draw_base*> mapDrawState_;
};

} // namespace graphic end
} // namespace mana end
