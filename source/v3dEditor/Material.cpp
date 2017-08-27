#include "Material.h"
#include "Logger.h"
#include "DeviceContext.h"
#include "DeletingQueue.h"
#include "SamplerFactory.h"
#include "GraphicsFactory.h"
#include "DynamicBuffer.h"
#include "Texture.h"
#include "IMesh.h"

//#define VE_MATERIAL_NO_TEXTURE

namespace ve {

	/***********************************/
	/* private - struct Material::Impl */
	/***********************************/

	struct Material::Impl
	{
		DeviceContextPtr deviceContext;

		uint64_t key;
		StringW name;
		uint32_t shaderFlags;

		Material::Uniform uniform;

		uint32_t curTexturesIndex;
		uint32_t nextTexturesIndex;
		collection::Array2<TexturePtr, 2, Material::TEXTURE_TYPE_COUNT> textures;
		collection::Array1<Material::TextureInfo, Material::TEXTURE_TYPE_COUNT> textureInfos;

		V3D_POLYGON_MODE polygonMode;
		V3D_CULL_MODE cullMode;
		BLEND_MODE blendMode;

		uint32_t dynamicOffset;
		DynamicBuffer* pUniformBuffer;
		IV3DDescriptorSet* pNativeDescriptorSet[2];

		uint8_t updateFlags;

		collection::Map<IMesh*, collection::Vector<Material::Session>> sessions;

		Impl(Material* pMaterial) :
			key(0),
			shaderFlags(0),
			uniform({}),
			curTexturesIndex(0),
			nextTexturesIndex(1),
			dynamicOffset(0),
			pUniformBuffer(nullptr),
			updateFlags(Material::UPDATE_ALL),
			polygonMode(V3D_POLYGON_MODE_FILL),
			cullMode(V3D_CULL_MODE_BACK),
			blendMode(BLEND_MODE_COPY)
		{
			key = reinterpret_cast<uint64_t>(pMaterial);

			uniform.diffuseColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			uniform.diffuseFactor = 1.0f;
			uniform.specularColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			uniform.specularFactor = 0.0f;
			uniform.shininess = 1.0f;
			uniform.emissiveFactor = 0.0f;

			for (size_t i = 0; i < 2; i++)
			{
				for (size_t j = 0; j < textures[i].size(); j++)
				{
					textures[i][j] = {};
				}
			}

			textureInfos[Material::TEXTURE_TYPE_DIFFUSE].filter = V3D_FILTER_LINEAR;
			textureInfos[Material::TEXTURE_TYPE_DIFFUSE].addressMode = V3D_ADDRESS_MODE_REPEAT;
			textureInfos[Material::TEXTURE_TYPE_DIFFUSE].mipLevel = 0;
			textureInfos[Material::TEXTURE_TYPE_DIFFUSE].anisotropyEnable = true;
			textureInfos[Material::TEXTURE_TYPE_DIFFUSE].enable = false;
			textureInfos[Material::TEXTURE_TYPE_DIFFUSE].update = false;

			textureInfos[Material::TEXTURE_TYPE_SPECULAR].filter = V3D_FILTER_LINEAR;
			textureInfos[Material::TEXTURE_TYPE_SPECULAR].addressMode = V3D_ADDRESS_MODE_REPEAT;
			textureInfos[Material::TEXTURE_TYPE_SPECULAR].mipLevel = 0;
			textureInfos[Material::TEXTURE_TYPE_SPECULAR].anisotropyEnable = true;
			textureInfos[Material::TEXTURE_TYPE_SPECULAR].enable = false;
			textureInfos[Material::TEXTURE_TYPE_SPECULAR].update = false;

			textureInfos[Material::TEXTURE_TYPE_BUMP].filter = V3D_FILTER_NEAREST;
			textureInfos[Material::TEXTURE_TYPE_BUMP].addressMode = V3D_ADDRESS_MODE_REPEAT;
			textureInfos[Material::TEXTURE_TYPE_BUMP].mipLevel = 0;
			textureInfos[Material::TEXTURE_TYPE_BUMP].anisotropyEnable = false;
			textureInfos[Material::TEXTURE_TYPE_BUMP].enable = false;
			textureInfos[Material::TEXTURE_TYPE_BUMP].update = false;

			pNativeDescriptorSet[Material::DST_COLOR] = nullptr;
			pNativeDescriptorSet[Material::DST_SHADOW] = nullptr;
		}

		bool Initialize(DeviceContextPtr deviceContext, const wchar_t* pName)
		{
			this->deviceContext = deviceContext;

			if (pName != nullptr)
			{
				name = pName;
			}

			pUniformBuffer = DynamicBuffer::Create(this->deviceContext, V3D_BUFFER_USAGE_UNIFORM, sizeof(Material::Uniform), V3D_PIPELINE_STAGE_FRAGMENT_SHADER, V3D_ACCESS_UNIFORM_READ, VE_INTERFACE_DEBUG_NAME(L"Material"));
			if (pUniformBuffer == nullptr)
			{
				return false;
			}

			dynamicOffset = pUniformBuffer->GetNativeRangeSize();

			return true;
		}

		void SetTexture(Material::TEXTURE_TYPE type, TexturePtr texture, uint32_t shaderBit)
		{
#ifndef VE_MATERIAL_NO_TEXTURE
			if (texture != nullptr)
			{
				if (textures[curTexturesIndex][type] != nullptr)
				{
					if (textures[curTexturesIndex][type] != texture)
					{
						VE_SET_BIT(updateFlags, Material::UPDATE_TEXTURES);

						textures[nextTexturesIndex][type] = texture;
						textureInfos[type].update = true;
					}
				}
				else
				{
					VE_SET_BIT(updateFlags, Material::UPDATE_PIPELINE);
					VE_SET_BIT(updateFlags, Material::RECREATE_DESCRIPTOR_SET);
					VE_SET_BIT(updateFlags, Material::UPDATE_TEXTURES);

					textures[nextTexturesIndex][type] = texture;
					textureInfos[type].update = true;
				}

				textureInfos[type].mipLevel = texture->GetNativeImageViewPtr()->GetDesc().levelCount - 1;
				textureInfos[type].enable = true;

				VE_SET_BIT(shaderFlags, shaderBit);
			}
			else
			{
				if (textures[curTexturesIndex][type] != nullptr)
				{
					VE_SET_BIT(updateFlags, Material::UPDATE_PIPELINE);
					VE_SET_BIT(updateFlags, Material::RECREATE_DESCRIPTOR_SET);
					VE_SET_BIT(updateFlags, Material::UPDATE_TEXTURES);

					textures[nextTexturesIndex][type] = texture;
					textureInfos[type].update = true;
				}

				textureInfos[type].mipLevel = 0;
				textureInfos[type].enable = false;

				VE_RESET_BIT(shaderFlags, shaderBit);
			}
#endif //VE_MATERIAL_NO_TEXTURE
		}

		VE_DECLARE_ALLOCATOR
	};

	/*********************/
	/* public - Material */
	/*********************/

	MaterialPtr Material::Create(DeviceContextPtr deviceContext)
	{
		MaterialPtr material = std::make_shared<Material>();

		if (material->impl->Initialize(deviceContext, nullptr) == false)
		{
			return nullptr;
		}

		return std::move(material);
	}

	MaterialPtr Material::Create(DeviceContextPtr deviceContext, const wchar_t* pName)
	{
		MaterialPtr material = std::make_shared<Material>();

		if (material->impl->Initialize(deviceContext, pName) == false)
		{
			return nullptr;
		}

		return std::move(material);
	}

	Material::Material() :
		impl(nullptr)
	{
		impl = VE_NEW_T(Material::Impl, this);
	}

	Material::~Material()
	{
		if (impl->pUniformBuffer != nullptr)
		{
			impl->pUniformBuffer->Destroy();
		}

		DeleteDeviceChild(impl->deviceContext->GetDeletingQueuePtr(), &impl->pNativeDescriptorSet[Material::DST_COLOR]);
		DeleteDeviceChild(impl->deviceContext->GetDeletingQueuePtr(), &impl->pNativeDescriptorSet[Material::DST_SHADOW]);

		for (size_t i = 0; i < 2; i++)
		{
			for (size_t j = 0; j < impl->textures[i].size(); j++)
			{
				impl->textures[i][j] = nullptr;
			}
		}

		VE_DELETE_T(impl, Impl);
	}

	uint64_t Material::GetKey() const
	{
		return impl->key;
	}

	const wchar_t* Material::GetName() const
	{
		return impl->name.c_str();
	}

	uint32_t Material::GetShaderFlags() const
	{
		return impl->shaderFlags;
	}

	const glm::vec4 Material::GetDiffuseColor() const
	{
		return impl->uniform.diffuseColor;
	}

	void Material::SetDiffuseColor(const glm::vec4& color)
	{
		impl->uniform.diffuseColor = color;
		VE_SET_BIT(impl->updateFlags, Material::UPDATE_UNIFORM_BUFFER);
	}

	float Material::GetDiffuseFactor() const
	{
		return impl->uniform.diffuseFactor;
	}

	void Material::SetDiffuseFactor(float factor)
	{
		impl->uniform.diffuseFactor = factor;
		VE_SET_BIT(impl->updateFlags, Material::UPDATE_UNIFORM_BUFFER);
	}

	float Material::GetEmissiveFactor() const
	{
		return impl->uniform.emissiveFactor;
	}

	void Material::SetEmissiveFactor(float factor)
	{
		impl->uniform.emissiveFactor = factor;
		VE_SET_BIT(impl->updateFlags, Material::UPDATE_UNIFORM_BUFFER);
	}

	float Material::GetSpecularFactor() const
	{
		return impl->uniform.specularFactor;
	}

	const glm::vec4 Material::GetSpecularColor() const
	{
		return impl->uniform.specularColor;
	}

	void Material::SetSpecularColor(const glm::vec4& color)
	{
		impl->uniform.specularColor = color;
		VE_SET_BIT(impl->updateFlags, Material::UPDATE_UNIFORM_BUFFER);
	}

	void Material::SetSpecularFactor(float factor)
	{
		impl->uniform.specularFactor = factor;
		VE_SET_BIT(impl->updateFlags, Material::UPDATE_UNIFORM_BUFFER);
	}

	float Material::GetShininess() const
	{
		return impl->uniform.shininess;
	}

	void Material::SetShininess(float shininess)
	{
		impl->uniform.shininess = std::max(1.0f, shininess);
		VE_SET_BIT(impl->updateFlags, Material::UPDATE_UNIFORM_BUFFER);
	}

	TexturePtr Material::GetDiffuseTexture()
	{
		return impl->textures[impl->curTexturesIndex][Material::TEXTURE_TYPE_DIFFUSE];
	}

	void Material::SetDiffuseTexture(TexturePtr texture)
	{
		impl->SetTexture(Material::TEXTURE_TYPE_DIFFUSE, texture, MATERIAL_SHADER_DIFFUSE_TEXTURE);
	}

	TexturePtr Material::GetSpecularTexture()
	{
		return impl->textures[impl->curTexturesIndex][Material::TEXTURE_TYPE_SPECULAR];
	}

	void Material::SetSpecularTexture(TexturePtr texture)
	{
		impl->SetTexture(Material::TEXTURE_TYPE_SPECULAR, texture, MATERIAL_SHADER_SPECULAR_TEXTURE);
	}

	TexturePtr Material::GetBumpTexture()
	{
		return impl->textures[impl->curTexturesIndex][Material::TEXTURE_TYPE_BUMP];
	}

	void Material::SetBumpTexture(TexturePtr texture)
	{
		impl->SetTexture(Material::TEXTURE_TYPE_BUMP, texture, MATERIAL_SHADER_BUMP_TEXTURE);
	}

	V3D_FILTER Material::GetTextureFilter() const
	{
		return impl->textureInfos[Material::TEXTURE_TYPE_DIFFUSE].filter;
	}

	void Material::SetTextureFilter(V3D_FILTER filter)
	{
		impl->textureInfos[Material::TEXTURE_TYPE_DIFFUSE].filter = filter;
		impl->textureInfos[Material::TEXTURE_TYPE_SPECULAR].filter = filter;

		VE_SET_BIT(impl->updateFlags, Material::UPDATE_SAMPLERS);
	}

	V3D_ADDRESS_MODE Material::GetTextureAddressMode() const
	{
		return impl->textureInfos[Material::TEXTURE_TYPE_DIFFUSE].addressMode;
	}

	void Material::SetTextureAddressMode(V3D_ADDRESS_MODE addressMode)
	{
		impl->textureInfos[Material::TEXTURE_TYPE_DIFFUSE].addressMode = addressMode;
		impl->textureInfos[Material::TEXTURE_TYPE_SPECULAR].addressMode = addressMode;
		impl->textureInfos[Material::TEXTURE_TYPE_BUMP].addressMode = addressMode;

		VE_SET_BIT(impl->updateFlags, Material::UPDATE_SAMPLERS);
	}

	V3D_POLYGON_MODE Material::GetPolygonMode() const
	{
		return impl->polygonMode;
	}

	void Material::SetPolygonMode(V3D_POLYGON_MODE polygonMode)
	{
		impl->polygonMode = polygonMode;

		VE_SET_BIT(impl->updateFlags, Material::UPDATE_PIPELINE);
	}

	V3D_CULL_MODE Material::GetCullMode() const
	{
		return impl->cullMode;
	}

	void Material::SetCullMode(V3D_CULL_MODE cullMode)
	{
		impl->cullMode = cullMode;

		VE_SET_BIT(impl->updateFlags, Material::UPDATE_PIPELINE);
	}

	BLEND_MODE Material::GetBlendMode() const
	{
		return impl->blendMode;
	}

	void Material::SetBlendMode(BLEND_MODE blendMode)
	{
		impl->blendMode = blendMode;

		if (blendMode == BLEND_MODE_COPY)
		{
			VE_RESET_BIT(impl->shaderFlags, MATERIAL_SHADER_TRANSPARENCY);
		}
		else
		{
			VE_SET_BIT(impl->shaderFlags, MATERIAL_SHADER_TRANSPARENCY);
		}

		VE_SET_BIT(impl->updateFlags, Material::UPDATE_PIPELINE);
	}

	void Material::Update()
	{
		// ----------------------------------------------------------------------------------------------------
		// パイプラインを更新
		// ----------------------------------------------------------------------------------------------------

		if (VE_TEST_BIT(impl->updateFlags, Material::UPDATE_PIPELINE))
		{
			auto it_session_begin = impl->sessions.begin();
			auto it_session_end = impl->sessions.end();

			for (auto it_session = it_session_begin; it_session != it_session_end; ++it_session)
			{
				auto it_begin = it_session->second.begin();
				auto it_end = it_session->second.end();

				for (auto it = it_begin; it != it_end; ++it)
				{
					it_session->first->UpdatePipeline(this, it->subsetIndex);
				}
			}

			impl->updateFlags = Material::UPDATE_ALL;
		}

		// ----------------------------------------------------------------------------------------------------
		// ユニフォームバッファーを更新
		// ----------------------------------------------------------------------------------------------------

		if (VE_TEST_BIT(impl->updateFlags, Material::UPDATE_UNIFORM_BUFFER))
		{
			void* pMemory = impl->pUniformBuffer->Map();
			memcpy_s(pMemory, impl->pUniformBuffer->GetNativeRangeSize(), &impl->uniform, sizeof(Material::Uniform));
			impl->pUniformBuffer->Unmap();
		}

		// ----------------------------------------------------------------------------------------------------
		// デスクリプタセットを作成
		// ----------------------------------------------------------------------------------------------------

		if (VE_TEST_BIT(impl->updateFlags, (Material::UPDATE_PIPELINE | Material::RECREATE_DESCRIPTOR_SET)))
		{
			// デスクリプタセットを破棄
			DeleteDeviceChild(impl->deviceContext->GetDeletingQueuePtr(), &impl->pNativeDescriptorSet[Material::DST_COLOR]);
			DeleteDeviceChild(impl->deviceContext->GetDeletingQueuePtr(), &impl->pNativeDescriptorSet[Material::DST_SHADOW]);

			// デスクリプタセットを作成
			impl->pNativeDescriptorSet[Material::DST_COLOR] = impl->deviceContext->GetGraphicsFactoryPtr()->CreateNativeDescriptorSet(GraphicsFactory::MPT_COLOR, impl->shaderFlags);
			VE_ASSERT(impl->pNativeDescriptorSet[Material::DST_COLOR] != nullptr);

			impl->pNativeDescriptorSet[Material::DST_SHADOW] = impl->deviceContext->GetGraphicsFactoryPtr()->CreateNativeDescriptorSet(GraphicsFactory::MPT_SHADOW, impl->shaderFlags);
			VE_ASSERT(impl->pNativeDescriptorSet[Material::DST_SHADOW] != nullptr);

			// ユニフォームバッファーをバインド
			impl->pNativeDescriptorSet[Material::DST_COLOR]->SetBuffer(Material::BINDING_UNIFORM_BUFFER, impl->pUniformBuffer->GetNativeBufferPtr(), 0, impl->pUniformBuffer->GetNativeRangeSize());
			impl->pNativeDescriptorSet[Material::DST_SHADOW]->SetBuffer(Material::BINDING_UNIFORM_BUFFER, impl->pUniformBuffer->GetNativeBufferPtr(), 0, impl->pUniformBuffer->GetNativeRangeSize());

			// すべてのテクスチャを再設定する
			for (uint32_t i = 0; i < Material::TEXTURE_TYPE_COUNT; i++)
			{
				Material::TextureInfo& info = impl->textureInfos[i];
				if (info.enable == true)
				{
					info.update = true;

					if (impl->textures[impl->nextTexturesIndex][i] == nullptr)
					{
						impl->textures[impl->nextTexturesIndex][i] = impl->textures[impl->curTexturesIndex][i];
					}
				}
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// デスクリプタセットのテクスチャを更新
		// ----------------------------------------------------------------------------------------------------

		if (VE_TEST_BIT(impl->updateFlags, Material::UPDATE_TEXTURES))
		{
			for (uint32_t i = 0; i < Material::TEXTURE_TYPE_COUNT; i++)
			{
				Material::TextureInfo& info = impl->textureInfos[i];
				if (info.update == true)
				{
					if (info.enable == true)
					{
						VE_ASSERT(impl->textures[impl->nextTexturesIndex][i] != nullptr);

						uint32_t binding = Material::BINDING_FIRST_TEXTURE + i;
						IV3DImageView* pNativeImageView = impl->textures[impl->nextTexturesIndex][i]->GetNativeImageViewPtr();

						// カラー
						impl->pNativeDescriptorSet[Material::DST_COLOR]->SetImageView(binding, pNativeImageView, V3D_IMAGE_LAYOUT_SHADER_READ_ONLY);

						// シャドウ
						if (i == Material::TEXTURE_TYPE_DIFFUSE)
						{
							impl->pNativeDescriptorSet[Material::DST_SHADOW]->SetImageView(binding, pNativeImageView, V3D_IMAGE_LAYOUT_SHADER_READ_ONLY);
						}
					}

					info.update = false;
				}
				else
				{
					impl->textures[impl->nextTexturesIndex][i] = impl->textures[impl->curTexturesIndex][i];
				}

				// 以前のテクスチャを破棄
				impl->textures[impl->curTexturesIndex][i] = nullptr;
			}

			// フリップ
			impl->curTexturesIndex = impl->nextTexturesIndex;
			impl->nextTexturesIndex = (impl->curTexturesIndex + 1) % 2;
		}

		// ----------------------------------------------------------------------------------------------------
		// デスクリプタセットのサンプラーを更新
		// ----------------------------------------------------------------------------------------------------

		if (VE_TEST_BIT(impl->updateFlags, Material::UPDATE_SAMPLERS))
		{
			SamplerFactory* pSamplerFactory = impl->deviceContext->GetSamplerFactoryPtr();

			for (uint32_t i = 0; i < Material::TEXTURE_TYPE_COUNT; i++)
			{
				const Material::TextureInfo& info = impl->textureInfos[i];
				if (info.enable == true)
				{
					IV3DSampler* pSampler = pSamplerFactory->Create(info.filter, info.addressMode, info.mipLevel, info.anisotropyEnable);
					VE_ASSERT(pSampler != nullptr);

					// カラー
					impl->pNativeDescriptorSet[Material::DST_COLOR]->SetSampler(Material::BINDING_FIRST_TEXTURE + i, pSampler);

					// シャドウ
					if (i == Material::TEXTURE_TYPE_DIFFUSE)
					{
						impl->pNativeDescriptorSet[Material::DST_SHADOW]->SetSampler(Material::BINDING_FIRST_TEXTURE + i, pSampler);
					}

					pSampler->Release();
				}
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// デスクリプタセットを更新
		// ----------------------------------------------------------------------------------------------------

		if (VE_TEST_BIT(impl->updateFlags, Material::UPDATE_DECRIPTOR_SET))
		{
			impl->pNativeDescriptorSet[Material::DST_COLOR]->Update();
			impl->pNativeDescriptorSet[Material::DST_SHADOW]->Update();
		}

		// ----------------------------------------------------------------------------------------------------

		impl->updateFlags = 0;

		// ----------------------------------------------------------------------------------------------------
	}

	/**********************/
	/* private - Material */
	/**********************/

	bool Material::Load(LoggerPtr logger, HANDLE fileHandle, const wchar_t* pDirPath)
	{
		// ----------------------------------------------------------------------------------------------------
		// ファイルヘッダーを読み込む
		// ----------------------------------------------------------------------------------------------------

		Material::File_FileHeader fileHeader;

		if (FileRead(fileHandle, sizeof(Material::File_FileHeader), &fileHeader) == false)
		{
			logger->PrintA(Logger::TYPE_ERROR, "Failed to read the file");
			return false;
		}

		if (fileHeader.magicNumber != ToMagicNumber('M', 'S', 'T', 'D'))
		{
			logger->PrintA(Logger::TYPE_ERROR, "Unsupported material in the model file");
			return false;
		}

		if (fileHeader.version != Material::CURRENT_VERSION)
		{
			logger->PrintA(Logger::TYPE_ERROR, "Unsupported material version : Version[0x%.8x] CurrentVersion[0x%.8x]", fileHeader.version, Material::CURRENT_VERSION);
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// データを読み込む
		// ----------------------------------------------------------------------------------------------------

		Material::File_Data data;

		if (FileRead(fileHandle, sizeof(Material::File_Data), &data) == false)
		{
			logger->PrintA(Logger::TYPE_ERROR, "Failed to read the file");
			return false;
		}

		impl->name = data.name;

		SetDiffuseColor(glm::vec4(data.diffuseColor[0], data.diffuseColor[1], data.diffuseColor[2], data.diffuseColor[3]));
		SetSpecularColor(glm::vec4(data.specularColor[0], data.specularColor[1], data.specularColor[2], data.specularColor[3]));
		SetDiffuseFactor(data.diffuseFactor);
		SetEmissiveFactor(data.emissiveFactor);
		SetSpecularFactor(data.specularFactor);
		SetShininess(data.shininess);

		SetPolygonMode(static_cast<V3D_POLYGON_MODE>(data.polygonMode));
		SetCullMode(static_cast<V3D_CULL_MODE>(data.cullMode));
		SetBlendMode(static_cast<BLEND_MODE>(data.blendMode));

		SetTextureFilter(static_cast<V3D_FILTER>(data.textureFilter));
		SetTextureAddressMode(static_cast<V3D_ADDRESS_MODE>(data.textureAddressMode));

		if (wcslen(data.diffuseTexture) > 0)
		{
			wchar_t filePath[1024];
			if (PathCombineW(filePath, pDirPath, data.diffuseTexture) != nullptr)
			{
				SetDiffuseTexture(Texture::Create(impl->deviceContext, filePath, V3D_PIPELINE_STAGE_FRAGMENT_SHADER, V3D_ACCESS_SHADER_READ));
			}
			else
			{
				logger->PrintA(Logger::TYPE_ERROR, "File path conversion failed : DirPath[%s] FilePath[%s]", pDirPath, data.diffuseTexture);
				return false;
			}
		}

		if (wcslen(data.specularTexture) > 0)
		{
			wchar_t filePath[1024];
			if (PathCombineW(filePath, pDirPath, data.specularTexture) != nullptr)
			{
				SetSpecularTexture(Texture::Create(impl->deviceContext, filePath, V3D_PIPELINE_STAGE_FRAGMENT_SHADER, V3D_ACCESS_SHADER_READ));
			}
			else
			{
				logger->PrintA(Logger::TYPE_ERROR, "File path conversion failed : DirPath[%s] FilePath[%s]", pDirPath, data.specularTexture);
			}
		}

		if (wcslen(data.bumpTexture) > 0)
		{
			wchar_t filePath[1024];
			if (PathCombineW(filePath, pDirPath, data.bumpTexture) != nullptr)
			{
				SetBumpTexture(Texture::Create(impl->deviceContext, filePath, V3D_PIPELINE_STAGE_FRAGMENT_SHADER, V3D_ACCESS_SHADER_READ));
			}
			else
			{
				logger->PrintA(Logger::TYPE_ERROR, "File path conversion failed : DirPath[%s] FilePath[%s]", pDirPath, data.bumpTexture);
			}
		}

		// ----------------------------------------------------------------------------------------------------

		return true;
	}

	bool Material::Save(LoggerPtr logger, HANDLE fileHandle, const wchar_t* pDirPath)
	{
		// ----------------------------------------------------------------------------------------------------
		// ファイルヘッダーを書き込む
		// ----------------------------------------------------------------------------------------------------

		Material::File_FileHeader fileHeader;
		fileHeader.magicNumber = ToMagicNumber('M', 'S', 'T', 'D');
		fileHeader.version = Material::CURRENT_VERSION;

		if (FileWrite(fileHandle, sizeof(Material::File_FileHeader), &fileHeader) == false)
		{
			logger->PrintA(Logger::TYPE_ERROR, "Failed to write the file");
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// データを書き込む
		// ----------------------------------------------------------------------------------------------------

		Material::File_Data data{};
		if (wcscpy_s(data.name, impl->name.c_str()) != 0)
		{
			logger->PrintW(Logger::TYPE_ERROR, L"The name of the material is too long. Please limit it to 255 characters or less. : MaterialName[%s]", impl->name.c_str());
			return false;
		}

		data.diffuseColor[0] = impl->uniform.diffuseColor.r;
		data.diffuseColor[1] = impl->uniform.diffuseColor.g;
		data.diffuseColor[2] = impl->uniform.diffuseColor.b;
		data.diffuseColor[3] = impl->uniform.diffuseColor.a;
		data.specularColor[0] = impl->uniform.specularColor.r;
		data.specularColor[1] = impl->uniform.specularColor.g;
		data.specularColor[2] = impl->uniform.specularColor.b;
		data.specularColor[3] = 1.0f;
		data.diffuseFactor = impl->uniform.diffuseFactor;
		data.emissiveFactor = impl->uniform.emissiveFactor;
		data.specularFactor = impl->uniform.specularFactor;
		data.shininess = impl->uniform.shininess;

		data.polygonMode = impl->polygonMode;
		data.cullMode = impl->cullMode;
		data.blendMode = impl->blendMode;
		data.reserve1[1] = 0;

		data.textureFilter = impl->textureInfos[Material::TEXTURE_TYPE_DIFFUSE].filter;
		data.textureAddressMode = impl->textureInfos[Material::TEXTURE_TYPE_DIFFUSE].addressMode;
		data.reserve2[0] = 0;

		if (GetDiffuseTexture() != nullptr)
		{
			if (ToRelativePathW(pDirPath, FILE_ATTRIBUTE_DIRECTORY, GetDiffuseTexture()->GetFilePath(), FILE_ATTRIBUTE_NORMAL, data.diffuseTexture, sizeof(data.diffuseTexture) >> 1) == false)
			{
				logger->PrintW(Logger::TYPE_ERROR, L"The faile path of the diffuse texture is too long. Please limit it to 511 characters or less. : MaterialName[%s] TextureFilePath[%s]", impl->name.c_str(), GetDiffuseTexture()->GetFilePath());
				return false;
			}
		}
		else
		{
			memset(data.diffuseTexture, 0, sizeof(data.diffuseTexture));
		}

		if (GetSpecularTexture() != nullptr)
		{
			if (ToRelativePathW(pDirPath, FILE_ATTRIBUTE_DIRECTORY, GetSpecularTexture()->GetFilePath(), FILE_ATTRIBUTE_NORMAL, data.specularTexture, sizeof(data.specularTexture) >> 1) == false)
			{
				logger->PrintW(Logger::TYPE_ERROR, L"The faile path of the specular texture is too long. Please limit it to 511 characters or less. : MaterialName[%s] TextureFilePath[%s]", impl->name.c_str(), GetSpecularTexture()->GetFilePath());
				return false;
			}
		}
		else
		{
			memset(data.specularTexture, 0, sizeof(data.specularTexture));
		}

		if (GetBumpTexture() != nullptr)
		{
			if (ToRelativePathW(pDirPath, FILE_ATTRIBUTE_DIRECTORY, GetBumpTexture()->GetFilePath(), FILE_ATTRIBUTE_NORMAL, data.bumpTexture, sizeof(data.bumpTexture) >> 1) == false)
			{
				logger->PrintW(Logger::TYPE_ERROR, L"The faile path of the bump texture is too long. Please limit it to 511 characters or less. : MaterialName[%s] TextureFilePath[%s]", impl->name.c_str(), GetBumpTexture()->GetFilePath());
				return false;
			}
		}
		else
		{
			memset(data.bumpTexture, 0, sizeof(data.bumpTexture));
		}

		if (FileWrite(fileHandle, sizeof(Material::File_Data), &data) == false)
		{
			logger->PrintA(Logger::TYPE_ERROR, "Failed to write the file");
			return false;
		}

		// ----------------------------------------------------------------------------------------------------

		return true;
	}

	void Material::Connect(IMesh* pMesh, uint32_t subsetIndex)
	{
		Material::Session session;
		session.subsetIndex = subsetIndex;

		auto it = impl->sessions.find(pMesh);
		if (it != impl->sessions.end())
		{
			it->second.push_back(session);
		}
		else
		{
			impl->sessions[pMesh].push_back(session);
		}
	}

	void Material::Disconnect(IMesh* pMesh)
	{
		auto it = impl->sessions.find(pMesh);
		if (it != impl->sessions.end())
		{
			impl->sessions.erase(it);
		}
	}

	uint32_t Material::GetDynamicOffset() const
	{
		return impl->dynamicOffset;
	}

	IV3DDescriptorSet* Material::GetNativeDescriptorSetPtr(Material::DESCRIPTOR_SET_TYPE type)
	{
		return impl->pNativeDescriptorSet[type];
	}

}