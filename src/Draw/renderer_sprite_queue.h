#pragma once

#include "renderer_2d_util.h"

namespace mana{
namespace draw{

struct render_target_cmd;
struct sprite_param;

//////////////////////////////////
/*! @brief スプライトキュー
 *
 *  renderer_2dへの描画リクエストは一度こいつに溜められ
 *  コマンド（マテリアル）ソートを行う
 *
 *　逆に言うと、renderer_2dはスプライトを描画するだけ。
 *　グラフィック・テキストもスプライトにしてから積む
 */
class renderer_sprite_queue
{
public:
	struct rt_priority
	{
		rt_priority(uint32_t nRT, uint32_t nPri):nRenderTargetID_(nRT),nPriority_(nPri){}
	
		uint32_t nRenderTargetID_;
		uint32_t nPriority_;
	};

	typedef set<rt_priority>							rt_priority_set;
	typedef unordered_map<uint32_t, render_target_cmd>	rt_cmd_map;

	// プライオリティリストに合わせて、コマンドリストを走査するためのイテレーター
	struct cmd_iterator
	{
	public:
		friend renderer_sprite_queue;

		cmd_iterator(rt_priority_set* pSet, rt_cmd_map* pMap):pSetPriority_(pSet),pMapRenderCmd_(pMap){}

		render_target_cmd& next(){ return (*pMapRenderCmd_)[(it_++)->nRenderTargetID_]; }
		bool			   is_end()const{ return it_==pSetPriority_->end(); }

	private:
		rt_priority_set* pSetPriority_;
		rt_cmd_map*		 pMapRenderCmd_;

		rt_priority_set::iterator it_;
	};

public:
	renderer_sprite_queue();

	//! レンダーターゲットコマンド追加
	bool add_cmd(uint32_t nRenderTargetID, uint32_t nPriority, uint32_t	nReserveSpriteCmdNum=32);

	//! スプライトコマンド追加
	bool add_cmd(const sprite_param& cmd, uint32_t nRenderTargetID=cmd::BACK_BUFFER_ID);

	//! 規定の順番にコマンドをソートする
	void sort_cmd();

	//! 次の描画に備えてコマンドクリア
	//! レンダーターゲットに入っているスプライトコマンドバッファだけをクリアする
	void clear_cmd();

	//! コマンドが積まれているかどうか
	bool is_cmd_empty()const;

	//! コマンド取得のイテレータ
	cmd_iterator iterator();

private:
	//! レンダーターゲットプライオリティセット。この順番にコマンドリストを引く
	rt_priority_set	setRTPriority_;
	//! レンダーテクスチャごとのコマンドリスト
	rt_cmd_map		mapRenderCmd_;
};

//! set向けのoperator<実装
inline bool operator<(const renderer_sprite_queue::rt_priority& lhs, const renderer_sprite_queue::rt_priority& rhs)
{
	return lhs.nPriority_ < rhs.nPriority_;
}

//! @brief レンダーターゲット描画コマンド
/*! このレンダーターゲットに描画するスプライト描画コマンドリストも持つ */
struct render_target_cmd
{
public:
	void clear_cmd(){ vecOpaqueCmd_.clear(); vecTransparentCmd_.clear(); }
	void add_cmd(const sprite_param& cmd);
	void sort_sprite_param(){ sort_opaque(); sort_transeparent(); }

public:
	uint32_t			nRenderTargetID_;

	vector<sprite_param>	vecOpaqueCmd_;		// 不透明のスプライトコマンド
	vector<sprite_param>	vecTransparentCmd_;	// 透明のスプライトコマンド

private:
	void sort_opaque();
	void sort_transeparent();

	void print_opaque();
	void print_transeparent();
};

//! スプライト描画パラメータ
//! スプライトといっているが三角も許容する。三角の場合は、uv[3]にNANをいれておく
struct sprite_param
{
public:
	sprite_param(){ clear(); }

	void clear();
	float z()const{ return pos_[0].fZ; }

	//! @defgroup sprite_param_helper 設定ヘルパー
	//! @{
	void set_pos_rect(float fLeft, float fTop, float fRight, float fBottom, float fZ);
	void set_uv_rect(float fLeft, float fTop, float fRight, float fBottom);
	void set_color_rect(uint32_t nIndex, DWORD nLT, DWORD nRT, DWORD nLB, DWORD nRB);
	void set_color(uint32_t nIndex, DWORD nColor=D3DCOLOR_ARGB(255,255,255,255));
	//! @}

public:
	uint32_t		nTextureID_;

	// 左上、右上、左下、右下
	POS_VERTEX		pos_[4]; // 2Dスクリーン座標
	UV_VERTEX		uv_[4];	 // テクスチャサイズ指定。UV変換は頂点への書き込み時に行う
	COLOR_VERTEX	color_[2][4];

	draw_mode		mode_;
};

} // namespace draw end
} // namespace mana end

/*
	using namespace mana::draw;
	using namespace mana::draw::cmd;
	render_2d_cmd_queue queue;
	vector<sprite_param>	vecSp;
	sprite_param			sp;
	//std::chrono::system_clock::now().time_since_epoch().count()
	std::mt19937 rnd(10000);
	for(uint32_t i=0; i<2560; ++i)
	{
		sp.nTextureID_ = (rnd()%256)+1;
		sp.vertex_[0].fZ = static_cast<float>(rnd()%101);
		sp.mode_ = MODE_TEX_COLOR;
		vecSp.emplace_back(sp);
	}

	for(auto it : vecSp)
		queue.add_cmd(it);

	timer::elapsed_timer t;
	t.start();
		queue.sort_cmd();
	t.end();

	logger::traceln(to_str(t.elasped_mill()));
*/
