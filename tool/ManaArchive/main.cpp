/*
	指定したフォルダをManaArchiveにする。
	デフォルトでファイルは圧縮される。
	圧縮しない指定(-nc)や、圧縮しないファイル拡張子(-e)などが指定できる
	ストリーム読みするファイルは圧縮しないこと。伸長は一括読み込みでのみ行われるため

	ManaArchive フォルダ名 -nc -e 拡張子列(ogg) -log
*/
#include <iostream>
using std::cout;
using std::endl;

#include <boost/filesystem.hpp>

#include "Memory/util_memory.h"
#include "File/archive.h"
#include "File/file.h"

using namespace mana::file;
using namespace mana::memory;
 
namespace fs = boost::filesystem;

// オプション解析結果
struct option
{
	path			sDir_;		// アーカイブ対象ディレクトリ
	bool			bComp_;		// 圧縮フラグ
	bool			bLog_;		// ログを出すか
	int				nLevel_;	// 圧縮レベル
	vector<string>	vecExt_;	// 圧縮しない拡張子

	option():bComp_(true),bLog_(false),nLevel_(Z_DEFAULT_COMPRESSION){}

	// 圧縮しない拡張子かどうか
	bool isNcExt(const path& file)const
	{
		string ext = file.ext();
		for(auto it : vecExt_)
		{
			if(ext==it) return true;
		}

		return false;
	}

	// sDir_と結合してパスを作る
	// file_infoに入るパスはDir無しのパスになるので、
	// ファイルサイズを取ったりする時にアクセス可能なパスにするのに使う
	string add_dir_str(const string& p)const
	{
		return sDir_.dir_str() + p;
	}


	void print()
	{
		cout << "path : " << sDir_.str() << endl;
		cout << "comp : " << bComp_ << endl;
		cout << "log  : " << bLog_ << endl;
		cout << "level: " << nLevel_ << endl;
		cout << "ext  : ";
		for(auto ext : vecExt_)	cout << ext << " ";
		cout << endl;
	}
};

// アーカイブに含まれるファイル情報
struct archive_file
{
	archive_file_info info_;
	bool bComp_;	// 圧縮するかしないか
	bool bValid_;	// 有効なファイル情報か

	archive_file():bComp_(true),bValid_(true){}

	void print()
	{
		cout << bComp_ << " " << info_.filePath_.str() << " " << info_.nFileSize_ << " " << info_.nArcSize_ << " " << info_.nFilePos_ << endl;
	}
};

// アーカイブ
struct archive_info
{
	archive_header		 header_;
	vector<archive_file> vecFile_;

	void file_print()
	{
		cout << "[Dir]" << endl;
		for(auto it : vecFile_)	it.print();
	}
};

// プロトタイプ宣言
bool opt_parse(option& opt, int argc, char* argv[]);
bool dir_parse(archive_info& arc, const option& opt);
bool create_archive(archive_info& arc, const option& opt);

////////////////////////////
// main
int main(int argc, char* argv[])
{
	// 何も指定されてなかったら使い方表示
	if(argc<2)
	{
		cout << "usage: フォルダ名 [-nc] [-e 拡張子列]" << endl;
		cout << "　      -nc: 圧縮しない" << endl;
		cout << "-e 拡張子列: 圧縮しないファイルの拡張子" << endl;
		cout << "       -log: ログを表示する" << endl;
		cout << "  -lv -1～9: 圧縮レベルを指定。-1はデフォルト値" << endl;
		return 1;
	}

	// オプション解析
	option opt;
	if(!opt_parse(opt,argc,argv))
	{
		cout << "オプション解析失敗" << endl;
		return 1; 
	}
	
	// 指定されたディクレトリに存在するファイルを検索
	// しつつ、アーカイブ情報を貯める
	archive_info arc;
	if(!dir_parse(arc, opt)) return 1;

	// アーカイブ情報に基づきアーカイブファイルを作る
	if(!create_archive(arc, opt)) return 1;

	return 0;
}

////////////////////////////
// オプションを解析する
bool opt_parse(option& opt, int argc, char* argv[])
{
	bool bLv =false;
	bool bExt=false;

	for(int i=1; i<argc; ++i)
	{
		string desc(argv[i]);

		if(desc=="-nc")
		{
			opt.bComp_=false;
			bLv  = false;
			bExt = false;
		}
		else if(desc=="-log")
		{
			opt.bLog_=true;
			bLv  = false;
			bExt = false;
		}
		else if(desc=="-lv")
		{
			bLv  = true;
			bExt = false;
		}
		else if(desc=="-e")
		{
			bLv  = false;
			bExt = true;
		}
		else if(bLv)
		{
			opt.nLevel_ = lexical_cast<int>(desc);
			bLv = false;
		}
		else if(bExt)
		{
			opt.vecExt_.push_back(desc);
		}
		else
		{// 何も指定されてないならディレクトリ指定
			string dir(argv[i]);

			if(!opt.sDir_.empty())
				cout << "すでにディレクトリが指定されています。: " << opt.sDir_.str() << " " << dir << endl;

			// 末尾が"/"じゃなかったら追加
			if(dir.size()==1 || (dir.at(dir.size()-1)!='/' && dir.at(dir.size()-1)!='\\'))
				dir+="/";

			// パスとして保存
			opt.sDir_ = dir;
		}
	}

	// 解析結果表示
	if(opt.bLog_) opt.print();

	return true;
}

////////////////////////////////
// ディレクトリを解析しアーカイブ情報を貯める

// フォルダを再帰的に下降しつつファイル名を集める
bool dir_rec_parse(archive_info& arc,  const option& opt, const fs::path& d)
{
	try
	{
		fs::directory_iterator end;
		for(auto it = fs::directory_iterator(d); it!=end; ++it)
		{
			// ディレクトリだったら潜る
			if(fs::is_directory(*it))
			{
				if(!dir_rec_parse(arc, opt, it->path())) return false;
			}
			else if(fs::is_regular_file(*it))
			{
				// \ を / に置換
				std::string s = it->path().string();
				boost::replace_all(s,"\\", "/");
			
				// 指定フォルダ文字列を消す
				boost::erase_first(s, opt.sDir_.str());

				archive_file file;
				file.info_.filePath_ = s.c_str();

				if(opt.isNcExt(file.info_.filePath_))
					file.bComp_ = false;

				arc.vecFile_.push_back(file);
			}
		}
	}
	catch(std::exception e)
	{
		cout << e.what() << endl;
		return false;
	}

	return true;
}

bool dir_parse(archive_info& arc, const option& opt)
{
	try
	{
		fs::path d(opt.sDir_.c_str());
		// ディレクトリの存在して、かつディレクトリ
		if(!(/*fs::exists(d) &&*/ fs::is_directory(d)))
		{
			cout << "ディレクトリが存在しないか、ディレクトリではありません" << endl;
			return false;
		}

		// ディレクトリを再帰的に下降しつつファイル名を集める
		if(!dir_rec_parse(arc, opt, d)) return false;

		// 探索したファイル数を入れておく
		arc.header_.nFileNum_ = arc.vecFile_.size();

		// ログ表示
		if(opt.bLog_)
		{
			arc.file_print();
			cout << "file num: " << arc.header_.nFileNum_ << endl;
		}
	}
	catch(std::exception e)
	{
		cout << e.what() << endl;
		return false;
	}

	return true;
}

////////////////////////////////
// アーカイブ情報に基づき、アーカイブファイルを作成する
bool remove_file(const string& p)
{
	try
	{
		if(!fs::remove(p.c_str()))
		{
			cout << "ファイルを削除できませんでした。: " << p << endl;
			return false;
		}
	}
	catch(std::exception e)
	{
		cout << e.what() << endl;
		cout << "ファイルを削除できませんでした。: " << p << endl;
		return false;
	}

	return true;
}

bool create_serialize_file(archive_info& arc, const option& opt)
{
	// ファイルだけをシリアライズしたファイルを作る
	file_access arc_tmp_file(opt.sDir_.dirname(true)+".tmp");
	if(!arc_tmp_file.open(file_access::WRITE_STREAM))
	{
		cout << "作成できませんでした。: " << arc_tmp_file.filepath().str() << endl;
		return false;
	}

	// ファイルを開けながら、いろいろやる
	file_access read_file;
	uint32_t filenum=0;
	uint32_t filepos=0;
	vector<archive_file>& vecFile = arc.vecFile_;
	for(uint32_t i=0; i<vecFile.size(); ++i)
	{
		read_file.close();

		string filename = opt.add_dir_str(vecFile[i].info_.filePath_.str());
		if(!read_file.open(filename, file_access::READ_ALL))
		{
			cout << "オープンできませんでした。: " << filename << endl;
			vecFile[i].bValid_ = false;
			continue;
		}

		uint32_t rsize;
		if(read_file.read(rsize)==file_access::FAIL)
		{
			cout << "読み込みできませんでした。: " << filename << endl;
			vecFile[i].bValid_ = false;
			continue;
		}

		if(rsize==0)
		{
			cout << "サイズ0のファイルは書き込み対象から外れます。: " << filename << endl;
			vecFile[i].bValid_ = false;
			continue;
		}

		// ファイルサイズ格納
		vecFile[i].info_.nFileSize_ = rsize;

		// 圧縮するかどうか
		if(opt.bComp_ && vecFile[i].bComp_)
		{
			// 圧縮バッファ確保
			uLong bsize = compressBound(rsize);
			scoped_alloc buf(bsize);
			// 圧縮！
			int r = compress2(buf.pMem_, &bsize, read_file.buf().get(), rsize, opt.nLevel_);
			if(r==Z_OK)
			{
				// 圧縮後サイズを格納
				vecFile[i].info_.nArcSize_ = bsize;

				// 圧縮したファイルを書き込み
				if(arc_tmp_file.write(buf.pMem_, bsize))
				{
					// ファイル位置設定
					vecFile[i].info_.nFilePos_ = filepos;
					filepos += bsize;
					++filenum;
				}
				else
				{
					cout << "書き込みができませんでした。: " << filename << endl;
				}

				continue; // 書き込みが終了したので次へ
			}

			cout << "圧縮できませんでした。非圧縮扱いにします。: " << r << " " << filename << endl;
		}
		
		// 圧縮せずにそのまま書き込む
		if(arc_tmp_file.write(read_file.buf().get(), rsize))
		{
			// ファイル位置設定
			vecFile[i].info_.nFilePos_ = filepos;
			filepos += rsize;
			++filenum;
		}
		else
		{
			cout << "書き込みができませんでした。: " << filename << endl;
			vecFile[i].bValid_ = false;
		}
	}
	// テンポラリファイル作成完了
	arc_tmp_file.close();

	// 実際に書き込めたファイル数だけを改めて格納
	arc.header_.nFileNum_ = filenum;

	// ログを表示
	if(opt.bLog_) arc.file_print();

	if(filepos==0)
	{// 書き込むファイルが無かった……
		remove_file(arc_tmp_file.filepath().str());
		return false;
	}

	return true;
}

bool create_archive(archive_info& arc, const option& opt)
{
	// 圧縮やファイル位置を決めつつ、対象ファイルを結合したファイルを作成する
	if(!create_serialize_file(arc, opt)) return false;

	// ファイル位置にヘッダとファイル情報のサイズを足す
	uint32_t nPosOffset = arc.header_.size();

	vector<archive_file>& vecFile = arc.vecFile_;
	for(uint32_t i=0; i<vecFile.size(); ++i)
	{
		// 無効なファイル情報だったらスキップ
		if(!vecFile[i].bValid_) continue;

		nPosOffset += vecFile[i].info_.size();
	}

	if(opt.bLog_) cout << "pos offset: " << nPosOffset << endl;
	
	// アーカイブファイル作成
	file_access arc_file(opt.sDir_.dirname(true)+".dat");
	if(!arc_file.open(file_access::WRITE_STREAM))
	{
		cout << "作成できませんでした。: " << arc_file.filepath().str() << endl;
		return false;
	}

	if(opt.bLog_)
	{
		cout << "filenum: " << arc.header_.nFileNum_ << endl;
	}

	// ヘッダ書き込み
	if(!arc.header_.write_file(arc_file))
	{
		cout << "ヘッダが書き込めませんでした。" << endl;
		arc_file.close();
		remove_file(arc_file.filepath().str());
		return false;
	}

	// ファイル情報書き込み
	for(uint32_t i=0; i<vecFile.size(); ++i)
	{
		// 無効なファイル情報だったらスキップ
		if(!vecFile[i].bValid_) continue;
		
		vecFile[i].info_.nFilePos_ += nPosOffset; // ヘッダと情報分補正

		if(!vecFile[i].info_.write_file(arc_file))
		{
			cout << "ファイル情報が書き込めませんでした。: " << vecFile[i].info_.filePath_.str() << endl;
			arc_file.close();
			remove_file(arc_file.filepath().str());
			return false;
		}
	}

	if(opt.bLog_) arc.file_print();

	// シリアライズしたテンポラリファイルを結合
	file_access read_tmp(opt.sDir_.dirname(true)+".tmp");
	if(!read_tmp.open(file_access::READ_STREAM))
	{
		cout << "シリアライズファイルをオープンできませんでした。: " << read_tmp.filepath().str() << endl;
		arc_file.close();
		remove_file(arc_file.filepath().str());

		remove_file(read_tmp.filepath().str());
		return false;
	}

	// 読みながら書く
	uint32_t rsize;
	int32_t r = read_tmp.read(rsize);
	while(r==file_access::SUCCESS)
	{
		if(!arc_file.write(read_tmp.buf().get(),rsize))
		{
			cout << "アーカイブファイルへの書き込みができませんでした。" << endl;
			arc_file.close();
			remove_file(arc_file.filepath().str());

			read_tmp.close();
			remove_file(read_tmp.filepath().str());
			return false;
		}

		r = read_tmp.read(rsize);
	}

	if(r==file_access::FAIL)
	{
		cout << "読み込みができませんでした。: " << read_tmp.filepath().str() << endl;
		arc_file.close();
		remove_file(arc_file.filepath().str());

		read_tmp.close();
		remove_file(read_tmp.filepath().str());
		return false;
	}

	if(rsize>0)
	{// 読み取り残ってたら、忘れずに書き込み
		if(!arc_file.write(read_tmp.buf().get(),rsize))
		{
			cout << "アーカイブファイルへの書き込みができませんでした。" << endl;
			arc_file.close();
			remove_file(arc_file.filepath().str());

			read_tmp.close();
			remove_file(read_tmp.filepath().str());
			return false;
		}
	}

	// アーカイブファイル作成完了
	arc_file.close();

	// テンポラリファイル削除
	read_tmp.close();
	remove_file(read_tmp.filepath().str());

	return true;
}