#pragma once

#include "mouse.h"
#include "di_keyboard.h"
#include "di_joystick.h"

namespace mana{
namespace input{

//! @brief 各種入力デバイスを統合する入力クラス
/*!
 *  例えば、マウスの左ボタン、キーボードのZ、ジョイスティックの0ボタンを
 *  同じ値を返すようにできるようにする
 * 
 *  何のデバイスが、そのデバイスの何を押すと、何が返る、というコマンドを積む
 *  使う時は、何が返る、で設定した値をis_系メソッドに渡すことで判定する
 *
 *  軸判定は、joystickへの透過してるだけ
 *  （マウスをXY軸に割り当てる？）
 *
 *  なお、各デバイスの入力を統合するだけなので、各デバイスの初期化・設定
 *  入力取得・終了は別途行うこと
 */
class unified_input
{
public:
	//! キー統合コマンド
	enum device
	{
		DEV_MOUSE,
		DEV_KEYBOARD,
		DEV_JOYSTICK,
	};

	typedef tuple<enum device, uint32_t>	input_cmd; //! デバイス種別・デバイス入力値のコマンドタプル
	typedef vector<input_cmd>				cmd_vec;
	typedef flat_map<uint32_t, cmd_vec>		cmd_map;

public:
	unified_input():pMouse_(nullptr), pKeyboard_(nullptr), pJoystick_(nullptr){ mapCmd_.reserve(16); }

	// デバイス設定
	void set_mouse(const mouse_ptr& pMouse){ pMouse_ = pMouse; }
	void set_keyboard(const di_keyboard_ptr& pKeyboard){ pKeyboard_ = pKeyboard; }
	void set_joystick(const di_joystick_ptr& pJoystick){ pJoystick_ = pJoystick; }

	//! @brief コマンド設定追加
	/*! @param[in] nUnifiedInput	eDeviceがnDeviceInputがあった場合に返す値
	 *  @param[in] eDevice			デバイス種別
	 *  @param[in] nDeviceInpupt	eDeviceで設定したデバイスの入力値 */
	void add_cmd(uint32_t nUnifiedInput, enum device eDevice, uint32_t nDeviceInput);

	//! @brief コマンド設定削除
	/*! 条件に合うコマンドを削除する 
	 * 
	 *  @param[in] nUnifiedInput	eDeviceがnDeviceInputがあった場合に返す値
	 *  @param[in] eDevice			デバイス種別
	 *  @param[in] nDeviceInpupt	eDeviceで設定したデバイスの入力値 */
	void remove_cmd(uint32_t nUnifiedInput, enum device eDevice, uint32_t nDeviceInput);

	//! @brief コマンド設定クリア
	void clear_cmd();

public:
	// ボタン
	bool is_press(uint32_t nUnifiedInput)const;
	bool is_push(uint32_t nUnifiedInput)const;
	bool is_release(uint32_t nUnifiedInput)const;

	// 軸
	float x_axis()const;
	float y_axis()const;
	float z_axis()const;

	float x_axis_prev()const;
	float y_axis_prev()const;
	float z_axis_prev()const;

	float x_rot()const;
	float y_rot()const;
	float z_rot()const;

	float x_rot_prev()const;
	float y_rot_prev()const;
	float z_rot_prev()const;

	// マウス関係
	int32_t x_mouse()const;
	int32_t y_mouse()const;

	int32_t x_prev_mouse()const;
	int32_t y_prev_mouse()const;

	bool wheel_up()const;
	bool wheel_down()const;

	void guard(bool bGuard);
	bool is_guard();

private:
	// 各種デバイスへのポインター
	mouse_ptr		pMouse_;
	di_keyboard_ptr pKeyboard_;
	di_joystick_ptr pJoystick_;

	//! コマンドマップ
	cmd_map mapCmd_;

	//! 入力ガード
	
};

} // namespace input end
} // namespace mana end

/* 使用例

	enum game_btn : uint32_t
	{
		BTN_ATK,
		BTN_JMP.
	};

	mouse		m;
	di_keyboard	k;
	di_joystick	j;
	// 上は初期化済みとする

	unified_input in;
	in.set_mouse(&m);
	in.set_keyboard(&k);
	in.set_joystick(&j);

	in.add_cmd(BTN_ATK, unified_input::DEV_MOUSE,	 mouse::BTN_L);
	in.add_cmd(BTN_ATK, unified_input::DEV_KEYBOAD,  DIK_Z);
	in.add_cmd(BTN_ATK, unified_input::DEV_JOYSTICK, 0);

	in.add_cmd(BTN_JMP, unified_input::DEV_MOUSE,	 mouse::BTN_R);
	in.add_cmd(BTN_JMP, unified_input::DEV_KEYBOAD,  DIK_X);
	in.add_cmd(BTN_JMP, unified_input::DEV_JOYSTICK, 1);

	// 入力はそれぞれで行う
	m.input();
	k.input();
	j.input();

	if(in.is_push(BTN_ATK))
	{// 攻撃処理
	}

	if(in.is_push(BTN_JMP))
	{// ジャンプ処理
	}
 */
