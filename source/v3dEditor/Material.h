#pragma once

namespace ve {

	class Model;
	class DynamicBuffer;
	class IMesh;

	class Material final
	{
	public:
		enum DESCRIPTOR_SET_TYPE
		{
			DST_COLOR = 0,
			DST_SHADOW = 1,
		};

		static MaterialPtr Create(DeviceContextPtr deviceContext);
		static MaterialPtr Create(DeviceContextPtr deviceContext, const wchar_t* pName);

		Material();
		~Material();

		uint64_t GetKey() const;

		const wchar_t* GetName() const;

		uint32_t GetShaderFlags() const;

		const glm::vec4 GetDiffuseColor() const;
		void SetDiffuseColor(const glm::vec4& color);

		float GetDiffuseFactor() const;
		void SetDiffuseFactor(float factor);

		float GetEmissiveFactor() const;
		void SetEmissiveFactor(float factor);

		const glm::vec4 GetSpecularColor() const;
		void SetSpecularColor(const glm::vec4& color);

		float GetSpecularFactor() const;
		void SetSpecularFactor(float factor);

		float GetShininess() const;
		void SetShininess(float shininess);

		TexturePtr GetDiffuseTexture();
		void SetDiffuseTexture(TexturePtr texture);

		TexturePtr GetSpecularTexture();
		void SetSpecularTexture(TexturePtr texture);

		TexturePtr GetBumpTexture();
		void SetBumpTexture(TexturePtr texture);

		V3D_FILTER GetTextureFilter() const;
		void SetTextureFilter(V3D_FILTER filter);

		V3D_ADDRESS_MODE GetTextureAddressMode() const;
		void SetTextureAddressMode(V3D_ADDRESS_MODE addressMode);

		V3D_POLYGON_MODE GetPolygonMode() const;
		void SetPolygonMode(V3D_POLYGON_MODE polygonMode);

		V3D_CULL_MODE GetCullMode() const;
		void SetCullMode(V3D_CULL_MODE cullMode);

		BLEND_MODE GetBlendMode() const;
		void SetBlendMode(BLEND_MODE blendMode);

		void Update();

		VE_DECLARE_ALLOCATOR

	private:
		// ----------------------------------------------------------------------------------------------------

		static constexpr uint32_t CURRENT_VERSION = 0x01000000;

		struct File_FileHeader
		{
			uint32_t magicNumber;
			uint32_t version;
		};

		struct File_Data
		{
			wchar_t name[256];

			float diffuseColor[4];
			float specularColor[4];
			float diffuseFactor;
			float emissiveFactor;
			float specularFactor;
			float shininess;

			uint8_t polygonMode;
			uint8_t cullMode;
			uint8_t blendMode;
			uint8_t reserve1[1];

			uint8_t textureFilter;
			uint8_t textureAddressMode;
			uint8_t reserve2[2];

			wchar_t diffuseTexture[512];
			wchar_t specularTexture[512];
			wchar_t bumpTexture[512];
		};

		// ----------------------------------------------------------------------------------------------------

		enum BINDING
		{
			BINDING_UNIFORM_BUFFER = 0,
			BINDING_DIFFUSE_TEXTURE = 1,
			BINDING_SPECULAR_TEXTURE = 2,
			BINDING_BUMP_TEXTURE = 3,

			BINDING_FIRST_TEXTURE = BINDING_DIFFUSE_TEXTURE,
		};

		enum TEXTURE_TYPE
		{
			TEXTURE_TYPE_DIFFUSE = 0,
			TEXTURE_TYPE_SPECULAR = 1,
			TEXTURE_TYPE_BUMP = 2,

			TEXTURE_TYPE_COUNT = 3,
		};

		enum UPDATE_FLAG : uint8_t
		{
			UPDATE_PIPELINE = 0x01,
			UPDATE_UNIFORM_BUFFER = 0x02,
			RECREATE_DESCRIPTOR_SET = 0x04,
			UPDATE_TEXTURES = 0x08,
			UPDATE_SAMPLERS = 0x10,

			UPDATE_ALL = UPDATE_PIPELINE | UPDATE_UNIFORM_BUFFER | RECREATE_DESCRIPTOR_SET | UPDATE_TEXTURES | UPDATE_SAMPLERS,
			UPDATE_DECRIPTOR_SET = UPDATE_PIPELINE | RECREATE_DESCRIPTOR_SET | UPDATE_TEXTURES | UPDATE_SAMPLERS,
		};

		struct TextureInfo
		{
			V3D_FILTER filter;
			V3D_ADDRESS_MODE addressMode;
			uint32_t mipLevel;
			bool anisotropyEnable;
			bool enable;
			bool update;
		};

		struct Uniform
		{
			glm::vec4 diffuseColor;
			glm::vec4 specularColor;
			float diffuseFactor;
			float specularFactor;
			float shininess;
			float emissiveFactor;
		};

		struct Session
		{
			uint32_t subsetIndex;
			uint32_t type;
		};

		// ----------------------------------------------------------------------------------------------------

		struct Impl;
		Impl* impl;

		// ----------------------------------------------------------------------------------------------------

		bool Load(LoggerPtr logger, HANDLE fileHandle, const wchar_t* pDirPath);
		bool Save(LoggerPtr logger, HANDLE fileHandle, const wchar_t* pDirPath);

		void Connect(IMesh* pMesh, uint32_t subsetIndex);
		void Disconnect(IMesh* pMesh);

		uint32_t GetDynamicOffset() const;
		IV3DDescriptorSet* GetNativeDescriptorSetPtr(Material::DESCRIPTOR_SET_TYPE type);

		friend class SkeletalMesh;
		friend class SkeletalModel;
	};

}