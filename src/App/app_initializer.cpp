#include "../mana_common.h"

#include "system_caps.h"
#include "app_initializer.h"

namespace mana{
namespace app{

app_initializer::app_initializer():hMutex_(INVALID_HANDLE_VALUE)
{
	// メモリーリーク検知
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
}

app_initializer::~app_initializer()
{
	if(hMutex_!=INVALID_HANDLE_VALUE)
	{
		::ReleaseMutex(hMutex_);
		::CloseHandle(hMutex_);
	}
}

bool app_initializer::init(const string& sMultiBootBan, bool bAeroDisEnable)
{
	// 多重起動禁止
	if(!sMultiBootBan.empty())
	{
		// アプリ名をグローバル空間に展開する
		hMutex_ = ::CreateMutex(NULL,TRUE,("Global\\"+sMultiBootBan).c_str());
		if(::GetLastError()==ERROR_ALREADY_EXISTS || hMutex_==NULL)
			return false;
	}

	// WindowsAero切り！
	if(bAeroDisEnable)
	{
		HMODULE hModule;
		void (__stdcall *dwm_enable_comp) (LPCTSTR);

		hModule = LoadLibrary("dwmapi.dll");
		if(hModule)
		{
			dwm_enable_comp = (void (__stdcall *)(LPCTSTR))GetProcAddress(hModule, "DwmEnableComposition");
			if(dwm_enable_comp)
				dwm_enable_comp(FALSE);
		}
	}

	system_caps_check();

	return true;
}

} // namespace app end
} // namespace mana end
