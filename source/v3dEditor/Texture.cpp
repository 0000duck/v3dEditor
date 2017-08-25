#include "Texture.h"
#include "ResourceMemoryManager.h"
#include "TextureManager.h"
#include "ImmediateContext.h"
#include "DeviceContext.h"
#include "DeletingQueue.h"

namespace ve {

	/**********************************/
	/* private - struct Texture::Impl */
	/**********************************/

	struct Texture::Impl
	{
		DeviceContextPtr deviceContext;

		StringW filePath;
		V3DFlags stageMask;
		V3DFlags accessMask;

		V3DImageDesc nativeDesc;
		IV3DImageView* pNativeImageView;
		ResourceAllocation imageAllocation;

		Impl() :
			pNativeImageView(nullptr),
			imageAllocation(nullptr)
		{
		}

		bool Initialize(DeviceContextPtr deviceContext, const wchar_t* pFilePath, V3DFlags stageMask, V3DFlags accessMask)
		{
			this->deviceContext = deviceContext;
			this->filePath = pFilePath;
			this->stageMask = stageMask;
			this->accessMask = accessMask;

			// ----------------------------------------------------------------------------------------------------
			// テクスチャを読み込む
			// ----------------------------------------------------------------------------------------------------

			StringA filePathA;
			ToMultibyteString(pFilePath, filePathA);

			gli::texture texture = gli::load(filePathA.c_str());
			if (texture.empty() == true)
			{
				return false;
			}

			if (Load(deviceContext, pFilePath, texture, stageMask, accessMask) == false)
			{
				return false;
			}

			return true;
		}

		bool Initialize(DeviceContextPtr deviceContext, const wchar_t* pFilePath, size_t srcSize, const void* pSrc, V3DFlags stageMask, V3DFlags accessMask)
		{
			this->deviceContext = deviceContext;
			this->filePath = pFilePath;
			this->stageMask = stageMask;
			this->accessMask = accessMask;

			// ----------------------------------------------------------------------------------------------------
			// テクスチャを読み込む
			// ----------------------------------------------------------------------------------------------------

			StringA filePathA;
			ToMultibyteString(pFilePath, filePathA);

			gli::texture texture = gli::load(static_cast<const char*>(pSrc), srcSize);
			if (texture.empty() == true)
			{
				return false;
			}

			if (Load(deviceContext, pFilePath, texture, stageMask, accessMask) == false)
			{
				return false;
			}

			return true;
		}

		bool Load(DeviceContextPtr deviceContext, const wchar_t* pFilePath, gli::texture& texture, V3DFlags stageMask, V3DFlags accessMask)
		{
			this->deviceContext = deviceContext;
			this->filePath = pFilePath;
			this->stageMask = stageMask;
			this->accessMask = accessMask;

			/************************/
			/* イメージの記述を作成 */
			/************************/

			gli::gl gl(gli::gl::PROFILE_GL33);
			gli::gl::format format = gl.translate(texture.format(), texture.swizzles());
			gli::gl::target target = gl.translate(texture.target());

			nativeDesc = V3DImageDesc{};

			V3DImageViewDesc imageViewDesc{};

			switch (target)
			{
			case gli::gl::TARGET_1D:
				nativeDesc.type = V3D_IMAGE_TYPE_1D;
				imageViewDesc.type = V3D_IMAGE_VIEW_TYPE_1D;
				break;
			case gli::gl::TARGET_1D_ARRAY:
				nativeDesc.type = V3D_IMAGE_TYPE_1D;
				imageViewDesc.type = V3D_IMAGE_VIEW_TYPE_1D_ARRAY;
				break;
			case gli::gl::TARGET_2D:
				nativeDesc.type = V3D_IMAGE_TYPE_2D;
				imageViewDesc.type = V3D_IMAGE_VIEW_TYPE_2D;
				break;
			case gli::gl::TARGET_2D_ARRAY:
				nativeDesc.type = V3D_IMAGE_TYPE_2D;
				imageViewDesc.type = V3D_IMAGE_VIEW_TYPE_2D_ARRAY;
				break;
			case gli::gl::TARGET_3D:
				nativeDesc.type = V3D_IMAGE_TYPE_3D;
				imageViewDesc.type = V3D_IMAGE_VIEW_TYPE_3D;
				break;
			case gli::gl::TARGET_CUBE:
				nativeDesc.type = V3D_IMAGE_TYPE_2D;
				nativeDesc.usageFlags = V3D_IMAGE_USAGE_CUBE_COMPATIBLE;
				imageViewDesc.type = V3D_IMAGE_VIEW_TYPE_CUBE;
				break;

			default:
				return false;
			}

			nativeDesc.format = Impl::ToV3DFormat(format.Internal);
			if (nativeDesc.format == V3D_FORMAT_UNDEFINED)
			{
				return false;
			}

			nativeDesc.width = texture.extent().x;
			nativeDesc.height = texture.extent().y;
			nativeDesc.depth = texture.extent().z;

			nativeDesc.levelCount = static_cast<uint32_t>(texture.levels());
			nativeDesc.layerCount = static_cast<uint32_t>((imageViewDesc.type == V3D_IMAGE_VIEW_TYPE_CUBE) ? texture.faces() : texture.layers());

			nativeDesc.samples = V3D_SAMPLE_COUNT_1;
			nativeDesc.tiling = V3D_IMAGE_TILING_OPTIMAL;
			nativeDesc.usageFlags |= V3D_IMAGE_USAGE_TRANSFER_DST | V3D_IMAGE_USAGE_SAMPLED;

			imageViewDesc.baseLevel = 0;
			imageViewDesc.levelCount = nativeDesc.levelCount;
			imageViewDesc.baseLayer = 0;
			imageViewDesc.layerCount = nativeDesc.layerCount;

			// ----------------------------------------------------------------------------------------------------
			// テクスチャを転送
			// ----------------------------------------------------------------------------------------------------

			IV3DDevice* pNativeDevice = deviceContext->GetNativeDevicePtr();
			ResourceMemoryManager* pResourceMemoryManager = deviceContext->GetResourceMemoryManagerPtr();

			/****************************/
			/* 転送元のバッファーを作成 */
			/****************************/

			V3DBufferDesc srcBufferDesc;
			srcBufferDesc.usageFlags = V3D_BUFFER_USAGE_TRANSFER_SRC;
			srcBufferDesc.size = texture.size();

			IV3DBuffer* pSrcBuffer;
			if (pNativeDevice->CreateBuffer(srcBufferDesc, &pSrcBuffer, VE_INTERFACE_DEBUG_NAME(L"VE_Texture_SrcBuffer")) != V3D_OK)
			{
				return false;
			}

			ResourceAllocation srcBufferAllocation = pResourceMemoryManager->Allocate(pSrcBuffer, V3D_MEMORY_PROPERTY_HOST_VISIBLE);
			if (srcBufferAllocation == nullptr)
			{
				pSrcBuffer->Release();
				return false;
			}

			void* pMemory;
			if (pSrcBuffer->Map(0, 0, &pMemory) == V3D_OK)
			{
				memcpy_s(pMemory, VE_U64_TO_U32(pSrcBuffer->GetResourceDesc().memorySize), texture.data(), texture.size());
				pSrcBuffer->Unmap();
			}
			else
			{
				pSrcBuffer->Release();
				pResourceMemoryManager->Free(srcBufferAllocation);
				return false;
			}

			/**************************/
			/* 転送先のイメージを作成 */
			/**************************/

			IV3DImage* pDstImage;
			if (pNativeDevice->CreateImage(nativeDesc, V3D_IMAGE_LAYOUT_UNDEFINED, &pDstImage, VE_INTERFACE_DEBUG_NAME(L"VE_Texture_DstImage")) != V3D_OK)
			{
				pSrcBuffer->Release();
				pResourceMemoryManager->Free(srcBufferAllocation);
				return false;
			}

			imageAllocation = pResourceMemoryManager->Allocate(pDstImage, V3D_MEMORY_PROPERTY_DEVICE_LOCAL);
			if (imageAllocation == nullptr)
			{
				pDstImage->Release();
				pSrcBuffer->Release();
				pResourceMemoryManager->Free(srcBufferAllocation);
				return false;
			}

			/********/
			/* 転送 */
			/********/

			collection::Vector<V3DCopyBufferToImageRange> copyRanges;
			copyRanges.reserve(nativeDesc.levelCount);

			V3DCopyBufferToImageRange tempRange{};
			tempRange.dstImageSubresource.baseLayer = 0;
			tempRange.dstImageSubresource.layerCount = nativeDesc.layerCount;

			for (uint32_t level = 0; level < nativeDesc.levelCount; level++)
			{
				tempRange.dstImageSubresource.level = level;

				gli::extent3d extent = texture.extent(level);
				tempRange.dstImageSize.width = extent.x;
				tempRange.dstImageSize.height = extent.y;
				tempRange.dstImageSize.depth = extent.z;

				copyRanges.push_back(tempRange);

				tempRange.srcBufferOffset += texture.size(level);
			}

			IV3DCommandBuffer* pCommandBuffer = deviceContext->GetImmediateContextPtr()->Begin();
			VE_ASSERT(pCommandBuffer != nullptr);

			V3DPipelineBarrier pipelineBarrier;
			pipelineBarrier.dependencyFlags = 0;

			V3DImageMemoryBarrier memoryBarrier;
			memoryBarrier.srcQueueFamily = V3D_QUEUE_FAMILY_IGNORED;
			memoryBarrier.dstQueueFamily = V3D_QUEUE_FAMILY_IGNORED;
			memoryBarrier.pImage = pDstImage;
			memoryBarrier.baseLevel = 0;
			memoryBarrier.levelCount = nativeDesc.levelCount;
			memoryBarrier.baseLayer = 0;
			memoryBarrier.layerCount = nativeDesc.layerCount;

			pipelineBarrier.srcStageMask = V3D_PIPELINE_STAGE_TOP_OF_PIPE;
			pipelineBarrier.dstStageMask = V3D_PIPELINE_STAGE_TRANSFER;
			memoryBarrier.srcAccessMask = 0;
			memoryBarrier.dstAccessMask = V3D_ACCESS_TRANSFER_WRITE;
			memoryBarrier.srcLayout = V3D_IMAGE_LAYOUT_UNDEFINED;
			memoryBarrier.dstLayout = V3D_IMAGE_LAYOUT_TRANSFER_DST;
			pCommandBuffer->Barrier(pipelineBarrier, memoryBarrier);

			pCommandBuffer->CopyBufferToImage(pDstImage, V3D_IMAGE_LAYOUT_TRANSFER_DST, pSrcBuffer, nativeDesc.levelCount, copyRanges.data());

			pipelineBarrier.srcStageMask = V3D_PIPELINE_STAGE_TRANSFER;
			pipelineBarrier.dstStageMask = stageMask;
			memoryBarrier.srcAccessMask = V3D_ACCESS_TRANSFER_WRITE;
			memoryBarrier.dstAccessMask = accessMask;
			memoryBarrier.srcLayout = V3D_IMAGE_LAYOUT_TRANSFER_DST;
			memoryBarrier.dstLayout = V3D_IMAGE_LAYOUT_SHADER_READ_ONLY;
			pCommandBuffer->Barrier(pipelineBarrier, memoryBarrier);

			deviceContext->GetImmediateContextPtr()->End();

			/********************/
			/* 不要なものを破棄 */
			/********************/

			pSrcBuffer->Release();
			pResourceMemoryManager->Free(srcBufferAllocation);

			/************************/
			/* イメージビューを作成 */
			/************************/

			if (pNativeDevice->CreateImageView(pDstImage, imageViewDesc, &pNativeImageView, VE_INTERFACE_DEBUG_NAME(pFilePath)) != V3D_OK)
			{
				pDstImage->Release();
				pResourceMemoryManager->Free(imageAllocation);
				return false;
			}

			pDstImage->Release();

			// ----------------------------------------------------------------------------------------------------

			return true;
		}

		static V3D_FORMAT ToV3DFormat(gli::gl::internal_format format)
		{
			V3D_FORMAT ret;

			switch (format)
			{
			case gli::gl::INTERNAL_BGRA8_UNORM:
				ret = V3D_FORMAT_B8G8R8A8_UNORM;
				break;

			case gli::gl::INTERNAL_R8_UNORM:
				ret = V3D_FORMAT_R8_UNORM;
				break;
			case gli::gl::INTERNAL_RG8_UNORM:
				ret = V3D_FORMAT_R8G8_UNORM;
				break;
			case gli::gl::INTERNAL_RGB8_UNORM:
				ret = V3D_FORMAT_R8G8B8_UNORM;
				break;
			case gli::gl::INTERNAL_RGBA8_UNORM:
				ret = V3D_FORMAT_R8G8B8A8_UNORM;
				break;

			case gli::gl::INTERNAL_R16_UNORM:
				ret = V3D_FORMAT_R16_UNORM;
				break;
			case gli::gl::INTERNAL_RG16_UNORM:
				ret = V3D_FORMAT_R16G16_UNORM;
				break;
			case gli::gl::INTERNAL_RGB16_UNORM:
				ret = V3D_FORMAT_R16G16B16_UNORM;
				break;
			case gli::gl::INTERNAL_RGBA16_UNORM:
				ret = V3D_FORMAT_R16G16B16A16_UNORM;
				break;

			case gli::gl::INTERNAL_RGB10A2_UNORM:
				ret = V3D_FORMAT_A2R10G10B10_UNORM;
				break;
			case gli::gl::INTERNAL_RGB10A2_SNORM_EXT:
				ret = V3D_FORMAT_A2R10G10B10_SNORM;
				break;

			case gli::gl::INTERNAL_R8_SNORM:
				ret = V3D_FORMAT_R8_SNORM;
				break;
			case gli::gl::INTERNAL_RG8_SNORM:
				ret = V3D_FORMAT_R8G8_SNORM;
				break;
			case gli::gl::INTERNAL_RGB8_SNORM:
				ret = V3D_FORMAT_R8G8B8_SNORM;
				break;
			case gli::gl::INTERNAL_RGBA8_SNORM:
				ret = V3D_FORMAT_R8G8B8A8_SNORM;
				break;

			case gli::gl::INTERNAL_R16_SNORM:
				ret = V3D_FORMAT_R16_SNORM;
				break;
			case gli::gl::INTERNAL_RG16_SNORM:
				ret = V3D_FORMAT_R16G16_SNORM;
				break;
			case gli::gl::INTERNAL_RGB16_SNORM:
				ret = V3D_FORMAT_R16G16B16_SNORM;
				break;
			case gli::gl::INTERNAL_RGBA16_SNORM:
				ret = V3D_FORMAT_R16G16B16A16_SNORM;
				break;

			case gli::gl::INTERNAL_R8U:
				ret = V3D_FORMAT_R8_UINT;
				break;
			case gli::gl::INTERNAL_RG8U:
				ret = V3D_FORMAT_R8G8_UINT;
				break;
			case gli::gl::INTERNAL_RGB8U:
				ret = V3D_FORMAT_R8G8B8_UINT;
				break;
			case gli::gl::INTERNAL_RGBA8U:
				ret = V3D_FORMAT_R8G8B8A8_UINT;
				break;

			case gli::gl::INTERNAL_R16U:
				ret = V3D_FORMAT_R16_UINT;
				break;
			case gli::gl::INTERNAL_RG16U:
				ret = V3D_FORMAT_R16G16_UINT;
				break;
			case gli::gl::INTERNAL_RGB16U:
				ret = V3D_FORMAT_R16G16B16_UINT;
				break;
			case gli::gl::INTERNAL_RGBA16U:
				ret = V3D_FORMAT_R16G16B16A16_UINT;
				break;

			case gli::gl::INTERNAL_R32U:
				ret = V3D_FORMAT_R32_UINT;
				break;
			case gli::gl::INTERNAL_RG32U:
				ret = V3D_FORMAT_R32G32_UINT;
				break;
			case gli::gl::INTERNAL_RGB32U:
				ret = V3D_FORMAT_R32G32B32_UINT;
				break;
			case gli::gl::INTERNAL_RGBA32U:
				ret = V3D_FORMAT_R32G32B32A32_UINT;
				break;

			case gli::gl::INTERNAL_RGB10A2U:
				ret = V3D_FORMAT_A2R10G10B10_UINT;
				break;
			case gli::gl::INTERNAL_RGB10A2I_EXT:
				ret = V3D_FORMAT_A2R10G10B10_SINT;
				break;

			case gli::gl::INTERNAL_R8I:
				ret = V3D_FORMAT_R8_SINT;
				break;
			case gli::gl::INTERNAL_RG8I:
				ret = V3D_FORMAT_R8G8_SINT;
				break;
			case gli::gl::INTERNAL_RGB8I:
				ret = V3D_FORMAT_R8G8B8_SINT;
				break;
			case gli::gl::INTERNAL_RGBA8I:
				ret = V3D_FORMAT_R8G8B8A8_SINT;
				break;

			case gli::gl::INTERNAL_R16I:
				ret = V3D_FORMAT_R16_SINT;
				break;
			case gli::gl::INTERNAL_RG16I:
				ret = V3D_FORMAT_R16G16_SINT;
				break;
			case gli::gl::INTERNAL_RGB16I:
				ret = V3D_FORMAT_R16G16B16_SINT;
				break;
			case gli::gl::INTERNAL_RGBA16I:
				ret = V3D_FORMAT_R16G16B16A16_SINT;
				break;

			case gli::gl::INTERNAL_R32I:
				ret = V3D_FORMAT_R32_SINT;
				break;
			case gli::gl::INTERNAL_RG32I:
				ret = V3D_FORMAT_R32G32_SINT;
				break;
			case gli::gl::INTERNAL_RGB32I:
				ret = V3D_FORMAT_R32G32B32_SINT;
				break;
			case gli::gl::INTERNAL_RGBA32I:
				ret = V3D_FORMAT_R32G32B32A32_SINT;
				break;

			case gli::gl::INTERNAL_R16F:
				ret = V3D_FORMAT_R16_SFLOAT;
				break;
			case gli::gl::INTERNAL_RG16F:
				ret = V3D_FORMAT_R16G16_SFLOAT;
				break;
			case gli::gl::INTERNAL_RGB16F:
				ret = V3D_FORMAT_R16G16B16_SFLOAT;
				break;
			case gli::gl::INTERNAL_RGBA16F:
				ret = V3D_FORMAT_R16G16B16A16_SFLOAT;
				break;

			case gli::gl::INTERNAL_R32F:
				ret = V3D_FORMAT_R32_SFLOAT;
				break;
			case gli::gl::INTERNAL_RG32F:
				ret = V3D_FORMAT_R32G32_SFLOAT;
				break;
			case gli::gl::INTERNAL_RGB32F:
				ret = V3D_FORMAT_R32G32B32_SFLOAT;
				break;
			case gli::gl::INTERNAL_RGBA32F:
				ret = V3D_FORMAT_R32G32B32A32_SFLOAT;
				break;

			case gli::gl::INTERNAL_R64F_EXT:
				ret = V3D_FORMAT_R64_SFLOAT;
				break;
			case gli::gl::INTERNAL_RG64F_EXT:
				ret = V3D_FORMAT_R64G64_SFLOAT;
				break;
			case gli::gl::INTERNAL_RGB64F_EXT:
				ret = V3D_FORMAT_R64G64B64_SFLOAT;
				break;
			case gli::gl::INTERNAL_RGBA64F_EXT:
				ret = V3D_FORMAT_R64G64B64A64_SFLOAT;
				break;

			case gli::gl::INTERNAL_SR8:
				ret = V3D_FORMAT_R8_SRGB;
				break;
			case gli::gl::INTERNAL_SRG8:
				ret = V3D_FORMAT_R8G8_SRGB;
				break;
			case gli::gl::INTERNAL_SRGB8:
				ret = V3D_FORMAT_R8G8B8_SRGB;
				break;
			case gli::gl::INTERNAL_SRGB8_ALPHA8:
				ret = V3D_FORMAT_R8G8B8A8_SRGB;
				break;

			case gli::gl::INTERNAL_RGB9E5:
				ret = V3D_FORMAT_E5B9G9R9_UFLOAT;
				break;
			case gli::gl::INTERNAL_RG11B10F:
				ret = V3D_FORMAT_B10G11R11_UFLOAT;
				break;
			case gli::gl::INTERNAL_R5G6B5:
				ret = V3D_FORMAT_R5G6B5_UNORM;
				break;
			case gli::gl::INTERNAL_RGB5A1:
				ret = V3D_FORMAT_R5G5B5A1_UNORM;
				break;
			case gli::gl::INTERNAL_RGBA4:
				ret = V3D_FORMAT_R4G4B4A4_UNORM;
				break;

			case gli::gl::INTERNAL_RG4_EXT:
				ret = V3D_FORMAT_R4G4_UNORM;
				break;

			case gli::gl::INTERNAL_RGB_DXT1:
				ret = V3D_FORMAT_BC1_RGB_UNORM;
				break;
			case gli::gl::INTERNAL_RGBA_DXT1:
				ret = V3D_FORMAT_BC1_RGBA_UNORM;
				break;
			case gli::gl::INTERNAL_RGBA_DXT3:
				ret = V3D_FORMAT_BC3_UNORM;
				break;
			case gli::gl::INTERNAL_RGBA_DXT5:
				ret = V3D_FORMAT_BC5_UNORM;
				break;

			case gli::gl::INTERNAL_RGB_ETC2:
				ret = V3D_FORMAT_ETC2_R8G8B8_UNORM;
				break;
			case gli::gl::INTERNAL_RGBA_ETC2:
				ret = V3D_FORMAT_ETC2_R8G8B8_UNORM;
				break;
			case gli::gl::INTERNAL_R11_EAC:
				ret = V3D_FORMAT_EAC_R11_UNORM;
				break;
			case gli::gl::INTERNAL_SIGNED_R11_EAC:
				ret = V3D_FORMAT_EAC_R11_SNORM;
				break;
			case gli::gl::INTERNAL_RG11_EAC:
				ret = V3D_FORMAT_EAC_R11G11_UNORM;
				break;
			case gli::gl::INTERNAL_SIGNED_RG11_EAC:
				ret = V3D_FORMAT_EAC_R11G11_SNORM;
				break;

			case gli::gl::INTERNAL_RGBA_ASTC_4x4:
				ret = V3D_FORMAT_ASTC_4X4_UNORM;
				break;
			case gli::gl::INTERNAL_RGBA_ASTC_5x4:
				ret = V3D_FORMAT_ASTC_5X4_UNORM;
				break;
			case gli::gl::INTERNAL_RGBA_ASTC_5x5:
				ret = V3D_FORMAT_ASTC_5X5_UNORM;
				break;
			case gli::gl::INTERNAL_RGBA_ASTC_6x5:
				ret = V3D_FORMAT_ASTC_6X5_UNORM;
				break;
			case gli::gl::INTERNAL_RGBA_ASTC_6x6:
				ret = V3D_FORMAT_ASTC_6X6_UNORM;
				break;
			case gli::gl::INTERNAL_RGBA_ASTC_8x5:
				ret = V3D_FORMAT_ASTC_8X5_UNORM;
				break;
			case gli::gl::INTERNAL_RGBA_ASTC_8x6:
				ret = V3D_FORMAT_ASTC_8X6_UNORM;
				break;
			case gli::gl::INTERNAL_RGBA_ASTC_8x8:
				ret = V3D_FORMAT_ASTC_8X8_UNORM;
				break;
			case gli::gl::INTERNAL_RGBA_ASTC_10x5:
				ret = V3D_FORMAT_ASTC_10X5_UNORM;
				break;
			case gli::gl::INTERNAL_RGBA_ASTC_10x6:
				ret = V3D_FORMAT_ASTC_10X6_UNORM;
				break;
			case gli::gl::INTERNAL_RGBA_ASTC_10x8:
				ret = V3D_FORMAT_ASTC_10X8_UNORM;
				break;
			case gli::gl::INTERNAL_RGBA_ASTC_10x10:
				ret = V3D_FORMAT_ASTC_10X10_UNORM;
				break;
			case gli::gl::INTERNAL_RGBA_ASTC_12x10:
				ret = V3D_FORMAT_ASTC_12X10_UNORM;
				break;
			case gli::gl::INTERNAL_RGBA_ASTC_12x12:
				ret = V3D_FORMAT_ASTC_12X12_UNORM;
				break;

			default:
				ret = V3D_FORMAT_UNDEFINED;
			}

			return ret;
		}

		VE_DECLARE_ALLOCATOR
	};

	/********************/
	/* public - Texture */
	/********************/

	TexturePtr Texture::Create(DeviceContextPtr deviceContext, const wchar_t* pFilePath, V3DFlags stageMask, V3DFlags accessMask)
	{
		TextureManager* pManager = deviceContext->GetTextureManagerPtr();

		TexturePtr texture = pManager->Find(pFilePath, stageMask, accessMask);
		if (texture != nullptr)
		{
			return texture;
		}

		texture = std::make_shared<Texture>();

		if (texture->impl->Initialize(deviceContext, pFilePath, stageMask, accessMask) == false)
		{
			return nullptr;
		}

		pManager->Add(pFilePath, stageMask, accessMask, texture);

		return texture;
	}

	TexturePtr Texture::Create(DeviceContextPtr deviceContext, const wchar_t* pFilePath, size_t srcSize, const void* pSrc, V3DFlags stageMask, V3DFlags accessMask)
	{
		TextureManager* pManager = deviceContext->GetTextureManagerPtr();

		TexturePtr texture = pManager->Find(pFilePath, stageMask, accessMask);
		if (texture != nullptr)
		{
			return texture;
		}

		texture = std::make_shared<Texture>();

		if (texture->impl->Initialize(deviceContext, pFilePath, srcSize, pSrc, stageMask, accessMask) == false)
		{
			return nullptr;
		}

		pManager->Add(pFilePath, stageMask, accessMask, texture);

		return texture;
	}

	Texture::Texture() :
		impl(VE_NEW_T(Texture::Impl))
	{
	}

	Texture::~Texture()
	{
		if (impl != nullptr)
		{
			impl->deviceContext->GetTextureManagerPtr()->Remove(impl->filePath.c_str(), impl->stageMask, impl->accessMask);
			DeleteResource(impl->deviceContext->GetDeletingQueuePtr(), &impl->pNativeImageView, &impl->imageAllocation);
		}

		VE_DELETE_T(impl, Impl);
	}

	const wchar_t* Texture::GetFilePath()
	{
		return impl->filePath.c_str();
	}

	const V3DImageDesc& Texture::GetNativeDesc() const
	{
		return impl->nativeDesc;
	}

	IV3DImageView* Texture::GetNativeImageViewPtr()
	{
		return impl->pNativeImageView;
	}

}