#include "FbxModelSource.h"
#include "DeviceContext.h"
#include "Logger.h"

namespace ve {

	ModelSourcePtr FbxModelSource::Create()
	{
		return std::move(std::make_shared<FbxModelSource>());
	}

	FbxModelSource::FbxModelSource() :
		m_MeshNodeCount(0)
	{
	}

	FbxModelSource::~FbxModelSource()
	{
	}

	bool FbxModelSource::Load(LoggerPtr logger, DeviceContextPtr deviceContext, const wchar_t* pFilePath, const ModelSourceConfig& config)
	{
		m_FilePath = pFilePath;

		FbxManager* pSdkManager = FbxManager::Create();
		if (pSdkManager == nullptr)
		{
			logger->PrintA(Logger::TYPE_ERROR, "Failed to create FbxManager");
			return false;
		}

		FbxIOSettings* pIOSetting = FbxIOSettings::Create(pSdkManager, IOSROOT);
		if (pIOSetting == nullptr)
		{
			logger->PrintA(Logger::TYPE_ERROR, "Failed to create FbxIOSettings");
			pSdkManager->Destroy();
			return false;
		}
		pSdkManager->SetIOSettings(pIOSetting);

		FbxImporter* pImporter = FbxImporter::Create(pSdkManager, "");
		if (pImporter == nullptr)
		{
			logger->PrintA(Logger::TYPE_ERROR, "Failed to create FbxImporter");
			pIOSetting->Destroy();
			pSdkManager->Destroy();
			return false;
		}

		StringA filePathA;
		ToMultibyteString(pFilePath, filePathA);

		if (pImporter->Initialize(filePathA.c_str(), -1, pSdkManager->GetIOSettings()) == false)
		{
			logger->PrintA(Logger::TYPE_ERROR, "Failed to initialize FbxImporter");
			pImporter->Destroy();
			pIOSetting->Destroy();
			pSdkManager->Destroy();
			return false;
		}

		FbxScene* pScene = FbxScene::Create(pSdkManager, "");
		if (pScene == nullptr)
		{
			logger->PrintA(Logger::TYPE_ERROR, "Failed to create FbxScene");
			pImporter->Destroy();
			pIOSetting->Destroy();
			pSdkManager->Destroy();
			return false;
		}

		if (pImporter->Import(pScene) == false)
		{
			logger->PrintA(Logger::TYPE_ERROR, "Failed to import FbxScene");
			pScene->Destroy();
			pImporter->Destroy();
			pIOSetting->Destroy();
			pSdkManager->Destroy();
			return false;
		}

		pImporter->Destroy();

		if (Load(logger, pScene, config) == false)
		{
			pIOSetting->Destroy();
			pSdkManager->Destroy();
			return false;
		}

		pIOSetting->Destroy();
		pSdkManager->Destroy();

		return true;
	}

	const wchar_t* FbxModelSource::GetFilePath() const
	{
		return m_FilePath.c_str();
	}

	const collection::Vector<IModelSource::Material>& FbxModelSource::GetMaterials() const
	{
		return m_Materials;
	}

	const collection::Vector<IModelSource::Polygon>& FbxModelSource::GetPolygons() const
	{
		return m_Polygons;
	}

	const collection::Vector<FbxModelSource::Node>& FbxModelSource::GetNodes() const
	{
		return m_Nodes;
	}

	size_t FbxModelSource::GetEmptyNodeCount() const
	{
		return m_Nodes.size() - m_MeshNodeCount;
	}

	size_t FbxModelSource::GetMeshNodeCount() const
	{
		return m_MeshNodeCount;
	}

	bool FbxModelSource::Load(LoggerPtr logger, FbxScene* pFbxScene, const ModelSourceConfig& config)
	{
		FbxNode* pFbxRootNode = pFbxScene->GetRootNode();
		if (pFbxRootNode == nullptr)
		{
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// ノードをロード
		// ----------------------------------------------------------------------------------------------------

		if (LoadNodes(logger, pFbxRootNode, -1, config) == false)
		{
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// トランスフォーム
		// ----------------------------------------------------------------------------------------------------

		if (config.flags & MODEL_SOURCE_TRANSFORM)
		{
			m_Nodes[0].localScaling *= config.scale;
			m_Nodes[0].localRotation *= glm::quat(config.rotation);
		}

		// ----------------------------------------------------------------------------------------------------
		// ボーンのノードインデックスを設定
		// ----------------------------------------------------------------------------------------------------

		{
			auto it_node_begin = m_Nodes.begin();
			auto it_node_end = m_Nodes.end();

			for (auto it_node = it_node_begin; it_node != it_node_end; ++it_node)
			{
				auto it_bone_begin = it_node->bones.begin();
				auto it_bone_end = it_node->bones.end();

				for (auto it_bone = it_bone_begin; it_bone != it_bone_end; ++it_bone)
				{
					auto it_bone_node = std::find_if(it_node_begin, it_node_end, [it_bone](const IModelSource::Node& node) { return (node.name.compare(it_bone->nodeName) == 0); });
					VE_ASSERT(it_bone_node != m_Nodes.end());

					it_bone->nodeIndex = static_cast<int32_t>(std::distance(it_node_begin, it_bone_node));
				}
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// マテリアルが割り当てられていないポリゴンを処理する
		// ----------------------------------------------------------------------------------------------------

		{
			int32_t defaultMaterialIndex = static_cast<int32_t>(m_Materials.size());

			IModelSource::Polygon* pPolygon = m_Polygons.data();
			IModelSource::Polygon* pPolygonEnd = pPolygon + m_Polygons.size();

			uint32_t notAssignedPolygonCount = 0;

			while (pPolygon != pPolygonEnd)
			{
				if (pPolygon->materialIndex == -1)
				{
					pPolygon->materialIndex = defaultMaterialIndex;
					notAssignedPolygonCount++;
				}

				pPolygon++;
			}

			if (notAssignedPolygonCount > 0)
			{
				bool safeName = false;

				for (int32_t i = 0; (i < 100) && (safeName == false); i++)
				{
					wchar_t tempName[32];
					if (i != 0)
					{
						wsprintf(tempName, L"default_%.3d", i);
					}
					else
					{
						wsprintf(tempName, L"default");
					}

					IModelSource::Material* pMaterial = m_Materials.data();
					IModelSource::Material* pMaterialEnd = pMaterial + m_Materials.size();

					safeName = true;

					while ((pMaterial != pMaterialEnd) && (safeName == true))
					{
						if (pMaterial->name.compare(tempName) != 0)
						{
							safeName = false;
						}

						pMaterial++;
					}

					if (safeName == true)
					{
						IModelSource::Material material = IModelSource::Material::Default(tempName);
						m_Materials.push_back(material);
					}
				}
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// マテリアルが割り当てられていないノードを処理する
		// ----------------------------------------------------------------------------------------------------

		{
			int32_t materialIndex = static_cast<int32_t>(m_Materials.size() - 1);

			auto it_node_begin = m_Nodes.begin();
			auto it_node_end = m_Nodes.end();

			for (auto it_node = it_node_begin; it_node != it_node_end; ++it_node)
			{
				if (it_node->materialIndices.empty() == true)
				{
					it_node->materialIndices.push_back(materialIndex);
				}
			}
		}

		return true;
	}

	bool FbxModelSource::LoadNodes(LoggerPtr logger, FbxNode* pFbxNode, int32_t parentNodeIndex, const ModelSourceConfig& config)
	{
		const char* pName = pFbxNode->GetName();

		const FbxDouble3& localTranslation = pFbxNode->LclTranslation;
		const FbxDouble3& localRotation = pFbxNode->LclRotation;
		const FbxDouble3& localScaling = pFbxNode->LclScaling;

		FbxEuler::EOrder fbxRotationOrder;
		pFbxNode->GetRotationOrder(FbxNode::eSourcePivot, fbxRotationOrder);

		glm::mat4 xMat = glm::rotate(glm::mat4(), glm::radians<float>(static_cast<float>(localRotation.mData[0])), glm::vec3(1.0f, 0.0f, 0.0f));
		glm::mat4 yMat = glm::rotate(glm::mat4(), glm::radians<float>(static_cast<float>(localRotation.mData[1])), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 zMat = glm::rotate(glm::mat4(), glm::radians<float>(static_cast<float>(localRotation.mData[2])), glm::vec3(0.0f, 0.0f, 1.0f));
		glm::mat4 rotMat;

		switch (fbxRotationOrder)
		{
		case FbxEuler::eOrderXYZ:
			rotMat = zMat * yMat * xMat;
			break;
		case FbxEuler::eOrderXZY:
			rotMat = yMat * zMat * xMat;
			break;
		case FbxEuler::eOrderYZX:
			rotMat = xMat * zMat * yMat;
			break;
		case FbxEuler::eOrderYXZ:
			rotMat = zMat * xMat * yMat;
			break;
		case FbxEuler::eOrderZXY:
			rotMat = yMat * xMat * zMat;
			break;
		case FbxEuler::eOrderZYX:
			rotMat = xMat * yMat * zMat;
			break;

		default:
			return false;
		}

		FbxModelSource::Node node;
		ToWideString(pFbxNode->GetName(), node.name);
		node.parentIndex = parentNodeIndex;
		node.localTranslation.x = static_cast<float>(localTranslation.mData[0]);
		node.localTranslation.y = static_cast<float>(localTranslation.mData[1]);
		node.localTranslation.z = static_cast<float>(localTranslation.mData[2]);
		node.localRotation = rotMat;
		node.localScaling.x = static_cast<float>(localScaling.mData[0]);
		node.localScaling.y = static_cast<float>(localScaling.mData[1]);
		node.localScaling.z = static_cast<float>(localScaling.mData[2]);
		node.firstPolygonIndex = 0;
		node.polygonCount = 0;
		node.hasUV = false;

		logger->PrintA(Logger::TYPE_INFO, "Node(%s) : T(%lf, %lf, %lf) R(%lf, %lf, %lf) S(%lf, %lf, %lf)",
			pName,
			localTranslation.mData[0], localTranslation.mData[1], localTranslation.mData[2],
			localRotation.mData[0], localRotation.mData[1], localRotation.mData[2],
			localScaling.mData[0], localScaling.mData[1], localScaling.mData[2]);

		if (LoadNodeMaterial(pFbxNode, node, config) == false)
		{
			return false;
		}

		logger->PushIndent();

		int32_t nodeIndex = static_cast<int32_t>(m_Nodes.size());

		FbxNodeAttribute* pFbxNodeAttribure = pFbxNode->GetNodeAttribute();
		if (pFbxNodeAttribure != nullptr)
		{
			FbxNodeAttribute::EType fbxNodeAttributeType = pFbxNodeAttribure->GetAttributeType();

			if (fbxNodeAttributeType == FbxNodeAttribute::eMesh)
			{
				FbxGeometryConverter converter(pFbxNode->GetFbxManager());
				pFbxNodeAttribure = converter.Triangulate(pFbxNodeAttribure, false);

				size_t prePolygonCount = m_Polygons.size();

				if (LoadMesh(logger, pFbxNode, static_cast<FbxMesh*>(pFbxNodeAttribure), node, nodeIndex, config) == true)
				{
					m_MeshNodeCount++;
				}
				else
				{
					m_Polygons.resize(prePolygonCount);
					logger->PrintA(Logger::TYPE_INFO, "Empty(error)");
				}
			}
			else
			{
				logger->PrintA(Logger::TYPE_INFO, "Empty");
			}
		}
		else
		{
			logger->PrintA(Logger::TYPE_INFO, "Empty(null)");
		}

		m_Nodes.push_back(node);

		logger->PopIndent();

		int32_t childCount = pFbxNode->GetChildCount();
		for (int32_t i = 0; i < childCount; i++)
		{
			FbxNode* pFbxChildNode = pFbxNode->GetChild(i);
			if (LoadNodes(logger, pFbxChildNode, nodeIndex, config) == false)
			{
				return false;
			}
		}

		return true;
	}

	bool FbxModelSource::LoadNodeMaterial(FbxNode* pFbxNode, FbxModelSource::Node& node, const ModelSourceConfig& config)
	{
		int32_t fbxMaterialCount = pFbxNode->GetMaterialCount();
		if (fbxMaterialCount == 0)
		{
			return true;
		}

		for (int32_t i = 0; i < fbxMaterialCount; i++)
		{
			FbxSurfaceMaterial* pFbxMaterial = pFbxNode->GetMaterial(i);

			StringW fbxMaterialName;
			ToWideString(pFbxMaterial->GetName(), fbxMaterialName);

			auto it_material = std::find_if(m_Materials.begin(), m_Materials.end(),
				[fbxMaterialName](const IModelSource::Material& material) { return wcscmp(fbxMaterialName.c_str(), material.name.c_str()) == 0; });

			if (it_material == m_Materials.end())
			{
				node.materialIndices.push_back(static_cast<int32_t>(m_Materials.size()));
			}
			else
			{
				node.materialIndices.push_back(static_cast<int32_t>(std::distance(m_Materials.begin(), it_material)));
				continue;
			}

			IModelSource::Material material = IModelSource::Material::Default(fbxMaterialName.c_str());

			if ((pFbxMaterial->GetClassId().Is(FbxSurfaceLambert::ClassId)) || (pFbxMaterial->GetClassId().Is(FbxSurfacePhong::ClassId)))
			{
				FbxSurfaceLambert* pFbxLambert = static_cast<FbxSurfaceLambert*>(pFbxMaterial);

				const FbxDouble3& fbxDiffuse = pFbxLambert->Diffuse;
				const FbxDouble& fbxDiffuseFactor = pFbxLambert->DiffuseFactor;
				const FbxDouble& fbxTransparencyFactor = pFbxLambert->TransparencyFactor;
				const FbxDouble& fbxEmissiveFactor = pFbxLambert->EmissiveFactor;

				material.diffuseColor.r = static_cast<float>(fbxDiffuse.mData[0]);
				material.diffuseColor.g = static_cast<float>(fbxDiffuse.mData[1]);
				material.diffuseColor.b = static_cast<float>(fbxDiffuse.mData[2]);
				material.diffuseColor.a = std::max(0.0f, static_cast<float>(1.0 - fbxTransparencyFactor));

				material.diffuseFactor = static_cast<float>(fbxDiffuseFactor);

				material.emissiveFactor = ToLuminance(pFbxLambert->Emissive) * static_cast<float>(fbxEmissiveFactor);

				pFbxLambert = nullptr;
			}
		
			if (pFbxMaterial->GetClassId().Is(FbxSurfacePhong::ClassId))
			{
				FbxSurfacePhong* pFbxPhong = static_cast<FbxSurfacePhong*>(pFbxMaterial);

				const FbxDouble3& fbxSpecular = pFbxPhong->Specular;
				const FbxDouble& fbxSpecularFactor = pFbxPhong->SpecularFactor;
				const FbxDouble& fbxShininess = pFbxPhong->Shininess;

				material.specularColor.r = static_cast<float>(fbxSpecular.mData[0]);
				material.specularColor.g = static_cast<float>(fbxSpecular.mData[1]);
				material.specularColor.b = static_cast<float>(fbxSpecular.mData[2]);
				material.specularColor.a = 1.0f;

				material.specularFactor = static_cast<float>(fbxSpecularFactor);
				material.shininess = static_cast<float>(fbxShininess);
			}

			FbxProperty diffuseProp = pFbxMaterial->FindProperty(FbxSurfaceMaterial::sDiffuse);
			GetMaterialTexture(diffuseProp, config.pathType, material.diffuseTexture);

			FbxProperty bumpProp = pFbxMaterial->FindProperty(FbxSurfaceMaterial::sBump);
			GetMaterialTexture(bumpProp, config.pathType, material.bumpTexture);
			if (material.bumpTexture.empty() == true)
			{
				FbxProperty normalMapProp = pFbxMaterial->FindProperty(FbxSurfaceMaterial::sNormalMap);
				GetMaterialTexture(normalMapProp, config.pathType, material.bumpTexture);
			}

			FbxProperty specularProp = pFbxMaterial->FindProperty(FbxSurfaceMaterial::sSpecular);
			GetMaterialTexture(specularProp, config.pathType, material.specularTexture);

			m_Materials.push_back(material);
		}

		return true;
	}

	float FbxModelSource::ToLuminance(const FbxDouble3& color)
	{
		static const glm::mat3 MAT = glm::mat3(0.4124, 0.2126, 0.0193, 0.3576, 0.7152, 0.1192, 0.1805, 0.0722, 0.9505);

		glm::vec3 xyz = MAT * glm::vec3(static_cast<float>(color.mData[0]), static_cast<float>(color.mData[1]), static_cast<float>(color.mData[2]));

		return xyz.g;
	}

	bool FbxModelSource::LoadMesh(LoggerPtr logger, FbxNode* pFbxNode, FbxMesh* pFbxMesh, FbxModelSource::Node& node, int32_t nodeIndex, const ModelSourceConfig& config)
	{
		int32_t controlPointCount = pFbxMesh->GetControlPointsCount();
		if (controlPointCount == 0)
		{
			logger->PrintA(Logger::TYPE_ERROR, "Mesh(%s) : No control point",pFbxMesh->GetName());
			return false;
		}

		int32_t polygonCount = pFbxMesh->GetPolygonCount();
		if (polygonCount == 0)
		{
			logger->PrintA(Logger::TYPE_ERROR, "Mesh(%s) : No polygon", pFbxMesh->GetName());
			return false;
		}

		int32_t layerCount = pFbxMesh->GetLayerCount();
		if (layerCount == 0)
		{
			logger->PrintA(Logger::TYPE_ERROR, "Mesh(%s) : No layer", pFbxMesh->GetName());
			return false;
		}

		FbxLayerElementUV* pFbxUVs = nullptr;
		FbxLayerElementNormal* pFbxNormals = nullptr;
		FbxLayerElementMaterial* pFbxMaterials = nullptr;

		for (int32_t i = 0; i < layerCount; i++)
		{
			FbxLayer* pFbxLayer = pFbxMesh->GetLayer(i);

			if (pFbxUVs == nullptr)
			{
				pFbxUVs = pFbxLayer->GetUVs();
			}

			if (pFbxNormals == nullptr)
			{
				pFbxNormals = pFbxLayer->GetNormals();
			}

			if (pFbxMaterials == nullptr)
			{
				pFbxMaterials = pFbxLayer->GetMaterials();
			}
		}

		logger->PrintA(Logger::TYPE_INFO, "Mesh(%s) : ControlPoint[%d] Polygon[%d] Layer[%d] UV[%s] Normal[%s] Material[%s]",
			pFbxMesh->GetName(),
			controlPointCount, polygonCount, layerCount,
			(pFbxUVs != nullptr) ? "○" : "×",
			(pFbxNormals != nullptr) ? "○" : "×",
			(pFbxMaterials != nullptr) ? "○" : "×");

		// ----------------------------------------------------------------------------------------------------
		// ボーンリスト、頂点毎のウェイトリストを作成
		// ----------------------------------------------------------------------------------------------------

		collection::Vector<collection::Vector<FbxModelSource::BoneWeight>> vertexBoneWeights;
		vertexBoneWeights.resize(controlPointCount);

		int32_t deformerCount = pFbxMesh->GetDeformerCount();

		for (int32_t i = 0; i < deformerCount; i++)
		{
			FbxDeformer* pFbxDeformer = pFbxMesh->GetDeformer(i);

			if (pFbxDeformer->GetDeformerType() != FbxDeformer::eSkin)
			{
				continue;
			}

			FbxSkin* pFbxSkin = FbxCast<FbxSkin>(pFbxDeformer);
			int clusterCount = pFbxSkin->GetClusterCount();

			for (int32_t j = 0; j < clusterCount; j++)
			{
				FbxCluster* pFbxCluster = pFbxSkin->GetCluster(j);
				FbxNode* pFbxBoneNode = pFbxCluster->GetLink();

				/**********/
				/* ボーン */
				/**********/

				FbxAMatrix fbxBoneMat;
				pFbxCluster->GetTransformMatrix(fbxBoneMat);

				FbxAMatrix fbxBoneLinkMat;
				pFbxCluster->GetTransformLinkMatrix(fbxBoneLinkMat);

				glm::mat4 boneMat = FbxModelSource::ToGlmMatrix(fbxBoneMat);
				glm::mat4 boneLinkMat = glm::inverse(FbxModelSource::ToGlmMatrix(fbxBoneLinkMat));

				IModelSource::Bone bone;
				ToWideString(pFbxBoneNode->GetName(), bone.nodeName);
				bone.offsetMatrix = boneLinkMat * boneMat;

				node.bones.push_back(bone);

				/********************/
				/* 頂点毎のウェイト */
				/********************/

				if (node.bones.size() > 256)
				{
					return false;
				}

				uint8_t boneIndex = static_cast<uint8_t>(node.bones.size() - 1);

				int32_t indicesCount = pFbxCluster->GetControlPointIndicesCount();

				int32_t* pIndices = pFbxCluster->GetControlPointIndices();
				double* pWeights = pFbxCluster->GetControlPointWeights();

				for (int32_t k = 0; k < indicesCount; k++)
				{
					FbxModelSource::BoneWeight weight;

					weight.index = boneIndex;
					weight.value = static_cast<float>(pWeights[k]);

					vertexBoneWeights[pIndices[k]].push_back(weight);
				}
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// ウェイトを正規化
		// ----------------------------------------------------------------------------------------------------

		{
			auto it_begin = vertexBoneWeights.begin();
			auto it_end = vertexBoneWeights.end();

			for (auto it = it_begin; it != it_end; ++it)
			{
				auto& weights = *it;

				if (weights.empty() == false)
				{
					size_t weightCount = weights.size();
					if (weightCount < 4)
					{
						size_t addCount = 4 - weightCount;
						for (size_t i = 0; i < addCount; i++)
						{
							weights.push_back(FbxModelSource::BoneWeight{});
						}
					}

					std::sort(weights.begin(), weights.end(), [](const FbxModelSource::BoneWeight& lhs, const FbxModelSource::BoneWeight& rhs) { return (lhs.value > rhs.value); });
					weights.resize(4);

					auto it_w_begin = weights.begin();
					auto it_w_end = weights.end();

					float normalizeValue = 0.0f;

					for (auto it_w = it_w_begin; it_w != it_w_end; ++it_w)
					{
						normalizeValue += it_w->value;
					}

					normalizeValue = VE_FLOAT_RECIPROCAL(normalizeValue);

					for (auto it_w = it_w_begin; it_w != it_w_end; ++it_w)
					{
						it_w->value *= normalizeValue;
					}
				}
				else
				{
					weights.resize(4);

					weights[0].index = 0;
					weights[1].index = 0;
					weights[2].index = 0;
					weights[3].index = 0;
					weights[0].value = 1.0f;
					weights[1].value = 0.0f;
					weights[2].value = 0.0f;
					weights[3].value = 0.0f;
				}
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// バーテックスリストを作成
		// ----------------------------------------------------------------------------------------------------

		collection::Vector<glm::vec3> controlPoints;
		controlPoints.reserve(controlPointCount);

		for (int32_t i = 0; i < controlPointCount; i++)
		{
			FbxVector4 fbxPoint = pFbxMesh->GetControlPointAt(i);
			glm::vec3 point;

			if (VE_DOUBLE_IS_ZERO(fbxPoint.mData[3]) == true)
			{
				point.x = static_cast<float>(fbxPoint.mData[0]);
				point.y = static_cast<float>(fbxPoint.mData[1]);
				point.z = static_cast<float>(fbxPoint.mData[2]);
			}
			else
			{
				point.x = static_cast<float>(VE_DOUBLE_DIV(fbxPoint.mData[0], fbxPoint.mData[3]));
				point.y = static_cast<float>(VE_DOUBLE_DIV(fbxPoint.mData[1], fbxPoint.mData[3]));
				point.z = static_cast<float>(VE_DOUBLE_DIV(fbxPoint.mData[2], fbxPoint.mData[3]));
			}

			controlPoints.push_back(point);
		}

		// ----------------------------------------------------------------------------------------------------
		// ボーン毎の頂点をまとめる
		// ----------------------------------------------------------------------------------------------------

		collection::Vector<collection::Vector<glm::vec3>> bonePoints;

		if (node.bones.empty() == false)
		{
			size_t boneCount = node.bones.size();

			bonePoints.resize(boneCount);

			size_t pointCount = vertexBoneWeights.size();

			for (size_t i = 0; i < pointCount; i++)
			{
				auto& point = controlPoints[i];
				auto& weights = vertexBoneWeights[i];

				for (size_t j = 0; (j < 4) && (VE_FLOAT_IS_ZERO(weights[j].value) == false); j++)
				{
					bonePoints[weights[j].index].push_back(point);
				}
			}

			node.boxes.reserve(boneCount);
		}
		else
		{
			bonePoints.resize(1);
			bonePoints[0] = controlPoints;

			node.boxes.reserve(1);
		}

		// ----------------------------------------------------------------------------------------------------
		// 未使用のボーンを削除
		// ----------------------------------------------------------------------------------------------------

		if (node.bones.empty() == false)
		{
			/******************************************/
			/* ボーンのインデックス参照テーブルを作成 */
			/******************************************/

			uint8_t boneCount = static_cast<uint8_t>(node.bones.size());

			collection::Vector<uint8_t> boneIndexTable;
			boneIndexTable.reserve(boneCount);

			for (uint8_t i = 0; i < boneCount; i++)
			{
				boneIndexTable.push_back(i);
			}

			auto it_bone = node.bones.begin();

			for (uint8_t i = 0; i < boneCount; i++)
			{
				if (bonePoints[i].empty() == true)
				{
					for (uint8_t j = i; j < boneCount; j++)
					{
						boneIndexTable[j] -= 1;
					}

					it_bone = node.bones.erase(it_bone);
				}
				else
				{
					it_bone++;
				}
			}

			/********************************/
			/* ウェイトのインデックスを修正 */
			/********************************/

			if (node.bones.size() != boneCount)
			{
				auto it_begin = vertexBoneWeights.begin();
				auto it_end = vertexBoneWeights.end();

				for (auto it = it_begin; it != it_end; ++it)
				{
					auto& weights = *it;

					auto it_w_begin = weights.begin();
					auto it_w_end = weights.end();

					float normalizeValue = 0.0f;

					for (auto it_w = it_w_begin; it_w != it_w_end; ++it_w)
					{
						it_w->index = boneIndexTable[it_w->index];
					}
				}
			}

			/******************************/
			/* 不要なボーンポイントを削除 */
			/******************************/

			auto it_bone_point = bonePoints.begin();
			while (it_bone_point != bonePoints.end())
			{
				if (it_bone_point->empty() == true)
				{
					it_bone_point = bonePoints.erase(it_bone_point);
				}
				else
				{
					it_bone_point++;
				}
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// ボックスを作成
		// ----------------------------------------------------------------------------------------------------

		if (node.bones.empty() == false)
		{
			auto it_begin = bonePoints.begin();
			auto it_end = bonePoints.end();

			for (auto it = it_begin; it != it_end; ++it)
			{
				VE_ASSERT(it->empty() == false);

				IModelSource::Box box;
				FbxModelSource::CreateOBB(*it, box.center, box.axis[0], box.axis[1], box.axis[2], box.halfExtent);
				node.boxes.push_back(box);
			}
		}
		else
		{
			auto it_begin = controlPoints.begin();
			auto it_end = controlPoints.end();

			IModelSource::Box box;

			glm::vec3 aabbMin = glm::vec3(+VE_FLOAT_MAX);
			glm::vec3 aabbMax = glm::vec3(-VE_FLOAT_MAX);

			for (auto it = it_begin; it != it_end; ++it)
			{
				aabbMin = glm::min(aabbMin, (*it));
				aabbMax = glm::max(aabbMax, (*it));
			}

			box.center = (aabbMin + aabbMax) * 0.5f;
			box.axis[0] = glm::vec3(1.0f, 0.0f, 0.0f);
			box.axis[1] = glm::vec3(0.0f, 1.0f, 0.0f);
			box.axis[2] = glm::vec3(0.0f, 0.0f, 1.0f);
			box.halfExtent = (aabbMax - aabbMin) * 0.5f;

			node.boxes.push_back(box);
		}

		// ----------------------------------------------------------------------------------------------------
		// ポリゴンリストを作成
		// ----------------------------------------------------------------------------------------------------

		FbxModelSource::PolygonVertexRefVector polygonVertexRefs;
		polygonVertexRefs.resize(controlPointCount);

		size_t firstPolygonIndex = m_Polygons.size();
		bool vertexWeightEnable = (vertexBoneWeights.empty() == false);

		uint32_t totalVertexCount = 0;

		for (int32_t i = 0; i < polygonCount; i++)
		{
			IModelSource::Polygon polygon{};
			polygon.materialIndex = -1;

			auto& vertices = polygon.vertices;

			int32_t vertexCount = pFbxMesh->GetPolygonSize(i);
			VE_ASSERT(vertexCount == 3);

			for (int32_t j = 0; j < vertexCount; j++)
			{
				int32_t controlPointIndex = pFbxMesh->GetPolygonVertex(i, j);
				Vertex* pVertex = &vertices[j];

				pVertex->pos = controlPoints[controlPointIndex];

				pVertex->uv.x = 0.0f;
				pVertex->uv.y = 0.0f;

				pVertex->normal.x = 0.0f;
				pVertex->normal.y = 0.0f;
				pVertex->normal.z = 0.0f;

				if (vertexWeightEnable == true)
				{
					const auto& weights = vertexBoneWeights[controlPointIndex];
					VE_ASSERT(weights.size() == 4);

					for (uint32_t k = 0; k < 4; k++)
					{
						pVertex->indices[k] = weights[k].index;
						pVertex->weights[k] = weights[k].value;
					}
				}

				std::array<int32_t, 2> ref = { i, j };
				polygonVertexRefs[controlPointIndex].push_back(ref);
			}

			totalVertexCount += vertexCount;

			m_Polygons.push_back(polygon);
		}

		// ----------------------------------------------------------------------------------------------------
		// UV をポリゴンに設定
		// ----------------------------------------------------------------------------------------------------

		if (pFbxUVs != nullptr)
		{
			if (LoadMeshUV(pFbxUVs, firstPolygonIndex, polygonCount, polygonVertexRefs) == false)
			{
				return false;
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// 法線をポリゴンに設定
		// ----------------------------------------------------------------------------------------------------

		if (pFbxNormals != nullptr)
		{
			if (LoadMeshNormal(pFbxNormals, firstPolygonIndex, polygonCount, polygonVertexRefs) == false)
			{
				return false;
			}
		}
		else
		{
			IModelSource::Polygon* pPolygon = m_Polygons.data() + firstPolygonIndex;
			IModelSource::Polygon* pPolygonEnd = pPolygon + polygonCount;

			while (pPolygon != pPolygonEnd)
			{
				IModelSource::Vertex* pVertices = &pPolygon->vertices[0];

				glm::vec3 ab;
				glm::vec3 bc;

				if (config.flags & MODEL_SOURCE_INVERT_NORMAL)
				{
					ab = pVertices[0].pos - pVertices[1].pos;
					bc = pVertices[2].pos - pVertices[1].pos;
				}
				else
				{
					ab = pVertices[0].pos - pVertices[1].pos;
					bc = pVertices[1].pos - pVertices[2].pos;
				}

				glm::vec3 normal = glm::normalize(glm::cross(ab, bc));

				pVertices[0].normal = normal;
				pVertices[1].normal = normal;
				pVertices[2].normal = normal;

				pPolygon++;
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// 接線空間を求める
		// ----------------------------------------------------------------------------------------------------

		if(pFbxUVs != nullptr)
		{
			IModelSource::Polygon* pPolygon = m_Polygons.data() + firstPolygonIndex;
			IModelSource::Polygon* pPolygonEnd = pPolygon + polygonCount;

			while (pPolygon != pPolygonEnd)
			{
				IModelSource::Vertex* pVertices[3] =
				{
					&(pPolygon->vertices[0]),
					&(pPolygon->vertices[1]),
					&(pPolygon->vertices[2]),
				};

				glm::vec3 p0 = pVertices[0]->pos;
				glm::vec3 p1 = pVertices[1]->pos;
				glm::vec3 p2 = pVertices[2]->pos;

				glm::vec2 t0 = pVertices[0]->uv;
				glm::vec2 t1 = pVertices[1]->uv;
				glm::vec2 t2 = pVertices[2]->uv;

				glm::vec3 cp0[3] =
				{
					glm::vec3(p0.x, t0.x, t0.y),
					glm::vec3(p0.y, t0.x, t0.y),
					glm::vec3(p0.z, t0.x, t0.y),
				};

				glm::vec3 cp1[3] =
				{
					glm::vec3(p1.x, t1.x, t1.y),
					glm::vec3(p1.y, t1.x, t1.y),
					glm::vec3(p1.z, t1.x, t1.y),
				};

				glm::vec3 cp2[3] =
				{
					glm::vec3(p2.x, t2.x, t2.y),
					glm::vec3(p2.y, t2.x, t2.y),
					glm::vec3(p2.z, t2.x, t2.y),
				};

				glm::vec3 v1;
				glm::vec3 v2;
				glm::vec3 abc;

				float u[3];
				float v[3];

				for (uint32_t i = 0; i < 3; i++)
				{
					v1 = cp1[i] - cp0[i];
					v2 = cp2[i] - cp1[i];

					abc = glm::cross(v1, v2);

					if ((-VE_FLOAT_EPSILON <= abc.x) && (VE_FLOAT_EPSILON >= abc.x))
					{
						//縮退しているポリゴン
						u[i] = 1.0f;
						v[i] = 1.0f;
					}
					else
					{
						u[i] = (abc.y / abc.x);
						v[i] = (abc.z / abc.x);
					}
				}

				for (uint32_t i = 0; i < 3; i++)
				{
					pVertices[i]->tangent.x += u[0];
					pVertices[i]->tangent.y += u[1];
					pVertices[i]->tangent.z += u[2];
					pVertices[i]->tangent = glm::normalize(pVertices[i]->tangent);

					pVertices[i]->binormal.x += v[0];
					pVertices[i]->binormal.y += v[1];
					pVertices[i]->binormal.z += v[2];
					pVertices[i]->binormal = glm::normalize(pVertices[i]->binormal);
				}

				pPolygon++;
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// マテリアルをポリゴンに設定
		// ----------------------------------------------------------------------------------------------------

		if (pFbxMaterials != nullptr)
		{
			if (LoadMeshMaterial(pFbxNode, pFbxMaterials, firstPolygonIndex, polygonCount, polygonVertexRefs) == false)
			{
				return false;
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// ポリゴンリストを完了
		// ----------------------------------------------------------------------------------------------------

		if (config.flags & (MODEL_SOURCE_FLIP_FACE | MODEL_SOURCE_INVERT_TEXCOORD_U | MODEL_SOURCE_INVERT_TEXCOORD_V))
		{
			IModelSource::Polygon* pPolygon = m_Polygons.data() + firstPolygonIndex;
			IModelSource::Polygon* pPolygonEnd = pPolygon + polygonCount;

			while (pPolygon != pPolygonEnd)
			{
				IModelSource::Vertex* vertices = &pPolygon->vertices[0];

				IModelSource::Vertex temp = vertices[0];
				vertices[0] = vertices[2];
				vertices[2] = temp;

				if (config.flags & MODEL_SOURCE_INVERT_TEXCOORD_U)
				{
					IModelSource::Vertex* pVertex = &vertices[0];
					IModelSource::Vertex* pVertexEnd = pVertex + 3;
					while (pVertex != pVertexEnd)
					{
						pVertex->uv.x = 1.0f - pVertex->uv.x;
						pVertex++;
					}
				}

				if (config.flags & MODEL_SOURCE_INVERT_TEXCOORD_V)
				{
					IModelSource::Vertex* pVertex = &vertices[0];
					IModelSource::Vertex* pVertexEnd = pVertex + 3;
					while (pVertex != pVertexEnd)
					{
						pVertex->uv.y = 1.0f - pVertex->uv.y;
						pVertex++;
					}
				}

				pPolygon++;
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// ノードの設定
		// ----------------------------------------------------------------------------------------------------

		node.firstPolygonIndex = static_cast<uint32_t>(firstPolygonIndex);
		node.polygonCount = polygonCount;
		node.hasUV = (pFbxUVs != nullptr);

		// ----------------------------------------------------------------------------------------------------

		return true;
	}

	bool FbxModelSource::LoadMeshUV(FbxLayerElementUV* pFbxUVs, size_t firstPolygonIndex, size_t polygonCount, const FbxModelSource::PolygonVertexRefVector& polygonVertexRefs)
	{
		FbxLayerElement::EMappingMode fbxMappingMode = pFbxUVs->GetMappingMode();
		FbxLayerElement::EReferenceMode fbxReferenceMode = pFbxUVs->GetReferenceMode();

		if (fbxMappingMode == FbxLayerElement::eByControlPoint)
		{
			if (fbxReferenceMode == FbxLayerElement::eDirect)
			{
				auto& directArray = pFbxUVs->GetDirectArray();
				VE_ASSERT(directArray.GetCount() == polygonVertexRefs.size());
				return false;
			}
			else if (fbxReferenceMode == FbxLayerElement::eIndexToDirect)
			{
				auto& directArray = pFbxUVs->GetDirectArray();
				auto& indexArray = pFbxUVs->GetIndexArray();
				VE_ASSERT(indexArray.GetCount() == polygonVertexRefs.size());
				return false;
			}
			else
			{
				return false;
			}
		}
		else if (fbxMappingMode == FbxLayerElement::eByPolygonVertex)
		{
			if (fbxReferenceMode == FbxLayerElement::eDirect)
			{
				auto& directArray = pFbxUVs->GetDirectArray();
				VE_ASSERT(directArray.GetCount() == polygonCount * 3);

				IModelSource::Polygon* pPolygon = m_Polygons.data() + firstPolygonIndex;
				IModelSource::Polygon* pPolygonEnd = pPolygon + polygonCount;

				int32_t index = 0;

				while (pPolygon != pPolygonEnd)
				{
					IModelSource::Vertex* pVertex = &pPolygon->vertices[0];
					IModelSource::Vertex* pVertexEnd = pVertex + 3;

					while (pVertex != pVertexEnd)
					{
						const FbxVector2& data = directArray[index++];

						pVertex->uv.x = static_cast<float>(data.mData[0]);
						pVertex->uv.y = static_cast<float>(data.mData[1]);

						pVertex++;
					}

					pPolygon++;
				}
			}
			else if (fbxReferenceMode == FbxLayerElement::eIndexToDirect)
			{
				auto& directArray = pFbxUVs->GetDirectArray();
				auto& indexArray = pFbxUVs->GetIndexArray();
				VE_ASSERT(indexArray.GetCount() == polygonCount * 3);

				IModelSource::Polygon* pPolygon = m_Polygons.data() + firstPolygonIndex;
				IModelSource::Polygon* pPolygonEnd = pPolygon + polygonCount;

				int32_t index = 0;

				while (pPolygon != pPolygonEnd)
				{
					IModelSource::Vertex* pVertex = &pPolygon->vertices[0];
					IModelSource::Vertex* pVertexEnd = pVertex + 3;

					while (pVertex != pVertexEnd)
					{
						const FbxVector2& data = directArray[indexArray[index++]];

						pVertex->uv.x = static_cast<float>(data.mData[0]);
						pVertex->uv.y = static_cast<float>(data.mData[1]);

						pVertex++;
					}

					pPolygon++;
				}
			}
			else
			{
				return false;
			}
		}
		else if (fbxMappingMode == FbxLayerElement::eByPolygon)
		{
			if (fbxReferenceMode == FbxLayerElement::eDirect)
			{
				auto& directArray = pFbxUVs->GetDirectArray();
				VE_ASSERT(directArray.GetCount() == polygonCount);

				IModelSource::Polygon* pPolygon = m_Polygons.data() + firstPolygonIndex;
				IModelSource::Polygon* pPolygonEnd = pPolygon + polygonCount;

				int32_t index = 0;

				while (pPolygon != pPolygonEnd)
				{
					const FbxVector2& data = directArray[index++];

					IModelSource::Vertex* pVertex = &pPolygon->vertices[0];
					IModelSource::Vertex* pVertexEnd = pVertex + 3;

					while (pVertex != pVertexEnd)
					{
						pVertex->uv.x = static_cast<float>(data.mData[0]);
						pVertex->uv.y = static_cast<float>(data.mData[1]);

						pVertex++;
					}

					pPolygon++;
				}
			}
			else if (fbxReferenceMode == FbxLayerElement::eIndexToDirect)
			{
				auto& directArray = pFbxUVs->GetDirectArray();
				auto& indexArray = pFbxUVs->GetIndexArray();
				VE_ASSERT(indexArray.GetCount() == polygonCount);

				IModelSource::Polygon* pPolygon = m_Polygons.data() + firstPolygonIndex;
				IModelSource::Polygon* pPolygonEnd = pPolygon + polygonCount;

				int32_t index = 0;

				while (pPolygon != pPolygonEnd)
				{
					const FbxVector2& data = directArray[indexArray[index++]];

					IModelSource::Vertex* pVertex = &pPolygon->vertices[0];
					IModelSource::Vertex* pVertexEnd = pVertex + 3;

					while (pVertex != pVertexEnd)
					{
						pVertex->uv.x = static_cast<float>(data.mData[0]);
						pVertex->uv.y = static_cast<float>(data.mData[1]);

						pVertex++;
					}

					pPolygon++;
				}
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}

		return true;
	}

	bool FbxModelSource::LoadMeshNormal(FbxLayerElementNormal* pFbxNormals, size_t firstPolygonIndex, size_t polygonCount, const FbxModelSource::PolygonVertexRefVector& polygonVertexRefs)
	{
		FbxLayerElement::EMappingMode fbxMappingMode = pFbxNormals->GetMappingMode();
		FbxLayerElement::EReferenceMode fbxReferenceMode = pFbxNormals->GetReferenceMode();

		if (fbxMappingMode == FbxLayerElement::eByControlPoint)
		{
			if (fbxReferenceMode == FbxLayerElement::eDirect)
			{
				auto& directArray = pFbxNormals->GetDirectArray();
				VE_ASSERT(directArray.GetCount() == polygonVertexRefs.size());
				return false;
			}
			else if (fbxReferenceMode == FbxLayerElement::eIndexToDirect)
			{
				auto& directArray = pFbxNormals->GetDirectArray();
				auto& indexArray = pFbxNormals->GetIndexArray();
				VE_ASSERT(indexArray.GetCount() == polygonVertexRefs.size());
				return false;
			}
			else
			{
				return false;
			}
		}
		else if (fbxMappingMode == FbxLayerElement::eByPolygonVertex)
		{
			if (fbxReferenceMode == FbxLayerElement::eDirect)
			{
				auto& directArray = pFbxNormals->GetDirectArray();
				VE_ASSERT(directArray.GetCount() == polygonCount * 3);

				IModelSource::Polygon* pPolygon = m_Polygons.data() + firstPolygonIndex;
				IModelSource::Polygon* pPolygonEnd = pPolygon + polygonCount;

				int32_t index = 0;

				while (pPolygon != pPolygonEnd)
				{
					IModelSource::Vertex* pVertex = &pPolygon->vertices[0];
					IModelSource::Vertex* pVertexEnd = pVertex + 3;

					while (pVertex != pVertexEnd)
					{
						const FbxVector4& data = directArray[index++];

						if (VE_DOUBLE_IS_ZERO(data.mData[3]) == true)
						{
							pVertex->normal.x = static_cast<float>(data.mData[0]);
							pVertex->normal.y = static_cast<float>(data.mData[1]);
							pVertex->normal.z = static_cast<float>(data.mData[2]);
						}
						else
						{
							pVertex->normal.x = static_cast<float>(VE_DOUBLE_DIV(data.mData[0], data.mData[3]));
							pVertex->normal.y = static_cast<float>(VE_DOUBLE_DIV(data.mData[1], data.mData[3]));
							pVertex->normal.z = static_cast<float>(VE_DOUBLE_DIV(data.mData[2], data.mData[3]));
						}

						pVertex++;
					}

					pPolygon++;
				}
			}
			else if (fbxReferenceMode == FbxLayerElement::eIndexToDirect)
			{
				auto& directArray = pFbxNormals->GetDirectArray();
				auto& indexArray = pFbxNormals->GetIndexArray();
				VE_ASSERT(indexArray.GetCount() == polygonCount * 3);

				IModelSource::Polygon* pPolygon = m_Polygons.data() + firstPolygonIndex;
				IModelSource::Polygon* pPolygonEnd = pPolygon + polygonCount;

				int32_t index = 0;

				while (pPolygon != pPolygonEnd)
				{
					IModelSource::Vertex* pVertex = &pPolygon->vertices[0];
					IModelSource::Vertex* pVertexEnd = pVertex + 3;

					while (pVertex != pVertexEnd)
					{
						const FbxVector4& data = directArray[index++];

						if (VE_DOUBLE_IS_ZERO(data.mData[3]) == true)
						{
							pVertex->normal.x = static_cast<float>(data.mData[0]);
							pVertex->normal.y = static_cast<float>(data.mData[1]);
							pVertex->normal.z = static_cast<float>(data.mData[2]);
						}
						else
						{
							pVertex->normal.x = static_cast<float>(VE_DOUBLE_DIV(data.mData[0], data.mData[3]));
							pVertex->normal.y = static_cast<float>(VE_DOUBLE_DIV(data.mData[1], data.mData[3]));
							pVertex->normal.z = static_cast<float>(VE_DOUBLE_DIV(data.mData[2], data.mData[3]));
						}

						pVertex++;
					}

					pPolygon++;
				}
			}
			else
			{
				return false;
			}
		}
		else if (fbxMappingMode == FbxLayerElement::eByPolygon)
		{
			if (fbxReferenceMode == FbxLayerElement::eDirect)
			{
				auto& directArray = pFbxNormals->GetDirectArray();
				VE_ASSERT(directArray.GetCount() == polygonCount);

				IModelSource::Polygon* pPolygon = m_Polygons.data() + firstPolygonIndex;
				IModelSource::Polygon* pPolygonEnd = pPolygon + polygonCount;

				int32_t index = 0;

				while (pPolygon != pPolygonEnd)
				{
					const FbxVector4& data = directArray[index++];

					IModelSource::Vertex* pVertex = &pPolygon->vertices[0];
					IModelSource::Vertex* pVertexEnd = pVertex + 3;

					while (pVertex != pVertexEnd)
					{
						if (VE_DOUBLE_IS_ZERO(data.mData[3]) == true)
						{
							pVertex->normal.x = static_cast<float>(data.mData[0]);
							pVertex->normal.y = static_cast<float>(data.mData[1]);
							pVertex->normal.z = static_cast<float>(data.mData[2]);
						}
						else
						{
							pVertex->normal.x = static_cast<float>(VE_DOUBLE_DIV(data.mData[0], data.mData[3]));
							pVertex->normal.y = static_cast<float>(VE_DOUBLE_DIV(data.mData[1], data.mData[3]));
							pVertex->normal.z = static_cast<float>(VE_DOUBLE_DIV(data.mData[2], data.mData[3]));
						}

						pVertex++;
					}

					pPolygon++;
				}
			}
			else if (fbxReferenceMode == FbxLayerElement::eIndexToDirect)
			{
				auto& directArray = pFbxNormals->GetDirectArray();
				auto& indexArray = pFbxNormals->GetIndexArray();
				VE_ASSERT(indexArray.GetCount() == polygonCount);

				IModelSource::Polygon* pPolygon = m_Polygons.data() + firstPolygonIndex;
				IModelSource::Polygon* pPolygonEnd = pPolygon + polygonCount;

				int32_t index = 0;

				while (pPolygon != pPolygonEnd)
				{
					const FbxVector4& data = directArray[index++];

					IModelSource::Vertex* pVertex = &pPolygon->vertices[0];
					IModelSource::Vertex* pVertexEnd = pVertex + 3;

					while (pVertex != pVertexEnd)
					{
						if (VE_DOUBLE_IS_ZERO(data.mData[3]) == true)
						{
							pVertex->normal.x = static_cast<float>(data.mData[0]);
							pVertex->normal.y = static_cast<float>(data.mData[1]);
							pVertex->normal.z = static_cast<float>(data.mData[2]);
						}
						else
						{
							pVertex->normal.x = static_cast<float>(VE_DOUBLE_DIV(data.mData[0], data.mData[3]));
							pVertex->normal.y = static_cast<float>(VE_DOUBLE_DIV(data.mData[1], data.mData[3]));
							pVertex->normal.z = static_cast<float>(VE_DOUBLE_DIV(data.mData[2], data.mData[3]));
						}

						pVertex++;
					}

					pPolygon++;
				}
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}

		return true;
	}

	bool FbxModelSource::LoadMeshMaterial(FbxNode* pFbxNode, FbxLayerElementMaterial* pFbxMaterials, size_t firstPolygonIndex, size_t polygonCount, const FbxModelSource::PolygonVertexRefVector& polygonVertexRefs)
	{
		FbxLayerElement::EMappingMode fbxMappingMode = pFbxMaterials->GetMappingMode();
		FbxLayerElement::EReferenceMode fbxReferenceMode = pFbxMaterials->GetReferenceMode();

		if (fbxMappingMode == FbxLayerElement::eByPolygon)
		{
			const char* pName = pFbxMaterials->GetName();

			if (fbxReferenceMode == FbxLayerElement::eIndexToDirect)
			{
				auto& indexArray = pFbxMaterials->GetIndexArray();
				int32_t count = indexArray.GetCount();
				VE_ASSERT(count == polygonCount);

				IModelSource::Polygon* pPolygon = m_Polygons.data() + firstPolygonIndex;
				IModelSource::Polygon* pPolygonEnd = pPolygon + polygonCount;

				int32_t index = 0;

				while (pPolygon != pPolygonEnd)
				{
					pPolygon->materialIndex = GetMaterialIndex(pFbxNode, indexArray[index++], m_Materials);
					pPolygon++;
				}
			}
			else
			{
				return false;
			}
		}
		else if(fbxMappingMode == FbxLayerElement::eAllSame)
		{
			if (fbxReferenceMode == FbxLayerElement::eIndexToDirect)
			{
				int32_t materialIndex = GetMaterialIndex(pFbxNode, pFbxMaterials->GetIndexArray()[0], m_Materials);

				IModelSource::Polygon* pPolygon = m_Polygons.data() + firstPolygonIndex;
				IModelSource::Polygon* pPolygonEnd = pPolygon + polygonCount;

				while (pPolygon != pPolygonEnd)
				{
					pPolygon->materialIndex = materialIndex;
					pPolygon++;
				}
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}

		return true;
	}

	void FbxModelSource::CreateOBB(
		collection::Vector<glm::vec3>& points,
		glm::vec3& center, glm::vec3& xAxis, glm::vec3& yAxis, glm::vec3& zAxis, glm::vec3& halfExtent)
	{
		const glm::vec3* pPointBegin = &(points[0]);
		const glm::vec3* pPointEnd = pPointBegin + points.size();
		const glm::vec3* pPoint;

		// ----------------------------------------------------------------------------------------------------
		// 中心座標および分散共分散行列を求める
		// ----------------------------------------------------------------------------------------------------

		center = glm::vec3(0.0f);

		pPoint = pPointBegin;
		while (pPoint != pPointEnd)
		{
			center += *pPoint++;
		}

		center /= static_cast<float>(points.size());

		// ----------------------------------------------------------------------------------------------------
		// 分散共分散行列を求める
		// ----------------------------------------------------------------------------------------------------

		const float& cx = center.x;
		const float& cy = center.y;
		const float& cz = center.z;

		float c00 = 0.0f;
		float c11 = 0.0f;
		float c22 = 0.0f;
		float c01 = 0.0f;
		float c02 = 0.0f;
		float c12 = 0.0f;

		pPoint = pPointBegin;
		while (pPoint != pPointEnd)
		{
			c00 += (pPoint->x - cx) * (pPoint->x - cx);
			c11 += (pPoint->y - cy) * (pPoint->y - cy);
			c22 += (pPoint->z - cz) * (pPoint->z - cz);
			c01 += (pPoint->x - cx) * (pPoint->y - cy);
			c02 += (pPoint->x - cx) * (pPoint->z - cz);
			c12 += (pPoint->y - cy) * (pPoint->z - cz);

			pPoint++;
		}

		float invSizeF = 1.0f / static_cast<float>(points.size());
		float vcMat[3][3];

		c00 *= invSizeF;
		c11 *= invSizeF;
		c22 *= invSizeF;
		c01 *= invSizeF;
		c02 *= invSizeF;
		c12 *= invSizeF;

		vcMat[0][0] = c00;
		vcMat[0][1] = c01;
		vcMat[0][2] = c02;
		vcMat[1][0] = c01;
		vcMat[1][1] = c11;
		vcMat[1][2] = c12;
		vcMat[2][0] = c02;
		vcMat[2][1] = c12;
		vcMat[2][2] = c22;

		// ----------------------------------------------------------------------------------------------------
		// 固有行列を求める
		// ----------------------------------------------------------------------------------------------------

		int32_t loop = 0;
		int32_t i;
		int32_t p;
		int32_t q;
		float jmax;
		float app, apq, aqq;
		float alpha, beta, gamma;
		float ab2;
		float s, c;
		float temp;
		float eigenMat[3][3]{};

		eigenMat[0][0] = 1.0f;
		eigenMat[1][1] = 1.0f;
		eigenMat[2][2] = 1.0f;

		do
		{
			jmax = Jacobi_GetMax(vcMat, p, q);

			app = vcMat[p][p];
			apq = vcMat[p][q];
			aqq = vcMat[q][q];

			alpha = (app - aqq) / 2.0f;
			beta = -apq;

			ab2 = alpha * alpha + beta * beta;
			if (VE_FLOAT_IS_ZERO(ab2) == false)
			{
				gamma = ::fabs(alpha) / ::sqrtf(alpha * alpha + beta * beta);
			}
			else
			{
				gamma = 0.0f;
			}

			s = ::sqrtf((1.0f - gamma) / 2.0f);
			c = ::sqrtf((1.0f + gamma) / 2.0f);
			if ((alpha * beta) < 0.0f)
			{
				s = -s;
			}

			for (i = 0; i < 3; i++)
			{
				temp = c * vcMat[p][i] - s * vcMat[q][i];
				vcMat[q][i] = s * vcMat[p][i] + c * vcMat[q][i];
				vcMat[p][i] = temp;
			}

			for (i = 0; i < 3; i++)
			{
				vcMat[i][p] = vcMat[p][i];
				vcMat[i][q] = vcMat[q][i];
			}

			vcMat[p][p] = (c * c * app) + (s * s * aqq) - (2.0f * s * c * apq);
			vcMat[p][q] = (s * c * (app - aqq)) + ((c * c - s*s) * apq);
			vcMat[q][p] = (s * c * (app - aqq)) + ((c * c - s * s) * apq);
			vcMat[q][q] = (s * s * app) + (c * c * aqq) + (2.0f * s * c * apq);

			for (i = 0; i < 3; i++)
			{
				temp = (c * eigenMat[i][p]) - (s * eigenMat[i][q]);
				eigenMat[i][q] = (s * eigenMat[i][p]) + (c * eigenMat[i][q]);
				eigenMat[i][p] = temp;
			}

		} while ((jmax > FLT_EPSILON) && (++loop < 100));

		// ----------------------------------------------------------------------------------------------------
		// OBB を求める
		// ----------------------------------------------------------------------------------------------------

		xAxis = glm::vec3(eigenMat[0][0], eigenMat[1][0], eigenMat[2][0]);
		yAxis = glm::vec3(eigenMat[0][1], eigenMat[1][1], eigenMat[2][1]);
		zAxis = glm::vec3(eigenMat[0][2], eigenMat[1][2], eigenMat[2][2]);

		glm::vec3 minimum(+VE_FLOAT_MAX);
		glm::vec3 maximum(-VE_FLOAT_MAX);

		pPoint = pPointBegin;
		while (pPoint != pPointEnd)
		{
			float lx = glm::dot(*pPoint, xAxis);
			float ly = glm::dot(*pPoint, yAxis);
			float lz = glm::dot(*pPoint, zAxis);

			if (minimum.x > lx) { minimum.x = lx; }
			if (minimum.y > ly) { minimum.y = ly; }
			if (minimum.z > lz) { minimum.z = lz; }

			if (maximum.x < lx) { maximum.x = lx; }
			if (maximum.y < ly) { maximum.y = ly; }
			if (maximum.z < lz) { maximum.z = lz; }

			pPoint++;
		}

		//中心
		center = (xAxis * ((minimum.x + maximum.x) * 0.5f)) + (yAxis * ((minimum.y + maximum.y) * 0.5f)) + (zAxis * ((minimum.z + maximum.z) * 0.5f));

		//長さ
		halfExtent = (maximum - minimum) * 0.5f;

		// ----------------------------------------------------------------------------------------------------
	}

	float FbxModelSource::Jacobi_GetMax(const float mat[3][3], int32_t& p, int32_t& q)
	{
		int i;
		int j;
		float ret;
		float temp;

		ret = ::fabs(mat[0][1]);
		p = 0;
		q = 1;

		for (i = 0; i < 3; i++)
		{
			for (j = (i + 1); j < 3; j++)
			{
				temp = ::fabs(mat[i][j]);

				if (temp > ret)
				{
					ret = temp;
					p = i;
					q = j;
				}
			}
		}

		return ret;
	}

	int32_t FbxModelSource::GetMaterialIndex(FbxNode* pFbxNode, int32_t fbxMaterialIndex, collection::Vector<IModelSource::Material>& materials)
	{
		FbxSurfaceMaterial* pFbxSurfaceMaterial = pFbxNode->GetMaterial(fbxMaterialIndex);
		if (pFbxSurfaceMaterial == nullptr)
		{
			return false;
		}

		StringW fbxMaterialName;
		ToWideString(pFbxSurfaceMaterial->GetName(), fbxMaterialName);

		auto it_material = std::find_if(materials.begin(), materials.end(),
			[fbxMaterialName](const IModelSource::Material& material) { return wcscmp(fbxMaterialName.c_str(), material.name.c_str()) == 0; });

		if (it_material == materials.end())
		{
			return -1;
		}

		return static_cast<int32_t>(std::distance(materials.begin(), it_material));
	}

	void FbxModelSource::GetMaterialTexture(const FbxProperty& prop, MODEL_SOURCE_PATH_TYPE pathType, StringW& texture)
	{
		int32_t layeredTextureCount = prop.GetSrcObjectCount<FbxLayeredTexture>();
		if (layeredTextureCount > 0)
		{
			for (int32_t i = 0; layeredTextureCount > i; i++)
			{
				FbxLayeredTexture* layeredTexture = prop.GetSrcObject<FbxLayeredTexture>(i);
				int textureCount = layeredTexture->GetSrcObjectCount<FbxLayeredTexture>();

				for (int k = 0; textureCount > k; k++)
				{
					FbxLayeredTexture* pTexture = prop.GetSrcObject<FbxLayeredTexture>(k);

					if (pTexture != nullptr)
					{
						ToWideString(pTexture->GetName(), texture);
					}
				}
			}
		}
		else
		{
			int32_t fileTextureCount = prop.GetSrcObjectCount<FbxFileTexture>();
			if (0 < fileTextureCount)
			{
				for (int i = 0; fileTextureCount > i; i++)
				{
					FbxFileTexture* pTexture = prop.GetSrcObject<FbxFileTexture>(i);
					if (pTexture != nullptr)
					{
						switch (pathType)
						{
						case MODEL_SOURCE_PATH_TYPE_RELATIVE:
							ToWideString(pTexture->GetRelativeFileName(), texture);
							break;
						case MODEL_SOURCE_PATH_TYPE_STRIP:
							ToWideString(pTexture->GetFileName(), texture);
							break;
						}
					}
				}
			}
		}
	}

	glm::mat4 FbxModelSource::ToGlmMatrix(const FbxAMatrix& fbxMatrix)
	{
		FbxVector4 row0 = fbxMatrix.GetRow(0);
		FbxVector4 row1 = fbxMatrix.GetRow(1);
		FbxVector4 row2 = fbxMatrix.GetRow(2);
		FbxVector4 row3 = fbxMatrix.GetRow(3);

		glm::mat4 mat;
		mat[0].x = static_cast<float>(row0.mData[0]);
		mat[0].y = static_cast<float>(row0.mData[1]);
		mat[0].z = static_cast<float>(row0.mData[2]);
		mat[0].w = static_cast<float>(row0.mData[3]);
		mat[1].x = static_cast<float>(row1.mData[0]);
		mat[1].y = static_cast<float>(row1.mData[1]);
		mat[1].z = static_cast<float>(row1.mData[2]);
		mat[1].w = static_cast<float>(row1.mData[3]);
		mat[2].x = static_cast<float>(row2.mData[0]);
		mat[2].y = static_cast<float>(row2.mData[1]);
		mat[2].z = static_cast<float>(row2.mData[2]);
		mat[2].w = static_cast<float>(row2.mData[3]);
		mat[3].x = static_cast<float>(row3.mData[0]);
		mat[3].y = static_cast<float>(row3.mData[1]);
		mat[3].z = static_cast<float>(row3.mData[2]);
		mat[3].w = static_cast<float>(row3.mData[3]);

		return mat;
	}

}