#pragma once

#include <string>
#include <vector>
#include "ShaderLibrary.h"

namespace Tsumi::Graphic::ShaderUtil {
std::string ComputeShaderCacheKey(const std::wstring& filePath, const std::string& target, const std::string& entry);
bool SaveCSOToDisk(const std::string& key, IDxcBlob* blob);
Microsoft::WRL::ComPtr<IDxcBlob> LoadCSOFromDisk(const std::string& key);
}