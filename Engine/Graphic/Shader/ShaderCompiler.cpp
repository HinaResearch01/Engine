#include "ShaderCompiler.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <filesystem>
#include <format>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <dxcapi.h>
#include <dxcapi.h>
#include <wrl.h>
#pragma comment(lib, "dxcompiler.lib")

using namespace Tsumi::Graphic;
using Microsoft::WRL::ComPtr;

static std::string ToUtf8(const std::wstring& w) {
	if (w.empty()) return {};
	int size = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
	std::string s(size, 0);
	WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), s.data(), size, nullptr, nullptr);
	return s;
}

ShaderBlob ShaderCompiler::CompileFromFile(const std::string& path, const ShaderCompileOptions& opt)
{
	std::string src = ReadFileToString(path);
	return CompileFromSource(src, path, opt);
}

ShaderBlob ShaderCompiler::CompileFromSource(const std::string& source, const std::string& virtualPath, const ShaderCompileOptions& opt)
{
    std::string key = ComputeCacheKey(source, opt);
    std::filesystem::path cacheDir = std::filesystem::temp_directory_path() / "tsumi_shader_cache";
    std::filesystem::create_directories(cacheDir);
    std::filesystem::path cacheFile = cacheDir / (key + ".cso");

    if (std::filesystem::exists(cacheFile)) {
        auto maybe = LoadBlobFromDisk(cacheFile.string());
        if (maybe.has_value()) {
            ShaderBlob b = maybe.value();
            b.sourcePath = virtualPath;
            return b;
        }
    }

    ComPtr<IDxcCompiler> compiler;
    ComPtr<IDxcUtils> utils;
    ComPtr<IDxcLibrary> library;

    HRESULT hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
    if (FAILED(hr)) throw std::runtime_error("DxcCreateInstance(CLSID_DxcCompiler) failed");
    hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
    if (FAILED(hr)) throw std::runtime_error("DxcCreateInstance(CLSID_DxcUtils) failed");
    hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));
    if (FAILED(hr)) throw std::runtime_error("DxcCreateInstance(CLSID_DxcLibrary) failed");

    ComPtr<IDxcBlobEncoding> sourceBlob;
    hr = library->CreateBlobWithEncodingOnHeapCopy(source.data(), static_cast<uint32_t>(source.size()), CP_UTF8, &sourceBlob);
    if (FAILED(hr)) throw std::runtime_error("CreateBlobWithEncodingOnHeapCopy failed");

    ComPtr<IDxcIncludeHandler> includeHandler;
    hr = utils->CreateDefaultIncludeHandler(&includeHandler);
    if (FAILED(hr)) throw std::runtime_error("CreateDefaultIncludeHandler failed");

    std::vector<std::wstring> tempStrings;
    std::vector<LPCWSTR> argsW;

    // optimization flags
    if (opt.optimize) {
        tempStrings.push_back(L"-O3");
        argsW.push_back(tempStrings.back().c_str());
    }
    else {
        tempStrings.push_back(L"-Od");
        argsW.push_back(tempStrings.back().c_str());
    }
    // defines
    for (auto& d : opt.defines) {
        std::wstring w = L"-D" + std::wstring(d.first.begin(), d.first.end()) + L"=" + std::wstring(d.second.begin(), d.second.end());
        tempStrings.push_back(std::move(w));
        argsW.push_back(tempStrings.back().c_str());
    }

    std::wstring entryW(opt.entry.begin(), opt.entry.end());
    std::wstring targetW(opt.target.begin(), opt.target.end());
    std::wstring virtualPathW(virtualPath.begin(), virtualPath.end());

    ComPtr<IDxcOperationResult> result;
    hr = compiler->Compile(
        sourceBlob.Get(),
        virtualPathW.c_str(),
        entryW.c_str(),
        targetW.c_str(),
        argsW.data(),
        static_cast<uint32_t>(argsW.size()),
        nullptr, 0,
        includeHandler.Get(),
        &result);
    if (FAILED(hr)) {
        throw std::runtime_error("DXC compile invocation failed");
    }

    HRESULT status;
    hr = result->GetStatus(&status);
    if (FAILED(hr) || FAILED(status)) {
        ComPtr<IDxcBlobEncoding> errors;
        result->GetErrorBuffer(&errors);
        if (errors && errors->GetBufferSize() > 0) {
            std::string msg((char*)errors->GetBufferPointer(), errors->GetBufferSize());
            throw std::runtime_error("Shader compile error:\n" + msg);
        }
        throw std::runtime_error("Shader compile failed (unknown reason)");
    }

    ComPtr<IDxcBlob> program;
    hr = result->GetResult(&program);
    if (FAILED(hr) || !program) {
        throw std::runtime_error("Failed to get compiled shader blob");
    }

    ShaderBlob blob;
    blob.bytecode.resize(program->GetBufferSize());
    memcpy(blob.bytecode.data(), program->GetBufferPointer(), program->GetBufferSize());
    blob.sourcePath = virtualPath;
    blob.entry = opt.entry;
    blob.target = opt.target;
    blob.hash = key;

    // save to disk cache (best-effort)
    SaveBlobToDisk(blob, cacheFile.string());

    return blob;
}

bool ShaderCompiler::SaveBlobToDisk(const ShaderBlob& blob, const std::string& outPath)
{
	try {
		std::ofstream ofs(outPath, std::ios::binary);
		if (!ofs) return false;
		uint32_t entryLen = static_cast<uint32_t>(blob.entry.size());
		uint32_t targetLen = static_cast<uint32_t>(blob.target.size());
		uint32_t hashLen = static_cast<uint32_t>(blob.hash.size());
		ofs.write(reinterpret_cast<const char*>(&entryLen), sizeof(entryLen));
		ofs.write(reinterpret_cast<const char*>(&targetLen), sizeof(targetLen));
		ofs.write(reinterpret_cast<const char*>(&hashLen), sizeof(hashLen));
		ofs.write(blob.entry.c_str(), entryLen);
		ofs.write(blob.target.c_str(), targetLen);
		ofs.write(blob.hash.c_str(), hashLen);
		uint64_t dataSize = static_cast<uint64_t>(blob.bytecode.size());
		ofs.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
		if (dataSize) ofs.write(reinterpret_cast<const char*>(blob.bytecode.data()), dataSize);
		return true;
	}
	catch (...) {
		return false;
	}
}

std::optional<ShaderBlob> ShaderCompiler::LoadBlobFromDisk(const std::string& inPath)
{
	try {
		std::ifstream ifs(inPath, std::ios::binary);
		if (!ifs) return std::nullopt;
		ShaderBlob b;
		uint32_t entryLen = 0, targetLen = 0, hashLen = 0;
		ifs.read(reinterpret_cast<char*>(&entryLen), sizeof(entryLen));
		ifs.read(reinterpret_cast<char*>(&targetLen), sizeof(targetLen));
		ifs.read(reinterpret_cast<char*>(&hashLen), sizeof(hashLen));
		b.entry.resize(entryLen);
		b.target.resize(targetLen);
		b.hash.resize(hashLen);
		ifs.read(b.entry.data(), entryLen);
		ifs.read(b.target.data(), targetLen);
		ifs.read(b.hash.data(), hashLen);
		uint64_t dataSize = 0;
		ifs.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));
		if (dataSize) {
			b.bytecode.resize(static_cast<size_t>(dataSize));
			ifs.read(reinterpret_cast<char*>(b.bytecode.data()), dataSize);
		}
		return b;
	}
	catch (...) {
		return std::nullopt;
	}
}

std::string ShaderCompiler::ComputeCacheKey(const std::string& source, const ShaderCompileOptions& opt)
{
	std::hash<std::string> h;
	std::string key = source + "|" + opt.entry + "|" + opt.target;
	for (auto& d : opt.defines) key += "|" + d.first + "=" + d.second;
	size_t hv = h(key);
	return std::to_string(hv);
}

std::string ShaderCompiler::ReadFileToString(const std::string& path)
{
	std::ifstream ifs(path, std::ios::binary);
	if (!ifs) throw std::runtime_error("Failed to open file: " + path);
	std::ostringstream ss;
	ss << ifs.rdbuf();
	return ss.str();
}
