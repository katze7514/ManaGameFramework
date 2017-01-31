#pragma once

namespace mana{
namespace graphic{

//! ノードの種類
enum draw_base_kind
{
	DRAW_BASE,
	DRAW_LABEL,
	DRAW_POLYGON,
	DRAW_SPRITE,
	DRAW_AUDIO_FRAME,
	DRAW_MESSAGE,
	DRAW_MOVIECLIP,
	DRAW_TIMELINE,
	DRAW_KEYFRAME,
	DRAW_TWEENFRAME,
	DRAW_END,
};

//! draw_baseイベントハンドラに渡されるイベントID
enum draw_base_event_id
{
	EV_START_FRAME,	//!< 毎フレーム処理の一番最初に呼ばれる
	EV_EXEC_FRAME,	//!< 毎フレーム前処理終了後(親子の描画関係処理後)～描画コマンド実行前の間に呼び出される
	EV_END_FRAME,	//!< 子ノード実行も終わった後、フレームの一番最後に呼ばれる
	EV_EVENT_ID_END,
};

//! デフューズカラー合成種類
enum color_mode_kind
{
	COLOR_NO,		//!< カラーなし合成(デフォルト)
	COLOR_THR,		//!< スルー合成
	COLOR_BLEND,	//!< αブレンド合成
	COLOR_MUL,		//!< 乗算合成
	COLOR_SCREEN,	//!< スクリーン合成
};

} // namespace graphic end
} // namespace mana end
