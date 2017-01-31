#include "../mana_common.h"

#include "system_caps.h"

namespace{

inline bool is_workstation(OSVERSIONINFOEX& osInfo)
{
	return osInfo.wProductType==VER_NT_WORKSTATION;
}

} // namespace end

namespace mana{
namespace app{

//! @brief アプリケーションの実行環境の情報を集約するクラス
/*! OSやCPU、メモリ量などを扱う。サウンドやGPUについては対応するAdapterの方で集約する */
class system_caps
{
public:
	system_caps();

	//! @brief 環境情報を収集する
	/*! @retval true  すべてのチェックが成功した
	 *  @retval false どこかのチェックが失敗している  */
	bool check();

	//! @defgroup sys_caps_access 各種アクセサ
	//! @{

	enum os			os()const{ return eOS_; }
	uint32_t		os_sp()const{ return nOsSp_; }

	const string&	cpu_name()const{ return sCpuName_; }
	uint32_t		cpu_phy_core()const{ return nPhyicalCore_; }
	uint32_t		cpu_logic_core()const{ return nLogicCore_; }

	uint32_t		main_memory()const{ return nMainMemory_; }

	uint32_t		monitor_width()const{ return nMonitorWidth_; }
	uint32_t		monitor_height()const{ return nMonitorHeight_; }
	uint32_t		monitor_color_bit()const{ return nMonitorColorBit_; }

	//! @}

private:
	bool check_os();
	bool check_cpu();
	bool check_memory();
	bool check_monitor();

private:
	// OS
	enum os		eOS_;
	uint32_t	nOsSp_;		//!< サービスパック
	uint32_t	nOsBit_;	//!< 32bit / 64bit

	// CPU
	string		sCpuName_;
	uint32_t	nPhyicalCore_;	//!< 物理コア数
	uint32_t	nLogicCore_;	//!< 論理コア数

	// メモリ
	uint32_t	nMainMemory_; //!< メモリ容量。MB単位

	// ディスプレイ
	uint32_t	nMonitorWidth_;		//!< メインモニタの幅
	uint32_t	nMonitorHeight_;	//!< メインモニタの高さ
	uint32_t	nMonitorColorBit_;		//!< メインモニタの色
};


system_caps::system_caps():eOS_(ETC),nOsSp_(0),nOsBit_(0),
						 nPhyicalCore_(0),nLogicCore_(0),
						 nMainMemory_(0),
						 nMonitorWidth_(0),nMonitorHeight_(0),nMonitorColorBit_(0){}

bool system_caps::check()
{
	bool bSuccess = true;

	bSuccess &= check_os();
	bSuccess &= check_cpu();
	bSuccess &= check_memory();
	bSuccess &= check_monitor();
	
	return bSuccess;
}

bool system_caps::check_os()
{
	// OS bit
	SYSTEM_INFO sysInfo;
	::GetNativeSystemInfo(&sysInfo);
	if(sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
		nOsBit_ = 64;
	else if(sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
		nOsBit_ = 32;

	// OS OSSP
#pragma warning(disable:4996)
	OSVERSIONINFOEX osInfo;
	osInfo.dwOSVersionInfoSize = sizeof(osInfo);
	if(FAILED(::GetVersionEx(reinterpret_cast<LPOSVERSIONINFO>(&osInfo))))
	{
		logger::warnln("[system_caps]OSのバージョンを取得できませんでした。");
		return false;
	}
#pragma warning(default:4996)
	
	// OS
	switch(osInfo.dwMajorVersion)
	{
	case 6:
		switch(osInfo.dwMinorVersion)
		{
		case 3:
			if(is_workstation(osInfo))
				eOS_ = WIN_8_1;
			else
				eOS_ = WIN_SERVER_2012_R2;
		break;

		case 2:
			if(is_workstation(osInfo))
				eOS_ = WIN_8;
			else
				eOS_ = WIN_SERVER_2012;
		break;

		case 1:
			if(is_workstation(osInfo))
				eOS_ = WIN_7;
			else
				eOS_ = WIN_SERVER_2008_R2;
		break;

		case 0:
			if(is_workstation(osInfo))
				eOS_ = WIN_VISTA;
			else
				eOS_ = WIN_SERVER_2008;
		break;

		default:
			// 3以上はWin8以上のOS
			eOS_ = WIN_AFTER;
		break;
		}
	break;

	case 5:
		switch(osInfo.dwMinorVersion)
		{
		case 2:
			if(::GetSystemMetrics(SM_SERVERR2)!=0)
				eOS_ = WIN_SERVER_2003_R2;
			else if(osInfo.wSuiteMask & VER_SUITE_WH_SERVER)
				eOS_ = WIN_HOME_SERVER;
			else if(is_workstation(osInfo) && nOsBit_==64)
				eOS_ = WIN_XP_64;	
			else
				eOS_ = WIN_SERVER_2003;
		break;

		case 1:
			eOS_ = WIN_XP;
		break;

		case 0:
			eOS_ = WIN_2000_BEFORE;
		break;
		}
	break;

	default:
		if(osInfo.dwMajorVersion>6)
			eOS_ = WIN_AFTER;
		else
			eOS_ = WIN_2000_BEFORE;
	break;
	}

	// OSSP
	nOsSp_ = osInfo.wServicePackMajor;

	return true;
}

bool system_caps::check_cpu()
{
	// CPU名を取得する
	int anCpuInfo[4]={-1};

	// x86なら、cpuid拡張領域 0x80000002～0x80000004 で取得できる
	__cpuid(anCpuInfo, 0x80000000);
	if(anCpuInfo[0]>=0x80000004)
	{
		char szCpu[17];

		__cpuid(anCpuInfo, 0x80000002);
		::memset(szCpu, 0, sizeof(szCpu));
		*(reinterpret_cast<int*>(&szCpu[0]))	= anCpuInfo[0];
		*(reinterpret_cast<int*>(&szCpu[4]))	= anCpuInfo[1];
		*(reinterpret_cast<int*>(&szCpu[8]))	= anCpuInfo[2];
		*(reinterpret_cast<int*>(&szCpu[12]))	= anCpuInfo[3];
		sCpuName_ = szCpu;

		__cpuid(anCpuInfo, 0x80000003);
		::memset(szCpu, 0, sizeof(szCpu));
		*(reinterpret_cast<int*>(&szCpu[0]))	= anCpuInfo[0];
		*(reinterpret_cast<int*>(&szCpu[4]))	= anCpuInfo[1];
		*(reinterpret_cast<int*>(&szCpu[8]))	= anCpuInfo[2];
		*(reinterpret_cast<int*>(&szCpu[12]))	= anCpuInfo[3];
		sCpuName_ += szCpu;

		__cpuid(anCpuInfo, 0x80000004);
		::memset(szCpu, 0, sizeof(szCpu));
		*(reinterpret_cast<int*>(&szCpu[0]))	= anCpuInfo[0];
		*(reinterpret_cast<int*>(&szCpu[4]))	= anCpuInfo[1];
		*(reinterpret_cast<int*>(&szCpu[8]))	= anCpuInfo[2];
		*(reinterpret_cast<int*>(&szCpu[12]))	= anCpuInfo[3];
		sCpuName_ += szCpu;
		boost::trim_left(sCpuName_);
	}
	else
	{
		sCpuName_ = "CPU名を取得できませんでした。";
		logger::warnln("[system_caps]"+sCpuName_);
	}

	// コア数を取得
	typedef BOOL (WINAPI *PGLPI)(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, PDWORD);
	PGLPI GetLogicProInfo = reinterpret_cast<PGLPI>(::GetProcAddress(::GetModuleHandle(TEXT("kernel32")),"GetLogicalProcessorInformation"));

	if(GetLogicProInfo==nullptr)
	{
		logger::warnln("[system_caps]GetLogicalProcessorInformation未対応OSです");
		return false;
	}


	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION pLpInfo = nullptr;
	DWORD nLength = 0;
	// 一度目の呼び出しは必要な構造体サイズを取得するために呼ぶ
	GetLogicProInfo(pLpInfo, &nLength);
	if(::GetLastError()==ERROR_INSUFFICIENT_BUFFER) // 必ず失敗するはず
	{
		
		pLpInfo = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION>(::malloc(nLength));
		if(pLpInfo==NULL)
		{
			logger::warnln("[system_caps]コア数取得のためのメモリ確保ができませんでした。");
			return false;
		}

		// 改めてCPU情報を取得する
		if(GetLogicProInfo(pLpInfo, &nLength))
		{
			for(uint32_t i=0; i<nLength/sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION); ++i)
			{
				// 物理コアの情報
				if(pLpInfo[i].Relationship==RelationProcessorCore)
				{
					++nPhyicalCore_;

					// Flagが立ってたら論理コアが含まれている
					if(pLpInfo[i].ProcessorCore.Flags==1)
						nLogicCore_ += bit_count(pLpInfo[i].ProcessorMask);
					else // なかったら物理コアと一緒(HT機能が無いCPUなど)
						++nLogicCore_;
				}
			}
		}
		::free(pLpInfo);
	}

	return true;
}

bool system_caps::check_memory()
{
	// メモリ情報取得
	MEMORYSTATUSEX status;
	status.dwLength = sizeof(MEMORYSTATUSEX);
	if(::GlobalMemoryStatusEx(&status)==FALSE)
	{
		logger::warnln("[system_caps]メモリ情報が取得できませんでした。");
		return false;
	}

	// MBにする
	nMainMemory_ = static_cast<uint32_t>(ceill(static_cast<long double>(status.ullTotalPhys) / 1024.0 / 1024.0));

	return true;
}

bool system_caps::check_monitor()
{
	// プライマリモニタの幅と高さ
	nMonitorWidth_  = ::GetSystemMetrics(SM_CXSCREEN);
	nMonitorHeight_ = ::GetSystemMetrics(SM_CYSCREEN);

	// プライマリモニタの色数
	// デスクトップの色数として取得する
	HWND hWnd = ::GetDesktopWindow();
	HDC hDc = ::GetDC(hWnd);
	nMonitorColorBit_ = ::GetDeviceCaps(hDc, BITSPIXEL);
	::ReleaseDC(hWnd, hDc);

	//// モニタの情報をちゃんと収集する
	//DISPLAY_DEVICE disdev;
	//disdev.cb = sizeof(DISPLAY_DEVICE);
	//DWORD nDevNum=0;
	//while(true)
	//{
	//	if(!::EnumDisplayDevices(NULL, nDevNum++, &disdev, 0))
	//		break;

	//	dprints(disdev.DeviceString);
	//	dprintln(disdev.DeviceName);

	//	DEVMODE mode;
	//	mode.dmSize = sizeof(DEVMODE);
	//	int i=0;
	//	while(true)
	//	{
	//		if(!::EnumDisplaySettings(disdev.DeviceName,i++,&mode)) break;
	//		dprints(mode.dmDriverVersion);
	//		dprints(mode.dmSpecVersion);
	//		dprints(mode.dmBitsPerPel);
	//		dprints(mode.dmDisplayFrequency);
	//		dprints(mode.dmPelsWidth);
	//		dprintln(mode.dmPelsHeight);
	//	}
	//}


	return nMonitorWidth_!=0 && nMonitorHeight_!=0 && nMonitorColorBit_!=0;
}


namespace{
// システム情報インスタンス実体
system_caps caps;
} // namespace end

bool system_caps_check(){ return caps.check(); }

enum os	os(){ return caps.os(); }
string	os_name()
{ 
	switch(caps.os())
	{
	case WIN_2000_BEFORE:	return "Win2000 以前";
	case WIN_XP:			return "WinXP";
	case WIN_XP_64:			return "WinXP 64bit";
	case WIN_SERVER_2003:	return "WinServer2003";
	case WIN_HOME_SERVER:	return "WinHome Server";
	case WIN_SERVER_2003_R2:return "WinServer2003 R2";
	case WIN_VISTA:			return "WinVista";
	case WIN_SERVER_2008:	return "WinServer2008";
	case WIN_SERVER_2008_R2:return "WinServer2008 R2";
	case WIN_7:				return "Win7";
	case WIN_SERVER_2012:	return "WinServer2012";
	case WIN_8:				return "Win8";
	case WIN_8_1:			return "Win8.1";
	case WIN_SERVER_2012_R2:return "WinServer2012 R2";
	case WIN_AFTER:			return "Win8.1 以後";
	default:				return "判定不能";
	}
}
uint32_t		os_sp(){ return caps.os_sp(); }

const string&	cpu_name(){ return caps.cpu_name(); }
uint32_t		cpu_phy_core(){ return caps.cpu_phy_core(); }
uint32_t		cpu_logic_core(){ return caps.cpu_logic_core(); }

uint32_t		main_memory(){ return caps.main_memory(); }

uint32_t		monitor_width(){ return caps.monitor_width(); }
uint32_t		monitor_height(){ return caps.monitor_height(); }
uint32_t		monitor_color_bit(){ return caps.monitor_color_bit(); }

} // namespace app end
} // namespace mana end
