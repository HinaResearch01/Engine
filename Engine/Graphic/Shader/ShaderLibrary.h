#pragma once

#include "ShaderProgram.h"
#include <memory>
#include <unordered_map>
#include <string>

namespace Tsumi::Graphic {

/*  */
class ShaderLibrary {

public:
    /// <summary>
    /// 
    /// </summary>
    ShaderLibrary() = default;

    /// <summary>
    /// 
    /// </summary>
    ShaderProgram* LoadOrGet(const std::string& key, const ShaderProgramDesc& desc);

    /// <summary>
    /// 
    /// </summary>
    ShaderProgram* Get(const std::string& key);

    /// <summary>
    /// 
    /// </summary>
    bool Reload(const std::string& key);

    /// <summary>
    /// 
    /// </summary>
    void LoadAllShaders();

private:
    std::unordered_map<std::string, std::unique_ptr<ShaderProgram>> map_;
    std::unordered_map<std::string, ShaderProgramDesc> descs_;
};

}