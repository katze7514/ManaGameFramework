#include "path.h"

namespace{

inline string::size_type size_type_max(string::size_type slash, string::size_type back)
{
	if(slash==string::npos)
		return back;
	else if(back==string::npos)
		return slash;
	else
		return slash > back ? slash : back;
}

// 与えたpathの最後の/か\の位置を返す
inline string::size_type find_last_delimiter(const string& str, string::size_type pos = string::npos)
{
	return str.find_last_of("/\\", pos);
}

inline string noslash_dir(const string& dir, bool bNoSlash)
{
	if(bNoSlash)
		return dir.substr(0, dir.size()-1);
	else
		return dir;
}

} // namespace end

namespace mana{
namespace file{

path& path::operator=(const string& sPath)
{
	sPath_ = sPath;
	clean_up();
	return *this;
}


path& path::operator=(const char* pPath)
{
	sPath_ = pPath;
	clean_up();
	return *this;
}

path& path::operator/=(const path& p)
{
	if(is_dir())
	{
		sPath_ += p.str();
		clean_up();
	}
	return *this;
}

string path::ext()const
{
	if(sFile_.empty()) return "";

	string::size_type delim = sFile_.find_last_of(".");
	if(delim == string::npos) return "";

	return sFile_.substr(delim+1);
}

void path::remove_file()
{
	string::size_type delim = find_last_delimiter(sPath_);
	if(delim!=string::npos)
	{
		sPath_ = sPath_.substr(0, delim+1);
		sDir_  = sPath_;
		sFile_.clear();
	}
}

string	path::dir_str(bool bNoSlash)const
{
	return noslash_dir(sDir_, bNoSlash);
}

string path::dirname(bool bNoSlash)const
{
	// 親ディレクトリ無し
	if(sDir_.size()<2) return sDir_;

	// 一番後ろの"/"の次の"/"を見つけるため、一番後ろの"/"を除いた位置から検索する
	string::size_type delim = find_last_delimiter(sDir_, sDir_.size()-2);

	// 見つかった後ろから2番目の"/"と一番後ろの"/"の間がカレントディレクトリ名
	if(delim==string::npos)
		return noslash_dir(sDir_, bNoSlash);
	else
		return noslash_dir(sDir_.substr(delim+1), bNoSlash);
}

path path::parent_dir()const
{
	return path(parent_dir_str());
}

string path::parent_dir_str(bool bNoSlash)const
{
	// 親ディレクトリ無し
	if(sDir_.size()<2) return "";

	// 一番後ろの"/"の次の"/"を見つけるため、一番後ろの"/"を除いた位置から検索する
	string::size_type delim = find_last_delimiter(sDir_, sDir_.size()-2);
	if(delim==string::npos)
		return "";
	else
		return noslash_dir(sDir_.substr(0,delim+1), bNoSlash);
}

string path::parent_dirname(bool bNoSlash)const
{
	// 親ディレクトリ無し
	if(sDir_.size()<2) return "";

	// 後ろから2番目の"/"を検索
	string::size_type delim = find_last_delimiter(sDir_, sDir_.size()-2);

	if(delim==string::npos)
	{// カレントディレクトリしか無かった
	 // a/ → ""
		return "";
	}
	else
	{
		// 後ろから3番目の"/"を検索
		string::size_type delim2 = find_last_delimiter(sDir_, delim-1);

		if(delim2==string::npos)
		{// 後ろから3番目の"/"は無かったので、先頭から2番目の"/"までが親
		 // a/b/ → a/
			return noslash_dir(sDir_.substr(0,delim+1), bNoSlash);
		}
		else
		{// 後ろから3番目の"/"があったので、後ろから3番目の"/"と後ろから2番目の"/"までが親
		 // a/b/c/ → b/
			return noslash_dir(sDir_.substr(delim2+1, delim-delim2), bNoSlash);
		}
	}
}

vector<string> path::dir_list()const
{
	using namespace boost::algorithm;
	vector<string> result;
	split(result, sPath_, is_any_of("/ \\"));
	return result;
}

string path::up_dir()
{
	// 親ディレクトリ無し
	if(sDir_.size()<2) return "";

	// 一番後ろの"/"の次の"/"を見つけるため、一番後ろの"/"を除いた位置から検索する
	string::size_type delim = find_last_delimiter(sDir_, sDir_.size()-2);
	// 親いなかった
	if(delim==string::npos) return "";

	// 親ディレクトリがあったら、現在のパスをそこにする
	string cur	= sDir_.substr(delim+1);
	sPath_		= sDir_.substr(0,delim+1);
	sDir_		= sPath_;

	// カレントだったディレクトリを返す
	return cur;
}

void path::clean_up()
{
	// ファイル名が含まれてるかのチェック
	string::size_type delim = find_last_delimiter(sPath_);
	if(delim==string::npos)
	{// ファイル名だけだったので、終了
		split_dir_file();
		return;
	}

	// ディレクトリ区切りが末尾じゃなかったらファイル名がある
	bool bFile = (delim!=(sPath_.size()-1));

	// パスを一端dir_listに分割
	vector<string> dirlist = dir_list();
	
	// 新しいパス
	string clean_path;

	// 後ろから探査していき、"."や".."を解決する
	uint32_t up_count=0;
	vector<string>::reverse_iterator it;
	for(it=dirlist.rbegin(); it!=dirlist.rend(); ++it)
	{
		if((*it).empty() || (*it)==".")
		{// 空(//とかなってる時)やカレント指定はスキップするだけ
			continue;
		}
		else if((*it)=="..")
		{// 上がる数を数える
			++up_count;
		}
		else
		{// ディレクトリ
			if(up_count>0)
			{// up_countがあったらスキップ
				--up_count;
				continue;
			}
			else
			{// なかったら結合
				// 後ろから見てるので前に結合する
				clean_path = (*it) + "/" + clean_path;
			}
		}
	}

	// .. が残ってたら、先頭に付与
	for(uint32_t i=up_count; i>0; --i)
		clean_path = "../" + clean_path;

	// ファイル名が含まれてたら最後の区切り文字は削除
	if(bFile) clean_path.erase(clean_path.end()-1);

	// 空になったらカレントパスだけということなので
	// カレントパスにしておく
	if(clean_path.empty()) clean_path = "./";

	// 解決したパスで上書き
	sPath_=clean_path;

	split_dir_file();
}

void path::split_dir_file()
{
	bAbs_=false;

	if(sPath_.empty())
	{
		sDir_.clear();
		sFile_.clear();
		return;
	}

	string::size_type delim = find_last_delimiter(sPath_);
	if(delim==string::npos)
	{// 無いならファイル名のみのはず
		sFile_ = sPath_;
		sDir_.clear();
		return;
	}
	else
	{// 分割！
		sDir_  = sPath_.substr(0, delim+1);
		sFile_ = sPath_.substr(delim+1);
	}

	// 絶対パスかどうかを判定
	// sDirにドライブレター(*:)もしくはネットワークパス(\\で始まる)が含まれているかをチェック
	
	// ネットワークパスかどうか
	if(size_type_max(sDir_.find("//", 0), sDir_.find("\\\\", 0))==0)
	{// ネットワークパス！
		bAbs_=true;
		return;
	}

	// ドライブレターがあるか
	string::size_type colon;
	colon	= sDir_.find(":", 0);
	delim	= sDir_.find_first_of("/\\", 0);

	// コロンが一番最初のディレクトリ区切りより前にあるなら、たぶん、正しいドライブレター形式
	if(colon!=string::npos && delim!=string::npos)
	{
		if(colon < delim) bAbs_=true;
	}
}

bool operator==(const path& lhs, const path& rhs)
{
	return lhs.str() == rhs.str();
}


} // namespace file end
} // namespace mana end

/*
mana::file::path local("csda:\\abd\\set\\");
//dprintln(local.is_abs());

mana::file::path net("c://abd\\set\\");
//dprintln(net.is_abs());
//dprintln(net.str());

mana::file::path cur("../..\\adas/sdfasdf/..\\a.txt");
dprintln(cur.is_abs());
dprintln(cur.str());
//dprintln(cur.dirname());
//dprintln(cur.filename());
	
local /= cur;
dprintln(local.is_abs());
dprintln(local.str());
*/