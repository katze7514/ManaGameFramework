#include "../mana_common.h"

//#include "../File/file.h"

#include "resource_file.h"

namespace mana{

resource_file::resource_file():bTextMode_(false),nState_(NONE)
{
	pFile_ = make_shared<file::file_access>();
}

void resource_file::operator()()
{
	if(!pFile_->open(bTextMode_ ? file::file_access::READ_ALL_TEXT : file::file_access::READ_ALL))
	{
		set_state(FAIL);
		return;
	}

	set_state(LOADING);

	uint32_t rsize;
	int32_t r = pFile_->read(rsize);
	pFile_->close();

	if(r==file::file_access::FAIL)
		set_state(FAIL);
	else
		set_state(SUCCESS);

	logger::debugln("[resource_file]ファイル読み込み終了 : " + pFile_->filepath().str() + " " + to_str(nState_));
}

void resource_file::set_filepath(const string& sFilepath, bool bTextMode)
{
	pFile_->set_filepath(file::path(sFilepath));
	bTextMode_	= bTextMode;
	set_state(NONE);
}

} // namespace mana end
