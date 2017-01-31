#pragma once

#include "../Utility/node.h"

namespace mana{

class actor_context;


/*! @brief actorクラス。ゲーム制御の基本となる
 *
 *  ポーズフラグ(16個)に応じて動作制御できる。ポーズフラグは16bitのビットマスク。 
 *  actor_contextに設定されたポーズフラグと一致した場合、exec_selfを呼ばない
 */
class actor : private utility::node<actor>
{
public:
	typedef utility::node<actor> base_type;
	friend base_type;

public:
	enum pause_flag : uint16_t
	{
		PAUSE_0		=	1,
		PAUSE_1		=	1<<1,
		PAUSE_2		=	1<<2,
		PAUSE_3		=	1<<3,
		PAUSE_4		=	1<<4,
		PAUSE_5		=	1<<5,
		PAUSE_6		=	1<<6,
		PAUSE_7		=	1<<7,
		PAUSE_8		=	1<<8,
		PAUSE_9		=	1<<9,
		PAUSE_10	=	1<<10,
		PAUSE_11	=	1<<11,
		PAUSE_12	=	1<<12,
		PAUSE_13	=	1<<13,
		PAUSE_14	=	1<<14,
		PAUSE_15	=	1<<15,

		PAUSE_ALL	=	0xFFFF,
		PAUSE_NO	=	0,
	};

public:
	actor():nPauseFlag_(PAUSE_ALL),bValid_(true),nType_(0),nAttr_(0),nState_(0){}
	virtual ~actor(){ clear_child(); }

public:
	virtual bool	init(actor_context& ctx);
	virtual void	reset(actor_context& ctx);
	virtual void	exec(actor_context& ctx);

protected:
	// 自分自身の動作
	virtual bool	init_self(actor_context& ctx){ return true; }
	virtual void	reset_self(actor_context& ctx){}
	virtual void	exec_self(actor_context& ctx){}

public:
	uint16_t		pause_flag()const{ return nPauseFlag_; }
	void			set_pause_flag(uint16_t nPauseFlag){ nPauseFlag_  = nPauseFlag; }

	bool			is_pause_flat(uint16_t nPauseFlag){ return bit_test<uint16_t>(nPauseFlag_, nPauseFlag); }
	void			add_pause_flag(uint16_t nPauseFlag){ nPauseFlag_ |= nPauseFlag; }
	void			remove_pause_flag(uint16_t nPauseFlag){ nPauseFlag_ &= ~nPauseFlag; }

	bool			is_valid()const{ return bValid_; }
	actor&			valid(bool bValid){ bValid_=bValid; return *this; }

	uint32_t		type()const{ return nType_; }
	actor&			set_type(uint32_t nKind){ nType_=nKind; return *this; }

	uint32_t		attr()const{ return nAttr_; }
	actor&			set_attr(uint32_t nAttr){ nAttr_=nAttr; return *this; }

	uint32_t		state()const{ return nState_; }
	actor&			set_state(uint32_t nState){ nState_=nState; return *this; }

public:
	uint32_t		id()const{ return base_type::id(); }
	actor&			set_id(uint32_t nID){ base_type::set_id(nID); return *this;}

	uint32_t		priority()const{ return base_type::priority(); }
	actor&			set_priority(uint32_t nPriority){ base_type::set_priority(nPriority); return *this; }

	const actor*	parent()const{ return base_type::parent(); }
		  actor*	parent(){ return base_type::parent(); }
	actor&			set_parent(actor* pParent){ base_type::set_parent(pParent); return *this; }

	//! @defgroup actor_child_ctrl actorの子操作
	//! @{
	virtual bool			add_child(actor* pChild, uint32_t nPriority){ return base_type::add_child(pChild,nPriority,this); }
	virtual actor*			remove_child(uint32_t nPriority, bool bDelete){ return base_type::remove_child(nPriority, bDelete); }
	virtual const actor*	child(uint32_t nPriority)const{ return base_type::child(nPriority); }
	virtual actor*			child(uint32_t nPriority){ return base_type::child(nPriority); }
	virtual void			clear_child(bool bDelete=true){ base_type::clear_child(bDelete); }
	virtual void			sort_child(bool bChild=false){ base_type::sort_child(bChild); }

	base_type::child_vector&		children(){ return vecChildren_; }
	const base_type::child_vector&	children()const{ return vecChildren_; }

	uint32_t						count_children()const{ return base_type::count_children(); }
	void							shrink_children(){ base_type::shrink_children(); }
	//! @}

protected:
	//! あらゆる状況をチェックして実行して良いかを判断する
	bool is_exec(actor_context& ctx);

protected:
	//! @brief 対応するポーズフラグ
	/*! bitが立っている所と、actor_contextのポーズフラグのbitの立っている部分が一致したらポーズ扱いとなる */
	uint16_t	nPauseFlag_;

	//! 動作フラグ。ポーズフラグ関係無しに止めるかどうかを指定できる
	//! 子に伝播する
	bool		bValid_;

	//! 種別
	uint32_t	nType_;
	//! 属性
	uint32_t	nAttr_;
	//! 状態
	uint32_t	nState_;

#ifdef MANA_DEBUG
public:
	const string&	debug_name()const{ return base_type::debug_name(); }
	void			set_debug_name(const string& sName){ base_type::set_debug_name(sName); }
#endif

	// xtal bind用メソッド
public:
	xtal::SmartPtr<actor>	parent_xtal(){ return xtal::SmartPtr<actor>(parent()); }
	bool					add_child_xtal(xtal::SmartPtr<actor> pChild, uint32_t nPri){ return add_child(pChild.get(), nPri); }
	xtal::SmartPtr<actor>	remove_child_xtal(uint32_t nPri, bool bDelete){ auto r = remove_child(nPri, bDelete); return xtal::SmartPtr<actor>(r); }
	xtal::SmartPtr<actor>	child_xtal(uint32_t nPri){ auto r = child(nPri); return xtal::SmartPtr<actor>(r); }
};

} // namespace mana end
