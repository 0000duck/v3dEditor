#pragma once

#ifdef VSC_DLL
#ifdef VSC_DLL_EXPORTS
#define VSC_DLL_API __declspec( dllexport )
#else // VSC_DLL_EXPORTS
#define VSC_DLL_API __declspec( dllimport )
#endif // VSC_DLL_EXPORTS
#else //VSC_DLL
#define VSC_DLL_API
#endif //VSC_DLL

#include <stdint.h>

namespace vsc {

	class VSC_DLL_API Result
	{
	public:
		Result();
		~Result();

		size_t GetBlobSize() const;
		const uint32_t* GetBlob() const;

		size_t GetWarningCount() const;

		size_t GetErrorCount() const;
		const char* GetErrorMessage(size_t errorIndex) const;

	private:
		struct Impl;
		Impl* impl;

		friend class Compiler;
	};

	class VSC_DLL_API CompilerOptions
	{
	public:
		CompilerOptions();
		~CompilerOptions();

		void SetOptimize(bool enable);
		void AddMacroDefinition(const char* pName);
		void AddMacroDefinition(const char* pName, const char* pValue);

	private:
		struct Impl;
		Impl* impl;

		friend class Compiler;
	};

	enum SHADER_TYPE
	{
		SHADER_TYPE_VERTEX = 0,
		SHADER_TYPE_TESSELLATION_CONTROL = 1,
		SHADER_TYPE_TESSELLATION_EVALUATION = 2,
		SHADER_TYPE_GEOMETRY = 3,
		SHADER_TYPE_FRAGMENT = 4,
		SHADER_TYPE_COMPUTE = 5,

		SHADER_TYPE_COUNT = 6,
	};

	class VSC_DLL_API Compiler final
	{
	public:
		Compiler();
		~Compiler();

		void GlslToSpv(const char* pSrcFileName, const char* pSrc, vsc::SHADER_TYPE shaderType, const char* pEntryPointName, vsc::CompilerOptions* pOptions, vsc::Result* pResult);

	private:
		struct Impl;
		Impl* impl;
	};

}