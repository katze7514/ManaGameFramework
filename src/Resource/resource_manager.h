#pragma once

namespace mana{

namespace concurrent{
class worker;
class future;
} // namespace concurrent end

class resource_file;

/*! @brief リソースマネージャ
 *
 *  指定したファイルを非同期に読み込む。
 *  ファイルは、ファイルパス(exeファイルからの相対パス)、IDで指定する。
 *
 *  add_resource_info/load_resource_infoでリソース情報を登録した後にrequestを出す。
 *  requestを出した後に、get_resourceでデータを取得する。
 *  optionalがtrueを示せばロード終了している
 */
class resource_manager
{
public:
	enum request_result
	{
		FAIL,		//!< リクエスト失敗
		LOADING,	//!< ロード中
		SUCCESS,	//!< リクエスト成功
		EXIST,		//!< キャッシュに存在している
	};

private:
	typedef tuple<weak_ptr<concurrent::future>, shared_ptr<resource_file>>	resource_type; // 成功/失敗 worker実行のチェック 

public:
	resource_manager();

public:
	//! リソースマネージャーで使うスレッド数を渡して初期化
	void				init(uint32_t nThreadNum=1);
	//! kick済みworkerを渡して初期化
	void				init(const shared_ptr<concurrent::worker>& pWorker);

public:
	//! リソース情報を追加する
	bool				add_resource_info(const string_fw& sID, const string& sFile, bool bTextMode=false);
	//! リソース情報を削除する
	void				remove_resource_info(const string_fw& sID);
	//! リソースロードリクエスト。ID指定
	request_result		request(const string_fw& sID);

	//! リソース取得
	/*! @return ロードが終わって無いファイルを指定した場合、無効な値が返ってくる */
	optional<shared_ptr<resource_file>>	resource(const string_fw& sID);
	//! リソースキャッシュ解放。ロード終了後のリソースにのみ有効
	void								release_cache(const string_fw& sID);

	//! IDからファイル名を取得する
	const string&		filename(const string_fw& sID);

public:
	//! IDとファイル名のマップファイルを読み込み登録する
	bool				load_resource_info(const string& sFilePath, bool bFile=true);

private:
	unordered_map<string_fw, tuple<string,bool>, string_fw_hash>	mapID_;			//!< IDとファイル名(テキストモード)のマップ
	unordered_map<string_fw, resource_type, string_fw_hash>			cacheResouce_;	//!< IDとファイル実体のキャッシュ

	shared_ptr<concurrent::worker>				pWorker_;
};

} // namespace mana end
