#include "stdafx.h"

namespace boost
{
void throw_exception( std::exception const& e )
{
	logger::fatalln(e.what());
}
}
