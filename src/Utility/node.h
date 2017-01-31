#pragma once

namespace mana{
namespace utility{

/*! @brief 親子関係操作を持つクラスベース
 *
 *	private継承して使う。子ノードはid昇順に追加される。追加後にidを変更した場合は、sort_childで正常化できる
 *
 *	@tparam T 親子関係を作りたいクラス */
template<class T>
class node
{
public:
	typedef node<T>		self_type;
	typedef vector<T*>	child_vector;

public:
	node(uint32_t nReserve=16):nID_(0),nPriority_(0),pParent_(nullptr){ vecChildren_.reserve(nReserve); }

public:
	uint32_t	id()const{ return nID_; }
	void		set_id(uint32_t nID){ nID_=nID; }

	uint32_t	priority()const{ return nPriority_; }
	void		set_priority(uint32_t nPriority){ nPriority_=nPriority; }
	
	T*			parent(){ return pParent_; }
	const T*	parent()const{ return pParent_; }
	void		set_parent(T* pParent){ pParent_ = pParent; }

	//! @defgroup node_child_ctrl Tの子操作
	//! @{
	//! 子ノードを追加する。exec実行中に呼んではいけない。メソッドチェーン対応
	/*! @param[in] pChild	追加する子ノード
	 *  @param[in] nPriority		子ノードのID
	 *  @param[in] pParent	追加子ノードの親ノード */
	bool		add_child(T* pChild, uint32_t nPriority, T* pParent);

	//! 子ノードを削除する。exec実行中に呼んではいけない
	/*! @param[in] nPriority		削除する子のID
	 *  @param[in] bDelete	削除時にdeleteを呼ぶかどうか
	 *  @return				bDeleteがfalseの時は削除したTのポインタが返る。それ以外はnullptr */
	T*			remove_child(uint32_t nPriority, bool bDelete);

	//! 子ノードを取得する
	const T*	child(uint32_t nPriority)const;
		  T*	child(uint32_t nPriority);

	//! 子ノードをすべて削除する。deleteを呼ぶ。exec実行中に呼んではいけない
	/*! @param[in] bDelete falseだと子をdeleteしない */
	void		clear_child(bool bDelete=true);

	//! @brief 子ノードを現在のIDに基づきソートしなおす。exec実行中に呼んではいけない
	/*! @param[in] bChild trueだと再帰的にsort_childを呼ぶ */
	void		sort_child(bool bChild=true);

	//! 子ノードの数を取得する
	uint32_t	count_children()const{ return vecChildren_.size(); }
	//! 子ノードリストをシュリンクする
	void		shrink_children(){ vecChildren_.shrink_to_fit(); }
	//! @}

protected:
	uint32_t		nID_;
	uint32_t		nPriority_;

	T*				pParent_;
	child_vector	vecChildren_;

#ifdef MANA_DEBUG
public:
	const string&	debug_name()const{ return sDebugName_; }
	void			set_debug_name(const string& sName){ sDebugName_ = sName; }

protected:
	string			sDebugName_; //!< デバッグ用のID
#endif
};

/////////////////////
// 実装
template<class T>
inline bool node<T>::add_child(T* pChild, uint32_t nPriority, T* pParent)
{
	if(!pChild) return false;

	pChild->set_priority(nPriority);
	pChild->set_parent(pParent);

	child_vector::iterator it;
	for(it = vecChildren_.begin(); it!=vecChildren_.end(); ++it)
	{
	#ifdef MANA_DEBUG
		if((*it)->priority()==nPriority)
		{
			logger::warnln("[node]同priorityのnodeがすでに存在します。: " + to_str_s(nPriority) + (*it)->debug_name() + " " + pChild->debug_name());

			if((*it)==pChild) return true;
			
		}
	#endif

		if((*it)->priority()>nPriority)
		{
			vecChildren_.insert(it, pChild);
			return true;
		}
	}

	vecChildren_.push_back(pChild);
	return true;
}

template<class T>
inline T* node<T>::remove_child(uint32_t nPriority, bool bDelete)
{
	child_vector::iterator it;
	for(it = vecChildren_.begin(); it!=vecChildren_.end(); ++it)
	{
		if((*it)->priority()==nPriority)
		{
			T* p = *it;
			p->set_parent(nullptr);
			vecChildren_.erase(it);


			if(bDelete)
			{
				delete p;
				return nullptr;
			}
			else
			{
				return p;
			}
		}
	}

	return nullptr;
}

template<class T>
inline const T* node<T>::child(uint32_t nPriority)const
{
	for(auto& n : vecChildren_)
	{
		if(n->priority()==nPriority) return n;
	}

	return nullptr;
}

template<class T>
inline T* node<T>::child(uint32_t nPriority)
{
	for(auto& n : vecChildren_)
	{
		if(n->priority()==nPriority) return n;
	}

	return nullptr;
}

template<class T>
inline void node<T>::clear_child(bool bDelete)
{
	if(bDelete)
	{
		child_vector::iterator it;
		for(it = vecChildren_.begin(); it!=vecChildren_.end(); ++it)
			delete *it;
	}
	
	vecChildren_.clear();
}

template<class T>
inline void node<T>::sort_child(bool bChild)
{
	if(bChild)
	{
		for(auto child : vecChildren_)
			child->sort_child(bChild);
	}

	// 基数(1byte)ソート
	array<vector<T*>, 256> buckets;
	uint32_t radix = 0;
	// バイト基数分繰り返す。IDは32bitなので4回
	for(uint32_t i=0; i<4; ++i)
	{
		for(auto& it : vecChildren_)
		{
			uint32_t key = (it->priority()>>radix)&255;
			buckets[key].emplace_back(it);
		}

		vecChildren_.clear();

		// バケットに合わせてつなぎ直し
		for(auto& b : buckets)
		{
			for(auto& c : b)
			{
				vecChildren_.emplace_back(c);
			}
			// 使用済みのbucketはクリア
			b.clear();
		}

		radix += 8;
	}
}

#define safe_delete_node(node) \
{ \
	if(node && !node->parent()) \
	{ \
		delete node; \
		node = nullptr; \
	} \
}

} // namespace utility end
} // namespace mana end
