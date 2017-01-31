#pragma once

namespace mana{
namespace utility{

typedef flyweight<string> string_fw;


//! string_fwをkeyとして使うためのhashクラス
struct string_fw_hash : std::unary_function<string_fw, std::size_t>
{
	std::size_t operator()(const string_fw& elm)const
	{
		return boost::hash<string>()(elm.get());
	}
};

} // namespace utility end
} // namespace mana end
