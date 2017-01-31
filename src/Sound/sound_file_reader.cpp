#include "../mana_common.h"

#include "sound_file_reader.h"

namespace mana{
namespace sound{

void sound_file_reader::fin()
{
	::ZeroMemory(&wvFmt_, sizeof(wvFmt_));
	wvFmt_.cbSize = sizeof(wvFmt_);

	close();
}

bool sound_file_reader::open(const string& sFilePath, bool bStream)
{
	fin();

	file_.set_filepath(sFilePath);

	if(file_.filepath().ext()=="wav")
		eFormat_ = FMT_WAV;
	else if(file_.filepath().ext()=="ogg" || file_.filepath().ext()=="oga")
		eFormat_ = FMT_OGG;

	if(eFormat_==FMT_NONE)
	{
		logger::warnln("[sound_file_reader]未対応サウンドフォーマットです。: " + to_str(file_.filepath().filename()));
		file_.set_filepath("");
		return false;
	}
	OV_EREAD;
	bStream_ = bStream;

	file::file_access::op_mode eOp;
	if(bStream_)
		eOp = file::file_access::READ_STREAM;
	else
		eOp = file::file_access::READ_ALL;

	if(!file_.open(eOp, 0))
	{
		logger::warnln("[sound_file_reader]サウンドファイルが開けませんでした。: " + sFilePath);
		return false;
	}

	if(bStream_)
	{
		if(!file_.filedata().is_decode())
		{
			logger::warnln("[sound_file_reader]圧縮ファイルはストリーム読みにできません。: " + sFilePath);
			return false;
		}
	}
	else
	{// 一括読み
		uint32_t rsize;
		if(file_.read(rsize)==file::file_access::FAIL)
		{
			logger::warnln("[sound_file_reader]ファイルを読み込めませんでした。: " + sFilePath);
			return false;
		}
		file_.close();
	}

	// WAVEFORMATを取得しつつ、データの先頭にシークする
	bool r = true;

	if(eFormat_==FMT_OGG)
		r = read_waveinfo_ogg();
	else
		r = read_waveinfo_wave();
	
	return r;
}

BYTE* sound_file_reader::data()
{
	if(bStream_) return nullptr;

	if(eFormat_==FMT_WAV)
	{
		BYTE* pBuf = file_.buf().get();

		if(!pBuf) return nullptr;

		return &pBuf[nDataFilePos_];
	}

	return nullptr;
}
	
uint32_t sound_file_reader::read(BYTE* pBuf, uint32_t nBufSize, bool bLoop, float fLoopSec)
{
	uint32_t rsize=0;

	if(eFormat_==FMT_WAV)
	{
		int32_t r = file_.read(rsize, pBuf, nBufSize);
		if(!r) return 0;
		if(r==file::file_access::END_FILE) bEndFile_ = true;

		if(bEndFile_ && bLoop)
		{// ループ設定で読めたバイト数が欲しいバイト数より少ない時は、先頭からさらに読む
			uint32_t past = rsize;
			if(seek_time(fLoopSec) && nBufSize-rsize>0)
			{
				file_.read(rsize, &pBuf[rsize], nBufSize-rsize);
				rsize += past;
			}

			bEndFile_=false;
		}
	}
	else if(eFormat_==FMT_OGG)
	{
		int bitstream=0;
		int nRequestSize = nBufSize;
		BYTE* pWriteBuf = pBuf;

		while(true)
		{
			int readedSize = ov_read(&ovFile_, reinterpret_cast<char*>(pWriteBuf), nRequestSize, 0, 2, 1, &bitstream);

			rsize += readedSize;

			if(readedSize==0)
			{// ファイル終端
				bEndFile_=true;

				if(bLoop)
				{// ループ
					ov_time_seek(&ovFile_, fLoopSec);
					bEndFile_=false;
				}
				else
				{// ループしないなら終了
					break;
				}
			}

			if(readedSize<nRequestSize)
			{// 読み込みが足りなかったら、残りを読み込む
				nRequestSize -= readedSize;
				pWriteBuf = &pWriteBuf[readedSize];
			}
			else
			{// 必要なだけ読み込んだ
				break;
			}
		}
	}


	return rsize;
}


bool sound_file_reader::seek_time(float fSec)
{
	bool ret=false;

	if(eFormat_==FMT_WAV)
	{
		// wavの時は、バイト変換する
		int32_t nSeek = static_cast<int32_t>(fSec * waveformat()->nAvgBytesPerSec);
		int32_t aline = waveformat()->nBlockAlign-1;
		nSeek = (nSeek+aline)&~aline; // Blockアライン

		ret = seek_data(nSeek, file::file_access::SEEK_BEGIN);
	}
	else if(eFormat_==FMT_OGG)
	{
		int r = ov_time_seek(&ovFile_, fSec);
		ret = r==0;
	}

	if(!ret) logger::warnln("[sound_file_reader]シークができませんでした。: " + file_.filepath().filename());

	return ret;
}

bool sound_file_reader::seek_data(int32_t nSeek, file::file_access::seek_base eBase)
{
	if(nDataFilePos_==0)
		return false;

	if(eBase==file::file_access::SEEK_RESET)
	{
		nSeek = 0;
		eBase = file::file_access::SEEK_BEGIN;
	}

	if(eFormat_==FMT_WAV)
		return file_.seek(nDataFilePos_+nSeek, eBase)!=file::file_access::FAIL;

	return false;
}

void sound_file_reader::close()
{
	file_.close();
	file_.buf_release();

	if(eFormat_==FMT_OGG) ov_clear(&ovFile_);

	eFormat_= FMT_NONE;

	bEndFile_	  = false;
	nDataSize_	  = 0;
	nDataFilePos_ = 0;
}

namespace{

// 対象のチャンクが見つかるまで進む
bool goto_riff_chunk(char* chunk, file::file_access& f)
{
	BYTE				buf[4];
	optional<int32_t>	n;
	uint32_t			rsize;
	int32_t				r;

	while(true)
	{
		// チャンクID
		r = f.read(rsize, buf, 4);
		if(r==file::file_access::FAIL) return false;

		// 対象チャンクだったらbreak
		if(buf[0]==chunk[0] && buf[1]==chunk[1] && buf[2]==chunk[2] && buf[3]==chunk[3])
			break;

		// fmtじゃなかったら、チャンクサイズ分飛ぶ
		n = f.read_val<int32_t>();
		if(!n) return false;
		
		f.seek(*n, file::file_access::SEEK_CURRENT);
		if((*n)%2)
		{// WinSDKで作ったRIFFだとチャンクサイズが奇数の時はNULLが追加されてることがあるらしいので確認
			optional<char> c = f.read_val<char>();
			if(c!=NULL)
			{// NULLじゃなかったら戻す
				f.seek(-1, file::file_access::SEEK_CURRENT);
			}
		}
	}

	return true;
}

}

bool sound_file_reader::read_waveinfo_wave()
{
	const int32_t FAIL = file::file_access::FAIL;

	BYTE			  buf[4];
	optional<int32_t> n;
	optional<int16_t> i;

	uint32_t rsize;
	int32_t	 r;

	uint32_t read=0;

	// ファイル位置をファイル先頭にする
	file_.seek(0, file::file_access::SEEK_BEGIN);

	// RIFF確認
	r = file_.read(rsize, buf, 4);
	if(r==FAIL
	|| !(buf[0]=='R' && buf[1]=='I' && buf[2]=='F' && buf[3]=='F'))
		goto END;

	++read;

	// ファイルサイズは読み飛ばし
	n = file_.read_val<int32_t>();
	if(!n) goto END;
	
	++read;

	// RIFF種別としてWAVE確認
	r = file_.read(rsize, buf, 4);
	if(r==FAIL
	|| !(buf[0]=='W' && buf[1]=='A' && buf[2]=='V' && buf[3]=='E'))
		goto END;

	++read;

	// fmtチャンクまで読み出す
	if(!goto_riff_chunk("fmt ", file_)) goto END;
	
	++read;

	// fmtチャンクのバイト数
	n = file_.read_val<int32_t>();
	if(!n) goto END;

	++read;

	// フォーマットID
	i = file_.read_val<int16_t>();
	if(!i) goto END;
	if((*i)!=WAVE_FORMAT_PCM)
	{
		logger::fatalln("FMT ID : "+ to_str(static_cast<int32_t>(*i)));
		goto END;
	}
	wvFmt_.wFormatTag = *i;

	++read;

	// チャンネル数
	i = file_.read_val<int16_t>();
	if(!i) goto END;
	wvFmt_.nChannels = *i;

	++read;

	// サンプリングレート
	n = file_.read_val<int32_t>();
	if(!n) goto END;
	wvFmt_.nSamplesPerSec = *n;

	++read;

	// データ速度
	n = file_.read_val<int32_t>();
	if(!n) goto END;
	wvFmt_.nAvgBytesPerSec = *n;

	++read;

	// ブロックサイズ
	i = file_.read_val<int16_t>();
	if(!i) goto END;
	wvFmt_.nBlockAlign = *i;

	++read;

	// サンプルあたりのbit数
	i = file_.read_val<int16_t>();
	if(!i) goto END;
	wvFmt_.wBitsPerSample = *i;

	++read;

	// dataを探す
	if(!goto_riff_chunk("data", file_)) goto END;

	++read;

	// dataのサイズ
	n = file_.read_val<int32_t>();
	if(!n) goto END;
	nDataSize_ = *n;

	++read;

	// 現在のデータ位置を取得
	nDataFilePos_ = file_.data_pos();

	return true;

END:
	logger::warnln("[sound_file_reader]読み込めないWAVE形式です。: " + to_str(read));
	return false;
}

///////////////////////////////////
// oggのコールバックに使う関数群
///////////////////////////////////

size_t ogg_file_read(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	sound_file_reader* pReader = reinterpret_cast<sound_file_reader*>(datasource);
	
	uint32_t rsize;
	int32_t r = pReader->file_.read(rsize, reinterpret_cast<BYTE*>(ptr), size*nmemb);

	if(rsize==0 || r==file::file_access::FAIL) return 0;
	return rsize / size;
}

int ogg_file_seek(void *datasource, ogg_int64_t offset, int whence)
{
	using namespace file;

	sound_file_reader* pReader = reinterpret_cast<sound_file_reader*>(datasource);

	file_access::seek_base base = file_access::SEEK_BEGIN;

	switch(whence)
	{
	case SEEK_CUR: // 現在位置から
		base = file_access::SEEK_CURRENT;
	break;

	case SEEK_END: // 後ろから
		// 先頭からに変換
		base = file_access::SEEK_BEGIN;
		offset = pReader->file_.filesize() + offset;
	break;

	case SEEK_SET: // 頭から
		base = file_access::SEEK_BEGIN;
	break;

	default: return -1;
	}

	if(pReader->file_.seek(static_cast<int32_t>(offset), base))
		return 0;
	else
		return -1;
}

long ogg_file_tell(void *datasource)
{
	sound_file_reader* pReader = reinterpret_cast<sound_file_reader*>(datasource);
	return pReader->file_.data_pos();
}

bool sound_file_reader::read_waveinfo_ogg()
{
	ov_callbacks ovCallBack;
	ovCallBack.read_func  = ogg_file_read;
	ovCallBack.seek_func  = ogg_file_seek;
	ovCallBack.close_func = NULL;
	ovCallBack.tell_func  = ogg_file_tell;

	int r = ov_open_callbacks(this, &ovFile_, 0, 0, ovCallBack);
	if(r!=0)
	{
		string sErr;
		switch(r)
		{
		case OV_EREAD:		sErr = "OV_EREAD"; break;
		case OV_ENOTVORBIS:	sErr = "OV_ENOTVORBIS"; break;
		case OV_EVERSION:	sErr = "OV_EVERSION"; break;
		case OV_EBADHEADER:	sErr = "OV_EBADHEADER"; break;
		case OV_EFAULT:		sErr = "OV_EFAULT"; break;
		}

		logger::warnln("[sound_file_reader]oggのオープンに失敗しました。: " + sErr);
	}

#pragma warning(disable:4244)
	vorbis_info* pInfo = ov_info(&ovFile_,-1);
	wvFmt_.wFormatTag		= WAVE_FORMAT_PCM;
	wvFmt_.nChannels		= pInfo->channels;
	wvFmt_.nSamplesPerSec	= pInfo->rate;
	wvFmt_.wBitsPerSample	= 16;
	wvFmt_.nBlockAlign		= wvFmt_.nChannels * wvFmt_.wBitsPerSample / 8;
	wvFmt_.nAvgBytesPerSec	= wvFmt_.nSamplesPerSec * wvFmt_.nBlockAlign;

	ogg_int64_t sampleTotal = ov_pcm_total(&ovFile_,-1);
	nDataSize_ = sampleTotal*wvFmt_.nBlockAlign;
#pragma warning(default:4244)

	return true;
}



} // namespace sound end
} // namespace mana end
