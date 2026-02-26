#include "DebugOutputSink.h"

#if defined(_WIN32)
#include <Windows.h>
#endif

namespace tme::util {

void DebugOutputSink::Write(std::string_view msg)
{
#if defined(_WIN32)
	OutputDebugStringA(msg.data());
#else
	(void)msg;
#endif
}

} 
