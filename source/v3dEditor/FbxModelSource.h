#pragma once

#include "fbxsdk.h"
#include "IModelSource.h"

namespace ve {

	class FbxModelSource final : public IModelSource
	{
	public:
		static ModelSourcePtr Create();

		FbxModelSource();
		virtual ~FbxModelSource();

		bool Load(LoggerPtr logger, DeviceContextPtr deviceContext, const wchar_t* pFilePath, const ModelSourceConfig& config) override;

		const wchar_t* GetFilePath() const override;
		const collection::Vector<IModelSource::Material>& GetMaterials() const override;
		const collection::Vector<IModelSource::Polygon>& GetPolygons() const override;
		const collection::Vector<IModelSource::Node>& GetNodes() const override;
		size_t GetEmptyNodeCount() const override;
		size_t GetMeshNodeCount() const override;

		VE_DECLARE_ALLOCATOR

	private:
		struct BoneWeight
		{
			uint8_t index;
			float value;
		};

		typedef collection::Vector<collection::Vector<std::array<int32_t, 2>>> PolygonVertexRefVector;

		StringW m_FilePath;
		collection::Vector<IModelSource::Material> m_Materials;
		collection::Vector<IModelSource::Polygon> m_Polygons;
		collection::Vector<IModelSource::Node> m_Nodes;
		size_t m_MeshNodeCount;

		bool Load(LoggerPtr logger, FbxScene* pFbxScene, const ModelSourceConfig& config);
		bool LoadNodes(LoggerPtr logger, FbxNode* pFbxNode, int32_t parentNodeIndex, const ModelSourceConfig& config);
		bool LoadNodeMaterial(FbxNode* pFbxNode, IModelSource::Node& node, const ModelSourceConfig& config);
		bool LoadMesh(LoggerPtr logger, FbxNode* pFbxNode, FbxMesh* pFbxMesh, IModelSource::Node& node, int32_t nodeIndex, const ModelSourceConfig& config);
		bool LoadMeshUV(FbxLayerElementUV* pFbxUVs, size_t firstPolygonIndex, size_t polygonCount, const FbxModelSource::PolygonVertexRefVector& polygonVertexRefs);
		bool LoadMeshNormal(FbxLayerElementNormal* pFbxNormals, size_t firstPolygonIndex, size_t polygonCount, const FbxModelSource::PolygonVertexRefVector& polygonVertexRefs);
		bool LoadMeshMaterial(FbxNode* pFbxNode, FbxLayerElementMaterial* pFbxMaterials, size_t firstPolygonIndex, size_t polygonCount, const FbxModelSource::PolygonVertexRefVector& polygonVertexRefs);

		static void CreateOBB(
			collection::Vector<glm::vec3>& points,
			glm::vec3& center, glm::vec3& xAxis, glm::vec3& yAxis, glm::vec3& zAxis, glm::vec3& halfExtent);

		static float Jacobi_GetMax(const float mat[3][3], int32_t& p, int32_t& q);

		static int32_t GetMaterialIndex(FbxNode* pFbxNode, int32_t fbxMaterialIndex, collection::Vector<IModelSource::Material>& materials);
		static void GetMaterialTexture(const FbxProperty& prop, MODEL_SOURCE_PATH_TYPE pathType, StringW& texture);

		static glm::mat4 ToGlmMatrix(const FbxAMatrix& fbxMatrix);
		static float ToLuminance(const FbxDouble3& color);
	};

}
