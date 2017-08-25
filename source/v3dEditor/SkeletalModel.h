#pragma once

#include "IModel.h"
#include "Frustum.h"
#include "Sphere.h"

namespace ve {

	class SkeletalModel final : public IModel
	{
	public:
		static SkeletalModelPtr Create(DeviceContextPtr deviceContext);

		SkeletalModel();
		~SkeletalModel();

		static void Finish(LoggerPtr logger, SkeletalModelPtr modelRenderer);

		/******************/
		/* Model */
		/******************/

		bool Load(LoggerPtr logger, ModelSourcePtr source, const ModelRendererConfig& config) override;
		bool Load(LoggerPtr logger, const wchar_t* pFilePath) override;
		bool Save(LoggerPtr logger) override;

		const wchar_t* GetFilePath() const override;

		NodePtr GetRootNode() override;

		uint32_t GetNodeCount() const override;
		NodePtr GetNode(uint32_t nodeIndex) override;

		uint32_t GetMaterialCount() const override;
		MaterialPtr GetMaterial(uint32_t materialIndex) override;

		uint32_t GetMeshCount() const override;
		MeshPtr GetMesh(uint32_t meshIndex) override;

		uint32_t GetPolygonCount() const override;

		/******************/
		/* NodeAttribute */
		/******************/

		NodeAttribute::TYPE GetType() const override;
		void Update(const glm::mat4& worldMatrix) override;

		VE_DECLARE_ALLOCATOR

	protected:
		bool Draw(
			const Frustum& frustum,
			uint32_t frameIndex,
			collection::DynamicContainer<OpacityDrawSet>& opacityDrawSets,
			collection::DynamicContainer<TransparencyDrawSet>& transparencyDrawSets) override;

		void DrawShadow(
			const Sphere& sphere,
			uint32_t frameIndex,
			collection::DynamicContainer<ShadowDrawSet>& shadowDrawSets,
			AABB& aabb) override;

		void DebugDraw(uint32_t flags, DebugRenderer* pDebugRenderer) override;

	private:
		// ----------------------------------------------------------------------------------------------------

		static constexpr uint32_t CURRENT_VERSION = 0x01000000;

		struct File_FileHeader
		{
			uint32_t magicNumber;
			uint32_t version;
		};

		struct File_InfoHeader
		{
			uint32_t nodeCount;
			uint32_t materialCount;
			uint32_t meshCount;
		};

		struct File_Node
		{
			wchar_t name[256];
			int32_t parentIndex;
			int32_t meshIndex;
			float localScale[4];
			float localRotation[4];
			float localTranslation[4];
		};

		// ----------------------------------------------------------------------------------------------------

		DeviceContextPtr m_DeviceContext;

		StringW m_FilePath;

		WeakPtr<Node> m_OwnerNode;

		collection::Vector<NodePtr> m_Nodes;
		collection::Vector<MaterialPtr> m_Materials;
		collection::Vector<SkeletalMeshPtr> m_Meshes;
		uint32_t m_PolygonCount;

		bool Load(LoggerPtr logger, HANDLE fileHandle, const wchar_t* pDirPath);
		bool Save(LoggerPtr logger, HANDLE fileHandle, const wchar_t* pDirPath);
	};

}