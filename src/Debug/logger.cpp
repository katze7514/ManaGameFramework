#include "../mana_common.h"

#include <boost/filesystem.hpp>

#include "../Concurrent/lock_helper.h"
#include "../File/file.h"

#include "logger_console.h"
#include "logger.h"

// このフラグが立つとLVや出力先変更ができなくなる
//#define MANA_LOGGER_STATE_CHANGE_INVALID

namespace mana{
namespace debug{
namespace logger{

using mana::file::file_access;
using mana::concurrent::spin_flag_lock;

// デフォルト値
#ifdef MANA_DEBUG
const uint32_t	LV_DEFAULT			= log_level::LV_ALL;
const uint32_t	OUT_DEFAULT			= log_output::OUT_VS;
#else
const uint32_t	LV_DEFAULT			= log_level::LV_FATAL|log_level::LV_WARNING|log_level::LV_INFO;
const uint32_t	OUT_DEFAULT			= OUT_FILE_ALL;
#endif


// デフォルトカテゴリ名
#ifdef MANA_DEBUG
const string	CATEGORY_DEFAULT	= "DEF";
#else
const string	CATEGORY_DEFAULT	= "";
#endif

class logger_files;

namespace{

const string	PRE_LOG_NAME = "log_";	//!< ログファイルの接頭辞
const string	ALL_FILENAME = "all";	//!< 全体統一ファイル名

std::atomic_uint32_t	g_nLevel(LV_DEFAULT);			//!< 出力レベル
flat_set<string>		g_setCategory;					//!< カテゴリ一覧
std::atomic_uint32_t	g_nOutPut(OUT_DEFAULT);			//!< 出力先ビット

logger_console			g_LogConsole;					//!< ログコンソール管理インスタンス

logger_files*			g_pLogFiles		= nullptr;		//!< ログファイル管理インスタンス
std::atomic_flag		g_bLogFilesSpin;				//!< ログファイルアクセス用ロックフラグ

// スレッドローカルのパラメタ
//thread_local bool			gtl_bValid			= true;		//!< 出力フラグ
thread_local string*		gtl_psThreadName	= nullptr;	//!< スレッド名。出力ファイル名などに使われる
thread_local logger_files*	gtl_pLogFiles		= nullptr;	//!< ログファイル管理インスタンス

} // namespace end

// logger_filesクラス実装include
#include "logger_files.inl"


////////////////////////////////////
// 制御系
////////////////////////////////////

void init()
{
	if(g_pLogFiles) return; // 初期化済み
	
	set_out_level(LV_DEFAULT);
	set_output(OUT_DEFAULT);
	g_bLogFilesSpin.clear();

	g_pLogFiles = new_ logger_files(ALL_FILENAME);
	add_out_category(CATEGORY_DEFAULT);
}

void fin()
{
	// 終了処理開始時は、ファイル出力フラグを落とす
	remove_output(log_output::OUT_FILE_ALL|log_output::OUT_FILE_THREAD|log_output::OUT_FILE_CATEGORY);
	safe_delete(g_pLogFiles);
	
	remove_output(log_output::OUT_CONSOLE);
}

///////////////////////////
// スレッド別
///////////////////////////

void init_thread(const string& sThreadName)
{
	if(gtl_pLogFiles) return; // 初期化済み

	gtl_psThreadName = new_ string(sThreadName);
	gtl_pLogFiles	 = new_ logger_files(sThreadName);
}

void fin_thread()
{
	safe_delete(gtl_pLogFiles);
	safe_delete(gtl_psThreadName);
}

//////////////////////////
// ログレベル
//////////////////////////

uint32_t get_out_level()
{
	return g_nLevel.load(std::memory_order_acquire);
}

void set_out_level(uint32_t nLevel)
{
#ifndef MANA_LOGGER_STATE_CHANGE_INVALID
	g_nLevel.store(nLevel, std::memory_order_release);
#endif
}

void add_out_level(uint32_t nLevel)
{
#ifndef MANA_LOGGER_STATE_CHANGE_INVALID
	g_nLevel.fetch_or(nLevel, std::memory_order_release);
#endif
}

void remove_out_level(uint32_t nLevel)
{
#ifndef MANA_LOGGER_STATE_CHANGE_INVALID
	g_nLevel.fetch_and(~nLevel, std::memory_order_release);
#endif
}

bool is_out_level(uint32_t nLevel)
{
	uint32_t lv = g_nLevel.load(std::memory_order_acquire);
	return (lv & nLevel)>0;
}

//////////////////////////
// カテゴリレベル
//////////////////////////

void add_out_category(const string& sCategory)
{
#ifndef MANA_LOGGER_STATE_CHANGE_INVALID
	if(gtl_pLogFiles)
		gtl_pLogFiles->add_category(sCategory, *gtl_psThreadName);

	spin_flag_lock lock(g_bLogFilesSpin);
	g_setCategory.insert(sCategory);

	if(g_pLogFiles)
		g_pLogFiles->add_category(sCategory, ALL_FILENAME);
#endif
}

void remove_out_category(const string& sCategory)
{
#ifndef MANA_LOGGER_STATE_CHANGE_INVALID
	spin_flag_lock lock(g_bLogFilesSpin);
	g_setCategory.erase(sCategory);
#endif
}

bool is_out_category(const string& sCategory)
{
	spin_flag_lock lock(g_bLogFilesSpin);
	return g_setCategory.find(sCategory)!=g_setCategory.end();
}

//////////////////////////
// 出力先
//////////////////////////

uint32_t get_output()
{
	return g_nOutPut.load(std::memory_order_acquire);
}

void set_output(uint32_t nOutput)
{
#ifndef MANA_LOGGER_STATE_CHANGE_INVALID
	g_nOutPut.store(std::memory_order_release);

	if(bit_test<uint32_t>(nOutput,log_output::OUT_CONSOLE))
		g_LogConsole.init();
	else
		g_LogConsole.fin();
#endif
}

void add_output(uint32_t nOutput)
{
#ifndef MANA_LOGGER_STATE_CHANGE_INVALID
	g_nOutPut.fetch_or(nOutput, std::memory_order_release);

	if(bit_test<uint32_t>(nOutput,log_output::OUT_CONSOLE))
		g_LogConsole.init();
#endif
}

void remove_output(uint32_t nOutput)
{
#ifndef MANA_LOGGER_STATE_CHANGE_INVALID
	g_nOutPut.fetch_and(~nOutput, std::memory_order_release);

	if(bit_test<uint32_t>(nOutput,log_output::OUT_CONSOLE))
		g_LogConsole.fin();
#endif
}

bool is_output(uint32_t nOutput)
{
	uint32_t out = g_nOutPut.load(std::memory_order_acquire);
	return bit_test<uint32_t>(out,nOutput);
}

////////////////////////////////////
// 出力系
////////////////////////////////////
namespace{
const string sLvStr[]={"【FTL】","【WRN】","【INF】","【TRC】", ""};

inline const string& lv_str(uint32_t nLv)
{
	switch(nLv)
	{
	case LV_FATAL:	return sLvStr[0];
	case LV_WARNING:return sLvStr[1];
	case LV_INFO:	return sLvStr[2];
	case LV_TRACE:	return sLvStr[3];
	default:		return sLvStr[4];
	}
}

inline string log_str(const string& sMes, uint32_t nLevel, const string& sCategory)
{
	// レベルやカテゴリ表示はちょっと保留
	/*if(sCategory!=CATEGORY_DEFAULT)
		return lv_str(nLevel) + "[" + sCategory + "] " + sMes;
	else
		return lv_str(nLevel) + sMes;*/

	return sMes;
}

} // namespace end

void lprint(const string& sMes, uint32_t nLevel, const string& sCategory)
{
	if(is_out_level(nLevel) && is_out_category(sCategory))
	{
		string sLog = log_str(sMes,nLevel,sCategory);

		// 出力先フラグを取得
		uint32_t outflag = get_output();

	#ifndef MANA_LOGGER_STATE_CHANGE_INVALID
		// VS
		if(bit_test<uint32_t>(outflag,log_output::OUT_VS))
			::OutputDebugString(sLog.c_str());

		// Cosole
		if(bit_test<uint32_t>(outflag,log_output::OUT_CONSOLE))
			g_LogConsole.write(sLog, nLevel==LV_WARNING || nLevel==LV_FATAL);
	#endif

		// file
		if(bit_test<uint32_t>(outflag,log_output::OUT_FILE_ALL|log_output::OUT_FILE_CATEGORY))
		{
			// thread_file
			if(bit_test<uint32_t>(outflag,log_output::OUT_FILE_THREAD))
			{
				if(gtl_pLogFiles)
					gtl_pLogFiles->write_log(sLog, sCategory, outflag, *gtl_psThreadName);
			}
			else if(g_pLogFiles)
			{
				spin_flag_lock lock(g_bLogFilesSpin);
				g_pLogFiles->write_log(sLog, sCategory, outflag, ALL_FILENAME);
			}
		}
	}
}

void lprints(const string& sMes, uint32_t nLevel, const string& sCategory)
{
	lprint(sMes+" ", nLevel, sCategory);
}

void lprintln(const string& sMes, uint32_t nLevel, const string& sCategory)
{
	lprint(sMes+"\r\n", nLevel, sCategory);
}

} // namespace logger end
} // namespace debug end
} // namespace mana end
