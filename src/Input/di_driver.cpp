#include "../mana_common.h"

#include "di_driver.h"

namespace mana{
namespace input{

bool di_driver::init()
{
	if(pDriver_)
	{
		logger::infoln("[di_driver]初期化済みです。");
		return true;
	}

	HRESULT r = DirectInput8Create(::GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, 
								   reinterpret_cast<LPVOID*>(&pDriver_), NULL);

	if(r!=DI_OK)
	{
		logger::fatalln("[di_driver]DirectInputの初期化に失敗しました。: " + to_str(r));
		return false;
	}

	logger::infoln("[di_driver]DirectInput初期化しました。");
	return true;
}

void di_driver::fin()
{
	safe_release(pDriver_);
}

} // namespace input end
} // namespace mana end
