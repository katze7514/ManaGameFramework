#pragma once

#include "archive.h"

namespace mana{
namespace file{

//! @brief ファイルを扱う基本クラス
/*! この単位でリソースキャッシュされる */
struct file_data
{
public:
	typedef shared_ptr<BYTE[]> data_sptr;

	file_data():nFileSize_(0),bDecode_(false){}

	bool allocData(uint32_t size);

	//! デコード済みか。非圧縮ファイルの時は常にtrue
	bool is_decode()const{ return bDecode_; }

	void clear(){ info_.clear(); pData_.reset(); nFileSize_=0; bDecode_=false; }

public:
	archive_file_info	info_; //!< このファイルの情報

	data_sptr			pData_;		//!< ファイル実体
	uint32_t			nFileSize_; //!< 実体のサイズ
	bool				bDecode_;	//!< デコード済みかどうか
};


/*!
 *	@brief ファイルを読み書きする
 *
 *	・一括読み込み
 *	・ストリーム読み書き
 *	・ManaArchiveシームレス読み込み
 *	  ・ManaArchiveは見かけ上フォルダとして振る舞う
 *	  　つまり、data1.dat というManaArchiveがあり、その中に、hero.png がある時は
 *	  　"data1/hero.png" というパスを渡す。
 *	  ・圧縮されてたら自動で伸長するのは、一括読み込み時のみ
 *	  　ストリームの時は、自前で伸長すること
 *
 *	 ・読みバッファは内部に確保することも、外のバッファを使うこともできる
 *
 *	 ・一括読み込みは、
 *	   open時にバッファがファイルサイズ分確保され、readを呼ぶとファイルを全部読む。
 *	   テキストモード時は、バッファを1バイト多く使い\0終端する。キャストするだけで文字列
 *	   として使える
 *	   一括読み込みした後は、内部バッファを対象にしたストリーム読みも出来る
 *	   一括読み込みreadした後に、バッファ指定readを使うと内部バッファから読み出すことになっている
 *
 *	 ・ストリームの時は、
 *	 　open時に指定したサイズ分バッファを確保し、read/writeを呼ぶとサイズ分読み書きし
 *	   毎回上書きされる。
 *	   バッファサイズに0を指定すると、内部バッファ確保は行わないので外部バッファ必須となる */
class file_access
{
public:
	enum{ DEFAULT_BUF_SIZE=4096 };

	enum op_mode{
		NONE,			//!< モード設定なし
		READ_ALL,		//!< 一括読み込み
		READ_ALL_TEXT,	//!< 一括読み込みテキストモード
		READ_STREAM,	//!< ストリーム読み込み
		WRITE_STREAM,	//!< ストリーム書き込み
	};

	enum result{
		FAIL,		//!< 失敗
		END_FILE,	//!< ファイル/バッファ末尾に到達
		SUCCESS,	//!< 成功
	};

	enum seek_base{
		SEEK_CURRENT,	//! 現在位置基点
		SEEK_BEGIN,		//! ファイルの先頭基点
		SEEK_RESET,		//! 先頭に戻す
	};

	file_access():eMode_(NONE),nBufSize_(DEFAULT_BUF_SIZE),hFile_(INVALID_HANDLE_VALUE),nDataPos_(0){}
	file_access(const path& p):eMode_(NONE),nBufSize_(DEFAULT_BUF_SIZE),hFile_(INVALID_HANDLE_VALUE),nDataPos_(0){ set_filepath(p); }
	~file_access(){ close(); }

	const file_data& filedata()const{ return fileData_; }
	file_data&		 filedata(){ return fileData_; }

	const path&		filepath()const{ return fileData_.info_.filePath_; }
	path&			filepath(){ return fileData_.info_.filePath_; }
	void			set_filepath(const path& p){ fileData_.info_.filePath_=p; }

	uint32_t		filesize()const{ return fileData_.nFileSize_; }

	file_data::data_sptr		buf(){ return fileData_.pData_; }
	const file_data::data_sptr	buf()const{ return fileData_.pData_; }

	//! 内部バッファ走査位置
	uint32_t data_pos()const{ return nDataPos_; }

	//! @brief ファイルオープン。存在チェックなど初期処理も行う
	/*!	@param[in] eMode 動作モード
	 *  @param[in] nBufSize ストリームモードの時に指定する読み書きサイズ(byte)。
	 *                      0だと内部バッファを使わない。読み書きにはバッファを外から指定するメソッドを使うこと
	 *  @retval false オープン失敗。ファイルが存在しない時やバッファが確保できなかった時に発生 */
	bool open(enum op_mode eMode, uint32_t nBufSize=DEFAULT_BUF_SIZE);

	//! @brief パス指定ファイルオープン。存在チェックなど初期処理も行う
	/*!	@param[in] p パス指定 
	 *  @param[in] eMode 動作モード
	 *  @param[in] nBufSize ストリームモードの時に指定する読み書きサイズ(byte)。
	 *                      0だと内部バッファを使わない。読み書きにはバッファを外から指定するメソッドを使うこと
	 *  @retval false オープン失敗。ファイルが存在しない時やバッファが確保できなかった時に発生 */
	bool open(const path& p, enum op_mode eMode, uint32_t nBufSize=DEFAULT_BUF_SIZE);

	//! ファイルがオープンしているかをチェックする
	bool is_open()const;

	//! ファイルクローズ。ハンドル閉じる。バッファ解放はしない
	void close();

	//! バッファを明示的に解放する
	void buf_release();

	//! @brief 読み込み
	/*! 一括の時は全部読む。
	 *  ストリームの時はストリームサイズだけ読む。もう一度呼ぶと上書きされる
	 * 
	 *  @param[out] rsize 実際に読み込めたサイズ
	 *  @return resultに従う */
	int32_t read(uint32_t& nRsize);

	//! @brief 与えられたバッファに読み込む
	/*! @param[out] nRsize 実際に読み込めたサイズ 
	 *  @param[out] pBuf   読み込んだデータを書き込むバッファ
	 *  @param[in]  nBufSize  読み込むサイズ
	 *  @return resultに従う */
	int32_t read(uint32_t& nRsize, void* pBuf, uint32_t nBufSize);

	//! @brief 単独変数読み込みのショートカット
	/*!　@return 正しく読め来ない時は、optional<T>()が返る */
	template<typename T>
	optional<T> read_val();

	//! @brief 書き込み
	/*! @param[in] pData 書き込むデータ
	 *  @param[in] nSize 書き込むサイズ
	 *  @return resultに従う */
	int32_t	write(const void* pData, uint32_t nSize);

	//! @brief 単独変数書き込み
	/*! @return resultに従う */
	template<typename T>
	int32_t write_var(T var);

	//! @brief ファイルシーク。ストリーム読みの時のみ有効
	/*! @param[in] nSeek シーク量。バイト単位
	 *  @param[in] eMode シークの基点。SEEK_RESETを指定すると先頭に戻る */
	int32_t seek(int32_t nSeek, seek_base eBase);

private:
	// 内部実装で使うヘルパー

	bool read_open();
	bool write_open();

	//! @brief アーカイブファイルのヘッダー部分を読む
	/*! @param[in] arcname アーカイブファイル名
		@return アーカイブの中にファイルがあるか無いか */
	bool read_archive_header(const string& sArcname);

	archive_file_info& fileinfo(){ return fileData_.info_; }

	void	 set_filesize(uint32_t nFsize){ fileData_.nFileSize_ = nFsize; }

private:
	file_data	fileData_;

	op_mode		eMode_;
	uint32_t	nBufSize_;

	HANDLE		hFile_;

	uint32_t	nDataPos_; // ファイルデータの先頭位置(fileinfo().nFilePos_)基準のデータ位置
};

///////////////////////
// template実装

template<typename T>
optional<T> file_access::read_val()
{
	T val;

	uint32_t rsize;
	if(read(rsize, &val, sizeof(val)))
	{
		if(rsize==sizeof(val))
			return optional<T>(val);
	}

	return optional<T>();
}

template<typename T>
int32_t file_access::write_var(T var)
{
	return write(&var, sizeof(var));
}


//////////////////////////
// ファイル系ユーティリティー

//! @brief ファイルから文字列を読み出し、stringstreamにセットする
/*! @param[out]	ss			読み出した文字列をいれるstringstream
 *  @param[in]	sFilePath	ファイルへのパス
 *	@param[in]	bFile		falseだとsFilePathは文字列そのもの */
extern inline bool load_file_to_string(std::stringstream& ss, const string& sFilePath, bool bFile=true);

} // namespace file end
} // namespace mana end

/*
	using namespace mana::file;
	file_access f("test/ManaArchive.cpp");

	if(f.open(file_access::ALL_READ))
	{
		uint32_t rsize;
		if(f.read(rsize))
		{
			f.close();

			BYTE* p = f.buf().get();
			char* c = new char[f.filedata().info_.nFileSize_+1];
			memcpy(c, p, f.filedata().info_.nFileSize_);
			c[f.filedata().info_.nFileSize_]=0;

			dprintln<string>(c);
		}
	}
*/
