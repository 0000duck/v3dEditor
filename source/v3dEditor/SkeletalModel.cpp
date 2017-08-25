#include "SkeletalModel.h"
#include "Logger.h"
#include "ResourceMemoryManager.h"
#include "ImmediateContext.h"
#include "DeviceContext.h"
#include "DeletingQueue.h"
#include "GraphicsFactory.h"
#include "DynamicBuffer.h"
#include "Node.h"
#include "Texture.h"
#include "Material.h"
#include "SkeletalMesh.h"
#include "IModelSource.h"

namespace ve {

	/**********************************/
	/* public - SkeletalModel */
	/**********************************/

	SkeletalModelPtr SkeletalModel::Create(DeviceContextPtr deviceContext)
	{
		SkeletalModelPtr modelRenderer = std::make_shared<SkeletalModel>();
		modelRenderer->m_DeviceContext = deviceContext;

		return std::move(modelRenderer);
	}

	SkeletalModel::SkeletalModel() :
		m_PolygonCount(0)
	{
	}

	SkeletalModel::~SkeletalModel()
	{
		if(m_Meshes.empty() == false)
		{
			auto it_begin = m_Meshes.begin();
			auto it_end = m_Meshes.end();

			for (auto it = it_begin; it != it_end; ++it)
			{
				(*it)->DisconnectMaterials(m_Materials);
				(*it)->Dispose();
			}
		}
	}

	bool SkeletalModel::Load(LoggerPtr logger, ModelSourcePtr source, const ModelRendererConfig& config)
	{
		// ----------------------------------------------------------------------------------------------------
		// ファイルパス
		// ----------------------------------------------------------------------------------------------------

		RemoveFileExtensionW(source->GetFilePath(), m_FilePath);

		// ----------------------------------------------------------------------------------------------------
		// マテリアルを作成
		// ----------------------------------------------------------------------------------------------------

		{
			auto materials = source->GetMaterials();

			auto it_begin = materials.begin();
			auto it_end = materials.end();

			m_Materials.reserve(materials.size());

			for (auto it = it_begin; it != it_end; ++it)
			{
				auto& srcMaterial = (*it);

				MaterialPtr dstMaterial = Material::Create(m_DeviceContext, srcMaterial.name.c_str());

				dstMaterial->SetDiffuseColor(srcMaterial.diffuseColor);
				dstMaterial->SetSpecularColor(srcMaterial.specularColor);
				dstMaterial->SetDiffuseFactor(srcMaterial.diffuseFactor);
				dstMaterial->SetEmissiveFactor(srcMaterial.emissiveFactor);
				dstMaterial->SetSpecularFactor(srcMaterial.specularFactor);
				dstMaterial->SetShininess(srcMaterial.shininess);

				if (srcMaterial.diffuseTexture.empty() == false)
				{
					StringW filePath;

					if (IsRelativePath(srcMaterial.diffuseTexture.c_str()) == true)
					{
						GetDirectoryPath(source->GetFilePath(), filePath);
						filePath += srcMaterial.diffuseTexture;
					}
					else
					{
						filePath = srcMaterial.diffuseTexture;
					}

					dstMaterial->SetDiffuseTexture(Texture::Create(m_DeviceContext, filePath.c_str()));
				}

				if (srcMaterial.specularTexture.empty() == false)
				{
					StringW filePath;

					if (IsRelativePath(srcMaterial.specularTexture.c_str()) == true)
					{
						GetDirectoryPath(source->GetFilePath(), filePath);
						filePath += srcMaterial.specularTexture;
					}
					else
					{
						filePath = srcMaterial.specularTexture;
					}

					dstMaterial->SetSpecularTexture(Texture::Create(m_DeviceContext, filePath.c_str()));
				}

				if (srcMaterial.bumpTexture.empty() == false)
				{
					StringW filePath;

					if (IsRelativePath(srcMaterial.bumpTexture.c_str()) == true)
					{
						GetDirectoryPath(source->GetFilePath(), filePath);
						filePath += srcMaterial.bumpTexture;
					}
					else
					{
						filePath = srcMaterial.bumpTexture;
					}

					dstMaterial->SetBumpTexture(Texture::Create(m_DeviceContext, filePath.c_str()));
				}

				m_Materials.push_back(dstMaterial);
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// ノードツリーを構築
		// ----------------------------------------------------------------------------------------------------

		collection::Vector<IModelSource::Polygon> polygons(source->GetPolygons().begin(), source->GetPolygons().end());

		const auto& nodes = source->GetNodes();

		if (nodes.empty() == false)
		{
			int32_t lastMeshID = static_cast<int32_t>(source->GetMeshNodeCount()) - 1;
			int32_t meshID = 0;
			int32_t nodeID = 0;

			auto it_begin = nodes.begin();
			auto it_end = nodes.end();

			for (auto it = it_begin; it != it_end; ++it)
			{
				const IModelSource::Node& srcNode = *it;

				NodePtr dstNode = Node::Create(nodeID);

				dstNode->SetName(srcNode.name.c_str());
				dstNode->SetLocalTransform(srcNode.localScaling, srcNode.localRotation, srcNode.localTranslation);

				if (srcNode.parentIndex >= 0)
				{
					Node::AddChild(m_Nodes[srcNode.parentIndex], dstNode);
				}

				if (srcNode.polygonCount > 0)
				{
					SkeletalMeshPtr mesh = SkeletalMesh::Create(m_DeviceContext, meshID);

					// 初期化
					mesh->AssignMaterials(srcNode.materialIndices);
					mesh->Preparation(srcNode.bones.size());

					// バーテックス、インデックスデータを作成
					if (mesh->BuildVertexIndexData(logger, polygons, srcNode.firstPolygonIndex, srcNode.polygonCount, config, meshID, lastMeshID) == false)
					{
						return nullptr;
					}

					// シェイプを追加
					auto it_shape_begin = srcNode.boxes.begin();
					auto it_shape_end = srcNode.boxes.end();
					for (auto it_shape = it_shape_begin; it_shape != it_shape_end; ++it_shape)
					{
						mesh->AddShape(it_shape->center, it_shape->axis[0], it_shape->axis[1], it_shape->axis[2], it_shape->halfExtent);
					}

					// マテリアルに接続
					mesh->ConnectMaterials(m_Materials);

					// ノードに設定
					Node::SetAttribute(dstNode, mesh);

					// メッシュリストに追加
					m_Meshes.push_back(mesh);
					meshID++;

					m_PolygonCount += mesh->GetPolygonCount();
				}

				m_Nodes.push_back(dstNode);
				nodeID++;
			}
		}
		else
		{
logger->PrintA(Logger::TYPE_ERROR, "The node does not exist");
return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// ボーンを設定
		// ----------------------------------------------------------------------------------------------------

		if (m_Meshes.empty() == false)
		{
			auto it_begin = m_Meshes.begin();
			auto it_end = m_Meshes.end();

			for (auto it = it_begin; it != it_end; ++it)
			{
				SkeletalMeshPtr dstMesh = (*it);

				VE_ASSERT(dstMesh->GetOwner()->GetID() >= 0);

				auto& srcBones = nodes[dstMesh->GetOwner()->GetID()].bones;

				if (srcBones.empty() == false)
				{
					auto it_bone_begin = srcBones.begin();
					auto it_bone_end = srcBones.end();

					for (auto it_bone = it_bone_begin; it_bone != it_bone_end; ++it_bone)
					{
						dstMesh->AddBone(m_Nodes[it_bone->nodeIndex], it_bone->offsetMatrix);
					}
				}
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// マテリアルを更新
		// ----------------------------------------------------------------------------------------------------

		{
			auto it_material_begin = m_Materials.begin();
			auto it_material_end = m_Materials.end();

			for (auto it_material = it_material_begin; it_material != it_material_end; ++it_material)
			{
				(*it_material)->Update();
			}
		}

		// ----------------------------------------------------------------------------------------------------

		return true;
	}

	bool SkeletalModel::Load(LoggerPtr logger, const wchar_t* pFilePath)
	{
		m_FilePath = pFilePath;

		HANDLE fileHandle = CreateFile(m_FilePath.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (fileHandle == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		StringW dirPath;
		if (RemoveFileSpecW(m_FilePath.c_str(), dirPath) == false)
		{
			return false;
		}

		if (Load(logger, fileHandle, dirPath.c_str()) == false)
		{
			CloseHandle(fileHandle);
			return false;
		}

		CloseHandle(fileHandle);

		return true;
	}

	bool SkeletalModel::Save(LoggerPtr logger)
	{
		HANDLE fileHandle = CreateFile(m_FilePath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (fileHandle == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		StringW dirPath;
		if (RemoveFileSpecW(m_FilePath.c_str(), dirPath) == false)
		{
			return false;
		}

		if (Save(logger, fileHandle, dirPath.c_str()) == false)
		{
			CloseHandle(fileHandle);
			return false;
		}

		CloseHandle(fileHandle);

		return true;
	}

	void SkeletalModel::Finish(LoggerPtr logger, SkeletalModelPtr model)
	{
		// ----------------------------------------------------------------------------------------------------
		// メッシュにモデルを設定
		// ----------------------------------------------------------------------------------------------------

		auto it_mesh_begin = model->m_Meshes.begin();
		auto it_mesh_end = model->m_Meshes.end();

		for (auto it_mesh = it_mesh_begin; it_mesh != it_mesh_end; ++it_mesh)
		{
			(*it_mesh)->SetOwnerModel(model);
		}

		logger->PrintA(Logger::TYPE_INFO, "Loading Completed!");
	}

	/************************************/
	/* public override - Model */
	/************************************/

	const wchar_t* SkeletalModel::GetFilePath() const
	{
		return m_FilePath.c_str();
	}

	NodePtr SkeletalModel::GetRootNode()
	{
		return m_Nodes[0];
	}

	uint32_t SkeletalModel::GetNodeCount() const
	{
		return static_cast<uint32_t>(m_Nodes.size());
	}

	NodePtr SkeletalModel::GetNode(uint32_t nodeIndex)
	{
		return m_Nodes[nodeIndex];
	}

	uint32_t SkeletalModel::GetMaterialCount() const
	{
		return static_cast<uint32_t>(m_Materials.size());
	}

	MaterialPtr SkeletalModel::GetMaterial(uint32_t materialIndex)
	{
		return m_Materials[materialIndex];
	}

	uint32_t SkeletalModel::GetMeshCount() const
	{
		return static_cast<uint32_t>(m_Meshes.size());
	}

	MeshPtr SkeletalModel::GetMesh(uint32_t meshIndex)
	{
		return m_Meshes[meshIndex];
	}

	uint32_t SkeletalModel::GetPolygonCount() const
	{
		return m_PolygonCount;
	}

	/**************************/
	/* public override - Node */
	/**************************/

	NodeAttribute::TYPE SkeletalModel::GetType() const
	{
		return NodeAttribute::TYPE_MODEL;
	}

	void SkeletalModel::Update(const glm::mat4& worldMatrix)
	{
		m_Nodes[0]->Update(worldMatrix);
	}

	/*****************************/
	/* protected - NodeAttribute */
	/*****************************/

	bool SkeletalModel::Draw(
		const Frustum& frustum,
		uint32_t frameIndex,
		collection::DynamicContainer<OpacityDrawSet>& opacityDrawSets,
		collection::DynamicContainer<TransparencyDrawSet>& transparencyDrawSets)
	{
		auto it_mesh_begin = m_Meshes.begin();
		auto it_mesh_end = m_Meshes.end();

		const Plane& nearPlane = frustum.GetPlane(Frustum::PLANE_TYPE_NEAR);

		uint32_t count = 0;

		for (auto it_mesh = it_mesh_begin; it_mesh != it_mesh_end; ++it_mesh)
		{
			SkeletalMesh* pMesh = it_mesh->get();

			if ((pMesh->GetVisible() == false) ||
				(frustum.Contains(pMesh->GetAABB()) == false))
			{
				continue;
			}

			pMesh->Draw(
				nearPlane,
				frameIndex,
				m_Materials,
				opacityDrawSets,
				transparencyDrawSets);

			count++;
		}

		return (count > 0);
	}

	void SkeletalModel::DrawShadow(
		const Sphere& sphere,
		uint32_t frameIndex,
		collection::DynamicContainer<ShadowDrawSet>& shadowDrawSets,
		AABB& aabb)
	{
		auto it_mesh_begin = m_Meshes.begin();
		auto it_mesh_end = m_Meshes.end();

		for (auto it_mesh = it_mesh_begin; it_mesh != it_mesh_end; ++it_mesh)
		{
			SkeletalMesh* pMesh = it_mesh->get();
			if ((pMesh->GetVisible() == false) || (pMesh->GetCastShadow() == false))
			{
				continue;
			}

			// 境界判定
			const AABB& srcAabb = pMesh->GetAABB();
			if (sphere.Contains(srcAabb) == false)
			{
				continue;
			}

			// 描画セットを収集
			pMesh->DrawShadow(frameIndex, m_Materials, shadowDrawSets);

			// AABB を結合
			for (uint32_t i = 0; i < 3; i++)
			{
				if (aabb.minimum[i] > srcAabb.minimum[i]) { aabb.minimum[i] = srcAabb.minimum[i]; }
				if (aabb.maximum[i] < srcAabb.maximum[i]) { aabb.maximum[i] = srcAabb.maximum[i]; }
			}

			aabb.center = (aabb.minimum + aabb.maximum) * 0.5f;
		}
	}

	void SkeletalModel::DebugDraw(uint32_t flags, DebugRenderer* pDebugRenderer)
	{
		auto it_mesh_begin = m_Meshes.begin();
		auto it_mesh_end = m_Meshes.end();

		for (auto it_mesh = it_mesh_begin; it_mesh != it_mesh_end; ++it_mesh)
		{
			(*it_mesh)->DebugDraw(flags, pDebugRenderer);
		}
	}

	bool SkeletalModel::Load(LoggerPtr logger, HANDLE fileHandle, const wchar_t* pDirPath)
	{
		// ----------------------------------------------------------------------------------------------------
		// ファイルヘッダーを読み込む
		// ----------------------------------------------------------------------------------------------------

		SkeletalModel::File_FileHeader fileHeader;

		if (FileRead(fileHandle, sizeof(SkeletalModel::File_FileHeader), &fileHeader) == false)
		{
			logger->PrintA(Logger::TYPE_ERROR, "Failed to read the file");
			return false;
		}

		if (fileHeader.magicNumber != ToMagicNumber('V', 'S', 'K', 'M'))
		{
			logger->PrintA(Logger::TYPE_ERROR, "It is not an SKM file");
			return false;
		}

		if (fileHeader.version != SkeletalModel::CURRENT_VERSION)
		{
			logger->PrintA(Logger::TYPE_ERROR, "Unsupported version");
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// インフォヘッダーを読み込む
		// ----------------------------------------------------------------------------------------------------

		SkeletalModel::File_InfoHeader infoHeader;

		if (FileRead(fileHandle, sizeof(SkeletalModel::File_InfoHeader), &infoHeader) == false)
		{
			logger->PrintA(Logger::TYPE_ERROR, "Failed to read the file");
			return false;
		}

		if ((infoHeader.nodeCount == 0) ||
			(infoHeader.meshCount == 0) ||
			(infoHeader.materialCount == 0))
		{
			logger->PrintA(Logger::TYPE_ERROR, "Illegal SKM file");
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// メッシュを準備する
		// ----------------------------------------------------------------------------------------------------

		m_Meshes.reserve(infoHeader.meshCount);
		for (uint32_t i = 0; i < infoHeader.meshCount; i++)
		{
			m_Meshes.push_back(SkeletalMesh::Create(m_DeviceContext, static_cast<int32_t>(i)));
		}

		// ----------------------------------------------------------------------------------------------------
		// ノードを読み込む
		// ----------------------------------------------------------------------------------------------------

		m_Nodes.reserve(infoHeader.nodeCount);

		{
			collection::Vector<SkeletalModel::File_Node> nodes;
			nodes.resize(infoHeader.nodeCount);

			if (FileRead(fileHandle, sizeof(SkeletalModel::File_Node) * infoHeader.nodeCount, nodes.data()) == false)
			{
				logger->PrintA(Logger::TYPE_ERROR, "Failed to read the file");
				return false;
			}

			auto it_begin = nodes.begin();
			auto it_end = nodes.end();

			int32_t nodeID = 0;

			for (auto it = it_begin; it != it_end; ++it)
			{
				const SkeletalModel::File_Node& srcNode = (*it);

				NodePtr dstNode = Node::Create(nodeID);

				dstNode->SetName(srcNode.name);
				dstNode->SetLocalTransform(
					glm::vec3(srcNode.localScale[0], srcNode.localScale[1], srcNode.localScale[2]),
					glm::quat(srcNode.localRotation[0], srcNode.localRotation[1], srcNode.localRotation[2], srcNode.localRotation[3]),
					glm::vec3(srcNode.localTranslation[0], srcNode.localTranslation[1], srcNode.localTranslation[2]));

				if (srcNode.parentIndex >= 0)
				{
					Node::AddChild(m_Nodes[srcNode.parentIndex], dstNode);
				}

				if (srcNode.meshIndex >= 0)
				{
					SkeletalMeshPtr mesh = m_Meshes[srcNode.meshIndex];
					Node::SetAttribute(dstNode, mesh);
				}

				m_Nodes.push_back(dstNode);
				nodeID++;
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// マテリアルを読み込む
		// ----------------------------------------------------------------------------------------------------

		m_Materials.reserve(infoHeader.materialCount);

		for (uint32_t i = 0; i < infoHeader.materialCount; i++)
		{
			MaterialPtr material = Material::Create(m_DeviceContext);

			if (material->Load(logger, fileHandle, pDirPath) == false)
			{
				return false;
			}

			m_Materials.push_back(material);
		}

		// ----------------------------------------------------------------------------------------------------
		// メッシュを読み込む
		// ----------------------------------------------------------------------------------------------------

		{
			auto it_begin = m_Meshes.begin();
			auto it_end = m_Meshes.end();

			for (auto it = it_begin; it != it_end; ++it)
			{
				SkeletalMeshPtr mesh = (*it);

				if (mesh->Load(logger, this, fileHandle) == false)
				{
					return false;
				}

				mesh->ConnectMaterials(m_Materials);

				m_PolygonCount += mesh->GetPolygonCount();
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// マテリアルを更新する
		// ----------------------------------------------------------------------------------------------------

		{
			auto it_begin = m_Materials.begin();
			auto it_end = m_Materials.end();

			for (auto it = it_begin; it != it_end; ++it)
			{
				MaterialPtr material = (*it);
				material->Update();
			}
		}

		// ----------------------------------------------------------------------------------------------------

		return true;
	}

	bool SkeletalModel::Save(LoggerPtr logger, HANDLE fileHandle, const wchar_t* pDirPath)
	{
		// ----------------------------------------------------------------------------------------------------
		// ノードリストを作成
		// ----------------------------------------------------------------------------------------------------

		collection::Vector<SkeletalModel::File_Node> nodes;
		nodes.reserve(m_Nodes.size());

		{
			auto it_begin = m_Nodes.begin();
			auto it_end = m_Nodes.end();

			for (auto it = it_begin; it != it_end; ++it)
			{
				Node* pSrcNode = it->get();

				SkeletalModel::File_Node dstNode;

				if (wcscpy_s(dstNode.name, pSrcNode->GetName()) != 0)
				{
					logger->PrintW(Logger::TYPE_ERROR, L"The name of the node is too long. Please limit it to 255 characters or less. : NodeName[%s]", pSrcNode->GetName());
					return false;
				}

				NodePtr parentNode = pSrcNode->GetParent();
				if (parentNode != nullptr)
				{
					VE_ASSERT(parentNode->GetID() >= 0);
					dstNode.parentIndex = parentNode->GetID();
				}
				else
				{
					dstNode.parentIndex = -1;
				}

				NodeAttributePtr attribute = pSrcNode->GetAttribute();
				if (attribute->GetType() == NodeAttribute::TYPE_MESH)
				{
					dstNode.meshIndex = std::static_pointer_cast<SkeletalMesh>(attribute)->GetID();
				}
				else
				{
					dstNode.meshIndex = -1;
				}

				const Transform& localTransform = pSrcNode->GetLocalTransform();

				dstNode.localScale[0] = localTransform.scale.x;
				dstNode.localScale[1] = localTransform.scale.y;
				dstNode.localScale[2] = localTransform.scale.z;
				dstNode.localScale[3] = 1.0f;

				dstNode.localRotation[0] = localTransform.rotation.w;
				dstNode.localRotation[1] = localTransform.rotation.x;
				dstNode.localRotation[2] = localTransform.rotation.y;
				dstNode.localRotation[3] = localTransform.rotation.z;

				dstNode.localTranslation[0] = localTransform.translation.x;
				dstNode.localTranslation[1] = localTransform.translation.y;
				dstNode.localTranslation[2] = localTransform.translation.z;
				dstNode.localTranslation[3] = 1.0f;

				nodes.push_back(dstNode);
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// ファイルヘッダーを書き込む
		// ----------------------------------------------------------------------------------------------------

		SkeletalModel::File_FileHeader fileHeader;
		fileHeader.magicNumber = ToMagicNumber('V', 'S', 'K', 'M');
		fileHeader.version = SkeletalModel::CURRENT_VERSION;

		if (FileWrite(fileHandle, sizeof(SkeletalModel::File_FileHeader), &fileHeader) == false)
		{
			logger->PrintA(Logger::TYPE_ERROR, "Failed to write the file");
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// インフォヘッダーを書き込む
		// ----------------------------------------------------------------------------------------------------

		SkeletalModel::File_InfoHeader infoHeader;
		infoHeader.nodeCount = static_cast<uint32_t>(nodes.size());
		infoHeader.meshCount = static_cast<uint32_t>(m_Meshes.size());
		infoHeader.materialCount = static_cast<uint32_t>(m_Materials.size());

		if (FileWrite(fileHandle, sizeof(SkeletalModel::File_InfoHeader), &infoHeader) == false)
		{
			logger->PrintA(Logger::TYPE_ERROR, "Failed to write the file");
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// ノードを書き込む
		// ----------------------------------------------------------------------------------------------------

		if (FileWrite(fileHandle, sizeof(SkeletalModel::File_Node) * nodes.size(), nodes.data()) == false)
		{
			logger->PrintA(Logger::TYPE_ERROR, "Failed to write the file");
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// マテリアルを書き込む
		// ----------------------------------------------------------------------------------------------------

		auto it_material_begin = m_Materials.begin();
		auto it_material_end = m_Materials.end();

		for (auto it_material = it_material_begin; it_material != it_material_end; ++it_material)
		{
			if ((*it_material)->Save(logger, fileHandle, pDirPath) == false)
			{
				return false;
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// メッシュを書き込む
		// ----------------------------------------------------------------------------------------------------

		auto it_mesh_begin = m_Meshes.begin();
		auto it_mesh_end = m_Meshes.end();

		for (auto it_mesh = it_mesh_begin; it_mesh != it_mesh_end; ++it_mesh)
		{
			if ((*it_mesh)->Save(logger, fileHandle) == false)
			{
				return false;
			}
		}

		// ----------------------------------------------------------------------------------------------------

		return true;
	}

}