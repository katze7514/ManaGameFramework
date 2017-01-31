
#include "../mana_common.h"

//#include <boost/accumulators/accumulators.hpp>
//#include <boost/accumulators/statistics.hpp>

#include "renderer_sprite_queue.h"

namespace mana{
namespace draw{

renderer_sprite_queue::renderer_sprite_queue()
{
	// バックバッファははじめからいれておく
	// バックバッファは必ず一番最後
	add_cmd(cmd::BACK_BUFFER_ID, UINT_MAX);
}

bool renderer_sprite_queue::add_cmd(const sprite_param& cmd, uint32_t nRenderTarget)
{
	if(is_mode_use_texture(cmd.mode_) && cmd.nTextureID_==0)
	{
		logger::warnln("[render_sprite_cmd_queue]テクスチャIDがセットされていません。");
		return false;
	}

	rt_cmd_map::iterator it = mapRenderCmd_.find(nRenderTarget);
	if(it==mapRenderCmd_.end())
	{
		logger::warnln("[render_sprite_cmd_queue]描画先のレンダーターゲットがありません。: " + to_str(nRenderTarget));
		return false;
	}
	it->second.add_cmd(cmd);
	return true;
}

bool renderer_sprite_queue::add_cmd(uint32_t nRenderTargetID, uint32_t nPriority, uint32_t nReserveSpriteCmdNum)
{
	render_target_cmd cmd;
	cmd.nRenderTargetID_ = nRenderTargetID;

	auto r = mapRenderCmd_.emplace(nRenderTargetID,cmd);

	if(!r.second)
	{
		logger::warnln("[render_sprite_cmd]レンダーテクスチャーコマンドをセットできませんでした。: " + to_str(nRenderTargetID));
		return false;
	}

	auto sr = setRTPriority_.emplace(nRenderTargetID, nPriority);
	if(!sr.second)
	{
		mapRenderCmd_.erase(r.first);
		logger::warnln("[render_sprite_cmd]レンダーテクスチャープライオリティをセットできませんでした。: " + to_str_s(nRenderTargetID) + to_str(nPriority));
		return false;
	}

	// スプライトコマンドバッファリザーブ
	r.first->second.vecOpaqueCmd_.reserve(nReserveSpriteCmdNum);
	r.first->second.vecTransparentCmd_.reserve(nReserveSpriteCmdNum);

	return true;
}

void renderer_sprite_queue::sort_cmd()
{
	for(rt_cmd_map::value_type& it : mapRenderCmd_)
	{
		it.second.sort_sprite_param();
	}
}

void renderer_sprite_queue::clear_cmd()
{
	for(rt_cmd_map::value_type& it : mapRenderCmd_)
	{
		it.second.vecOpaqueCmd_.clear();
		it.second.vecTransparentCmd_.clear();
	}
}

bool renderer_sprite_queue::is_cmd_empty()const
{
	for(const rt_cmd_map::value_type& it : mapRenderCmd_)
	{
		if(it.second.vecOpaqueCmd_.size()>0
		|| it.second.vecTransparentCmd_.size()>0)
			return false;
	}

	return true;
}

renderer_sprite_queue::cmd_iterator renderer_sprite_queue::iterator()
{
	cmd_iterator it(&setRTPriority_, &mapRenderCmd_);
	it.it_ = setRTPriority_.begin();
	return std::move(it);
}

///////////////////////////
// render_taget_cmd
void render_target_cmd::add_cmd(const sprite_param& cmd)
{
	if(is_mode_use_blend(cmd.mode_))
		vecTransparentCmd_.emplace_back(cmd);
	else if(cmd.mode_!=MODE_NONE)
		vecOpaqueCmd_.emplace_back(cmd);
}

void render_target_cmd::sort_opaque()
{
	if(vecOpaqueCmd_.size()==0) return;

	//print_opaque();

	// テクスチャごとにまとめるために、一度テクスチャIDソート
	std::sort(vecOpaqueCmd_.begin(), vecOpaqueCmd_.end(),
				[](const sprite_param& lhs, const sprite_param& rhs)
				{  return lhs.nTextureID_ < rhs.nTextureID_; });


	// テクスチャごとの範囲を取る
	typedef vector<sprite_param>::iterator sp_cmd_iterator;
	// テクスチャ最初のit,最後のit,代表Z値,テクスチャID
	typedef tuple<sp_cmd_iterator, sp_cmd_iterator, float, uint32_t> sprite_param_it_tuple;
	vector<sprite_param_it_tuple> vecSpTuple;

	sprite_param_it_tuple t;
	t.get<0>() = vecOpaqueCmd_.begin();
	t.get<1>() = vecOpaqueCmd_.end();
	t.get<2>() = 0.0f;
	t.get<3>() = t.get<0>()->nTextureID_;

	for(sp_cmd_iterator it=vecOpaqueCmd_.begin(); it!=vecOpaqueCmd_.end(); ++it)
	{
		if(t.get<3>()!=it->nTextureID_)
		{
			t.get<1>()=it;
			vecSpTuple.emplace_back(t);

			t.get<0>() = it;
			t.get<1>() = vecOpaqueCmd_.end();
			t.get<2>() = it->z();
			t.get<3>() = it->nTextureID_;
		}
		else
		{
			// 各テクスチャーごとの代表Z値(合計値)を求める
			t.get<2>()+=it->z();
		}
	}
	vecSpTuple.emplace_back(t); // 終了時に残っているのを設定

	/*logger::traceln("****** Opaque Range ******");
	std::for_each(vecSpTuple.begin(), vecSpTuple.end(),
					[](const sprite_param_it_tuple& t)
					{ logger::traceln(to_str_s(t.get<0>()->nTextureID_) + to_str_s(t.get<2>())); }); */

	// 各テクスチャーごとにZ昇順ソート
	for(auto& it : vecSpTuple)
	{
		std::sort(it.get<0>(), it.get<1>(),
			     [](const sprite_param& lhs, const sprite_param& rhs)
				 {	return lhs.z() < rhs.z(); });
	}

	// テクスチャごとの範囲を取り直す
	vector<sprite_param_it_tuple>::iterator t_it = vecSpTuple.begin();
	t_it->get<0>() = vecOpaqueCmd_.begin();

	for(sp_cmd_iterator it=vecOpaqueCmd_.begin(); it!=vecOpaqueCmd_.end(); ++it)
	{
		if(it->nTextureID_!=t_it->get<3>())
		{
			t_it->get<1>() = it;
			++t_it;
			t_it->get<0>() = it;
		}
	}
	t_it->get<1>() = vecOpaqueCmd_.end();

	// 各テクスチャの代表Z値に合わせてコマンドタプルをソート
	std::sort(vecSpTuple.begin(), vecSpTuple.end(), 
				[](const sprite_param_it_tuple& lhs, const sprite_param_it_tuple& rhs)
				{	return lhs.get<2>() < rhs.get<2>();	});

	/*logger::traceln("****** Opaque Range ******");
	std::for_each(vecSpTuple.begin(), vecSpTuple.end(),
					[](const sprite_param_it_tuple& t)
					{ logger::traceln(to_str_s(t.get<0>()->nTextureID_) + to_str_s(t.get<2>())); }); */

	// コマンドタプルでソートされたテクスチャの順番にsprite_paramを並べ変える
	vector<sprite_param> vecSortedCmd;
	for(auto& it : vecSpTuple)
		vecSortedCmd.insert(vecSortedCmd.end(), it.get<0>(), it.get<1>());

	vecOpaqueCmd_ = std::move(vecSortedCmd);
	//print_opaque();
}

void render_target_cmd::sort_transeparent()
{
	if(vecTransparentCmd_.size()==0) return;

	// 半透明はZ降順順ソート。一回テクスチャソートすることでテクスチャIDが並ぶことを期待
	//print_transeparent();

	// テクスチャソート
	std::sort(vecTransparentCmd_.begin(), vecTransparentCmd_.end(),
				[](const sprite_param& lhs, const sprite_param& rhs)
				{  return lhs.nTextureID_ < rhs.nTextureID_; });

	// Z降順ソート
	std::sort(vecTransparentCmd_.begin(), vecTransparentCmd_.end(),
				[](const sprite_param& lhs, const sprite_param& rhs)
				{	return lhs.z() > rhs.z(); });

	//print_transeparent();
}

void render_target_cmd::print_opaque()
{
	logger::traceln("****** Opaque ******");

	std::for_each(vecOpaqueCmd_.begin(), vecOpaqueCmd_.end(),
					[](const sprite_param& e)
					{	logger::traceln(to_str_s(e.nTextureID_) + to_str_s(e.z())); });
}

void render_target_cmd::print_transeparent()
{
	logger::traceln("****** Transparent ******");

	std::for_each(vecTransparentCmd_.begin(), vecTransparentCmd_.end(),
					[](const sprite_param& e)
					{	logger::traceln(to_str_s(e.nTextureID_) + to_str_s(e.z())); });
}

///////////////////////////
// sprite_param
void sprite_param::clear()
{
	nTextureID_ = 0;
	::ZeroMemory(pos_, sizeof(pos_));
	set_uv_rect(0,0,0,0);
	set_color(0);
	set_color(1);
	mode_  = MODE_NONE;
}

void sprite_param::set_pos_rect(float fLeft, float fTop, float fRight, float fBottom, float fZ)
{
	pos_[0].fX = fLeft;
	pos_[0].fY = fTop;
	pos_[0].fZ = fZ;

	pos_[1].fX = fRight;
	pos_[1].fY = fTop;
	pos_[1].fZ = fZ;

	pos_[2].fX = fLeft;
	pos_[2].fY = fBottom;
	pos_[2].fZ = fZ;

	pos_[3].fX = fRight;
	pos_[3].fY = fBottom;
	pos_[3].fZ = fZ;
}

void sprite_param::set_uv_rect(float fLeft, float fTop, float fRight, float fBottom)
{
	uv_[0].fU = fLeft;
	uv_[0].fV = fTop;

	uv_[1].fU = fRight;
	uv_[1].fV = fTop;

	uv_[2].fU = fLeft;
	uv_[2].fV = fBottom;

	uv_[3].fU = fRight;
	uv_[3].fV = fBottom;
}

void sprite_param::set_color_rect(uint32_t nIndex, DWORD nLT, DWORD nRT, DWORD nLB, DWORD nRB)
{
	color_[nIndex][0].nColor = nLT;
	color_[nIndex][1].nColor = nRT;
	color_[nIndex][2].nColor = nLB;
	color_[nIndex][3].nColor = nRB;
}

void sprite_param::set_color(uint32_t nIndex, DWORD nColor)
{
	color_[nIndex][0].nColor = color_[nIndex][1].nColor = color_[nIndex][2].nColor = color_[nIndex][3].nColor = nColor;
}

} // namespace draw end
} // namespace mana end
