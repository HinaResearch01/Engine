#include "ShaderProgram.h"
#include "ShaderCompiler.h"
#include <stdexcept>

using namespace Tsumi::Graphic;

void ShaderProgram::AttachModule(ShaderStage stage, std::unique_ptr<ShaderModule> module)
{
	modules_[stage] = std::move(module);
}

void ShaderProgram::CompileAndAttach(ShaderStage stage, const std::string& path, const ShaderCompileOptions& opt)
{
	ShaderBlob b = ShaderCompiler::CompileFromFile(path, opt);
	auto module = std::make_unique<ShaderModule>(b);
	AttachModule(stage, std::move(module));
}

const ShaderModule* ShaderProgram::GetModule(ShaderStage stage) const
{
	auto it = modules_.find(stage);
	if (it == modules_.end()) return nullptr;
	return it->second.get();
}
