#pragma once

namespace mana{
namespace debug{
namespace logger{

//! ログレベル
enum log_level : uint32_t
{
	LV_FATAL	=1,		//!< 致命的なエラー
	LV_WARNING	=1<<1,	//!< 警告的なエラー
	LV_INFO		=1<<2,	//!< 重要な状態変化などのポイントとなる情報

	// これ以下はRelease時には出力されない
	LV_DEBUG	=1<<3,	//!< デバッグレベル
	LV_TRACE	=1<<4,	//!< トレース。printfデバッグレベル

	LV_ALL		=0xFFFFFFFF, //!< すべてを表す特別なレベル
	LV_NONE		=0,			 //!< レベルなしを表す特別なレベル
};

/*! @brief ログの出力先
 *
 *	・出力先をビットで指定できる。
 *  ・有効なビットの出力先に対してすべてに出力する。
 *　・OUT_FILE_THREAD が指定されるとスレッド別にファイルを作り、全スレッド統合ファイルは作らない
 *  ・FILE系を有効するのは、logger_initを呼んだ後にすること
 *  ・デフォルトは、OUT_VS
 *
 */
enum log_output : uint32_t
{
	OUT_VS				=1,		//!< VSの出力ウインドウ
	OUT_CONSOLE			=1<<1,	//!< コンソール
	OUT_FILE_ALL		=1<<2,	//!< ファイル。全部をまとめた一つのファイルを作る
	OUT_FILE_CATEGORY	=1<<3,	//!< ファイル。カテゴリ別の出力ファイルを作る
	OUT_FILE_THREAD		=1<<4,	//!< ファイル。スレッド別にファイルを出力する

	OUT_NONE			=0,		//!< 出力先無し
};

//! log_outputのデフォルト値を表す定数。OUT_VS
extern const uint32_t OUT_DEFAULT;

//! カテゴリのデフォルト値を表す定数。"DEF"
extern const string CATEGORY_DEFAULT;

//! @defgroup logger_base Logger制御関数
//! @{

//! Logger初期化
extern void init();

//! Logger終了
extern void fin();

//!　@brief Logger初期化スレッド別
//! Loggerを使うスレッドは呼ぶこと
extern void init_thread(const string& sThreadName);

//!　@brief Logger終了スレッド別
//! Loggerを使うスレッドは呼ぶこと
extern void fin_thread();

//! 現在のログレベルを取得する
extern uint32_t get_out_level();

//! @brief 出力するログレベルを設定する
/*! @param[in] output 設定するビット列。この値に置き換わるので注意 */
extern void set_out_level(uint32_t nLevel);

//! 出力するログレベルを追加する
extern void add_out_level(uint32_t nLevel);

//! 出力するログレベルを削除する
extern void remove_out_level(uint32_t nLevel);

//! 出力するログレベルをチェックする
extern bool is_out_level(uint32_t nLevel);

//! 出力するログカテゴリを追加する
extern void add_out_category(const string& sCategory);

//! 出力するログカテゴリを削除する
extern void remove_out_category(const string& sCategory);

//! 指定したカテゴリがあるかをチェック
extern bool is_out_category(const string& sCategory);

//! 現在の出力先を取得する
extern uint32_t get_output();

//! @brief 有効な出力先設定
/*! @param[in] output 設定するビット列。この値に置き換わるので注意 */
extern void set_output(uint32_t nOutput);

//! 有効な出力先設定を追加する
extern void add_output(uint32_t nOutput);

//! 有効な出力先設定を削除する
extern void remove_output(uint32_t nOutput);

//! 有効な出力先をチェックする
extern bool is_output(uint32_t nOutput);

//! @}


/*! @defgroup logger_base_print Logger基本の出力関数
 *
 *  用意はするがこちらは使わず便利関数群の方を使うのを推奨
 */
//! @{

//! logをログレベル(level)/ログカテゴリ(c)で出力する
extern void lprint(const string& sMes, uint32_t nLevel, const string& sCategory=CATEGORY_DEFAULT);

//! logの末尾にスペースを追加して、ログレベル(level)/ログカテゴリ(c)で出力する
extern void lprints(const string& sMes, uint32_t nLevel, const string& sCategory=CATEGORY_DEFAULT);

//! logの末尾に改行を追加して、ログレベル(level)/ログカテゴリ(c)で出力する
extern void lprintln(const string& sMes, uint32_t nLevel, const string& sCategory=CATEGORY_DEFAULT);

//! @}


/*! @defgroup logger_base_print Logger便利関数やクラス
 *
 *  実際のログ出力にはこっちの関数を使う
 */
//! @{

//! コンストラクタでinitを呼び、デストラクタでfinを呼ぶ便利クラス
struct initializer
{
	initializer()
	{
		init();
	}

	~initializer()
	{
		fin();
	}
};

//! コンストラクタでinit_threadを呼び、デストラクタでfin_threadを呼ぶ便利クラス
struct initializer_thread
{
	initializer_thread(const string& sThreadName)
	{
		init_thread(sThreadName);
	}

	~initializer_thread()
	{
		fin_thread();
	}
};

template<typename T>
inline void fatal(const T& mes, const string& sCategory=CATEGORY_DEFAULT){ lprint(to_str(mes), LV_FATAL, sCategory); }
template<typename T>
inline void fatalln(const T& mes, const string& sCategory=CATEGORY_DEFAULT){ lprintln(to_str(mes), LV_FATAL, sCategory); }

inline void fatal(const string& sMes, const string& sCategory=CATEGORY_DEFAULT){ lprint(sMes, LV_FATAL, sCategory); }
inline void fatalln(const string& sMes, const string& sCategory=CATEGORY_DEFAULT){ lprintln(sMes, LV_FATAL, sCategory); }

template<typename T>
inline void warn(const T& mes, const string& sCategory=CATEGORY_DEFAULT){ lprint(to_str(mes), LV_WARNING, sCategory); }
template<typename T>
inline void warnln(const T& mes, const string& sCategory=CATEGORY_DEFAULT){ lprintln(to_str(mes), LV_WARNING, sCategory); }

inline void warn(const string& sMes, const string& sCategory=CATEGORY_DEFAULT){ lprint(sMes, LV_WARNING, sCategory); }
inline void warnln(const string& sMes, const string& sCategory=CATEGORY_DEFAULT){ lprintln(sMes, LV_WARNING, sCategory); }

template<typename T>
inline void info(const T& mes, const string& sCategory=CATEGORY_DEFAULT){ lprint(to_str(mes), LV_INFO, sCategory); }
template<typename T>
inline void infoln(const T& mes, const string& sCategory=CATEGORY_DEFAULT){ lprintln(to_str(mes), LV_INFO, sCategory); }

inline void info(const string& sMes, const string& sCategory=CATEGORY_DEFAULT){ lprint(sMes, LV_INFO, sCategory); }
inline void infoln(const string& sMes, const string& sCategory=CATEGORY_DEFAULT){ lprintln(sMes, LV_INFO, sCategory); }

template<typename T>
inline void debug(const T& mes, const string& sCategory=CATEGORY_DEFAULT){ lprint(to_str(mes), LV_DEBUG, sCategory); }
template<typename T>
inline void debugln(const T& mes, const string& sCategory=CATEGORY_DEFAULT){ lprintln(to_str(mes), LV_DEBUG, sCategory); }

inline void debug(const string& sMes, const string& sCategory=CATEGORY_DEFAULT){ lprint(sMes, LV_DEBUG, sCategory); }
inline void debugln(const string& sMes, const string& sCategory=CATEGORY_DEFAULT){ lprintln(sMes, LV_DEBUG, sCategory); }

template<typename T>
inline void trace(const T& mes, const string& sCategory=CATEGORY_DEFAULT){ lprint(to_str(mes), LV_TRACE, sCategory); }
template<typename T>
inline void traceln(const T& mes, const string& sCategory=CATEGORY_DEFAULT){ lprintln(to_str(mes), LV_TRACE, sCategory); }

inline void trace(const string& sMes, const string& sCategory=CATEGORY_DEFAULT){ lprint(sMes, LV_TRACE, sCategory); }
inline void traceln(const string& sMes, const string& sCategory=CATEGORY_DEFAULT){ lprintln(sMes, LV_TRACE, sCategory); }

//! @}

} // namespace logger end
} // namespace debug end
} // namesoace mana end
