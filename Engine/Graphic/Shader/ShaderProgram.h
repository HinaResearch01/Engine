#pragma once

#include "ShaderModule.h"
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

namespace Tsumi::Graphic {

enum class ShaderStage { VS, PS, GS, HS, DS, CS };

struct ShaderProgramDesc {
    std::unordered_map<ShaderStage, std::string> sources;
};
struct ReflectionInfo {
    std::vector<std::string> cbNames;
    std::vector<std::string> srvNames;
};

/*  */
class ShaderProgram {

public:
    /// <summary>
    /// 
    /// </summary>
    ShaderProgram() = default;

    /// <summary>
    /// 
    /// </summary>
    void AttachModule(ShaderStage stage, std::unique_ptr<ShaderModule> module);
    
    /// <summary>
    /// 
    /// </summary>
    void CompileAndAttach(ShaderStage stage, const std::string& path, const ShaderCompileOptions& opt);

    /// <summary>
    /// 
    /// </summary>
    const ShaderModule* GetModule(ShaderStage stage) const;

#pragma region Accessor
    const ReflectionInfo& GetReflection() const { return reflection_; }
#pragma endregion

private:
    std::unordered_map<ShaderStage, std::unique_ptr<ShaderModule>> modules_;
    ReflectionInfo reflection_;
};

}