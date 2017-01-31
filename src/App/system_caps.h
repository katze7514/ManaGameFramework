#pragma once

namespace mana{
namespace app{

enum os
{
	ETC=-1,
	WIN_2000_BEFORE, // 2000以前
	WIN_XP,
	WIN_XP_64,
	WIN_SERVER_2003,
	WIN_HOME_SERVER,
	WIN_SERVER_2003_R2,
	WIN_VISTA,
	WIN_SERVER_2008,
	WIN_SERVER_2008_R2,
	WIN_7,
	WIN_SERVER_2012,
	WIN_8,
	WIN_8_1,
	WIN_SERVER_2012_R2,
	WIN_AFTER, // 上記以降のOS
};

//! @defgroup sys_caps システム情報を取得する関数群
//! @{

//! システム情報を収集する。以降の関数を呼ぶ前に呼ぶこと
extern bool system_caps_check();


// 結果を取得

extern enum os		os();
extern string		os_name();
extern uint32_t		os_sp();

extern const string&	cpu_name();
extern uint32_t			cpu_phy_core();
extern uint32_t			cpu_logic_core();

extern uint32_t		main_memory();			//!< メインメモリ容量（MB単位）

extern uint32_t		monitor_width();
extern uint32_t		monitor_height();
extern uint32_t		monitor_color_bit();	//!< プライマリモニタのカラービット数


//! @}

} // namespace app end
} // namespace mana end


/*
using namespace mana;
	app::system_caps_check();
	
	dprintln(app::os());
	dprintln(app::cpu_name());
	dprintln(app::cpu_phy_core());
	dprintln(app::cpu_logic_core());
	dprintln(app::main_memory());
	dprintln(app::monitor_height());
*/
