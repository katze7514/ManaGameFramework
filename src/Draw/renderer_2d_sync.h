#pragma once

#include "renderer_2d.h"

namespace mana{
namespace draw{

/*! @brief 同期版2D描画レンダラー
 *
 *  非同期版と宣言を合わせるために冗長になっている所がある
 *  非同期版と相互運用しないならば、requestの後にrenderを呼ぶだけで使える */
class renderer_2d_sync : public renderer_2d
{
public:
	renderer_2d_sync();
	~renderer_2d_sync();

public:
	//! @brief レンダラー初期化
	//! @param[in] nReserveRequestNum 積むリクエストの推定数
	bool	init_render(uint32_t nReserveRequestNum=128, uint32_t nAffinity=0)override;

	//! 動作を終了する
	void	fin() override;

	//! @defgroup renderer_2d_sync_request リクエスト処理
	//! @{
	//! @ brief request積み処理を開始する
	bool			start_request(bool bWait=true)override{ return !is_device_lost(); }
	//! @brief requestを積む
	void			request(const cmd::render_2d_cmd& cmd)override;
	//! request積みの終了
	void			end_request()override{}
	//! 積んだリクエストの処理を開始する
	render_result	render(bool bWait=true)override;
	//! @}

	//! デバイスロストしてるかどうか
	bool is_device_lost()const override{ return eRenderResult_==RENDER_DEVICE_LOST; }

private:
	render_result eRenderResult_;

#ifdef MANA_RENDER_2D_CMD_COUNT
	uint32_t nMaxCmd_;
	uint32_t nCurCmd_;
#endif

private:
	NON_COPIABLE(renderer_2d_sync);
};

} // namespace draw end
} // namespace mana end
