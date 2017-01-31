#include "../mana_common.h"

#include "xtal_lib.h"

using namespace xtal;

namespace mana{
namespace script{

uint_t logger_stream_lib::write_stdout_stream(void* stdout_stream_object, const void* src, uint_t size)
{
	if(size==0) return 0;

	logger::info(reinterpret_cast<const char*>(src), "XTL");
	return size;
}

uint_t logger_stream_lib::write_stderr_stream(void* stderr_stream_object, const void* src, uint_t size)
{
	if(size==0) return 0;

	logger::warn(reinterpret_cast<const char*>(src), "XTL");
	return size;
}

} // namespace mana end
} // namespace script end
