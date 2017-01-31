#include "../mana_common.h"

#include "../Memory/util_memory.h"

#include "archive.h"
#include "file.h"

namespace mana{
namespace file{

namespace archive{
const char*		ARCHIVE_ID		= "manapack";
const uint32_t	HEADER_VER		= 1;
const uint32_t	FILE_INFO_VER	= 1;
} // namespace archive end

///////////////////////////
// archive_header
///////////////////////////

bool archive_header::read_file(file_access& file)
{
	uint32_t rsize;
	// ID読み込み
	BYTE  szID[9];
	::ZeroMemory(szID, sizeof(szID));

	if(!file.read(rsize, szID, sizeof(szID)-1))
	{
		logger::warnln("[archive_header]ファイルから読むことができませんでした。: ID");
		return false;
	}
	
	string sID(reinterpret_cast<char*>(szID));

	if(sID!=archive::ARCHIVE_ID)
	{
		logger::warnln("[archive_header]ヘッダーIDが異なります。: " + sID);
		return false;
	}

	// バージョン読み込み
	optional<uint32_t> r = file.read_val<uint32_t>();
	if(r)
	{
		nVer_ = *r;
	}
	else
	{
		logger::warnln("[archive_header]ファイルから読むことができませんでした。: バージョン");
		return false;
	}

	// ファイル数読みコミ
	r = file.read_val<uint32_t>();
	if(r)
	{
		nFileNum_ = *r;
	}
	else
	{
		logger::warnln("[archive_header]ファイルから読むことができませんでした。: ファイル数");
		return false;
	}

	return true;
}

bool archive_header::write_file(file_access& file)
{
	// ID書き込み
	if(!file.write(sID_.c_str(), sID_.size()))
	{
		logger::warnln("[archive_header]ファイルに書き込みことができませんでした。: ID");
		return false;
	}

	// バージョン書き込み
	if(!file.write_var(nVer_))
	{
		logger::warnln("[archive_header]ファイルに書き込みことができませんでした。: バージョン");
		return false;
	}

	// ファイル数書き込み
	if(!file.write_var(nFileNum_))
	{
		logger::warnln("[archive_header]ファイルに書き込みことができませんでした。: ファイル数");
		return false;
	}

	return true;
}


///////////////////////////
// archive_file_info
///////////////////////////

bool archive_file_info::read_file(file_access& file)
{
	// バージョン読み込み
	optional<uint32_t> r = file.read_val<uint32_t>();
	if(r)
	{
		nVer_ = *r;
	}
	else
	{
		logger::warnln("[archive_file_info]ファイルから読むことができませんでした。: バージョン");
		return false;
	}

	// ファイルパス名の長さ
	string::size_type nFilenameLen;
	optional<string::size_type> s = file.read_val<string::size_type>();
	if(s)
	{
		nFilenameLen = *s;
	}
	else
	{
		logger::warnln("[archive_file_info]ファイルから読むことができませんでした。: ファイル名の長さ");
		return false;
	}

	{// ファイル名
		memory::scoped_alloc buf(nFilenameLen+1);
		uint32_t rsize;
		int32_t r = file.read(rsize, buf.pMem_, nFilenameLen);
		if(r && rsize==nFilenameLen)
		{
			filePath_ = reinterpret_cast<char*>(buf.pMem_);
		}
		else
		{
			logger::warnln("[archive_file_info]ファイルから読むことができませんでした。: ファイル名");
			return false;
		}
	}

	// 圧縮前のファイルサイズ
	r = file.read_val<uint32_t>();
	if(r)
	{
		nFileSize_ = *r;
	}
	else
	{
		logger::warnln("[archive_file_info]ファイルから読むことができませんでした。: 圧縮前のファイルサイズ");
		return false;
	}

	// 圧縮後のファイルサイズ
	r = file.read_val<uint32_t>();
	if(r)
	{
		nArcSize_ = *r;
	}
	else
	{
		logger::warnln("[archive_file_info]ファイルから読むことができませんでした。: 圧縮後のファイルサイズ");
		return false;
	}

	// ファイル位置
	r = file.read_val<uint32_t>();
	if(r)
	{
		nFilePos_ = *r;
	}
	else
	{
		logger::warnln("[archive_file_info]ファイルから読むことができませんでした。: ファイル位置");
		return false;
	}

	return true;
}

bool archive_file_info::write_file(file_access& file)
{
	// ファイル情報バージョン書き込み
	if(!file.write_var(nVer_))
	{
		logger::warnln("[archive_file_info]ファイルに書き込みことができませんでした。: バージョン");
		return false;
	}

	// ファイルパスの長さ
	if(!file.write_var(filePath_.str().size()))
	{
		logger::warnln("[archive_file_info]ファイルに書き込みことができませんでした。: ファイルパスの長さ");
		return false;
	}

	// ファイルパス
	if(!file.write(filePath_.c_str(), filePath_.str().size()))
	{
		logger::warnln("[archive_file_info]ファイルに書き込みことができませんでした。: ファイル名");
		return false;
	}

	// 圧縮前ファイルサイズ
	if(!file.write_var(nFileSize_))
	{
		logger::warnln("[archive_file_info]ファイルに書き込みことができませんでした。: 圧縮前ファイルサイズ");
		return false;
	}

	// 圧縮後ファイルサイズ
	if(!file.write_var(nArcSize_))
	{
		logger::warnln("[archive_file_info]ファイルに書き込みことができませんでした。: 圧縮後ファイルサイズ");
		return false;
	}

	// ファイル位置
	if(!file.write_var(nFilePos_))
	{
		logger::warnln("[archive_file_info]ファイルに書き込みことができませんでした。: ファイル位置");
		return false;
	}

	return true;
}

} // namespace file end
} // namespace mana end
