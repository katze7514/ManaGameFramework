#include "../mana_common.h"

#include "unified_input.h"

namespace mana{
namespace input{

///////////////////////////////
// コマンド
///////////////////////////////
void unified_input::add_cmd(uint32_t nUnifiedInput, enum device eDevice, uint32_t nDeviceInput)
{
	cmd_map::iterator it = mapCmd_.find(nUnifiedInput);
	if(it!=mapCmd_.end())
	{
		for(auto& cmd : it->second)
		{
			if(cmd.get<0>()==eDevice && cmd.get<1>()==nDeviceInput)
			{// すでに存在してるので終了
				return;
			}
		}

		// なければ、追加
		it->second.emplace_back(make_tuple(eDevice,nDeviceInput));
	}

	// nUnifyInputから登録
	cmd_vec v;
	v.reserve(3); // 3デバイスから1つずつが基本のはず
	v.emplace_back(make_tuple(eDevice, nDeviceInput));
	mapCmd_.insert(cmd_map::value_type(nUnifiedInput,v));
}

void unified_input::remove_cmd(uint32_t nUnifiedInput, enum device eDevice, uint32_t nDeviceInput)
{
	cmd_map::iterator it = mapCmd_.find(nUnifiedInput);
	if(it!=mapCmd_.end())
	{
		cmd_vec::const_iterator v;
		for(v=it->second.cbegin(); v!=it->second.cend(); ++v)
		{
			if(v->get<0>()==eDevice && v->get<1>()==nDeviceInput)
			{// 見つけたので削除！
				it->second.erase(v);
				if(it->second.size()==0)
				{// cmd_vecサイズが0になったので、mapからも削除
					mapCmd_.erase(it);
				}
				return;
			}
		}
	}
}

void unified_input::clear_cmd()
{
	mapCmd_.clear();
}


///////////////////////////////
// ボタン
///////////////////////////////
bool unified_input::is_press(uint32_t nUnifiedInput)const
{
	cmd_map::const_iterator it = mapCmd_.find(nUnifiedInput);
	if(it!=mapCmd_.end())
	{
		for(auto& cmd : it->second)
		{
			switch(cmd.get<0>())
			{
			case DEV_MOUSE:
				if(pMouse_ && pMouse_->is_press(cmd.get<1>())) return true;
			break;

			case DEV_KEYBOARD:
				if(pKeyboard_ && pKeyboard_->is_press(cmd.get<1>())) return true;
			break;

			case DEV_JOYSTICK:
				if(pJoystick_ && pJoystick_->is_press(cmd.get<1>())) return true;
			break;
			}
		}
	}
	return false;
}

bool unified_input::is_push(uint32_t nUnifiedInput)const
{
	cmd_map::const_iterator it = mapCmd_.find(nUnifiedInput);
	if(it!=mapCmd_.end())
	{
		for(auto& cmd : it->second)
		{
			switch(cmd.get<0>())
			{
			case DEV_MOUSE:
				if(pMouse_ && pMouse_->is_push(cmd.get<1>())) return true;
			break;

			case DEV_KEYBOARD:
				if(pKeyboard_ && pKeyboard_->is_push(cmd.get<1>())) return true;
			break;

			case DEV_JOYSTICK:
				if(pJoystick_ && pJoystick_->is_push(cmd.get<1>())) return true;
			break;
			}
		}
	}
	return false;
}

bool unified_input::is_release(uint32_t nUnifiedInput)const
{
	cmd_map::const_iterator it = mapCmd_.find(nUnifiedInput);
	if(it!=mapCmd_.end())
	{
		for(auto& cmd : it->second)
		{
			switch(cmd.get<0>())
			{
			case DEV_MOUSE:
				if(pMouse_ && pMouse_->is_release(cmd.get<1>())) return true;
			break;

			case DEV_KEYBOARD:
				if(pKeyboard_ && pKeyboard_->is_release(cmd.get<1>())) return true;
			break;

			case DEV_JOYSTICK:
				if(pJoystick_ && pJoystick_->is_release(cmd.get<1>())) return true;
			break;
			}
		}
	}
	return false;
}

///////////////////////////////
// 軸
///////////////////////////////
float unified_input::x_axis()const
{
	if(pJoystick_) return pJoystick_->x();
	return 0.f;
}

float unified_input::y_axis()const
{
	if(pJoystick_) return pJoystick_->y();
	return 0.f;
}

float unified_input::z_axis()const
{
	if(pJoystick_) return pJoystick_->z();
	return 0.f;
}

float unified_input::x_axis_prev()const
{
	if(pJoystick_) return pJoystick_->x_prev();
	return 0.f;
}

float unified_input::y_axis_prev()const
{
	if(pJoystick_) return pJoystick_->x_prev();
	return 0.f;
}

float unified_input::z_axis_prev()const
{
	if(pJoystick_) return pJoystick_->z_prev();
	return 0.f;
}

float unified_input::x_rot()const
{
	if(pJoystick_) return pJoystick_->x_rot();
	return 0.f;
}

float unified_input::y_rot()const
{
	if(pJoystick_) return pJoystick_->y_rot();
	return 0.f;
}

float unified_input::z_rot()const
{
	if(pJoystick_) return pJoystick_->z_rot();
	return 0.f;
}

float unified_input::x_rot_prev()const
{
	if(pJoystick_) return pJoystick_->x_rot_prev();
	return 0.f;
}

float unified_input::y_rot_prev()const
{
	if(pJoystick_) return pJoystick_->y_rot_prev();
	return 0.f;
}

float unified_input::z_rot_prev()const
{
	if(pJoystick_) return pJoystick_->z_rot_prev();
	return 0.f;
}

//////////////////////////
// マウス関係
//////////////////////////
int32_t unified_input::x_mouse()const
{
	if(pMouse_) return pMouse_->x();
	return 0;
}

int32_t unified_input::y_mouse()const
{
	if(pMouse_) return pMouse_->y();
	return 0;
}

int32_t unified_input::x_prev_mouse()const
{
	if(pMouse_) return pMouse_->x_prev();
	return 0;
}

int32_t unified_input::y_prev_mouse()const
{
	if(pMouse_) return pMouse_->y_prev();
	return 0;
}

bool unified_input::wheel_up()const
{
	if(pMouse_) return pMouse_->wheel_up();
	return false;
}

bool unified_input::wheel_down()const
{
	if(pMouse_) return pMouse_->wheel_down();
	return false;
}

} // namespace input end
} // namespace mana end
