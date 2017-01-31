#pragma once

namespace mana{
namespace utility{

// コピー禁止
#define NON_COPIABLE(name) \
	name(const name&); \
	name& operator=(const name&);

// スレッドローカル宣言
#define thread_local __declspec(thread)

//! delete後にnullptrを設定する
#define safe_delete(p) \
{ \
	delete p; \
	p = nullptr; \
}

//! Relesa後にnullptrを設定する
#define safe_release(p) \
{ \
	if(p!=nullptr) \
	{ \
		p->Release(); \
		p = nullptr; \
	} \
}

// いわゆるC-stringからstring_fwにするマクロ
#define string_fw_id(szChar) std::move(string_fw(std::move(string(szChar))))

} // namespace utility end
} // namespace mana end
