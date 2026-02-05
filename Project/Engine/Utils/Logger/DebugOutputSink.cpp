#include "DebugOutputSink.h"

#if defined(_WIN32)
#include <Windows.h>
#endif

namespace Tsumi::Utils {

void DebugOutputSink::Write(std::string_view msg)
{
#if defined(_WIN32)
	OutputDebugStringA(msg.data());
#else
	(void)msg;
#endif
}

} 
