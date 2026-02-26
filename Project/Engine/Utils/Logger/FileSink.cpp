#include "FileSink.h"

namespace tme::util {

FileSink::FileSink(const std::filesystem::path& path)
	: file_(path, std::ios::out | std::ios::trunc)
{
}

void FileSink::Write(std::string_view msg)
{
	if (!file_.is_open()) return;
	file_ << msg;
	file_.flush();
}

} 
