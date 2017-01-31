#pragma once

#include "draw_base.h"

namespace mana{
namespace graphic{

class keyframe;

/*! @brief タイムラインノード
 *
 *　キーフレームを持って指定されたフレーム経過すると
 *　次のフレームを実行する
 *
 *　今までMovieClipの中でLayerとしていたクラス
 *　Layerを単体で使えるようにしたもの
 */
class timeline : public draw_base
{
public:
	timeline(uint32_t nReserve=CHILD_RESERVE);
	virtual ~timeline(){}

public:
	void exec(draw_context& ctx)override;

public:
	timeline& set_color(uint8_t a, uint8_t r, uint8_t g, uint8_t b)override;
	timeline& set_color(DWORD color)override;
	timeline& set_color_mode(color_mode_kind eMode)override;

public:
	//! @brief 現在実行中のキーフレーム番号
	/*! 1オリジン。0の場合は実行前 */
	uint32_t		cur_keyframe_no()const{ return nCurKeyFrameNo_; }
	const keyframe*	cur_keyframe_draw_base()const{ return pCurKeyFrameNode_; }
	uint32_t		total_frame_count()const{ return nTotalFrameCount_; }

public:
	//! @defgroup timeline_frame_ctrl フレーム操作
	/*! contextを引数を取るメソッドはjump系メソッドを使い指定フレームまでexecが呼ばれる */
	//! @{
	void	next_keyframe();
	void	next_keyframe(draw_context& ctx);
	void	prev_keyframe();
	void	prev_keyframe(draw_context& ctx);
	void	jump_keyframe(uint32_t nKeyFrame, draw_context& ctx);
	void	jump_frame(uint32_t nFrame, draw_context& ctx);
	//! @}

#ifdef MANA_DEBUG
public:
	bool	add_child(draw_base* pChild, uint32_t nID)override;
#endif

protected:
	void init_self()override;

	//! 現在の状態に合わせてpCurKeyFrameNode_を設定する
	void change_keyframe();

	//! 1フレームだけ実行する
	void exec_frame(draw_context& ctx);

protected:
	uint32_t	nCurKeyFrameNo_;	//!< 現在実行中のフレーム番号
	keyframe*	pCurKeyFrameNode_;	//!< 現在実行中のフレーム
	
	uint32_t	nTotalFrameCount_;	//!< 実行していたフレームカウント
	uint32_t	nChangeFrameCount_;	//!< キーフレームを変更するフレームカウント
};

} // namespace graphic end
} // namespace mana end
