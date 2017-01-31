#pragma once

#include <boost/intrusive/slist_hook.hpp>
#include <boost/intrusive/slist.hpp>

#include "../Concurrent/lock_helper.h"

namespace mana{
namespace memory{

//! shared_ptr用カスタムデリータ
template<typename T, template<typename obj> class obj_pool>
struct obj_pool_deleter
{
	obj_pool_deleter(obj_pool<T>* pPool):pPool_(pPool){}
	void operator()(T* p){ 	if(pPool_) pPool_->delete_obj(p); }

	obj_pool<T>* pPool_;
};

/*!
 *  @brief 簡易的なメモリプール
 *
 *   フリーリストを使った簡単なメモリプール
 *   LIFO方式で受け渡しをする。コンストラクタとデストラクタを呼ぶ
 *   足りなくても自動拡張はしない
 *
 *   @tparam T メモリプール対象となるクラス
 */
template<class T>
class easily_obj_pool
{
public:
	typedef boost::intrusive::slist_base_hook<>		free_list_node;
	typedef boost::intrusive::slist<free_list_node>	free_list;

	easily_obj_pool():pMemoryArea_(nullptr){}
	easily_obj_pool(size_t nNum):memory_area_(nullptr){	init(nNum);	}
	~easily_obj_pool(){	fin();	}

	//! num個のプールを作る
	void	init(size_t nNum);
	//! 確保したメモリ領域を解放する
	void	fin();

	//! @brief プールからnewする。デフォルトコンストラクタが呼ばれる
	/*! @retval nullptr プールの空きが無い  */
	T*		new_obj();

	//! @brief プールに戻る。デストラクタが呼ばれる
	/*! 戻したメモリはすぐにフリーリストノードとして使われるので
	 *  戻したメモリは絶対に使わないこと */
	void	delete_obj(T* pObj);

#ifdef MANA_DEBUG
	void	dump();
	size_t	nNum_;
#endif

private:
	// フリーリスト
	free_list listFree_;

	// 確保したメモリ領域
	void* pMemoryArea_;
};

template<class T>
void easily_obj_pool<T>::init(size_t nNum)
{
#ifdef MANA_DEBUG
	nNum_ = nNum;
#endif

	// すでにメモリが確保されてたら、強制終了
	if(pMemoryArea_) fin();

	// 少なくともfree_nodeのサイズは必要
	size_t nSize = sizeof(T) > sizeof(free_list_node) ? sizeof(T) : sizeof(free_list_node);

	// メモリ確保
	pMemoryArea_ = ::malloc(nSize*nNum);

	// フリーリストに分解していく
	if(pMemoryArea_)
	{
		for(uint32_t i=0; i<nNum; ++i)
		{
			void* pHeap = reinterpret_cast<void*>(reinterpret_cast<uint32_t>(pMemoryArea_) + nSize*i);
			free_list_node* pNode = new(pHeap) free_list_node();
			listFree_.push_front(*pNode);
		}
	}
}

template<class T>
void easily_obj_pool<T>::fin()
{
	if(pMemoryArea_)
	{
		listFree_.clear();
		::free(pMemoryArea_);
		pMemoryArea_ = nullptr;
	}
}

template<class T>
T* easily_obj_pool<T>::new_obj()
{
	if(listFree_.empty()) return nullptr;
	
	free_list_node* pNode = &listFree_.front();
	listFree_.pop_front();
	return new(reinterpret_cast<void*>(pNode)) T();
}

template<class T>
void easily_obj_pool<T>::delete_obj(T* pObj)
{
	if(pObj==nullptr) return;

	pObj->~T();

	free_list_node* pNode = new(reinterpret_cast<void*>(pObj)) free_list_node;
	listFree_.push_front(*pNode);
}


#ifdef MANA_DEBUG
template<class T>
void easily_obj_pool<T>::dump()
{
	logger::infoln("");

	logger::infos(reinterpret_cast<uint32_t>(pMemoryArea_));
	logger::infos(sizeof(T));
	logger::infos(sizeof(free_list_node));
	logger::infoln(nNum_);
		
	logger::infoln("");

	free_list::iterator it;
	for(it=listFree_.begin(); it!=listFree_.end(); ++it)
	{
		logger::infoln(reinterpret_cast<uint32_t>(&(*it)));
	}
}
#endif

/*!
 *  @brief 簡易的なメモリプールのスレッドセーフ版
 *
 *   @tparam T メモリプール対象となるクラス
 */
template<class T>
class easily_obj_shared_pool
{
public:
	easily_obj_shared_pool(){ flag_.clear(); }
	easily_obj_shared_pool(size_t nNum):pool_(nNum){}

	//! num個のプールを作る
	void	init(size_t nNum)
	{ 
		concurrent::spin_flag_lock lock(flag_);
		pool_.init(nNum);
		
	}
	//! 確保したメモリ領域を解放する
	void	fin()
	{
		concurrent::spin_flag_lock lock(flag_);
		pool_.fin();
	}

	//! @brief プールからnewする。デフォルトコンストラクタが呼ばれる
	/*! @retval nullptr プールの空きが無い  */
	T*		new_obj()
	{ 
		concurrent::spin_flag_lock lock(flag_);
		return pool_.new_obj();
	}

	//! @brief プールに戻る。デストラクタが呼ばれる
	/*! 戻したメモリはすぐにフリーリストノードとして使われるので
	 *  戻したメモリは絶対に使わないこと */
	void	delete_obj(T* pObj)
	{ 
		concurrent::spin_flag_lock lock(flag_);
		pool_.delete_obj(pObj);
	}

#ifdef MANA_DEBUG
	void	dump()
	{ 
		concurrent::spin_flag_lock lock(flag_);
		pool_.dump();
	}
#endif

private:
	easily_obj_pool<T> pool_;

	std::atomic_flag flag_; // spinロック用フラグ
};

} // namespace memory end
} // namespace mana end
