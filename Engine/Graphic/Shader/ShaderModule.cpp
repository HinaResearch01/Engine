#include "ShaderModule.h"

using namespace Tsumi::Graphic;

ShaderModule::ShaderModule(const ShaderBlob& blob)
    : blob_(blob)
{
    bindings_.clear();
}

const void* ShaderModule::GetBytecodePtr() const
{
	return blob_.bytecode.empty() ? nullptr : blob_.bytecode.data();
}

size_t ShaderModule::GetBytecodeSize() const
{
	return blob_.bytecode.size();
}
