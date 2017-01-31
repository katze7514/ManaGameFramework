#pragma once

#include "label.h"

namespace mana{
namespace graphic{

/*! @brief メッセージ送りなどの機能があるテキスト描画ノード
 *
 *  文字送りの速さ(1文字表示する速度(フレーム))、一緒にならすSE
 *　文字送りの再生・停止など設定できる
 *
 *  サウンドは1文字表示される時に鳴るので、瞬間表示の時は鳴らない
 *
 *　すべての文字を表示するとpauseするので、繰り返す場合はinitを呼ぶ
 *
 *  文字送り動作は、draw_contextのexecがfalseだとWaitが進まない
 */
class message : public label
{
public:
	message(bool bCreate=true, uint32_t nReserve=CHILD_RESERVE);
	virtual ~message(){}

protected:
	virtual void init_self()override;
	virtual void exec_self(draw_context& ctx)override;

public:
	uint32_t text_char_num()const{ return nTextCharNum_; }
	message& set_text_char_num(uint32_t nTextCharNum){ nTextCharNum_=nTextCharNum; return *this; }
	uint32_t next()const{ return nNext_; }
	message& set_next(uint32_t nNext){ nNext_=nNext; return *this; }
	uint32_t sound_id()const{ return nSoundID_; }
	message& set_sound_id(uint32_t nSoundID){ nSoundID_=nSoundID; return *this; }

	uint32_t cur_char_num()const{ return nCurCharNum_; }
	message& set_cur_char_num(uint32_t nCurCharNum){ nCurCharNum_=nCurCharNum; return *this; }

protected:
	uint32_t nTextCharNum_; // 文字数(設定値もしくはdraw_builderのロードの際に計算される)
	uint32_t nNext_;		// 次の文字を表示する速度(フレーム数)。0だと瞬間表示
	uint32_t nSoundID_;		// 文字を表示する際に慣らすSEのID。1文字ずつ鳴る

	uint32_t nCurCharNum_;  // 現在表示する文字数
	uint32_t nWait_;		// 次に表示する文字を増やすまでのカウンター
};

} // namespace graphic end
} // namespace mana end
