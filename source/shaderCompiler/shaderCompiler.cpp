#include "shaderCompiler.h"
#include "shaderc\shaderc.hpp"

namespace vsc {

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// Blob
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct Result::Impl
	{
		std::vector<uint32_t> blob;
		size_t warningCount;
		std::vector<std::string> errorMessages;
	};

	Result::Result() :
		impl(new Impl)
	{
	}

	Result::~Result()
	{
		delete impl;
	}

	size_t Result::GetBlobSize() const
	{
		return sizeof(uint32_t) * impl->blob.size();
	}

	const uint32_t* Result::GetBlob() const
	{
		return impl->blob.data();
	}

	size_t Result::GetWarningCount() const
	{
		return impl->warningCount;
	}

	size_t Result::GetErrorCount() const
	{
		return impl->errorMessages.size();
	}

	const char* Result::GetErrorMessage(size_t errorIndex) const
	{
		return impl->errorMessages[errorIndex].c_str();
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// CompilerOptions
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct CompilerOptions::Impl
	{
		shaderc::CompileOptions options;
	};

	CompilerOptions::CompilerOptions() :
		impl(new Impl)
	{
	}

	CompilerOptions::~CompilerOptions()
	{
		delete impl;
	}

	void CompilerOptions::SetOptimize(bool enable)
	{
		impl->options.SetOptimizationLevel((enable == true) ? shaderc_optimization_level_size : shaderc_optimization_level_zero);
	}

	void CompilerOptions::AddMacroDefinition(const char* pName)
	{
		impl->options.AddMacroDefinition(pName);
	}

	void CompilerOptions::AddMacroDefinition(const char* pName, const char* pValue)
	{
		impl->options.AddMacroDefinition(pName, pValue);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// Compiler
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct Compiler::Impl
	{
		shaderc::Compiler compiler;
	};

	Compiler::Compiler() :
		impl(new Impl)
	{
	}

	Compiler::~Compiler()
	{
		delete impl;
	}

	void Compiler::GlslToSpv(const char* pSrcFileName, const char* pSrc, vsc::SHADER_TYPE shaderType, const char* pEntryPointName, vsc::CompilerOptions* pOptions, vsc::Result* pResult)
	{
		static constexpr shaderc_shader_kind shaderKindTable[vsc::SHADER_TYPE_COUNT] =
		{
			shaderc_vertex_shader,
			shaderc_tess_control_shader,
			shaderc_tess_evaluation_shader,
			shaderc_geometry_shader,
			shaderc_fragment_shader,
			shaderc_compute_shader,
		};

		pResult->impl->blob.clear();
		pResult->impl->warningCount = 0;
		pResult->impl->errorMessages.clear();

		shaderc::SpvCompilationResult result = impl->compiler.CompileGlslToSpv(pSrc, shaderKindTable[shaderType], pSrcFileName, pEntryPointName, pOptions->impl->options);

		pResult->impl->blob = { result.begin(), result.end() };
		pResult->impl->warningCount = result.GetNumWarnings();

		size_t errorCount = result.GetNumErrors();
		pResult->impl->errorMessages.reserve(errorCount);

		for (size_t i = 0; i < errorCount; i++)
		{
			pResult->impl->errorMessages.push_back(result.GetErrorMessage());
		}
	}

}