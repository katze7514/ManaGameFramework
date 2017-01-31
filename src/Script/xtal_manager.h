#pragma once

#include "xtal_code.h"

namespace xtal{
class ChCodeLib;
class StdStreamLib;
class FilesystemLib;
class ThreadLib;
} // namespace xtal end

namespace mana{
class resource_manager;

namespace script{

/*! @brief Xtal全体の管理をするクラス
 *
 *  このクラスのinitを呼んだ以降にxtalが使えるようになる
 *  resource_managerを設定すると、ファイルの読み込みをresource_managerを介して行える
 *
 *  add_code_infoやcode_infoメソッドでxtal_codeインスタンスを取得し、
 *  取得後はxtal_codeを介して操作する */
class xtal_manager
{
public:
	friend xtal_code;

private:
	typedef unordered_map<string_fw, shared_ptr<xtal_code>, string_fw_hash>	code_map; // IDとdataのマップ

public:
	xtal_manager():bInit_(false){}
	~xtal_manager();

public:
	//! 初期化。リソースマネージャーを使う時は引数に渡す
	void init(const shared_ptr<resource_manager>& pResource=nullptr);
	void fin();

	void exec();

public:
	//! コード情報を追加する。追加できなかったら、nullptrが返る
	const shared_ptr<xtal_code>&	create_code(const string_fw& sID);
	void							destroy_code(const string_fw& sID);

	//! コード情報を取得する。存在してなかたらnullptrが返る
	const shared_ptr<xtal_code>&	code(const string_fw& sID);

private:
	//! 機種依存のライブラリ部分の初期化
	void init_lib();
	//! 全体で使うものをバインド。xtal初期化直後に呼ばれる
	void init_bind();
	//! init_bindでバインドしたクラスの後始末(object_orphenを呼ぶこと)
	//! xtal終了の直前に呼ばれる
	void fin_bind();

private:
	bool bInit_;

	code_map						mapCode_;
	shared_ptr<resource_manager>	pResource_;


	xtal::ChCodeLib*		pCodeLib_;
	xtal::StdStreamLib*		pStreamLib_;
	xtal::FilesystemLib*	pFileSystemLib_;
	xtal::ThreadLib*		pThreadLib_;
};

} // namespace script end
} // namespace mana end

/*
	xtal_manager mgr;
	mgr.init(pResMgr)
	
	shared_ptr<xtal_code> pTest = mgr.create_code("TEST");
	pTest->set_filepath("test.xtal");
	if(pTest->compile())
	{
		if(pTest->state()==xtal_code::OK)
		{
			pTest->code()->member(Xid(test))->call();
			check_xtal_result();
		}
	}

	pTest2 = mgr.create_code("TEST_2");
	pTest2->set_resource("XTAL_TEST");
	if(pTest2->compile())
	{
		// リソース呼びの場合、リソースがロードしてくるのを待つので
		// コンパイルが終わって何かしらの結果がでるまで、待つこと。
		while(pTest2->state()==xtal_code::COMPILING);
	}

	// リロードする時は、ファイルを書き換えた状態でreloadメソッドを呼べば良い
	// 関連するスクリプトに通知をしたい時は、関連するスクリプトIDを設定しておくと
	// 対象のスクリプトに定義されているon_reload関数を呼びに行く
	pTest->add_reload_id("TEST_2");
	if(pTest->reload())
	{
		if(pTest->state()==xtal_code::OK)
			pTest->code()->member(Xid())->call();
	}
 */
