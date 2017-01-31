#pragma once

#include "path.h"

namespace mana{
namespace file{

// 前方宣言
class file_access;

/*
 *	ManaFrameworkアーカイブ形式を扱う
 * 
 *   ManaFrameworkのアーカイブ形式は、
 *
 *   header(識別子、ver、ファイル数などを持つ)
 *   file_info×ファイル数（ver、ファイル名サイズ、ファイル名、ファイルサイズ、圧縮、ファイル位置）
 *   file実体×ファイル数
 *
 *   と並ぶバイナリファイル。
 *   ファイル名は、アーカイブファイルをルートする相対パスで一意に決め、シリアライズされる
 *   つまり、
 *    
 *    archive---a.txt
 *            |
 *            --graphic---hero.png
 *            
 *
 *    の時、archiveフォルダをアーカイブ化すると、
 *
 *    a.txt
 *    graphic/hero.png
 *
 *    となる。
 *
 *    また、各表現クラスはPOD型では内ので、
 *
 *       archive_header header;
 *       ::ReadFile(hFile, &header, sizeof(header));
 *
 *    では読めないので各メンバごとに読むか、
 *    
 *       BYTE* b = new BYTE[header.size()];
 *       ::ReadFile(hFile, b, header.size());
 *
 *    で読んでから各メンバにばらすこと。
 */

namespace archive{
extern const char*		ARCHIVE_ID; // manapack
extern const uint32_t	HEADER_VER;
extern const uint32_t	FILE_INFO_VER;
} // namespace archive end

//! ManaFrameworkアーカイブヘッダ
struct archive_header
{
public:
	archive_header():sID_(archive::ARCHIVE_ID), nVer_(archive::HEADER_VER), nFileNum_(0){}
	archive_header& operator=(const archive_header& h){ nVer_=h.nVer_; nFileNum_=h.nFileNum_; }

	// headerのサイズ。読み書きの際に使う。sizeof(archive_header)とは一致しないので注意
	uint32_t size()const{ return sID_.size()+sizeof(nVer_)+sizeof(nFileNum_); }	

	bool read_file(file_access& file);
	bool write_file(file_access& file);

public:
	const string	sID_;		//!< manapack固定
	uint32_t		nVer_;		//!< ヘッダ形式のバージョン
	uint32_t		nFileNum_;	//!< このアーカイブに含まれているファイル数
};

//! ManaFrameworkアーカイブファイル情報
struct archive_file_info
{
public:
	archive_file_info():nVer_(archive::FILE_INFO_VER),nFileSize_(0),nArcSize_(0),nFilePos_(0){}

	bool read_file(file_access& file);
	bool write_file(file_access& file);

	// 圧縮ファイルかどうか
	bool IsArcFile()const{ return nArcSize_>0; }

	// file_infoのサイズ。読み書きの際に使う。sizeof(file_info)とは一致しないので注意
	uint32_t size()const{ return sizeof(nVer_)+sizeof(string::size_type)+filePath_.str().size()+sizeof(nFileSize_)+sizeof(nArcSize_)+sizeof(nFilePos_); }

	void clear(){ filePath_.clean_up(); nFileSize_=nArcSize_=nFilePos_=0;  }

public:
	uint32_t	nVer_;		//!< ファイル情報形式のバージョン
	path		filePath_;	//!< ファイルパス
	uint32_t	nFileSize_; //!< 圧縮前のファイルサイズ
	uint32_t	nArcSize_;	//!< 圧縮後のファイルサイズ（0の時は非圧縮ファイル）
	uint32_t	nFilePos_;  //!< アーカイバ内のファイル位置。アーカイブファイル先頭からの位置
};

} // namespace file end
} // namespace mana end
