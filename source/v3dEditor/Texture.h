#pragma once

namespace ve {

	class Texture final
	{
	public:
		static TexturePtr Create(DeviceContextPtr deviceContext, const wchar_t* pFilePath, V3DFlags stageMask = V3D_PIPELINE_STAGE_FRAGMENT_SHADER, V3DFlags accessMask = V3D_ACCESS_SHADER_READ);
		static TexturePtr Create(DeviceContextPtr deviceContext, const wchar_t* pFilePath, size_t srcSize, const void* pSrc, V3DFlags stageMask = V3D_PIPELINE_STAGE_FRAGMENT_SHADER, V3DFlags accessMask = V3D_ACCESS_SHADER_READ);

		Texture();
		~Texture();

		const wchar_t* GetFilePath();

		const V3DImageDesc& GetNativeDesc() const;
		IV3DImageView* GetNativeImageViewPtr();

		VE_DECLARE_ALLOCATOR

	private:
		struct Impl;
		Impl* impl;
	};

}