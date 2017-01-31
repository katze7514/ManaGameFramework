#include "../mana_common.h"

#include <zlib.h>

#include "file.h"

namespace{

// 読み込み用CreateFileのショートカット
HANDLE OpenFileRead(LPCSTR filename)
{
	return ::CreateFile(filename,
						GENERIC_READ, 
						FILE_SHARE_READ,  // 読み込み同士は共有
						NULL, 
						OPEN_EXISTING,	  // ファイルが存在してないとエラー
						FILE_ATTRIBUTE_NORMAL, 
						NULL);
}

} // namespace end

namespace mana{
namespace file{

/////////////////////////////
// file_data
/////////////////////////////

bool file_data::allocData(uint32_t size)
{	
	pData_ = make_shared<BYTE[]>(size);
	
	if(pData_==nullptr)
	{
		logger::fatalln("[file_data]メモリが確保できませんでした。:" + to_str(size) + "byte");
		return false;
	}

	return true;
}

/////////////////////////////
// file_access
/////////////////////////////

bool file_access::open(enum op_mode eMode, uint32_t nBufSize)
{
	if(hFile_!=INVALID_HANDLE_VALUE)
	{
		logger::infoln("[file_access]すでにファイルを開いています。: " + filepath().str());
		return false;
	}

	if(eMode==NONE)
	{
		logger::warnln("[file_access]不正なモード指定");
		return false;
	}

	filedata().clear();

	eMode_	  = eMode;
	nBufSize_ = nBufSize;
	nDataPos_ = 0;

	if(eMode_ == READ_ALL || eMode_==READ_ALL_TEXT || eMode_ == READ_STREAM)
		return read_open();
	else
		return write_open();
}

bool file_access::open(const path& p, enum op_mode eMode, uint32_t nBufSize)
{
	set_filepath(p);
	return open(eMode, nBufSize);
}

bool file_access::is_open()const
{
	return hFile_!=INVALID_HANDLE_VALUE;
}

void file_access::close()
{
	if(hFile_!=INVALID_HANDLE_VALUE)
	{
		::CloseHandle(hFile_);
		hFile_=INVALID_HANDLE_VALUE;
	}

	eMode_	  = NONE;
	nBufSize_ = DEFAULT_BUF_SIZE;
}

void file_access::buf_release()
{
	fileData_.pData_.reset();
}

/////////////////////////
// read

bool file_access::read_open()
{
	logger::infoln("[file_access]ファイルを開きます。: " + filepath().str());

	hFile_ = OpenFileRead(filepath().c_str());

	if(hFile_!=INVALID_HANDLE_VALUE)
	{// ファイル見つかったら、ファイルサイズ取得

		// 4GBごえのファイルは未対応でいいよねぇ
		DWORD nFsize = ::GetFileSize(hFile_, NULL);

		if(nFsize==-1)
		{
			logger::warnln("[file_access]ファイルサイズが取得できないか、ファイルサイズが大きすぎます");
			close();
			return false;
		}

		set_filesize(nFsize);

		// 通常のファイル読みなので、デコード済み扱いにする
		filedata().bDecode_ = true;
	}
	else
	{// 読み込みの時は、アーカイブ読み込みの可能性があるので
	 // オープンが成功するまで、パスを駆け上がる
		// 駆け上がり用にディレクトリパスを取得
		path tmp_path = filepath().dir();

		while(true)
		{
			// ディクレクトリ区切りの"/"を取り除き、アーカイブファイル名にする
			// アーカイブ拡張子は".dat"
			hFile_ = OpenFileRead((tmp_path.dir_str(true) + ".dat").c_str());

			if(hFile_!=INVALID_HANDLE_VALUE)
			{// 見つかった
				// 4GBごえのファイルは未対応でいいよねぇ
				DWORD nFsize = ::GetFileSize(hFile_, NULL);

				if(nFsize==-1)
				{
					logger::warnln("[file_access]ファイルサイズが取得できないか、ファイルサイズが大きすぎます");
					close();
					return false;
				}

				set_filesize(nFsize);
				if(!read_archive_header(tmp_path.dir_str()))
				{// アーカイブ中にファイルが存在しないか、シークができなかった
					close();
					return false;
				}
				// 存在したのでループを抜ける
				break;
			}

			// 駆け上がり
			if(tmp_path.up_dir()=="")
			{// 上がれなかったら終了
				close();
				logger::warnln("[file_access]ファイルが存在しません :" + filepath().str());
				return false;
			}
		}
	}

	// バッファを確保

	// 一括読み込みだったらバッファサイズをファイルサイズにする
	if(eMode_==READ_ALL || eMode_==READ_ALL_TEXT)
		nBufSize_ = filesize();
	
	if(nBufSize_>0)
	{
		uint32_t nRealBufSize = nBufSize_;
		if(eMode_==READ_ALL_TEXT) nRealBufSize += 1; // テキストモードの時はバッファ+1して確保

		if(!filedata().allocData(nRealBufSize))
		{// バッファ確保できなかった
			close();
			return false;
		}
	}

	// 読み込み準備完了
	return true;
}

bool file_access::read_archive_header(const string& sArcname)
{
	// アーカイブファイルヘッダとファイル情報を読み込んで
	// ファイルが存在するかをチェック

	// 読み込みモードをSTREAMに変更
	file_access::op_mode pastMode = eMode_;

	BOOST_SCOPE_EXIT_ALL(&pastMode, this){
		eMode_ = pastMode;
	};

	eMode_ = file_access::READ_STREAM;

	// ヘッダー
	archive_header header;
	if(!header.read_file(*this))
	{
		logger::warnln("[file_access]アーカイブヘッダの読み込みができませんでした");
		return false;
	}

	// 検索用パスに変更
	// file_infoにはアーカイブフォルダ名は含まれていないので削除
	string sSerachpath = boost::erase_first_copy(filepath().str(), sArcname);

	// ファイル情報
	archive_file_info info;
	for(uint32_t i=0; i<header.nFileNum_; ++i)
	{
		if(info.read_file(*this))
		{// ファイル名をチェック
			if(info.filePath_.str() == sSerachpath)
			{// ファイル存在！
				// ファイル情報をコピーして終了
				fileinfo().nFileSize_ = info.nFileSize_;
				fileinfo().nArcSize_  = info.nArcSize_;
				fileinfo().nFilePos_  = info.nFilePos_;

				// 非圧縮ファイルだったら、デコード済み扱い
				filedata().bDecode_ = (info.nArcSize_==0);

				// 読み込むファイルサイズを設定
				// 圧縮ファイルだったら読み込むのは圧縮ファイルサイズ分
				set_filesize(info.nArcSize_==0 ? info.nFileSize_ : info.nArcSize_);

				// ファイルを読み込み先頭までシーク
				if(!seek(0, SEEK_RESET)) return false;

				return true;
			}
		}
		else
		{
			logger::warnln("[file_access]アーカイブファイル情報の読み込みができませんでした。: " + sSerachpath);
			return false;
		}
	}

	logger::warnln("[file_access]アーカイブファイルが存在しませんでした。: " + sSerachpath);
	return false;
}

int32_t file_access::read(uint32_t& nRsize)
{
	if(!is_open())
	{
		logger::warnln("[file_access]ファイルがオープンされていません。");
		return result::FAIL;
	}

	if(nBufSize_==0)
	{// バッファサイズが0の時は使えない
		logger::warnln("[file_access]内部バッファを使わない設定になっています");
		return result::FAIL;
	}
	
	op_mode pastMode = eMode_;
	eMode_=READ_STREAM; // 一時的にStreamにしてファイルから読む
	int32_t r = read(nRsize, buf().get(), nBufSize_);
	eMode_ = pastMode; // 戻す

	if(!r) return result::FAIL;

	// デコードされてなかったら、デコードする
	if((eMode_==READ_ALL || eMode_==READ_ALL_TEXT) && !filedata().is_decode())
	{
		// 元ファイルサイズ。テキストモードだったら+1しておく
		uint32_t nDestBufSize = fileinfo().nFileSize_;
		if(eMode_==READ_ALL_TEXT) nDestBufSize += 1;

		// 元のファイルサイズ分バッファを確保
		auto dest = make_shared<BYTE[]>(nDestBufSize);
		if(dest==nullptr)
		{
			logger::warnln("[file_access]圧縮後のファイル展開領域を確保できませんでした");
			return result::FAIL;
		}

		// zlib一括伸長
		uLongf nDsize = fileinfo().nFileSize_;
		uLongf nSsize = fileinfo().nArcSize_;

		int r = uncompress(dest.get(),              &nDsize,
						   filedata().pData_.get(),  nSsize);

		if(r!=Z_OK)
		{
			logger::warnln("[file_access]圧縮ファイルの伸長に失敗 : " + to_str(r));
			return result::FAIL;
		}

		// 伸長済みのデータに入れ替える
		filedata().pData_ = dest;

		// ファイルサイズを伸長後のものにする
		set_filesize(nDsize);

		// デコード済みにする
		filedata().bDecode_=true;

		// シーク位置を戻して置く
		seek(0, SEEK_RESET);
	}

	// テキストモードだったら末尾を\0終端
	if(eMode_==READ_ALL_TEXT)
	{
		filedata().pData_[filesize()]='\0';
	}

	return r;
}

int32_t file_access::read(uint32_t& nRsize, void* pBuf, uint32_t nBufSize)
{
	if(pBuf==nullptr)
	{
		logger::warnln("[file_access]読み出しバッファがnullです。");
		return result::FAIL;
	}

	if(nBufSize<=0) return result::SUCCESS;

	DWORD nReadbytes=nBufSize;
	int32_t r=result::SUCCESS;

	if(eMode_==READ_STREAM)
	{
		if(!is_open())
		{
			logger::warnln("[file_access]ファイルが開かれていません。");
			return result::FAIL;
		}

		if(nReadbytes<=0)
		{
			nRsize=0;
			return result::END_FILE;
		}

		BOOL  result = ::ReadFile(hFile_, pBuf, nBufSize, &nReadbytes, NULL);

		nRsize = nReadbytes;

		if(nReadbytes<nBufSize)
		{
		#ifdef MANA_DEBUG
			DWORD err = ::GetLastError();
			logger::debugln("[file_access]要求サイズ以下しか読み込めませんでした。: " + to_str(err));
		#endif
			r = result::END_FILE;
		}

		if(!result)
		{
			DWORD err = ::GetLastError();
			logger::warnln("[file_access]読み込みに失敗しました。: " + to_str(err));
			return result::FAIL;
		}
		else
		{
			nDataPos_ += nReadbytes;
			return r;
		}
	}
	else
	{// 内部バッファからの読み込み
		BYTE* pInnerBuf = buf().get();
		if(!pInnerBuf)
		{
			logger::warnln("[file_access]ファイルが読み込まれていません。");
			return result::FAIL;
		}

		nReadbytes = nBufSize;
		// ファイルサイズを超えていたら補正
		if((nDataPos_ + nBufSize)>filesize())
		{
			nReadbytes = filesize() - nDataPos_;
			r = result::END_FILE;
		}

		if(nReadbytes<=0)
		{
			nRsize=0;
			return result::END_FILE;
		}

		memcpy_s(pBuf, nBufSize, &pInnerBuf[nDataPos_], nReadbytes);

		nRsize = nReadbytes;
		nDataPos_ += nReadbytes; // Posを進める

		return r;
	}
}

/////////////////////////
// write

bool file_access::write_open()
{
	hFile_ = ::CreateFile(filepath().c_str(),
						  GENERIC_WRITE,
						  0,
						  NULL, 
						  TRUNCATE_EXISTING, // ファイルを作り直す
						  FILE_ATTRIBUTE_NORMAL, 
						  NULL);

	// ファイルが存在しないらしい
	if(hFile_==INVALID_HANDLE_VALUE)
	{
		hFile_ = ::CreateFile(filepath().c_str(),
							  GENERIC_WRITE,
							  0,
							  NULL, 
							  CREATE_NEW, // ファイルを作る
							  FILE_ATTRIBUTE_NORMAL, 
							  NULL);

		if(hFile_==INVALID_HANDLE_VALUE)
		{// ファイルが作れない……だと……？
			logger::warnln("[file_access]ファイルが作成できませんでした。");
			return false;
		}
	}

	return true;
}

int32_t file_access::write(const void* pData, uint32_t nSize)
{
	if(!is_open())
	{

		logger::warnln("[file_access]ファイルが開かれていません。");
		return result::FAIL;
	}

	if(pData==nullptr)
	{
		logger::warnln("[file_access]書き込み元バッファがnullです。");
		return result::FAIL;
	}

	DWORD nWsize;
	BOOL  result = ::WriteFile(hFile_, pData, nSize, &nWsize, NULL);

	if(!result || nWsize<nSize)
	{
		DWORD err = ::GetLastError();
		logger::warnln("[file_access]書き込みに失敗しました。： " + to_str(err));
		return result::FAIL;
	}
	else
	{	return result::SUCCESS;	}
}

/////////////////////////
// seek

int32_t file_access::seek(int32_t nSeek, seek_base eBase)
{

	if(eMode_==READ_STREAM
	|| eMode_==WRITE_STREAM)
	{
		if(!is_open())
		{
			logger::warnln("[file_access]ファイルが開かれていません。");
			return result::FAIL;
		}

		DWORD r=0;

		switch(eBase)
		{
		case SEEK_CURRENT:
			r = ::SetFilePointer(hFile_, nSeek, NULL, FILE_CURRENT);
		break;

		case SEEK_BEGIN:
			r = ::SetFilePointer(hFile_, fileinfo().nFilePos_+nSeek, NULL, FILE_BEGIN);
		break;

		case SEEK_RESET:
			r = ::SetFilePointer(hFile_, fileinfo().nFilePos_, NULL, FILE_BEGIN);
		break;
		}

		if(r==INVALID_SET_FILE_POINTER)
		{
			logger::warnln("[file_access]ファイルがシークができませんでした。: " + to_str_s(GetLastError()) + to_str_s(nSeek) + to_str(eBase));
			return result::FAIL;
		}
	}
	
	// DataPosの移動は共通
	switch(eBase)
	{
	case SEEK_CURRENT:
		if(nDataPos_ + nSeek<0)
			nDataPos_  = 0;
		else
			nDataPos_ += nSeek;
	break;

	case SEEK_BEGIN:
		if(nSeek>=0) nDataPos_ = nSeek;
	break;

	case SEEK_RESET:
		if(eMode_==READ_STREAM)
			nDataPos_ = fileinfo().nFilePos_;
		else
			nDataPos_ = 0;
	break;
	}

	return result::SUCCESS;
}


//////////////////////////
// ファイル読みユーティリティー
bool load_file_to_string(std::stringstream& ss, const string& sFilePath, bool bFile)
{
	if(bFile)
	{
		file_access file(sFilePath);
		if(!file.open(file_access::READ_ALL_TEXT)) return false;

		uint32_t rsize;
		if(file.read(rsize)==file_access::FAIL)	return false;

		ss << file.buf().get();
	}
	else
	{
		ss.str(sFilePath);
	}

	return true;
}


} // namespace file end
} // namespace mana end
