#pragma once

#include "../Utility/node.h"
#include "../draw/renderer_2d_util.h"
#include "draw_util.h"

namespace mana{
namespace graphic{

class draw_context;

/*! @brief 描画ツリーのベースクラス
 *
 *  各種描画パラメータなどを持つ。子を複数持てる
 *
 *　親子関係は、
 *  　pivot/scale/rot/posのaffine変換行列が生成される
 *  　カラーは、アルファのみ乗算合成
 *　　Zは、自動計算される
 *
 *  パラメータ設定系はメソッドチェーン対応
 */
class draw_base : private utility::node<draw_base>
{
public:
	//typedef function<void (draw_base*, draw_base_event_id)>	event_handler_type;
	
	typedef utility::node<draw_base> base_type;
	friend	base_type;

public:
	enum draw_base_const
	{
		CHILD_RESERVE=5,
	};

public:
	draw_base(uint32_t nReserve=CHILD_RESERVE);
	virtual ~draw_base();

	//! 初期化
	virtual void	init();

	//! 動作を実行する。子がいる場合は子も呼ぶ
	virtual void	exec(draw_context& ctx);

	draw_base_kind	kind()const{ return eKind_; }

	bool			is_visible()const{ return bVisible_; }
	draw_base&		visible(bool bVisible){ bVisible_=bVisible; return *this; }

	bool			is_pause()const{ return bPause_; }
	draw_base&		pause(bool bPause){ bPause_ = bPause; return *this; }

	bool			is_visible_ctx(draw_context& ctx)const;
	bool			is_pause_ctx(draw_context& ctx)const;

public:
	//! @defgroup draw_base_draw_param 描画パラメータ
	//! @{
	const draw::draw_info&  draw_info()const{ return drawInfo_; }
	draw_base&				set_draw_info(const draw::draw_info& info){ drawInfo_=info; return *this; }

	const draw::POS&	pos()const{ return drawInfo_.pos_; }
	draw_base&			set_pos(const draw::POS& pos){ drawInfo_.pos_=pos; return *this; }
	draw_base&			set_pos(float fX, float fY, float fZ=0.0f){ set_x(fX); set_y(fY); set_z(fZ); return *this; }

	float				x()const{ return drawInfo_.pos_.fX; }
	draw_base&			set_x(float fX){ drawInfo_.pos_.fX = fX; return *this; }
	float				y()const{ return drawInfo_.pos_.fY; }
	draw_base&			set_y(float fY){ drawInfo_.pos_.fY = fY; return *this; }
	float				z()const{ return drawInfo_.pos_.fZ; }
	draw_base&			set_z(float fZ){ drawInfo_.pos_.fZ = fZ;  return *this; }

	const draw::SIZE&	scale()const{ return drawInfo_.scale_; }
	draw_base&			set_scale(float fWidth, float fHeight){ set_width(fWidth); set_height(fHeight); return *this; }
	float				width()const{ return drawInfo_.scale_.fWidth; }
	draw_base&			set_width(float fWidth){ drawInfo_.scale_.fWidth=fWidth; return *this; }
	float				height()const{ return drawInfo_.scale_.fHeight; }
	draw_base&			set_height(float fHeight){ drawInfo_.scale_.fHeight=fHeight; return *this; }

	float				angle()const{ return drawInfo_.angle_; }
	draw_base&			set_angle(float fAngle){ drawInfo_.angle_=fAngle; return *this; }

	const draw::POS&	pivot()const{ return pivot_; }
	draw_base&			set_pivot(const draw::POS& pivot){ pivot_=pivot; return *this; }
	draw_base&			set_pivot(float fX, float fY){ pivot_.fX = fX; pivot_.fY = fY; return *this; }
	float				pivot_x()const{ return pivot_.fX; }
	draw_base&			set_pivot_x(float fX){ pivot_.fX = fX; return *this; }
	float				pivot_y()const{ return pivot_.fY; }
	draw_base&			set_pivot_y(float fY){ pivot_.fY = fY; return *this; }

	uint8_t				alpha()const{ return drawInfo_.alpha(); }
	virtual draw_base&	set_alpha(uint8_t alpha){ drawInfo_.set_alpha(alpha); return *this; }

	DWORD				color()const{ return nColor_; }
	virtual draw_base&	set_color(uint8_t a, uint8_t r, uint8_t g, uint8_t b){ nColor_=D3DCOLOR_ARGB(a,r,g,b); return *this; }
	virtual draw_base&	set_color(DWORD nColor){ nColor_=nColor; return *this; }

	color_mode_kind		color_mode()const{ return eColorMode_; }
	virtual draw_base&	set_color_mode(color_mode_kind eMode){ eColorMode_=eMode; return *this; }

	const D3DXMATRIX&	world_matrix()const{ return worldMat_; }
	uint8_t				world_alpha()const{ return nWorldAlpha_; }
	DWORD				world_alpha_color()const{ return D3DCOLOR_ARGB(nWorldAlpha_,255,255,255); }
	float				world_z()const{ return fWorldZ_; }
	//! @}

	//! @defgroup draw_base_envent_handler_ctrl イベントハンドラ
	//! @{
	//const event_handler_type&	event_handler()const{ return handler_; }
	//draw_base&					set_event_handler(const event_handler_type& handler){ handler_=handler; }
	//! イベントハンドラを呼ぶ
	//void						event_invoke(draw_base_event_id eEvnet){ handler_(this, eEvnet); }
	//! @}

public:
	uint32_t			id()const{ return base_type::id(); }
	draw_base&			set_id(uint32_t nID){ base_type::set_id(nID); return *this; }

	uint32_t			priority()const{ return base_type::priority(); }
	draw_base&			set_priority(uint32_t nPriority){ base_type::set_priority(nPriority); return *this; }

	const draw_base*	parent()const{ return base_type::parent(); }
	draw_base*			parent(){ return base_type::parent(); }
	draw_base&			set_parent(draw_base* pParent){ base_type::set_parent(pParent); return *this; }

	//! @defgroup draw_base_child_ctrl draw_baseの子操作
	//! @{
	virtual bool		add_child(draw_base* pChild, uint32_t nPriority){ return base_type::add_child(pChild,nPriority,this); }
	draw_base*			remove_child(uint32_t nPriority, bool bDelete){ return base_type::remove_child(nPriority, bDelete); }
	draw_base*			child(uint32_t nPriority){ return base_type::child(nPriority); }
	const draw_base*	child(uint32_t nPriority)const{ return base_type::child(nPriority); }
	virtual void		clear_child(bool bDelete=true){ base_type::clear_child(bDelete); }
	void				sort_child(bool bChild=false){ base_type::sort_child(bChild); }

	base_type::child_vector&		children(){ return vecChildren_; }
	const base_type::child_vector&	children()const{ return vecChildren_; }

	uint32_t						count_children()const{ return base_type::count_children(); }
	void							shrink_children(){ base_type::shrink_children(); }
	//! @}

protected:
	virtual void	init_self(){}
	//! 自分自身の動作
	virtual void	exec_self(draw_context& ctx);

	//! ワールド行列やアルファを構築する
	void			calc_world(draw_context& ctx);

protected:
	draw_base_kind		eKind_;

	draw::draw_info		drawInfo_;
	draw::POS			pivot_;		//!< 描画・拡縮・回転の原点

	DWORD				nColor_;	//!< 上に重ねる色
	color_mode_kind		eColorMode_;//!< カラー合成モード

	bool				bVisible_;	//!< falseだと描画コマンドを生成しない
	bool				bPause_;

	//! ノードをワールド変換する行列
	//! 親までのworld_、pos_,drawInfo_まで処理済みの行列
	float				fWorldZ_;
	D3DXMATRIX			worldMat_;
	uint8_t				nWorldAlpha_;

	//event_handler_type	handler_;

#ifdef MANA_DEBUG
public:
	const string&	debug_name()const{ return base_type::debug_name(); }
	void			set_debug_name(const string& sName){ base_type::set_debug_name(sName); }
#endif

public:
	// xtal用メソッド
	xtal::SmartPtr<draw_base> set_x_xtal(float fX){ set_x(fX); return xtal::SmartPtr<draw_base>(this); }
	xtal::SmartPtr<draw_base> set_y_xtal(float fY){ set_y(fY); return xtal::SmartPtr<draw_base>(this); }
	xtal::SmartPtr<draw_base> set_z_xtal(float fZ){ set_z(fZ); return xtal::SmartPtr<draw_base>(this); }

	xtal::SmartPtr<draw_base> set_scale_xtal(float fWidth, float fHeight){ set_scale(fWidth, fHeight); return xtal::SmartPtr<draw_base>(this); }
	xtal::SmartPtr<draw_base> set_width_xtal(float fWidth){ set_width(fWidth); return xtal::SmartPtr<draw_base>(this); }
	xtal::SmartPtr<draw_base> set_height_xtal(float fHeight){ set_height(fHeight); return xtal::SmartPtr<draw_base>(this); }
	xtal::SmartPtr<draw_base> set_angle_xtal(float fAngle){ set_angle(fAngle); return xtal::SmartPtr<draw_base>(this); }

	xtal::SmartPtr<draw_base> set_pivot_xtal(float fX, float fY){ set_pivot(fX,fY); return xtal::SmartPtr<draw_base>(this); }
	xtal::SmartPtr<draw_base> set_pivot_x_xtal(float fX){ set_pivot_x(fX); return xtal::SmartPtr<draw_base>(this); }
	xtal::SmartPtr<draw_base> set_pivot_y_xtal(float fY){ set_pivot_y(fY); return xtal::SmartPtr<draw_base>(this); }

	xtal::SmartPtr<draw_base> set_color_xtal(uint8_t a, uint8_t r, uint8_t g, uint8_t b){ set_color(a,r,g,b); return xtal::SmartPtr<draw_base>(this); }
	xtal::SmartPtr<draw_base> set_alpha_xtal(uint8_t a){ set_alpha(a); return xtal::SmartPtr<draw_base>(this); }

	xtal::SmartPtr<draw_base> set_id_xtal(uint32_t nID){ set_id(nID); return xtal::SmartPtr<draw_base>(this); }
	xtal::SmartPtr<draw_base> set_priority_xtal(uint32_t nPri){ set_priority(nPri); return xtal::SmartPtr<draw_base>(this); }

	xtal::SmartPtr<draw_base> parent_xtal(){ return xtal::SmartPtr<draw_base>(parent()); }
	bool					  add_child_xtal(xtal::SmartPtr<draw_base> pChild, uint32_t nPri){ return add_child(pChild.get(), nPri); }
	xtal::SmartPtr<draw_base> remove_child_xtal(uint32_t nPri, bool bDelete){ auto r = remove_child(nPri, bDelete); return xtal::SmartPtr<draw_base>(r); }
	xtal::SmartPtr<draw_base> child_xtal(uint32_t nPri){ auto r = child(nPri); return xtal::SmartPtr<draw_base>(r); }
};

} // namespace graphic end
} // namespace mana end
