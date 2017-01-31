#pragma once

namespace mana{
namespace input{

class di_driver
{
public:
	di_driver():pDriver_(nullptr){}
	~di_driver(){ fin(); }

	bool init();
	void fin();

	LPDIRECTINPUT8 driver()const{ return pDriver_; }

private:
	LPDIRECTINPUT8 pDriver_;
};

} // namespace input end
} // namespace mana end
