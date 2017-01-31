#pragma once

#include "../Concurrent/lock_helper.h"

namespace mana{
namespace utility{

/*! @brief IDマネージャー
 *
 *  スレッドセーフ
 *
 *  ある型のIDを数値IDとして扱えるようにするクラス
 *  数値のassignをして、必要がなくなったらeraseする
 *  eraseした数値は再利用される。
 *  数値は0以上の値が割り振られる
 */ 
template<class T>
class id_manager
{
public:
	id_manager(){ flag_.clear(); }

	void	 init(uint32_t nReserveIDNum, uint32_t nStartID, const T& invalid);

	uint32_t assign_id(const T& value);
	void	 erase_id(const T& value);
	void	 erase_id(uint32_t nID);

	const T&			id(uint32_t nID);
	optional<uint32_t>  id(const T& ID);

private:
	vector<T>	id_;

	uint32_t	nStartID_; // IDの開始位置
	T			invalid_;  // 無効と見なす値

	std::atomic_flag flag_; // スレッドセーフ化するためのフラグ
};

template<class T>
inline void id_manager<T>::init(uint32_t nReserveIDNum, uint32_t nStartID, const T& invalid)
{
	concurrent::spin_flag_lock lock(flag_);

	id_.reserve(nReserveIDNum);
	nStartID_ = nStartID;
	invalid_  = invalid;

	for(uint32_t i=0; i<nReserveIDNum; ++i)
		id_.emplace_back(invalid_);
}

template<class T>
inline uint32_t id_manager<T>::assign_id(const T& value)
{
	concurrent::spin_flag_lock lock(flag_);

	// 空いてるID欄を探す
	for(uint32_t i=0; i<id_.size(); ++i)
	{
		if(id_[i]==invalid_)
		{
			id_[i]=value;
			return i+nStartID_;
		}
	}

	// 無いなら追加
	id_.emplace_back(value);
	return id_.size()+nStartID_;
}

template<class T>
inline void id_manager<T>::erase_id(const T& value)
{
	concurrent::spin_flag_lock lock(flag_);

	for(uint32_t i=0; i<id_.size(); ++i)
	{
		if(id_[i]==value)
		{
			id_[i]=invalid_;
		}
	}
}

template<class T>
inline void id_manager<T>::erase_id(uint32_t nID)
{
	concurrent::spin_flag_lock lock(flag_);

	if(nID<nStartID_ || nID>id_.size()) return;

	id_[nID-nStartID_] = invalid_;
	
}

template<class T>
inline const T& id_manager<T>::id(uint32_t nID)
{
	concurrent::spin_flag_lock lock(flag_);

	if(nID<nStartID_ || nID>id_.size()) return invalid_;
	return id_[nID-nStartID_];
}

template<class T>
inline optional<uint32_t> id_manager<T>::id(const T& tID)
{
	concurrent::spin_flag_lock lock(flag_);

	for(uint32_t i=0; i<id_.size(); ++i)
	{
		if(id_[i]==tID) return optional<uint32_t>(i+nStartID_);
	}

	return optional<uint32_t>();
}

} // utility
} // namespace mana end
