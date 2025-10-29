#pragma once

#include "ShaderCompiler.h"
#include <string>
#include <vector>
#include <d3d12.h>
#include <wrl.h>

namespace Tsumi::Graphic {

struct ShaderReflectionBinding {
    std::string name;
    UINT bindPoint = 0;
    UINT bindCount = 0;
    D3D_SHADER_INPUT_TYPE inputType = D3D_SIT_CBUFFER; // placeholder
};

/*  */
class ShaderModule {

public:
    /// <summary>
    /// 
    /// </summary>
    explicit ShaderModule(const ShaderBlob& blob);

    /// <summary>
    /// 
    /// </summary>
    ~ShaderModule() = default;

    /// <summary>
    /// 
    /// </summary>
    const void* GetBytecodePtr() const;

    /// <summary>
    /// 
    /// </summary>
    size_t GetBytecodeSize() const;

#pragma region Accessor
    const std::string& GetEntry() const { return blob_.entry; }
    const std::string& GetTarget() const { return blob_.target; }
    const std::string& GetHash() const { return blob_.hash; }
    const std::vector<ShaderReflectionBinding>& GetBindings() const { return bindings_; }
#pragma endregion

private:
    ShaderBlob blob_;
    std::vector<ShaderReflectionBinding> bindings_;
};

}

