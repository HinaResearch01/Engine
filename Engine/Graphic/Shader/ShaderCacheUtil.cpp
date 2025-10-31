#include "ShaderCacheUtil.h"
#include "Utils/Func/UtilFunc.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <functional>
#include <Windows.h>
using namespace Tsumi::Graphic::ShaderUtil;

std::string Tsumi::Graphic::ShaderUtil::ComputeShaderCacheKey(const std::wstring& filePath, const std::string& target, const std::string& entry)
{
    // ファイルの最終更新時刻
    auto ftime = std::filesystem::last_write_time(std::filesystem::path(filePath));
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - std::filesystem::file_time_type::clock::now()
        + std::chrono::system_clock::now()
    );
    auto t = std::chrono::system_clock::to_time_t(sctp);

    std::ostringstream ss;
    // filePath を UTF-8 に変換してハッシュ化
    std::string pathUtf8 = Tsumi::Utils::WstringToUtf8(filePath);
    ss << std::hash<std::string>{}(pathUtf8) << "_" << t << "_" << target << "_" << entry;
    return ss.str();
}

bool Tsumi::Graphic::ShaderUtil::SaveCSOToDisk(const std::string& key, IDxcBlob* blob)
{
    if (!blob) return false;
    std::filesystem::path cacheDir = std::filesystem::temp_directory_path() / "tsumi_shader_cache";
    std::filesystem::create_directories(cacheDir);
    std::filesystem::path out = cacheDir / (key + ".cso");
    std::ofstream ofs(out, std::ios::binary);
    if (!ofs) return false;

    uint64_t size = blob->GetBufferSize();
    ofs.write(reinterpret_cast<const char*>(&size), sizeof(size));
    ofs.write(reinterpret_cast<const char*>(blob->GetBufferPointer()), size);
    return ofs.good();
}

Microsoft::WRL::ComPtr<IDxcBlob> Tsumi::Graphic::ShaderUtil::LoadCSOFromDisk(const std::string& key)
{
    std::filesystem::path cacheDir = std::filesystem::temp_directory_path() / "tsumi_shader_cache";
    std::filesystem::path in = cacheDir / (key + ".cso");
    if (!std::filesystem::exists(in)) return nullptr;

    std::ifstream ifs(in, std::ios::binary);
    if (!ifs) return nullptr;
    uint64_t size = 0;
    ifs.read(reinterpret_cast<char*>(&size), sizeof(size));

    std::vector<char> buf(size);
    ifs.read(buf.data(), size);

    // DXC Blob に変換
    Microsoft::WRL::ComPtr<IDxcLibrary> library;
    Microsoft::WRL::ComPtr<IDxcBlobEncoding> encoding;
    DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));
    library->CreateBlobWithEncodingOnHeapCopy(buf.data(), (UINT32)size, CP_UTF8, &encoding);
    return encoding;
}

