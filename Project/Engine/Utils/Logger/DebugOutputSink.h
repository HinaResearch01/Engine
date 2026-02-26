#pragma once

#include "ILogSink.h"

namespace tme::util {

class DebugOutputSink final : public ILogSink {
public:
	void Write(std::string_view msg) override;
};

} 