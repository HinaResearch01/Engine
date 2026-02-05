#pragma once

#include "ILogSink.h"

namespace Tsumi::Utils {

class DebugOutputSink final : public ILogSink {
public:
	void Write(std::string_view msg) override;
};

} 