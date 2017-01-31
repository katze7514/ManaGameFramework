#pragma once

#include "../File/file.h"

namespace mana{

//! resource_manager上で使われるファイルクラス 
class resource_file
{
public:
	enum state : uint32_t{
		FAIL,
		SUCCESS,
		LOADING,
		NONE,
	};

public:
	resource_file();

public:
	void operator()();

public:
	void set_filepath(const string& sFilepath, bool bTextMode);

	shared_ptr<file::file_access>	file(){ return pFile_; }
	uint32_t						state()const{ return nState_; }

	uint32_t						filesize(){ return pFile_->filesize(); }

	file::file_data::data_sptr		buf(){ return pFile_->buf(); }
	file::file_data::data_sptr		buf()const{ return pFile_->buf(); }

private:
	void set_state(enum state eState){ nState_.store(eState, std::memory_order_release); }

private:
	shared_ptr<file::file_access> pFile_;

	bool				 bTextMode_;	//!< テキストモードかどうか
	std::atomic_uint32_t nState_;		//!< 処理状態
};

} // namespace mana end
