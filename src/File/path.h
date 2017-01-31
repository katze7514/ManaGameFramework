#pragma once

namespace mana{
namespace file{

/*!
	@brief ファイル/ディレクトリパスを操作するクラス

	パスを解析して、../ などをちゃんと処理する。
	区切りは "/" "\" のどちらでも使える。
	ディレクトリだけの時は、最後を閉じること。
*/
class path
{
public:
	path():bAbs_(false){}
	path(const string& sPath){ operator=(sPath); }
	path(const char* pPath){ operator=(pPath); }

	//! 文字列での代入。パスを解析して適切にデータを入れる
	path& operator=(const string& sPath);

	//! 文字列での代入。パスを解析して適切にデータを入れる
	path& operator=(const char* pPath);

	//! @brief パス結合。../ が来ると一つ上がる。
	/*! ../ は、ファイルを表してる時は無効	*/
	path& operator/=(const path& p);

	//! パス文字列を返す
	const string&	str()const{ return sPath_; }

	//! パス文字列を返す
	const char*		c_str()const{ return sPath_.c_str(); }

	//! 空パスかどうか
	bool			empty()const{ return sPath_.empty(); }

	//! ファイル名を返す。ディレクトリ表現の時は空
	const string&	filename()const{ return sFile_; }

	//! 拡張子名を返す。ディレクトリ表現の時は空
	string			ext()const;

	//! パスからファイルネームを削除する（破壊的操作）
	void	remove_file();

	//! @brief ディレクトリのみのパスを返す
	/*! a/b/c/d/file.txt の時、a/b/c/d/ を返す。 */
	path	dir()const{ return path(dir_str()); }

	//! @brief ディレクトリのみの文字列を返す
	/*! a/b/c/d/file.txt の時、a/b/c/d/ or a/b/c/d を返す。 
	 *  
	 *  @param[in] bNoSlash 末尾のスラッシュを含まないか
	 *                      true だと末尾のスラッシュを含まない */
	string	dir_str(bool bNoSlash=false)const;

	//! @brief カレントディレクトリ名を返す
	/*! a/b/c/d/ の時、d/ or d を返す。
	 *  
	 *  @param[in] bNoSlash 末尾のスラッシュを含まないか
	 *                      true だと末尾のスラッシュを含まない */
	string	dirname(bool bNoSlash=false)const;

	//! @brief 親ディレクトリパスを返す
	/*! a/b/c/d/ の時、a/b/c/ を返す。 */
	path	parent_dir()const;

	//! @brief 親ディレクトリ文字列を返す
	/*! a/b/c/d/ の時、a/b/c/ or a/b/c を返す。
	 *  
	 *  @param[in] bNoSlash 末尾のスラッシュを含まないか
	 *                      true だと末尾のスラッシュを含まない */
	string	parent_dir_str(bool bNoSlash=false)const;

	//! @brief 親ディレクトリ名を返す
	/*! a/b/c/d/ の時、c/ or cを返す。
	 *  
	 *  @param[in] bNoSlash 末尾のスラッシュを含まないか
	 *                      true だと末尾のスラッシュを含まない */
	string	parent_dirname(bool bNoSlash=false)const;

	//! パスを/もしくは\ で分割した結果を返す
	vector<string> dir_list()const;

	//! @brief ディレクトリ階層を一つ上がる（破壊的操作）
	/*! a/b/c/d/ の時呼ぶと、d/を返し、a/b/c/になる
	 *
	 * @retval string 上がったディレクトリ名を返す */
	string	up_dir();

	//! @brief パスに含まれている"../"を解決する（破壊的操作）
	void	clean_up();

	//! ファイルを表してるパスかどうか
	bool is_file()const{ return !sPath_.empty() && !sFile_.empty(); }
	//! ディレクトリを表してるパスかどうか
	bool is_dir()const{ return !sPath_.empty() && sFile_.empty(); }
	//! 絶対パスを表しているかどうか
	bool is_abs()const{ return bAbs_; }

private:
	//! 設定されてるpathをディレクトリとファイルに分ける
	void split_dir_file();
	
private:
	string	sPath_;	//!< 設定されたパス 

	string	sDir_;	//!< パスのディレクトリ部分
	string	sFile_;	//!< パスのファイル名部分

	bool	bAbs_;	//1< 絶対パスかどうかは、ドライブレターかネットワークパスが含まれているかで判断
};

// 等価
extern bool operator==(const path& lhs, const path& rhs);

} // namespace file end
} // namespace mana end
