#pragma once

#include "Actor/actor_context.h"
#include "Actor/actor_machine.h"

namespace mana{
class actor_context;

namespace input{
class di_keyboard;
} // namespace input end

namespace graphic{
class draw_builder;
} // namespace graphic end

namespace script{
class xtal_code;
} // namespace script end

} // namespace mana end

class check_scene_factory : public mana::actor_factory
{
public:
	mana::actor* create_actor(uint32_t nID)override;
};

/////////////////////////

class check_context : public mana::actor_context
{
public:
	const shared_ptr<mana::input::di_keyboard>& keyboard(){ return pKeyboard_; }
	void										set_keyboard(const shared_ptr<mana::input::di_keyboard>& pKeyboard){ pKeyboard_=pKeyboard; }

private:
	shared_ptr<mana::input::di_keyboard> pKeyboard_;
};

/////////////////////////

class check_scene : public mana::actor
{
private:
	enum check_state : uint32_t
	{
		INIT, 
		LOAD_FILE,
		LOAD_SOUND,
		LOAD_DRAW,
		EXEC,
		END,
	};

protected:
	bool init_self(mana::actor_context& ctx)override;
	void reset_self(mana::actor_context& ctx)override;
	void exec_self(mana::actor_context& ctx)override;

private:
	shared_ptr<mana::graphic::draw_builder> pDrawBuilder_;
};

/////////////////////////

class check_xtal_scene : public mana::actor
{
private:
	enum check_state : uint32_t
	{
		REQUEST,
		LOAD_WAIT,
		EXEC,
		END,
	};

protected:
	bool init_self(mana::actor_context& ctx)override;
	void exec_self(mana::actor_context& ctx)override;

	void exec_simple(check_context& ctx);
	void exec_reload(check_context& ctx);

private:
	shared_ptr<mana::script::xtal_code> pXtalCode_;
};
