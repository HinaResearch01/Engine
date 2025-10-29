#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <optional>

namespace Tsumi::Graphic {

struct ShaderCompileOptions {
    std::string entry = "main";
    std::string target = "vs_6_0"; // "vs_6_0", "ps_6_0", etc.
    std::vector<std::pair<std::string, std::string>> defines;
    bool optimize = true;
};

struct ShaderBlob {
    std::vector<uint8_t> bytecode; 
    std::string sourcePath;
    std::string entry;
    std::string target;
    std::string hash;              
};


/* shaderの読み込みクラス */
class ShaderCompiler {

public:
    /// <summary>
    /// 
    /// </summary>
    static ShaderBlob CompileFromFile(const std::string& path, const ShaderCompileOptions& opt);

    /// <summary>
    /// 
    /// </summary>
    static ShaderBlob CompileFromSource(const std::string& source, const std::string& virtualPath, const ShaderCompileOptions& opt);

    /// <summary>
    /// 
    /// </summary>
    static bool SaveBlobToDisk(const ShaderBlob& blob, const std::string& outPath);

    /// <summary>
    /// 
    /// </summary>
    static std::optional<ShaderBlob> LoadBlobFromDisk(const std::string& inPath);

private:
    /// <summary>
    /// 
    /// </summary>
    static std::string ComputeCacheKey(const std::string& source, const ShaderCompileOptions& opt);

    /// <summary>
    /// 
    /// </summary>
    static std::string ReadFileToString(const std::string& path);
};

}