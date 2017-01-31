#include "mana_common.h"

#include "File/file.h"
#include "App/app_initializer.h"
#include "App/system_caps.h"
#include "check_window.h"

using namespace mana;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	logger::initializer log_init;

	bool bAeroCut = true;

#ifdef MANA_DEBUG
	bAeroCut = false;
#endif

	app::app_initializer ini;
	if(ini.init("ManaFrameworkCheck", bAeroCut))
	{
		logger::initializer_thread log_init_thread("main");

		if(app::os()<=app::os::WIN_XP)
		{
			::MessageBox(NULL, "未対応OSです。Windows7以降のOSで起動して下さい。", "ManaFraeworkCheck", MB_OK|MB_ICONWARNING);
			return 0;
		}


		check_window win;
		win.set_render_size(800,600);
		win.create("ManaFrameworkCheck",false,win.render_width(),win.render_height());
		win.wait_fin();
	}
	else
	{
		::MessageBox(NULL, "多重起動禁止", "ManaFrameworkCheck", MB_OK|MB_ICONWARNING);
	}

	return 0;
}
