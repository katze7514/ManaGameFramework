#pragma once

namespace mana{
namespace script{

class xtal_manager;

/*! @brief xtal操作のインターフェイスクラス
 *
 *  xtal_managerから取得した後は基本的にこのクラスを通して操作する。
 *  xtal_codeの元になるファイルやバインド関数を設定後に、compileを呼ぶ
 *  コンパイル呼び出し後は、stateでSUCCESSになったのを確認してから使用する
 *
 *　コンパイルエラーが発生した時は、stateがERRになりエラーハンドラが呼ばれる
 *　エラーハンドラの処理に応じて、すぐに再コンパイルにしいくか一端処理を戻すことを選択できる
 *  処理を戻した場合は、修正した後にもう一度引数にtrueを渡してcompile/reloadを呼ぶ。
 *  エラーハンドラーが設定されてない時は処理を戻す。
 *
 *  リロードしたことを関連する他のコードに伝えたい時は、reloadメソッドを使う。
 *  リロード前と後に、関連コードに"on_reload(CodePtr,bool)"という形の関数があれば
 *  それを呼ぶ。CodePtrはリロードする/したcodeで、boolはリロード直前false、リロード後はtrueで
 *　呼ばれる
 */
class xtal_code
{
public:
	friend xtal_manager;

	//! bind対象のcode、boolはバインドする時はtrue・バインドを外す時はfalseが入ってくる
	typedef function<void(const xtal::CodePtr&, bool)> bind_handler;
	//! コンパイルエラーが発生した時に呼ばれるイベントハンドラ
	//! trueを返すとコンパイルエラーを修正したとして、すぐに再コンパイルしにいく
	//! falseだと一端処理を戻す
	typedef function<bool(const string& sErr)> err_handler;

	enum state
	{
		ERR,		// エラー発生中
		OK,			// Code使用OK
		COMPILING,	// コンパイル中
		NONE,		// 未コンパイル
	};

	enum type
	{
		TYPE_RESOURCE,	// リソースマネージャー
		TYPE_FILEPATH,	// ファイルパス
		TYPE_STR,		// XtalScriptそのものの文字列
		TYPE_NONE,
	};

private:
	enum compile_result
	{
		COMP_ERR,
		COMP_SUCCESS,
		COMP_REREAD,
	};

public:
	xtal_code();
	~xtal_code(){ call_bind(false); pCode_ = xtal::null; }

public:
	enum state				state();
	const string_fw&		id()const{ return sID_; }

	type					type()const{ return eType_;}
	const string&			file()const{ return sFile_; }
	xtal_code&				set_resource(const string& sResID){ eType_=TYPE_RESOURCE; sFile_=sResID; return *this; }
	xtal_code&				set_filepath(const string& sFilePath){ eType_=TYPE_FILEPATH; sFile_=sFilePath; return *this; }
	xtal_code&				set_str(const string& sStr){ eType_=TYPE_STR; sFile_=sStr; return *this; }

	xtal_code&				set_bind_handler(const bind_handler& bind){ bind_=bind; return *this; }
	xtal_code&				set_err_handler(const err_handler& err){ err_=err; return *this;}				

	xtal_code&				add_reload_id(const string_fw& sID);
	xtal_code&				remove_reload_id(const string_fw& sID);

	const xtal::CodePtr&	code(){ return pCode_; }

public:
	//! @param[in] bReread trueだと強制出来にファイルを読み直す
	//! @return falseが返って来たら、コンパイルリクエスト失敗している
	bool					compile(bool bReread=false);
	//! reloadはコンパイルしたことを関連コードに通知する機能がついたコンパイル
	//! またreload時は強制敵にファイルを読み直します
	bool					reload();

private:
	void					set_id(const string_fw& sID){ sID_=sID; }
	void					call_bind(bool b){ if(bind_ && pCode_) bind_(pCode_, b); } // バインドする時はtrue、外す時はfalse 

	bool					compile_resource(bool bReread);
	bool					compile_filepath();
	bool					compile_str();
	compile_result			compile_stream(BYTE* p, uint32_t size);

	void					reload_notify(bool bAfter); // リロード前とリロード後の2回呼ばれる

private:
	string_fw			sID_;
	enum state			eState_;

	enum type			eType_;
	string				sFile_;

	vector<string_fw>	vecReloadID_; // このxtal_dataをリロードした際にリロードを通知する先

	xtal::CodePtr	pCode_;
	bind_handler	bind_;
	err_handler		err_;

	bool			bReload_;

	xtal_manager*	pMgr_;	// 自分自身を管理してるマネージャー
};

//! Xtalの実行結果をチェックする。trueの場合は成功。falseは失敗している
extern bool check_xtal_result();
//! Xtalの実行結果をチェックする。trueの場合は成功。falseは失敗している。
/*! @param[out] sMes エラーメッセージが入る */ 
extern bool check_xtal_result(string& sMes);

} // namespace script end
} // namespace mana end
