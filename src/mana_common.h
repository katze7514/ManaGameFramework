// ManaFramework共通ヘッダ
#pragma once

#define _SCL_SECURE_NO_WARNINGS

// Windowsライブラリ
#include <Windows.h>

#ifndef MANA_DIRECTX_NOT_USE // ツールを作る時など、DirectXが必要無い時に定義する
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h> 

#include <d3d9.h>
#include <d3dx9.h>

#include <dsound.h>
#include <mmreg.h>
#endif // MANA_DIRECTX_NOT_USE


// 標準ライブラリ
#include <memory>
#include <new>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <process.h>

#include <random>
#define _ENABLE_ATOMIC_ALIGNMENT_FIX
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <sstream>

#include <algorithm>

// 外部ライブラリ

// boost
#define BOOST_RESULT_OF_USE_DECLTYPE
#define BOOST_NO_EXCEPTIONS
#define BOOST_AUTO_LINK_TAGGED

#pragma warning(disable:4913 4127)
#include <boost/system/error_code.hpp>

#include <boost/scope_exit.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/cxx11/any_of.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/clamp.hpp>
#include <boost/math/constants/constants.hpp>

#pragma warning(disable:4702)
#include <boost/algorithm/hex.hpp>
#pragma warning(default:4702)

#include <boost/function.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/optional.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/variant.hpp>
#include <boost/flyweight.hpp>

#include <boost/array.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/unordered_map.hpp>

#pragma warning(disable:4100)
//#include <boost/container/string.hpp>
#include <boost/container/list.hpp>
#include <boost/container/vector.hpp>
#include <boost/container/map.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/set.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/container/deque.hpp>
#include <boost/container/slist.hpp>

#include <boost/ptr_container/ptr_vector.hpp>
#pragma warning(default:4100)

#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>

#pragma warning(disable:4324)
#include <boost/lockfree/queue.hpp>
#pragma warning(default:4324)

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <boost/xpressive/xpressive_static.hpp>

#pragma warning(default:4913 4127)

using boost::lexical_cast;
using boost::algorithm::any_of_equal;
using boost::algorithm::clamp;

using boost::function;
using boost::logic::tribool;
using boost::logic::indeterminate;
using boost::optional;
using boost::scoped_ptr;
using boost::scoped_array;
using boost::shared_ptr;
using boost::shared_array;
using boost::weak_ptr;
using boost::make_shared;
using boost::variant;
using boost::flyweight;

using boost::array;
using boost::circular_buffer;
using boost::unordered_map;

//using boost::container::string;
using std::string;
using boost::container::list;
using boost::container::vector;
using boost::container::map;
using boost::container::flat_map;
using boost::container::set;
using boost::container::flat_set;
using boost::container::deque;
using boost::container::slist;

using boost::ptr_vector;

using boost::tuple;
using boost::make_tuple;
using boost::tie;

namespace lockfree	= boost::lockfree;
namespace p_tree	= boost::property_tree;

#ifndef MANA_XTAL_NOT_USE // ツールを作る時など、Xtalが必要無い時に定義する
// xtal
#include <xtal.h>
#include <xtal_macro.h>
#endif // MANA_XTAL_NOT_USE

// ManaFrameworkヘッダ
#include "Utility/util_macro.h"
#include "Utility/util_fun.h"
#include "Utility/string_fw.h"
#include "Debug/logger.h"
#include "Timer/elapsed_timer.h"

using mana::utility::string_fw;
using mana::utility::string_fw_hash;
using mana::utility::to_str;
using mana::utility::to_str_s;
using mana::utility::bit_count;
using mana::utility::bit_test;
using mana::utility::bit_add;
using mana::utility::bit_remove;
using mana::utility::check_hresult;
using mana::utility::hex_to_uint32;

namespace logger = mana::debug::logger;

#if _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define new_ ::new(_NORMAL_BLOCK, __FILE__, __LINE__)
#else
#define new_ ::new
#endif
