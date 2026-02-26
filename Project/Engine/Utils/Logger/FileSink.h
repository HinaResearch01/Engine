#pragma once

#include "ILogSink.h"
#include <fstream>
#include <filesystem>

namespace tme::util {

class FileSink final : public ILogSink {
public:
	explicit FileSink(const std::filesystem::path& path);
	void Write(std::string_view msg) override;

private:
	std::ofstream file_;
};

}