#pragma once
#include <string_view>

namespace tme::util {

class ILogSink {
public:
	virtual ~ILogSink() = default;
	virtual void Write(std::string_view msg) = 0;
};

}
