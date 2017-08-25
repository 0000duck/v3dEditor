#pragma once

namespace ve {

	class IModelSource
	{
	public:
		struct Vertex
		{
			glm::vec3 pos;
			glm::vec2 uv;
			glm::vec3 tangent;
			glm::vec3 binormal;
			glm::vec3 normal;
			uint8_t indices[4];
			float weights[4];
		};

		struct Polygon
		{
			IModelSource::Vertex vertices[3];
			int32_t materialIndex;
		};

		struct Material
		{
			StringW name;

			glm::vec4 diffuseColor;
			float diffuseFactor;

			glm::vec4 specularColor;
			float specularFactor;
			float shininess;

			float emissiveFactor;

			StringW diffuseTexture;
			StringW specularTexture;
			StringW bumpTexture;

			static Material Default(const wchar_t* pName)
			{
				Material ret;
				ret.name = pName;

				ret.diffuseColor.r = 1.0f;
				ret.diffuseColor.g = 1.0f;
				ret.diffuseColor.b = 1.0f;
				ret.diffuseColor.a = 1.0f;

				ret.diffuseFactor = 1.0f;

				ret.specularColor.r = 1.0f;
				ret.specularColor.g = 1.0f;
				ret.specularColor.b = 1.0f;
				ret.specularColor.a = 1.0f;

				ret.specularFactor = 0.0f;

				ret.shininess = 1.0f;

				ret.emissiveFactor = 0.0f;

				return ret;
			}
		};

		struct Bone
		{
			StringW nodeName;
			int32_t nodeIndex;
			glm::mat4 offsetMatrix;
		};

		struct Box
		{
			glm::vec3 center;
			glm::vec3 axis[3];
			glm::vec3 halfExtent;
		};

		struct Node
		{
			StringW name;
			int32_t parentIndex;

			glm::vec3 localTranslation;
			glm::quat localRotation;
			glm::vec3 localScaling;

			collection::Vector<IModelSource::Bone> bones;

			collection::Vector<IModelSource::Box> boxes;

			collection::Vector<int32_t> materialIndices;

			uint32_t firstPolygonIndex;
			uint32_t polygonCount;

			bool hasUV;
		};

		virtual bool Load(LoggerPtr logger, DeviceContextPtr deviceContext, const wchar_t* pFilePath, const ModelSourceConfig& config) = 0;

		virtual const wchar_t* GetFilePath() const = 0;
		virtual const collection::Vector<IModelSource::Material>& GetMaterials() const = 0;
		virtual const collection::Vector<IModelSource::Polygon>& GetPolygons() const = 0;
		virtual const collection::Vector<IModelSource::Node>& GetNodes() const = 0;
		virtual size_t GetEmptyNodeCount() const = 0;
		virtual size_t GetMeshNodeCount() const = 0;

	protected:
		virtual ~IModelSource() {}
	};

}