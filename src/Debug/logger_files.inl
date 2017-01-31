/*
	logger.cppにてincludeされている
*/
namespace{

//! ファイルサイズが0だったら削除する
void remove_file_size_zero(file_access& file)
{
	namespace fs =  boost::filesystem;
	boost::system::error_code ec;

	fs::path p(file.filepath().c_str());

	if(fs::exists(p,ec))
	{
		uintmax_t size = fs::file_size(p,ec);
		if(size==0) fs::remove(p,ec);
	}
}

} // namespace end

//! ログファイル管理クラス
class logger_files
{
public:
	typedef flat_map<string, file_access*> category_map;

	logger_files(const string& sAllFilename)
	{
		if(!all_.open(PRE_LOG_NAME + sAllFilename + ".txt", file_access::WRITE_STREAM))
			warnln("[logger_files]統合ファイルを作成出来ませんでした: "+ all_.filepath().str());
	}

	~logger_files(){ fin(); }

	void fin()
	{
		all_.close();
		remove_file_size_zero(all_);

		category_map::iterator it;
		for(it=mapCategory_.begin(); it!=mapCategory_.end(); ++it)
		{
			it->second->close();
			remove_file_size_zero(*(it->second));
			delete it->second;
		}

		mapCategory_.clear();
	}

	void add_category(const string& sCategory, const string& sFilename)
	{
		if(find_category(sCategory)==nullptr) // 見つからなかったら追加
		{
			file_access* pFile = new_ file_access();
			if(pFile->open(PRE_LOG_NAME + sFilename + "_" + sCategory + ".txt", file_access::WRITE_STREAM))
				mapCategory_.insert(category_map::value_type(sCategory, pFile));
			else
				warnln("[logger_files]カテゴリ別ファイルを作成できませんでした: " + pFile->filepath().str());
		}
	}

	file_access* find_category(const string& sCategory)
	{
		category_map::iterator it = mapCategory_.find(sCategory);
		if(it==mapCategory_.end()) return nullptr;
		return it->second;
	}

	//! @brief ログファイルにログを書き込む。書き込む時にカテゴリ表記を追加する
	/*! 有効ログレベルや有効カテゴリの判定はしないので、呼び出し側で制御すること 
	 *  @param[in] sLog ログ文字列
	 *  @param[in] sCategory カテゴリ名  
	 *  @param[in] outflag 出力先フラグ
	 *  @param[in] sFilename 出力ファイル名	 */
	void write_log(const string& sLog, const string& sCategory, uint32_t outflag, const string& sFilename)
	{
		if(bit_test<uint32_t>(outflag, log_output::OUT_FILE_ALL))
			all_.write(sLog.c_str(), sLog.size());

		if(sCategory!="" && bit_test<uint32_t>(outflag,log_output::OUT_FILE_CATEGORY))
		{
			file_access* pCategoryFile = find_category(sCategory);
			if(pCategoryFile==nullptr) // 見つからなかったら追加
			{
				add_category(sCategory, sFilename);
				pCategoryFile = find_category(sCategory);
			}

			if(pCategoryFile)
				pCategoryFile->write(sLog.c_str(), sLog.size());
		}
	}

private:
	//! カテゴリ統合出力
	file_access all_;

	//! カテゴリ別出力
	category_map mapCategory_;
};
