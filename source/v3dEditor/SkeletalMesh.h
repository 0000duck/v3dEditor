#pragma once

#include "IMesh.h"
#include "IModelSource.h"
#include "GraphicsFactory.h"
#include "DynamicContainer.h"
#include "Plane.h"
#include "Node.h"

namespace ve {

	class DynamicBuffer;
	class Node;
	class Material;
	class DebugRenderer;

	class SkeletalMesh final : public IMesh
	{
	public:
		struct Vertex
		{
			glm::vec3 pos;
			glm::vec2 uv;
			glm::vec3 normal;
			glm::vec3 tangent;
			glm::vec3 binormal;
			glm::ivec4 indices;
			glm::vec4 weights;
		};

		static SkeletalMeshPtr Create(DeviceContextPtr deviceContext, int32_t id);

		SkeletalMesh();
		virtual ~SkeletalMesh();

		int32_t GetID() const;

		bool Load(LoggerPtr logger, IModel* pModel, HANDLE fileHandle);
		bool Save(LoggerPtr logger, HANDLE fileHandle);

		bool Preparation(size_t boneCount);
		void AssignMaterials(const collection::Vector<int32_t>& materialIndices);
		bool BuildVertexIndexData(LoggerPtr logger, collection::Vector<IModelSource::Polygon>& polygons, uint32_t firstPolygon, uint32_t polygonCount, const ModelRendererConfig& config, int32_t id, int32_t lastID);
		void AddShape(const glm::vec3& center, const glm::vec3& xAxis, const glm::vec3& yAxis, const glm::vec3& zAxis, const glm::vec3& halfExtent);
		void AddBone(NodePtr node, const glm::mat4& offsetMatrix);

		void ConnectMaterials(collection::Vector<MaterialPtr>& materials);
		void DisconnectMaterials(collection::Vector<MaterialPtr>& materials);

		void SetOwnerModel(ModelPtr model);

		void Draw(
			const Plane& nearPlane,
			uint32_t frameIndex,
			collection::Vector<MaterialPtr>& materials,
			collection::DynamicContainer<OpacityDrawSet>& opacityDrawSets,
			collection::DynamicContainer<TransparencyDrawSet>& transparencyDrawSets);

		void DrawShadow(
			uint32_t frameIndex,
			collection::Vector<MaterialPtr>& materials,
			collection::DynamicContainer<ShadowDrawSet>& shadowDrawSets);

		void DebugDraw(uint32_t flags, DebugRenderer* pDebugRenderer);

		void Dispose();

		VE_DECLARE_ALLOCATOR

		/*********/
		/* IMesh */
		/*********/

		ModelPtr GetModel() override;

		uint32_t GetPolygonCount() const override;
		uint32_t GetBoneCount() const override;
		uint32_t GetMaterialCount() const override;
		MaterialPtr GetMaterial(uint32_t materialIndex) override;
		const AABB& GetAABB() const override;

		bool GetVisible() const override;
		void SetVisible(bool enable) override;
		bool GetCastShadow() const override;
		void SetCastShadow(bool enable) override;

		/******************/
		/* NodeAttribute */
		/******************/

		NodeAttribute::TYPE GetType() const override;
		void Update(const glm::mat4& worldMatrix) override;

	protected:
		/*********/
		/* IMesh */
		/*********/

		void UpdatePipeline(Material* pMaterial, uint32_t subsetIndex) override;

		/******************/
		/* NodeAttribute */
		/******************/

		bool IsSelectSupported() const override;
		void SetSelectKey(uint32_t key) override;
		void DrawSelect(uint32_t frameIndex, SelectDrawSet& drawSet) override;

	private:
		// ----------------------------------------------------------------------------------------------------

		static constexpr uint32_t OLD_VERSION_1_0_0_0 = 0x01000000;
		static constexpr uint32_t CURRENT_VERSION = 0x01010000;

		enum File_INFO_FLAGS
		{
			File_INFO_VISIBLE = 0x00000001,
			File_INFO_CAST_SHADOW = 0x00000002,

			File_INFO_ALL_FLAGS = File_INFO_VISIBLE | File_INFO_CAST_SHADOW,
		};

		struct File_FileHeader
		{
			uint32_t magicNumber;
			uint32_t version;
		};

		struct File_InfoHeader_1_0_0_0
		{
			uint32_t polygonCount;
			uint32_t materialCount;
			uint32_t debriVertexCount;
			uint32_t colorSubsetCount;
			uint32_t boneCount;
			uint32_t shapeCount;
			uint64_t vertexBufferSize;
			uint64_t indexBufferSize;
			uint32_t indexType;
		};

		struct File_InfoHeader
		{
			uint32_t flags;
			uint32_t polygonCount;
			uint32_t materialCount;
			uint32_t debriVertexCount;
			uint32_t colorSubsetCount;
			uint32_t boneCount;
			uint32_t shapeCount;
			uint64_t vertexBufferSize;
			uint64_t indexBufferSize;
			uint32_t indexType;
		};

		struct File_DebriVertex
		{
			float pos[4];
			uint32_t worldMatrixIndex;
		};

		struct File_ColorSubset
		{
			uint32_t materialIndex;
			uint32_t debriVertexCount;
			uint32_t firstDebriVertex;
			uint32_t indexCount;
			uint32_t firstIndex;
		};

		struct File_SelectSubset
		{
			uint32_t indexCount;
			uint32_t firstIndex;
		};

		struct File_Bone
		{
			int32_t nodeIndex;
			float offsetMatrix[4][4];
		};

		struct File_Shape
		{
			float center[4];
			float xAxis[4];
			float yAxis[4];
			float zAxis[4];
			float halfExtent[4];
		};

		// ----------------------------------------------------------------------------------------------------

		struct Optimize_Face
		{
			uint32_t indices[3];
		};

		struct Optimize_Subset
		{
			uint32_t materialIndex;

			uint32_t firstIndex;
			uint32_t indexCount;

			uint32_t firstFace;
			uint32_t faceCount;
			std::vector<IModelSource::Vertex> vertices;
			std::vector<uint32_t> indices;
		};

		struct Optimize_Index
		{
			uint32_t index;
			bool assigned;
		};

		// ----------------------------------------------------------------------------------------------------

		enum DESCRIPTOR_SET_TYPE
		{
			DST_COLOR = 0,
			DST_SHADOW = 1,
		};

		struct DebriVertex
		{
			glm::vec4 pos;

			uint32_t worldMatrixIndex;
			glm::mat4* pWorldMatrix;
		};

		struct DebriPolygon
		{
			DebriVertex vertices[3];
		};

		struct ColorSubset
		{
			uint32_t materialIndex; // m_MaterialIndices を参照するインデックスではなく、SimpleModelRenderer 内のマテリアルを参照するインデックス

			PipelineHandlePtr pipelineHandle;
			PipelineHandlePtr shadowPipelineHandle;

			uint32_t indexCount;
			uint32_t firstIndex;

			collection::Vector<DebriPolygon> debriPolygon;
		};

		struct SelectSubset
		{
			PipelineHandlePtr pipelineHandle;

			uint32_t indexCount;
			uint32_t firstIndex;
		};

		struct Bone
		{
			WeakPtr<Node> node;
			glm::mat4 offsetMatrix;
		};

		struct Shape
		{
			glm::vec3 center;
			glm::vec3 axis[3];
			glm::vec3 halfExtent;
		};

		// ----------------------------------------------------------------------------------------------------

		DeviceContextPtr m_DeviceContext;
		WeakPtr<IModel> m_OwnerModel;
		int32_t m_ID;

		MeshUniform m_Uniform;
		DynamicBuffer* m_pUniformBuffer;
		IV3DDescriptorSet* m_pNativeDescriptorSet[2];

		uint64_t m_VertexBufferSize;
		Buffer m_VertexBuffer;

		uint64_t m_IndexBufferSize;
		Buffer m_IndexBuffer;
		V3D_INDEX_TYPE m_IndexType;

		uint32_t m_PolygonCount;
		collection::Vector<SkeletalMesh::ColorSubset> m_ColorSubsets;
		SkeletalMesh::SelectSubset m_SelectSubset;

		collection::Vector<uint32_t> m_MaterialIndices;

		collection::Vector<SkeletalMesh::Bone> m_Bones;
		collection::Vector<SkeletalMesh::Shape> m_Shapes;
		collection::Vector<glm::mat4> m_WorldMatrices;

		AABB m_AABB;

		bool m_Visible;
		bool m_CastShadow;

		// ----------------------------------------------------------------------------------------------------

		void OptimizeVertexIndexData(
			LoggerPtr logger,
			collection::Vector<IModelSource::Polygon>& polygons, uint32_t firstPolygon, uint32_t polygonCount,
			bool smoosingEnable, float smoosingCos,
			int32_t lastID,
			collection::Vector<SkeletalMesh::Vertex>& vertices, collection::Vector<uint32_t>& indices);

		void NotOptimizeVertexIndexData(
			LoggerPtr logger,
			collection::Vector<IModelSource::Polygon>& polygons, uint32_t firstPolygon, uint32_t polygonCount,
			int32_t lastID,
			collection::Vector<SkeletalMesh::Vertex>& vertices, collection::Vector<uint32_t>& indices);

		static inline void UpdateAABB(const SkeletalMesh::Shape* pShape, const glm::mat4& worldMatrix, AABB& aabb)
		{
			glm::mat3 normalMatrix = worldMatrix;

			glm::vec3 points[8];

			glm::vec3 center = worldMatrix * glm::vec4(pShape->center, 1.0f);

			const glm::vec3& halfExtent = pShape->halfExtent;
			glm::vec3 vx = normalMatrix * (pShape->axis[0] * halfExtent.x);
			glm::vec3 vy = normalMatrix * (pShape->axis[1] * halfExtent.y);
			glm::vec3 vz = normalMatrix * (pShape->axis[2] * halfExtent.z);

			points[0] = center - vx + vy + vz;
			points[1] = center + vx + vy + vz;
			points[2] = center + vx + vy - vz;
			points[3] = center - vx + vy - vz;
			points[4] = center - vx - vy + vz;
			points[5] = center + vx - vy + vz;
			points[6] = center + vx - vy - vz;
			points[7] = center - vx - vy - vz;

			glm::vec3* pPoint = &points[0];
			glm::vec3* pPointEnd = pPoint + 8;

			while (pPoint != pPointEnd)
			{
				aabb.minimum = glm::min(aabb.minimum, *pPoint);
				aabb.maximum = glm::max(aabb.maximum, *pPoint);
				pPoint++;
			}
		}
	};

}