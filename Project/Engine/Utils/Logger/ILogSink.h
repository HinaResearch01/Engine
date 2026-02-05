#pragma once
#include <string_view>

namespace Tsumi::Utils {

class ILogSink {
public:
	virtual ~ILogSink() = default;
	virtual void Write(std::string_view msg) = 0;
};

}
