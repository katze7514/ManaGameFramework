#pragma once

namespace mana{
namespace script{

class logger_stream_lib : public xtal::StdStreamLib
{
public:
	xtal::uint_t write_stdout_stream(void* stdout_stream_object, const void* src, xtal::uint_t size);
	xtal::uint_t write_stderr_stream(void* stderr_stream_object, const void* src, xtal::uint_t size);
};

} // namespace script end
} // namespace mana end
