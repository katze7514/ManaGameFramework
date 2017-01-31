#pragma once

namespace mana{
namespace debug{
namespace logger{

//! コンソール管理クラス
class logger_console
{
public:
	void init()
	{
		::AllocConsole();
		hStdOut_ = ::GetStdHandle(STD_OUTPUT_HANDLE);
		hErrOut_ = ::GetStdHandle(STD_ERROR_HANDLE);
	}

	void fin()
	{
		::FreeConsole();
	}

	void write(const string& sLog, bool bErr=false)
	{
		DWORD nWords;

		if(bErr)
			::WriteConsole(hErrOut_, sLog.c_str(), sLog.size(), &nWords, NULL);
		else
			::WriteConsole(hStdOut_, sLog.c_str(), sLog.size(), &nWords, NULL);
	}

private:
	HANDLE hStdOut_;
	HANDLE hErrOut_;
};

} // namespace logger end
} // namespace debug end
} // namespace mana end
