#pragma once

#include <vorbis/vorbisfile.h>

#include "../File/file.h"

namespace mana{
namespace sound{

/*! @brief サウンドファイルを読み込んでフォーマットを解析して必要な情報を取り出す
 *
 *  対応フォーマットは、waveとogg vorbis
 *  ogg vorbisはストリーム読みのみ対応
 */
class sound_file_reader
{
public:
	friend size_t	ogg_file_read(void *ptr, size_t size, size_t nmemb, void *datasource);
	friend int		ogg_file_seek(void *datasource, ogg_int64_t offset, int whence);
	friend long		ogg_file_tell(void *datasource);

public:
	enum sound_format
	{
		FMT_WAV,
		FMT_OGG,
		FMT_NONE,
	};

public:
	sound_file_reader():bStream_(false),bEndFile_(false),eFormat_(FMT_NONE),nDataSize_(0),nDataFilePos_(0){ ::ZeroMemory(&wvFmt_,sizeof(wvFmt_)); wvFmt_.cbSize = sizeof(wvFmt_); }
	~sound_file_reader(){ fin(); }

	//! 終了処理
	void fin();

	//! @brief 初期化。フォーマット情報まで読み込む
	/*! @param[in] sFilename サウンドファイルへのパス
	 *                       拡張子でフォーマットを判定するので、WAVEならwav、oggならogg/ogaにすること
	 *  @param[in] bStream ストリーム読み込みするかどうか
	 *  @return 未対応フォーマットだったり、WAVEフォーマットが上手く読めなかったら失敗する */
	bool open(const string& sFilePath, bool bStream);

	bool is_open()const{ return file_.is_open(); }

	//! 設定したファイルのWAVEフォーマットを取得する
	//! このポインタはそのままDSBUFFERDESCに渡して良い
	WAVEFORMATEX* waveformat(){ return &wvFmt_; }

	//! @brief サウンドデータを必要なだけ取得する
	/*! @param[in] pBuf 読み込んだデータの書き込み先
	 *  @param[in] pBufSize 読み込むデータ量
	 *  @param[in] bLoop trueだと末尾に到達したfLoopSec秒から読み直す
	 *	@param[in] fLoopSec ループ時に戻る位置（秒数）
	 *  @return 実際に読み込んだデータ量を返す。0の時はエラーかファイルの終端に到達済み */
	uint32_t	read(BYTE* pBuf, uint32_t nBufSize, bool bLoop, float fLoopSec=0.0f);

	//! サウンドデータを時間でシークする
	bool		seek_time(float fSec);

	//! サウンドデータをデータ単位でシークする。wavの時のみ有効
	bool		seek_data(int32_t nSeek, file::file_access::seek_base eBase);

	//! ファイルを閉じる。読み込み済みのバッファも解放する
	void		close();

	//! サウンドデータを取得する。MODE_ALLの時のみ有効
	BYTE*		data();
	//! サウンドデータサイズ
	uint32_t	data_size()const{ return nDataSize_; }
	//! データの終わりに到達
	bool		is_data_file_end()const{ return bEndFile_; }

private:
	//! @defgroup sound_reder_helper サウンドファイル読み込みヘルパー
	//! @{
	bool read_waveinfo_wave();
	bool read_waveinfo_ogg();
	//! @}


private:
	file::file_access	file_;
	bool				bStream_;
	bool				bEndFile_;

	sound_format		eFormat_;
	WAVEFORMATEX		wvFmt_;
	uint32_t			nDataSize_;		//!< サウンドデータだけのサイズ
	int32_t				nDataFilePos_;  //!< データへの先頭ファイル位置

	OggVorbis_File		ovFile_;
};

} // namespace sound end
} // namespace mana end
