#include "SkeletalMesh.h"
#include "Logger.h"
#include "DeviceContext.h"
#include "DeletingQueue.h"
#include "ImmediateContext.h"
#include "ResourceMemoryManager.h"
#include "DynamicBuffer.h"
#include "Material.h"
#include "DebugRenderer.h"
#include "IModel.h"

namespace ve {

	/*************************/
	/* public - SkeletalMesh */
	/*************************/

	SkeletalMeshPtr SkeletalMesh::Create(DeviceContextPtr deviceContext, int32_t id)
	{
		SkeletalMeshPtr mesh = std::make_shared<SkeletalMesh>();

		mesh->m_DeviceContext = deviceContext;
		mesh->m_ID = id;

		return std::move(mesh);
	}

	SkeletalMesh::SkeletalMesh() :
		m_ID(-1),
		m_PolygonCount(0),
		m_SelectSubset({}),
		m_Uniform({}),
		m_pUniformBuffer(nullptr),
		m_VertexBufferSize(0),
		m_VertexBuffer({}),
		m_IndexBufferSize(0),
		m_IndexBuffer({}),
		m_IndexType(V3D_INDEX_TYPE_UINT16),
		m_Visible(true),
		m_CastShadow(true)
	{
		m_pNativeDescriptorSet[SkeletalMesh::DST_COLOR] = nullptr;
		m_pNativeDescriptorSet[SkeletalMesh::DST_SHADOW] = nullptr;
	}

	SkeletalMesh::~SkeletalMesh()
	{
		Dispose();
	}

	int32_t SkeletalMesh::GetID() const
	{
		return m_ID;
	}

	bool SkeletalMesh::Load(LoggerPtr logger, IModel* pModel, HANDLE fileHandle)
	{
		// ----------------------------------------------------------------------------------------------------
		// ファイルヘッダーを読み込む
		// ----------------------------------------------------------------------------------------------------

		SkeletalMesh::File_FileHeader fileHeader;

		if (FileRead(fileHandle, sizeof(SkeletalMesh::File_FileHeader), &fileHeader) == false)
		{
			logger->PrintA(Logger::TYPE_ERROR, "Failed to read the file");
			return false;
		}

		if (fileHeader.magicNumber != ToMagicNumber('M', 'E', 'S', 'H'))
		{
			logger->PrintA(Logger::TYPE_ERROR, "Unsupported mesh in the model file");
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// インフォヘッダーを読み込む
		// ----------------------------------------------------------------------------------------------------

		SkeletalMesh::File_InfoHeader infoHeader;

		if (fileHeader.version == SkeletalMesh::CURRENT_VERSION)
		{
			if (FileRead(fileHandle, sizeof(SkeletalMesh::File_InfoHeader), &infoHeader) == false)
			{
				logger->PrintA(Logger::TYPE_ERROR, "Failed to read the file");
				return false;
			}
		}
		else if (fileHeader.version == SkeletalMesh::OLD_VERSION_1_0_0_0)
		{
			SkeletalMesh::File_InfoHeader_1_0_0_0 oldInfoHeader;

			if (FileRead(fileHandle, sizeof(SkeletalMesh::File_InfoHeader_1_0_0_0), &oldInfoHeader) == false)
			{
				logger->PrintA(Logger::TYPE_ERROR, "Failed to read the file");
				return false;
			}

			infoHeader.flags = SkeletalMesh::File_INFO_ALL_FLAGS;
			infoHeader.polygonCount = oldInfoHeader.polygonCount;
			infoHeader.materialCount = oldInfoHeader.materialCount;
			infoHeader.debriVertexCount = oldInfoHeader.debriVertexCount;
			infoHeader.colorSubsetCount = oldInfoHeader.colorSubsetCount;
			infoHeader.boneCount = oldInfoHeader.boneCount;
			infoHeader.shapeCount = oldInfoHeader.shapeCount;
			infoHeader.vertexBufferSize = oldInfoHeader.vertexBufferSize;
			infoHeader.indexBufferSize = oldInfoHeader.indexBufferSize;
			infoHeader.indexType = oldInfoHeader.indexType;
		}
		else
		{
			logger->PrintA(Logger::TYPE_ERROR, "Unsupported mesh version : Version[0x%.8x] CurrentVersion[0x%.8x]", fileHeader.version, SkeletalMesh::CURRENT_VERSION);
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// 前準備
		// ----------------------------------------------------------------------------------------------------

		m_Visible = (infoHeader.flags & SkeletalMesh::File_INFO_VISIBLE) ? true : false;
		m_CastShadow = (infoHeader.flags & SkeletalMesh::File_INFO_CAST_SHADOW) ? true : false;

		m_PolygonCount = infoHeader.polygonCount;

		m_VertexBufferSize = infoHeader.vertexBufferSize;
		m_IndexBufferSize = infoHeader.indexBufferSize;
		m_IndexType = static_cast<V3D_INDEX_TYPE>(infoHeader.indexType);

		// m_Bones.reserve
		// m_Shapes.reserve
		// m_WorldMatrices.resize
		if (Preparation(infoHeader.boneCount) == false)
		{
			logger->PrintA(Logger::TYPE_ERROR, "Mesh preparation failed");
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// マテリアルインデックスを読み込む
		// ----------------------------------------------------------------------------------------------------

		m_MaterialIndices.resize(infoHeader.materialCount);

		if (FileRead(fileHandle, sizeof(uint32_t) * m_MaterialIndices.size(), m_MaterialIndices.data()) == false)
		{
			logger->PrintA(Logger::TYPE_ERROR, "Failed to read the file");
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// サブセットを読み込む
		// ----------------------------------------------------------------------------------------------------

		m_ColorSubsets.reserve(infoHeader.colorSubsetCount);

		{
			/**********************/
			/* デブリバーテックス */
			/**********************/

			collection::Vector<SkeletalMesh::File_DebriVertex> debriVertices;
			debriVertices.resize(infoHeader.debriVertexCount);

			if (FileRead(fileHandle, sizeof(File_DebriVertex) * debriVertices.size(), debriVertices.data()) == false)
			{
				logger->PrintA(Logger::TYPE_ERROR, "Failed to read the file");
				return false;
			}

			/********************/
			/* カラーサブセット */
			/********************/

			collection::Vector<SkeletalMesh::File_ColorSubset> colorSubsets;
			colorSubsets.resize(infoHeader.colorSubsetCount);

			if (FileRead(fileHandle, sizeof(File_ColorSubset) * colorSubsets.size(), colorSubsets.data()) == false)
			{
				logger->PrintA(Logger::TYPE_ERROR, "Failed to read the file");
				return false;
			}

			auto it_cs_begin = colorSubsets.begin();
			auto it_cs_end = colorSubsets.end();

			for (auto it_cs = it_cs_begin; it_cs != it_cs_end; ++it_cs)
			{
				const SkeletalMesh::File_ColorSubset& srcColorSubset = (*it_cs);

				VE_ASSERT((srcColorSubset.debriVertexCount % 3) == 0);

				m_ColorSubsets.push_back(SkeletalMesh::ColorSubset{});

				SkeletalMesh::ColorSubset& dstColorSubset = m_ColorSubsets.back();
				dstColorSubset.materialIndex = srcColorSubset.materialIndex;
				dstColorSubset.indexCount = srcColorSubset.indexCount;
				dstColorSubset.firstIndex = srcColorSubset.firstIndex;
				dstColorSubset.debriPolygon.reserve(srcColorSubset.debriVertexCount / 3);

				for (uint32_t i = 0; i < srcColorSubset.debriVertexCount; i += 3)
				{
					SkeletalMesh::DebriPolygon dstDebriPolygon;

					for (uint32_t j = 0; j < 3; j++)
					{
						const SkeletalMesh::File_DebriVertex& srcDebriVertex = debriVertices[srcColorSubset.firstDebriVertex + i + j];
						dstDebriPolygon.vertices[j].pos[0] = srcDebriVertex.pos[0];
						dstDebriPolygon.vertices[j].pos[1] = srcDebriVertex.pos[1];
						dstDebriPolygon.vertices[j].pos[2] = srcDebriVertex.pos[2];
						dstDebriPolygon.vertices[j].worldMatrixIndex = srcDebriVertex.worldMatrixIndex;
						dstDebriPolygon.vertices[j].pWorldMatrix = &m_WorldMatrices[srcDebriVertex.worldMatrixIndex];
					}

					dstColorSubset.debriPolygon.push_back(dstDebriPolygon);
				}
			}

			/**********************/
			/* セレクトサブセット */
			/**********************/

			SkeletalMesh::File_SelectSubset srcSelectSubset;

			if (FileRead(fileHandle, sizeof(SkeletalMesh::File_SelectSubset), &srcSelectSubset) == false)
			{
				logger->PrintA(Logger::TYPE_ERROR, "Failed to read the file");
				return false;
			}

			m_SelectSubset.indexCount = srcSelectSubset.indexCount;
			m_SelectSubset.firstIndex = srcSelectSubset.firstIndex;
		}

		// ----------------------------------------------------------------------------------------------------
		// ボーンを読み込む
		// ----------------------------------------------------------------------------------------------------

		{
			collection::Vector<SkeletalMesh::File_Bone> bones;
			bones.resize(infoHeader.boneCount);

			if (FileRead(fileHandle, sizeof(SkeletalMesh::File_Bone) * bones.size(), bones.data()) == false)
			{
				logger->PrintA(Logger::TYPE_ERROR, "Failed to read the file");
				return false;
			}

			auto it_begin = bones.begin();
			auto it_end = bones.end();

			for (auto it = it_begin; it != it_end; ++it)
			{
				const SkeletalMesh::File_Bone& srcBone = (*it);

				SkeletalMesh::Bone dstBone;
				dstBone.node = pModel->GetNode(srcBone.nodeIndex);
				dstBone.offsetMatrix[0] = glm::vec4(srcBone.offsetMatrix[0][0], srcBone.offsetMatrix[0][1], srcBone.offsetMatrix[0][2], srcBone.offsetMatrix[0][3]);
				dstBone.offsetMatrix[1] = glm::vec4(srcBone.offsetMatrix[1][0], srcBone.offsetMatrix[1][1], srcBone.offsetMatrix[1][2], srcBone.offsetMatrix[1][3]);
				dstBone.offsetMatrix[2] = glm::vec4(srcBone.offsetMatrix[2][0], srcBone.offsetMatrix[2][1], srcBone.offsetMatrix[2][2], srcBone.offsetMatrix[2][3]);
				dstBone.offsetMatrix[3] = glm::vec4(srcBone.offsetMatrix[3][0], srcBone.offsetMatrix[3][1], srcBone.offsetMatrix[3][2], srcBone.offsetMatrix[3][3]);

				m_Bones.push_back(dstBone);
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// シェイプを読み込む
		// ----------------------------------------------------------------------------------------------------

		{
			collection::Vector<SkeletalMesh::File_Shape> shapes;
			shapes.resize(infoHeader.shapeCount);

			if (FileRead(fileHandle, sizeof(SkeletalMesh::File_Shape) * shapes.size(), shapes.data()) == false)
			{
				logger->PrintA(Logger::TYPE_ERROR, "Failed to read the file");
				return false;
			}

			auto it_begin = shapes.begin();
			auto it_end = shapes.end();

			for (auto it = it_begin; it != it_end; ++it)
			{
				const SkeletalMesh::File_Shape& srcShape = (*it);

				SkeletalMesh::Shape dstShape;
				dstShape.center.x = srcShape.center[0];
				dstShape.center.y = srcShape.center[1];
				dstShape.center.z = srcShape.center[2];
				dstShape.axis[0].x = srcShape.xAxis[0];
				dstShape.axis[0].y = srcShape.xAxis[1];
				dstShape.axis[0].z = srcShape.xAxis[2];
				dstShape.axis[1].x = srcShape.yAxis[0];
				dstShape.axis[1].y = srcShape.yAxis[1];
				dstShape.axis[1].z = srcShape.yAxis[2];
				dstShape.axis[2].x = srcShape.zAxis[0];
				dstShape.axis[2].y = srcShape.zAxis[1];
				dstShape.axis[2].z = srcShape.zAxis[2];
				dstShape.halfExtent.x = srcShape.halfExtent[0];
				dstShape.halfExtent.y = srcShape.halfExtent[1];
				dstShape.halfExtent.z = srcShape.halfExtent[2];

				m_Shapes.push_back(dstShape);
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// バッファーを読み込む
		// ----------------------------------------------------------------------------------------------------

		{
			/************************************/
			/* バーテックスバッファーを読み込む */
			/************************************/

			collection::Vector<uint8_t> srcVertexBuffer;
			srcVertexBuffer.resize(VE_U64_TO_U32(infoHeader.vertexBufferSize));

			if (FileRead(fileHandle, infoHeader.vertexBufferSize, srcVertexBuffer.data()) == false)
			{
				logger->PrintA(Logger::TYPE_ERROR, "Failed to read the file");
				return false;
			}

			/************************************/
			/* インデックスバッファーを読み込む */
			/************************************/

			collection::Vector<uint8_t> srcIndexBuffer;
			srcIndexBuffer.resize(VE_U64_TO_U32(infoHeader.indexBufferSize));

			if (FileRead(fileHandle, infoHeader.indexBufferSize, srcIndexBuffer.data()) == false)
			{
				logger->PrintA(Logger::TYPE_ERROR, "Failed to read the file");
				return false;
			}

			/****************/
			/* アップロード */
			/****************/

			bool uploadResult;

			m_DeviceContext->GetImmediateContextPtr()->Begin();

			// バーテックスバッファー
			uploadResult = m_DeviceContext->GetImmediateContextPtr()->Upload(
				srcVertexBuffer.data(),
				infoHeader.vertexBufferSize,
				V3D_BUFFER_USAGE_TRANSFER_SRC | V3D_BUFFER_USAGE_VERTEX,
				V3D_PIPELINE_STAGE_VERTEX_INPUT, V3D_ACCESS_VERTEX_READ,
				&m_VertexBuffer.pResource, &m_VertexBuffer.resourceAllocation, VE_INTERFACE_DEBUG_NAME(L"VE_VertexBuffer"));

			if (uploadResult == false)
			{
				logger->PrintA(Logger::TYPE_ERROR, "Vertex buffer upload failed");
			}

			// インデックスバッファー
			if (uploadResult == true)
			{
				uploadResult = m_DeviceContext->GetImmediateContextPtr()->Upload(
					srcIndexBuffer.data(),
					infoHeader.indexBufferSize,
					V3D_BUFFER_USAGE_TRANSFER_SRC | V3D_BUFFER_USAGE_INDEX,
					V3D_PIPELINE_STAGE_VERTEX_INPUT, V3D_ACCESS_INDEX_READ,
					&m_IndexBuffer.pResource, &m_IndexBuffer.resourceAllocation, VE_INTERFACE_DEBUG_NAME(L"VE_IndexBuffer"));
			}

			m_DeviceContext->GetImmediateContextPtr()->End();

			if (uploadResult == false)
			{
				logger->PrintA(Logger::TYPE_ERROR, "Index buffer upload failed");
				return false;
			}
		}

		// ----------------------------------------------------------------------------------------------------

		return true;
	}

	bool SkeletalMesh::Save(LoggerPtr logger, HANDLE fileHandle)
	{
		// ----------------------------------------------------------------------------------------------------
		// デブリバーテックス、各種サブセットのリストを作成
		// ----------------------------------------------------------------------------------------------------

		collection::Vector<SkeletalMesh::File_DebriVertex> debriVertices;
		collection::Vector<SkeletalMesh::File_ColorSubset> colorSubsets;
		SkeletalMesh::File_SelectSubset selectSubset;

		{
			auto it_begin = m_ColorSubsets.begin();
			auto it_end = m_ColorSubsets.end();

			size_t debriPolygonCount = 0;
			for (auto it = it_begin; it != it_end; ++it)
			{
				debriPolygonCount += it->debriPolygon.size();
			}

			debriVertices.reserve(debriPolygonCount * 3);
			colorSubsets.reserve(m_ColorSubsets.size());

			for (auto it = it_begin; it != it_end; ++it)
			{
				const SkeletalMesh::ColorSubset& srcColorSubset = (*it);

				size_t firstDebriVertex = debriVertices.size();
				auto it_dp_begin = srcColorSubset.debriPolygon.begin();
				auto it_dp_end = srcColorSubset.debriPolygon.end();
				for (auto it_dp = it_dp_begin; it_dp != it_dp_end; ++it_dp)
				{
					const SkeletalMesh::DebriPolygon& srcDebriPolygon = (*it_dp);

					for (uint32_t i = 0; i < 3; i++)
					{
						const SkeletalMesh::DebriVertex& srcDebriVertex = srcDebriPolygon.vertices[i];

						SkeletalMesh::File_DebriVertex dstDebriVertex;
						dstDebriVertex.pos[0] = srcDebriVertex.pos.x;
						dstDebriVertex.pos[1] = srcDebriVertex.pos.y;
						dstDebriVertex.pos[2] = srcDebriVertex.pos.z;
						dstDebriVertex.pos[3] = 1.0f;
						dstDebriVertex.worldMatrixIndex = srcDebriVertex.worldMatrixIndex;

						debriVertices.push_back(dstDebriVertex);
					}
				}

				SkeletalMesh::File_ColorSubset dstColorSubset;
				dstColorSubset.materialIndex = srcColorSubset.materialIndex;
				dstColorSubset.debriVertexCount = static_cast<uint32_t>(srcColorSubset.debriPolygon.size() * 3);
				dstColorSubset.firstDebriVertex = static_cast<uint32_t>(firstDebriVertex);
				dstColorSubset.indexCount = srcColorSubset.indexCount;
				dstColorSubset.firstIndex = srcColorSubset.firstIndex;

				colorSubsets.push_back(dstColorSubset);
			}

			selectSubset.indexCount = m_SelectSubset.indexCount;
			selectSubset.firstIndex = m_SelectSubset.firstIndex;
		}

		// ----------------------------------------------------------------------------------------------------
		// ボーンのリストを作成
		// ----------------------------------------------------------------------------------------------------

		collection::Vector<SkeletalMesh::File_Bone> bones;
		bones.reserve(m_Bones.size());

		{
			auto it_begin = m_Bones.begin();
			auto it_end = m_Bones.end();

			for (auto it = it_begin; it != it_end; ++it)
			{
				const SkeletalMesh::Bone& srcBone = (*it);

				SkeletalMesh::File_Bone dstBone;
				dstBone.nodeIndex = srcBone.node.lock()->GetID();
				dstBone.offsetMatrix[0][0] = srcBone.offsetMatrix[0].x;
				dstBone.offsetMatrix[0][1] = srcBone.offsetMatrix[0].y;
				dstBone.offsetMatrix[0][2] = srcBone.offsetMatrix[0].z;
				dstBone.offsetMatrix[0][3] = srcBone.offsetMatrix[0].w;
				dstBone.offsetMatrix[1][0] = srcBone.offsetMatrix[1].x;
				dstBone.offsetMatrix[1][1] = srcBone.offsetMatrix[1].y;
				dstBone.offsetMatrix[1][2] = srcBone.offsetMatrix[1].z;
				dstBone.offsetMatrix[1][3] = srcBone.offsetMatrix[1].w;
				dstBone.offsetMatrix[2][0] = srcBone.offsetMatrix[2].x;
				dstBone.offsetMatrix[2][1] = srcBone.offsetMatrix[2].y;
				dstBone.offsetMatrix[2][2] = srcBone.offsetMatrix[2].z;
				dstBone.offsetMatrix[2][3] = srcBone.offsetMatrix[2].w;
				dstBone.offsetMatrix[3][0] = srcBone.offsetMatrix[3].x;
				dstBone.offsetMatrix[3][1] = srcBone.offsetMatrix[3].y;
				dstBone.offsetMatrix[3][2] = srcBone.offsetMatrix[3].z;
				dstBone.offsetMatrix[3][3] = srcBone.offsetMatrix[3].w;

				bones.push_back(dstBone);
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// シェイプのリストを作成
		// ----------------------------------------------------------------------------------------------------

		collection::Vector<SkeletalMesh::File_Shape> shapes;
		shapes.reserve(m_Shapes.size());

		{
			auto it_begin = m_Shapes.begin();
			auto it_end = m_Shapes.end();

			for (auto it = it_begin; it != it_end; ++it)
			{
				const SkeletalMesh::Shape& srcShape = (*it);

				SkeletalMesh::File_Shape dstShape;
				dstShape.center[0] = srcShape.center.x;
				dstShape.center[1] = srcShape.center.y;
				dstShape.center[2] = srcShape.center.z;
				dstShape.center[3] = 1.0f;
				dstShape.xAxis[0] = srcShape.axis[0].x;
				dstShape.xAxis[1] = srcShape.axis[0].y;
				dstShape.xAxis[2] = srcShape.axis[0].z;
				dstShape.xAxis[3] = 1.0f;
				dstShape.yAxis[0] = srcShape.axis[1].x;
				dstShape.yAxis[1] = srcShape.axis[1].y;
				dstShape.yAxis[2] = srcShape.axis[1].z;
				dstShape.yAxis[3] = 1.0f;
				dstShape.zAxis[0] = srcShape.axis[2].x;
				dstShape.zAxis[1] = srcShape.axis[2].y;
				dstShape.zAxis[2] = srcShape.axis[2].z;
				dstShape.zAxis[3] = 1.0f;
				dstShape.halfExtent[0] = srcShape.halfExtent.x;
				dstShape.halfExtent[1] = srcShape.halfExtent.y;
				dstShape.halfExtent[2] = srcShape.halfExtent.z;
				dstShape.halfExtent[3] = 1.0f;

				shapes.push_back(dstShape);
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// ファイルヘッダーを書き込む
		// ----------------------------------------------------------------------------------------------------

		SkeletalMesh::File_FileHeader fileHeader;
		fileHeader.magicNumber = ToMagicNumber('M', 'E', 'S', 'H');
		fileHeader.version = SkeletalMesh::CURRENT_VERSION;

		if (FileWrite(fileHandle, sizeof(SkeletalMesh::File_FileHeader), &fileHeader) == false)
		{
			logger->PrintA(Logger::TYPE_ERROR, "Failed to write the file");
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// インフォヘッダーを書き込む
		// ----------------------------------------------------------------------------------------------------

		SkeletalMesh::File_InfoHeader infoHeader;
		infoHeader.flags = 0;
		if (m_Visible == true) { infoHeader.flags |= SkeletalMesh::File_INFO_VISIBLE; }
		if (m_CastShadow == true) { infoHeader.flags |= SkeletalMesh::File_INFO_CAST_SHADOW; }
		infoHeader.polygonCount = m_PolygonCount;
		infoHeader.materialCount = static_cast<uint32_t>(m_MaterialIndices.size());
		infoHeader.debriVertexCount = static_cast<uint32_t>(debriVertices.size());
		infoHeader.colorSubsetCount = static_cast<uint32_t>(colorSubsets.size());
		infoHeader.boneCount = static_cast<uint32_t>(bones.size());
		infoHeader.shapeCount = static_cast<uint32_t>(shapes.size());
		infoHeader.vertexBufferSize = m_VertexBufferSize;
		infoHeader.indexBufferSize = m_IndexBufferSize;
		infoHeader.indexType = m_IndexType;

		if (FileWrite(fileHandle, sizeof(SkeletalMesh::File_InfoHeader), &infoHeader) == false)
		{
			logger->PrintA(Logger::TYPE_ERROR, "Failed to write the file");
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// マテリアルインデックスを書き込む
		// ----------------------------------------------------------------------------------------------------

		if (FileWrite(fileHandle, sizeof(uint32_t) * m_MaterialIndices.size(), m_MaterialIndices.data()) == false)
		{
			logger->PrintA(Logger::TYPE_ERROR, "Failed to write the file");
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// デブリバーテックスを書き込む
		// ----------------------------------------------------------------------------------------------------

		if (FileWrite(fileHandle, sizeof(File_DebriVertex) * debriVertices.size(), debriVertices.data()) == false)
		{
			logger->PrintA(Logger::TYPE_ERROR, "Failed to write the file");
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// カラーサブセットを書き込む
		// ----------------------------------------------------------------------------------------------------

		if (FileWrite(fileHandle, sizeof(File_ColorSubset) * colorSubsets.size(), colorSubsets.data()) == false)
		{
			logger->PrintA(Logger::TYPE_ERROR, "Failed to write the file");
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// セレクトサブセットを書き込む
		// ----------------------------------------------------------------------------------------------------

		if (FileWrite(fileHandle, sizeof(File_SelectSubset), &selectSubset) == false)
		{
			logger->PrintA(Logger::TYPE_ERROR, "Failed to write the file");
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// ボーンを書き込む
		// ----------------------------------------------------------------------------------------------------

		if (bones.empty() == false)
		{
			if (FileWrite(fileHandle, sizeof(File_Bone) * bones.size(), bones.data()) == false)
			{
				logger->PrintA(Logger::TYPE_ERROR, "Failed to write the file");
				return false;
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// シェイプを書き込む
		// ----------------------------------------------------------------------------------------------------

		if (FileWrite(fileHandle, sizeof(File_Shape) * shapes.size(), shapes.data()) == false)
		{
			logger->PrintA(Logger::TYPE_ERROR, "Failed to write the file");
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// バッファーを書き込む
		// ----------------------------------------------------------------------------------------------------

		bool downloadResult;

		/**************************/
		/* バーテックスバッファー */
		/**************************/

		IV3DBuffer* pHostVertexBuffer;
		ResourceAllocation hostVertexBufferAllocation;

		m_DeviceContext->GetImmediateContextPtr()->Begin();
		downloadResult = m_DeviceContext->GetImmediateContextPtr()->Download(V3D_PIPELINE_STAGE_VERTEX_INPUT, V3D_ACCESS_VERTEX_READ, m_VertexBuffer.pResource, &pHostVertexBuffer, &hostVertexBufferAllocation);
		m_DeviceContext->GetImmediateContextPtr()->End();

		if (downloadResult == false)
		{
			logger->PrintA(Logger::TYPE_ERROR, "Vertex buffer download failed");
			return false;
		}

		void* pMemory;
		if (pHostVertexBuffer->Map(0, 0, &pMemory) == V3D_OK)
		{
			if (FileWrite(fileHandle, m_VertexBufferSize, pMemory) == false)
			{
				logger->PrintA(Logger::TYPE_ERROR, "Failed to write the file");
				pHostVertexBuffer->Release();
				m_DeviceContext->GetResourceMemoryManagerPtr()->Free(hostVertexBufferAllocation);
				return false;
			}

			pHostVertexBuffer->Unmap();
		}
		else
		{
			logger->PrintA(Logger::TYPE_ERROR, "Failed to read the vertex buffer");
			pHostVertexBuffer->Release();
			m_DeviceContext->GetResourceMemoryManagerPtr()->Free(hostVertexBufferAllocation);
			return false;
		}

		pHostVertexBuffer->Release();
		m_DeviceContext->GetResourceMemoryManagerPtr()->Free(hostVertexBufferAllocation);

		/**************************/
		/* インデックスバッファー */
		/**************************/

		IV3DBuffer* pHostIndexBuffer;
		ResourceAllocation hostIndexBufferAllocation;

		m_DeviceContext->GetImmediateContextPtr()->Begin();
		downloadResult = m_DeviceContext->GetImmediateContextPtr()->Download(V3D_PIPELINE_STAGE_VERTEX_INPUT, V3D_ACCESS_INDEX_READ, m_IndexBuffer.pResource, &pHostIndexBuffer, &hostIndexBufferAllocation);
		m_DeviceContext->GetImmediateContextPtr()->End();

		if (downloadResult == false)
		{
			logger->PrintA(Logger::TYPE_ERROR, "Index buffer download failed");
			return false;
		}

		if (pHostIndexBuffer->Map(0, 0, &pMemory) == V3D_OK)
		{
			if (FileWrite(fileHandle, m_IndexBufferSize, pMemory) == false)
			{
				logger->PrintA(Logger::TYPE_ERROR, "Failed to write the file");
				pHostIndexBuffer->Release();
				m_DeviceContext->GetResourceMemoryManagerPtr()->Free(hostIndexBufferAllocation);
				return false;
			}

			pHostIndexBuffer->Unmap();
		}
		else
		{
			logger->PrintA(Logger::TYPE_ERROR, "Failed to read the index buffer");
			pHostIndexBuffer->Release();
			m_DeviceContext->GetResourceMemoryManagerPtr()->Free(hostIndexBufferAllocation);
			return false;
		}

		pHostIndexBuffer->Release();
		m_DeviceContext->GetResourceMemoryManagerPtr()->Free(hostIndexBufferAllocation);

		// ----------------------------------------------------------------------------------------------------

		return true;
	}

	bool SkeletalMesh::Preparation(size_t boneCount)
	{
		size_t worldMatrixCount = (boneCount > 0) ? boneCount : 1;

		// ----------------------------------------------------------------------------------------------------
		// ユニフォームバッファーを作成
		// ----------------------------------------------------------------------------------------------------

		const wchar_t* pDebugName = nullptr;
		VE_DEBUG_CODE(StringW debugName = StringW(L"SkeletalMesh_") + std::to_wstring(m_ID));
		VE_DEBUG_CODE(pDebugName = debugName.c_str());

		m_pUniformBuffer = DynamicBuffer::Create(m_DeviceContext, V3D_BUFFER_USAGE_UNIFORM, sizeof(glm::mat4) * worldMatrixCount + sizeof(uint32_t), V3D_PIPELINE_STAGE_VERTEX_SHADER, V3D_ACCESS_UNIFORM_READ, pDebugName);
		if (m_pUniformBuffer == nullptr)
		{
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// デスクリプタセットを作成 : カラー
		// ----------------------------------------------------------------------------------------------------

		m_pNativeDescriptorSet[SkeletalMesh::DST_COLOR] = m_DeviceContext->GetGraphicsFactoryPtr()->CreateNativeDescriptorSet(GraphicsFactory::DST_MESH);
		if (m_pNativeDescriptorSet == nullptr)
		{
			return false;
		}

		if (m_pNativeDescriptorSet[SkeletalMesh::DST_COLOR]->SetBuffer(0, m_pUniformBuffer->GetNativeBufferPtr(), 0, m_pUniformBuffer->GetNativeRangeSize()) != V3D_OK)
		{
			return false;
		}

		m_pNativeDescriptorSet[SkeletalMesh::DST_COLOR]->Update();

		// ----------------------------------------------------------------------------------------------------
		// デスクリプタセットを作成 : シャドウ
		// ----------------------------------------------------------------------------------------------------

		m_pNativeDescriptorSet[SkeletalMesh::DST_SHADOW] = m_DeviceContext->GetGraphicsFactoryPtr()->CreateNativeDescriptorSet(GraphicsFactory::DST_MESH_SHADOW);
		if (m_pNativeDescriptorSet == nullptr)
		{
			return false;
		}

		if (m_pNativeDescriptorSet[SkeletalMesh::DST_SHADOW]->SetBuffer(0, m_pUniformBuffer->GetNativeBufferPtr(), 0, m_pUniformBuffer->GetNativeRangeSize()) != V3D_OK)
		{
			return false;
		}

		m_pNativeDescriptorSet[SkeletalMesh::DST_SHADOW]->Update();

		// ----------------------------------------------------------------------------------------------------
		// 前準備
		// ----------------------------------------------------------------------------------------------------

		if (boneCount > 0)
		{
			m_Bones.reserve(boneCount);
		}

		m_Shapes.reserve(worldMatrixCount);
		m_WorldMatrices.resize(worldMatrixCount);

		// ----------------------------------------------------------------------------------------------------

		return true;
	}

	void SkeletalMesh::AssignMaterials(const collection::Vector<int32_t>& materialIndices)
	{
		if (materialIndices.empty() == false)
		{
			m_MaterialIndices.reserve(materialIndices.size());

			auto it_begin = materialIndices.begin();
			auto it_end = materialIndices.end();

			for (auto it = it_begin; it != it_end; ++it)
			{
				m_MaterialIndices.push_back(static_cast<uint32_t>(*it));
			}
		}
	}

	bool SkeletalMesh::BuildVertexIndexData(
		LoggerPtr logger,
		collection::Vector<IModelSource::Polygon>& polygons, uint32_t firstPolygon, uint32_t polygonCount,
		const ModelRendererConfig& config,
		int32_t id, int32_t lastID)
	{
		m_PolygonCount = polygonCount;

		// ----------------------------------------------------------------------------------------------------
		// ポリゴンをマテリアル単位でソート
		// ----------------------------------------------------------------------------------------------------

		auto it_polygon_begin = polygons.begin() + firstPolygon;
		auto it_polygon_end = it_polygon_begin + polygonCount;

		std::sort(it_polygon_begin, it_polygon_end, [](const IModelSource::Polygon& rhs, const IModelSource::Polygon& lhs) { return rhs.materialIndex < lhs.materialIndex; });

		// ----------------------------------------------------------------------------------------------------
		// サブセット、バーテックスリスト、インデックスリストを作成
		// ----------------------------------------------------------------------------------------------------

		collection::Vector<SkeletalMesh::Vertex> vertices;
		vertices.reserve(polygonCount * 3);

		collection::Vector<uint32_t> indices;
		indices.reserve(polygonCount);

		if (config.optimizeEnable == true)
		{
			OptimizeVertexIndexData(logger, polygons, firstPolygon, polygonCount, config.smoosingEnable, config.smoosingCos, lastID, vertices, indices);
		}
		else
		{
			NotOptimizeVertexIndexData(logger, polygons, firstPolygon, polygonCount, lastID, vertices, indices);
		}

		m_SelectSubset.indexCount = static_cast<uint32_t>(indices.size());
		m_SelectSubset.firstIndex = 0;

		m_VertexBufferSize = sizeof(SkeletalMesh::Vertex) * vertices.size();

		m_IndexType = (indices.size() <= USHRT_MAX) ? V3D_INDEX_TYPE_UINT16 : V3D_INDEX_TYPE_UINT32;
		m_IndexBufferSize = (m_IndexType == V3D_INDEX_TYPE_UINT16) ? (sizeof(uint16_t) * indices.size()) : (sizeof(uint32_t) * indices.size());

		// ----------------------------------------------------------------------------------------------------
		// アップロード
		// ----------------------------------------------------------------------------------------------------

		bool uploadResult;

		m_DeviceContext->GetImmediateContextPtr()->Begin();

		/**************************/
		/* バーテックスバッファー */
		/**************************/

		uploadResult = m_DeviceContext->GetImmediateContextPtr()->Upload(
			vertices.data(), sizeof(SkeletalMesh::Vertex) * vertices.size(),
			V3D_BUFFER_USAGE_TRANSFER_SRC | V3D_BUFFER_USAGE_VERTEX, V3D_PIPELINE_STAGE_VERTEX_INPUT, V3D_ACCESS_VERTEX_READ, &m_VertexBuffer.pResource, &m_VertexBuffer.resourceAllocation,
			VE_INTERFACE_DEBUG_NAME(L"VE_VertexBuffer"));

		/**************************/
		/* インデックスバッファー */
		/**************************/

		if (uploadResult == true)
		{
			if (m_IndexType == V3D_INDEX_TYPE_UINT16)
			{
				collection::Vector<uint16_t> nweIndices;
				nweIndices.reserve(indices.size());

				auto it_begin = indices.begin();
				auto it_end = indices.end();
				for (auto it = it_begin; it != it_end; ++it)
				{
					nweIndices.push_back(static_cast<uint16_t>(*it));
				}

				uploadResult = m_DeviceContext->GetImmediateContextPtr()->Upload(
					nweIndices.data(), sizeof(uint16_t) * nweIndices.size(),
					V3D_BUFFER_USAGE_TRANSFER_SRC | V3D_BUFFER_USAGE_INDEX, V3D_PIPELINE_STAGE_VERTEX_INPUT, V3D_ACCESS_INDEX_READ, &m_IndexBuffer.pResource, &m_IndexBuffer.resourceAllocation,
					VE_INTERFACE_DEBUG_NAME(L"VE_IndexBuffer"));
			}
			else
			{
				uploadResult = m_DeviceContext->GetImmediateContextPtr()->Upload(
					indices.data(), sizeof(uint32_t) * indices.size(),
					V3D_BUFFER_USAGE_TRANSFER_SRC | V3D_BUFFER_USAGE_INDEX, V3D_PIPELINE_STAGE_VERTEX_INPUT, V3D_ACCESS_INDEX_READ, &m_IndexBuffer.pResource, &m_IndexBuffer.resourceAllocation,
					VE_INTERFACE_DEBUG_NAME(L"VE_IndexBuffer"));
			}
		}

		m_DeviceContext->GetImmediateContextPtr()->End();

		if (uploadResult == false)
		{
			return false;
		}

		// ----------------------------------------------------------------------------------------------------

		return true;
	}

	void SkeletalMesh::AddShape(const glm::vec3& center, const glm::vec3& xAxis, const glm::vec3& yAxis, const glm::vec3& zAxis, const glm::vec3& halfExtent)
	{
		SkeletalMesh::Shape shape;
		shape.center = center;
		shape.axis[0] = xAxis;
		shape.axis[1] = yAxis;
		shape.axis[2] = zAxis;
		shape.halfExtent = halfExtent;

		m_Shapes.push_back(shape);
	}

	void SkeletalMesh::AddBone(NodePtr node, const glm::mat4& offsetMatrix)
	{
		SkeletalMesh::Bone bone;
		bone.node = node;
		bone.offsetMatrix = offsetMatrix;

		m_Bones.push_back(bone);
	}

	void SkeletalMesh::ConnectMaterials(collection::Vector<MaterialPtr>& materials)
	{
		uint32_t subsetCount = static_cast<uint32_t>(m_ColorSubsets.size());

		for (uint32_t subsetIndex = 0; subsetIndex < subsetCount; subsetIndex++)
		{
			materials[m_ColorSubsets[subsetIndex].materialIndex]->Connect(this, subsetIndex);
		}
	}

	void SkeletalMesh::DisconnectMaterials(collection::Vector<MaterialPtr>& materials)
	{
		uint32_t subsetCount = static_cast<uint32_t>(m_ColorSubsets.size());

		for (uint32_t subsetIndex = 0; subsetIndex < subsetCount; subsetIndex++)
		{
			materials[m_ColorSubsets[subsetIndex].materialIndex]->Disconnect(this);
		}
	}

	void SkeletalMesh::SetOwnerModel(ModelPtr model)
	{
		m_OwnerModel = model;
	}

	void SkeletalMesh::Draw(
		const Plane& nearPlane,
		uint32_t frameIndex,
		collection::Vector<MaterialPtr>& materials,
		collection::DynamicContainer<OpacityDrawSet>& opacityDrawSets,
		collection::DynamicContainer<TransparencyDrawSet>& transparencyDrawSets)
	{
		size_t subsetCount = m_ColorSubsets.size();

		SkeletalMesh::ColorSubset* pSubset = m_ColorSubsets.data();
		SkeletalMesh::ColorSubset* pSubsetEnd = pSubset + subsetCount;

		while (pSubset != pSubsetEnd)
		{
			Material* pMaterial = materials[pSubset->materialIndex].get();
			BLEND_MODE blendMode = pMaterial->GetBlendMode();

			if (pMaterial->GetBlendMode() == BLEND_MODE_COPY)
			{
				OpacityDrawSet* pDrawSet = *opacityDrawSets.Add(1);

				pDrawSet->sortKey = pMaterial->GetKey();
				pDrawSet->pPipeline = pSubset->pipelineHandle->GetPtr();
				pDrawSet->descriptorSet[0] = m_pNativeDescriptorSet[SkeletalMesh::DST_COLOR];
				pDrawSet->descriptorSet[1] = pMaterial->GetNativeDescriptorSetPtr(Material::DST_COLOR);
				pDrawSet->dynamicOffsets[0] = m_pUniformBuffer->GetNativeRangeSize() * frameIndex;
				pDrawSet->dynamicOffsets[1] = pMaterial->GetDynamicOffset() * frameIndex;
				pDrawSet->pVertexBuffer = m_VertexBuffer.pResource;
				pDrawSet->pIndexBuffer = m_IndexBuffer.pResource;
				pDrawSet->indexType = m_IndexType;
				pDrawSet->indexCount = pSubset->indexCount;
				pDrawSet->firstIndex = pSubset->firstIndex;
			}
			else
			{
				size_t debriPolygonCount = pSubset->debriPolygon.size();
				SkeletalMesh::DebriPolygon* pDebriPolygon = pSubset->debriPolygon.data();
				SkeletalMesh::DebriPolygon* pDebriPolygonEnd = pDebriPolygon + debriPolygonCount;

				TransparencyDrawSet** ppDrawSet = transparencyDrawSets.Add(debriPolygonCount);

				uint32_t firstIndex = pSubset->firstIndex;

				while (pDebriPolygon != pDebriPolygonEnd)
				{
					SkeletalMesh::DebriVertex* pDebriVertex = &pDebriPolygon->vertices[0];
					SkeletalMesh::DebriVertex* pDebriVertexEnd = pDebriVertex + 3;

					float sortKey = +VE_FLOAT_MAX;
					glm::vec3 worldPos;

					while (pDebriVertex != pDebriVertexEnd)
					{
						worldPos = *pDebriVertex->pWorldMatrix * pDebriVertex->pos;					
						sortKey = std::min(sortKey, glm::dot(nearPlane.normal, worldPos) + nearPlane.d);

						pDebriVertex++;
					}

					TransparencyDrawSet* pDrawSet = *ppDrawSet;
					
					pDrawSet->sortKey = sortKey;
					pDrawSet->pPipeline = pSubset->pipelineHandle->GetPtr();
					pDrawSet->descriptorSet[0] = m_pNativeDescriptorSet[SkeletalMesh::DST_COLOR];
					pDrawSet->descriptorSet[1] = pMaterial->GetNativeDescriptorSetPtr(Material::DST_COLOR);
					pDrawSet->dynamicOffsets[0] = m_pUniformBuffer->GetNativeRangeSize() * frameIndex;
					pDrawSet->dynamicOffsets[1] = pMaterial->GetDynamicOffset() * frameIndex;
					pDrawSet->pVertexBuffer = m_VertexBuffer.pResource;
					pDrawSet->pIndexBuffer = m_IndexBuffer.pResource;
					pDrawSet->indexType = m_IndexType;
					pDrawSet->indexCount = 3;
					pDrawSet->firstIndex = firstIndex;

					firstIndex += 3;
					pDebriPolygon++;

					ppDrawSet++;
				}
			}

			pSubset++;
		}
	}

	void SkeletalMesh::DrawShadow(
		uint32_t frameIndex,
		collection::Vector<MaterialPtr>& materials,
		collection::DynamicContainer<ShadowDrawSet>& shadowDrawSets)
	{
		size_t subsetCount = m_ColorSubsets.size();

		SkeletalMesh::ColorSubset* pSubset = m_ColorSubsets.data();
		SkeletalMesh::ColorSubset* pSubsetEnd = pSubset + subsetCount;

		while (pSubset != pSubsetEnd)
		{
			Material* pMaterial = materials[pSubset->materialIndex].get();
			BLEND_MODE blendMode = pMaterial->GetBlendMode();

			if (pMaterial->GetBlendMode() == BLEND_MODE_COPY)
			{
				ShadowDrawSet* pDrawSet = *shadowDrawSets.Add(1);

				pDrawSet->sortKey1 = VE_FLOAT_MAX;
				pDrawSet->sortKey2 = pMaterial->GetKey();
				pDrawSet->pPipeline = pSubset->shadowPipelineHandle->GetPtr();
				pDrawSet->descriptorSet[0] = m_pNativeDescriptorSet[SkeletalMesh::DST_SHADOW];
				pDrawSet->descriptorSet[1] = pMaterial->GetNativeDescriptorSetPtr(Material::DST_SHADOW);
				pDrawSet->dynamicOffsets[0] = m_pUniformBuffer->GetNativeRangeSize() * frameIndex;
				pDrawSet->dynamicOffsets[1] = pMaterial->GetDynamicOffset() * frameIndex;
				pDrawSet->pVertexBuffer = m_VertexBuffer.pResource;
				pDrawSet->pIndexBuffer = m_IndexBuffer.pResource;
				pDrawSet->indexType = m_IndexType;
				pDrawSet->indexCount = pSubset->indexCount;
				pDrawSet->firstIndex = pSubset->firstIndex;
			}
			else
			{
			}

			pSubset++;
		}
	}

	void SkeletalMesh::DebugDraw(uint32_t flags, DebugRenderer* pDebugRenderer)
	{
		if (flags & DEBUG_DRAW_MESH_BOUNDS)
		{
			pDebugRenderer->AddAABB(DEBUG_COLOR_TYPE_MESH_BOUNDS, m_AABB);
		}
	}

	void SkeletalMesh::Dispose()
	{
		DeleteDeviceChild(m_DeviceContext->GetDeletingQueuePtr(), &m_pNativeDescriptorSet[SkeletalMesh::DST_COLOR]);
		DeleteDeviceChild(m_DeviceContext->GetDeletingQueuePtr(), &m_pNativeDescriptorSet[SkeletalMesh::DST_SHADOW]);

		if (m_pUniformBuffer != nullptr)
		{
			m_pUniformBuffer->Destroy();
			m_pUniformBuffer = nullptr;
		}

		DeleteResource(m_DeviceContext->GetDeletingQueuePtr(), &m_VertexBuffer.pResource, &m_VertexBuffer.resourceAllocation);
		DeleteResource(m_DeviceContext->GetDeletingQueuePtr(), &m_IndexBuffer.pResource, &m_IndexBuffer.resourceAllocation);
	}

	/***************************/
	/* public override - Mesh */
	/***************************/

	ModelPtr SkeletalMesh::GetModel()
	{
		return m_OwnerModel.lock();
	}

	uint32_t SkeletalMesh::GetPolygonCount() const
	{
		return m_PolygonCount;
	}

	uint32_t SkeletalMesh::GetBoneCount() const
	{
		return static_cast<uint32_t>(m_Bones.size());
	}

	uint32_t SkeletalMesh::GetMaterialCount() const
	{
		return static_cast<uint32_t>(m_MaterialIndices.size());
	}

	MaterialPtr SkeletalMesh::GetMaterial(uint32_t materialIndex)
	{
		return m_OwnerModel.lock()->GetMaterial(m_MaterialIndices[materialIndex]);
	}

	const AABB& SkeletalMesh::GetAABB() const
	{
		return m_AABB;
	}

	bool SkeletalMesh::GetVisible() const
	{
		return m_Visible;
	}

	void SkeletalMesh::SetVisible(bool enable)
	{
		m_Visible = enable;
	}

	bool SkeletalMesh::GetCastShadow() const
	{
		return m_CastShadow;
	}

	void SkeletalMesh::SetCastShadow(bool enable)
	{
		m_CastShadow = enable;
	}

	/**************************/
	/* public override - Node */
	/**************************/

	NodeAttribute::TYPE SkeletalMesh::GetType() const
	{
		return NodeAttribute::TYPE_MESH;
	}

	void SkeletalMesh::Update(const glm::mat4& worldMatrix)
	{
		// ----------------------------------------------------------------------------------------------------
		// ボーン
		// ----------------------------------------------------------------------------------------------------

		if (m_Bones.empty() == false)
		{
			VE_ASSERT(m_Bones.size() == m_WorldMatrices.size());

			SkeletalMesh::Bone* pBone = m_Bones.data();
			SkeletalMesh::Bone* pBoneEnd = pBone + m_Bones.size();

			glm::mat4* pWorldMatrix = m_WorldMatrices.data();

			while (pBone != pBoneEnd)
			{
				*pWorldMatrix++ = pBone->node.lock()->GetWorldMatrix() * pBone->offsetMatrix;
				pBone++;
			}
		}
		else
		{
			m_WorldMatrices[0] = worldMatrix;
		}

		// ----------------------------------------------------------------------------------------------------
		// AABB を求める
		// ----------------------------------------------------------------------------------------------------

		m_AABB.minimum = glm::vec3(+VE_FLOAT_MAX);
		m_AABB.maximum = glm::vec3(-VE_FLOAT_MAX);

		if (m_Bones.empty() == false)
		{
			SkeletalMesh::Shape* pShape = m_Shapes.data();
			SkeletalMesh::Shape* pShapeEnd = pShape + m_Shapes.size();

			const glm::mat4* pWorldMatrix = m_WorldMatrices.data();

			while (pShape != pShapeEnd)
			{
				UpdateAABB(pShape, *pWorldMatrix, m_AABB);

				pWorldMatrix++;
				pShape++;
			}
		}
		else
		{
			UpdateAABB(m_Shapes.data(), m_WorldMatrices[0], m_AABB);
		}

		m_AABB.UpdateCenterAndPoints();

		// ----------------------------------------------------------------------------------------------------
		// ユニフォームバッファーを更新
		// ----------------------------------------------------------------------------------------------------

		uint8_t* pMemory = static_cast<uint8_t*>(m_pUniformBuffer->Map());
		VE_ASSERT(pMemory != nullptr);

		size_t worldMatricesSize = sizeof(glm::mat4) * m_WorldMatrices.size();
		memcpy_s(pMemory, m_pUniformBuffer->GetNativeRangeSize(), m_WorldMatrices.data(), worldMatricesSize);
		memcpy_s(pMemory + worldMatricesSize, m_pUniformBuffer->GetNativeRangeSize() - worldMatricesSize, &m_Uniform.key, sizeof(uint32_t));

		m_pUniformBuffer->Unmap();
	}

	/*****************************/
	/* protected override - Mesh */
	/*****************************/

	void SkeletalMesh::UpdatePipeline(Material* pMaterial, uint32_t subsetIndex)
	{
		SkeletalMesh::ColorSubset& colorSubset = m_ColorSubsets[subsetIndex];

		colorSubset.pipelineHandle = m_DeviceContext->GetGraphicsFactoryPtr()->GetPipelineHandle(
			GraphicsFactory::MPT_COLOR,
			MATERIAL_SHADER_SKELETAL | pMaterial->GetShaderFlags(),
			m_Bones.size(),
			sizeof(SkeletalMesh::Vertex),
			pMaterial->GetPolygonMode(), pMaterial->GetCullMode(),
			pMaterial->GetBlendMode());

		colorSubset.shadowPipelineHandle = m_DeviceContext->GetGraphicsFactoryPtr()->GetPipelineHandle(
			GraphicsFactory::MPT_SHADOW,
			MATERIAL_SHADER_SKELETAL | pMaterial->GetShaderFlags(),
			m_Bones.size(),
			sizeof(SkeletalMesh::Vertex),
			pMaterial->GetPolygonMode(), pMaterial->GetCullMode(),
			pMaterial->GetBlendMode());

		if (m_SelectSubset.pipelineHandle == nullptr)
		{
			m_SelectSubset.pipelineHandle = m_DeviceContext->GetGraphicsFactoryPtr()->GetPipelineHandle(
				GraphicsFactory::MPT_SELECT,
				MATERIAL_SHADER_SKELETAL | pMaterial->GetShaderFlags(),
				m_Bones.size(),
				sizeof(SkeletalMesh::Vertex),
				pMaterial->GetPolygonMode(), pMaterial->GetCullMode(),
				pMaterial->GetBlendMode());
		}
	}

	/*****************************/
	/* protected override - Node */
	/*****************************/

	bool SkeletalMesh::IsSelectSupported() const
	{
		return true;
	}

	void SkeletalMesh::SetSelectKey(uint32_t key)
	{
		m_Uniform.key = key;
	}

	void SkeletalMesh::DrawSelect(uint32_t frameIndex, SelectDrawSet& drawSet)
	{
		drawSet.pPipeline = m_SelectSubset.pipelineHandle->GetPtr();
		drawSet.pDescriptorSet = m_pNativeDescriptorSet[SkeletalMesh::DST_COLOR];
		drawSet.dynamicOffset = m_pUniformBuffer->GetNativeRangeSize() * frameIndex;
		drawSet.pVertexBuffer = m_VertexBuffer.pResource;
		drawSet.pIndexBuffer = m_IndexBuffer.pResource;
		drawSet.indexType = m_IndexType;
		drawSet.indexCount = m_SelectSubset.indexCount;
		drawSet.firstIndex = m_SelectSubset.firstIndex;
	}

	/**************************/
	/* private - SkeletalMesh */
	/**************************/

	void SkeletalMesh::OptimizeVertexIndexData(
		LoggerPtr logger,
		collection::Vector<IModelSource::Polygon>& polygons, uint32_t firstPolygon, uint32_t polygonCount,
		bool smoosingEnable, float smoosingCos,
		int32_t lastID,
		collection::Vector<SkeletalMesh::Vertex>& vertices, collection::Vector<uint32_t>& indices)
	{
		// ----------------------------------------------------------------------------------------------------
		// ポリゴンを展開
		// ----------------------------------------------------------------------------------------------------

		collection::Vector<IModelSource::Vertex> optVertices;
		collection::Vector<SkeletalMesh::Optimize_Face> optFaces;
		collection::Vector<SkeletalMesh::Optimize_Subset> optSubsets;

		{
			optVertices.resize(polygonCount * 3);
			optFaces.resize(polygonCount);

			IModelSource::Polygon* pPolygon = &polygons[firstPolygon];
			IModelSource::Polygon* pPolygonEnd = pPolygon + polygonCount;

			SkeletalMesh::Optimize_Subset dummySubset{};
			dummySubset.materialIndex = ~0U;
			dummySubset.firstIndex = 0;
			dummySubset.indexCount = 0;
			dummySubset.firstFace = 0;
			dummySubset.faceCount = 0;

			IModelSource::Vertex* pVertex = optVertices.data();
			SkeletalMesh::Optimize_Face* pFace = optFaces.data();
			SkeletalMesh::Optimize_Subset* pSubset = &dummySubset;

			while (pPolygon != pPolygonEnd)
			{
				if (pSubset->materialIndex != pPolygon->materialIndex)
				{
					SkeletalMesh::Optimize_Subset preSubset = *pSubset;

					optSubsets.push_back(SkeletalMesh::Optimize_Subset{});
					pSubset = &optSubsets.back();

					pSubset->materialIndex = pPolygon->materialIndex;
					pSubset->firstIndex = preSubset.firstIndex + preSubset.indexCount;
					pSubset->indexCount = 0;
					pSubset->firstFace = preSubset.firstFace + preSubset.faceCount;
					pSubset->faceCount = 0;
				}

				*pVertex++ = pPolygon->vertices[0];
				*pVertex++ = pPolygon->vertices[1];
				*pVertex++ = pPolygon->vertices[2];

				pFace->indices[0] = pSubset->indexCount++;
				pFace->indices[1] = pSubset->indexCount++;
				pFace->indices[2] = pSubset->indexCount++;
				pFace++;

				pSubset->faceCount++;

				pPolygon++;
			}
		}

		{
			SkeletalMesh::Optimize_Subset* pSubset = optSubsets.data();
			SkeletalMesh::Optimize_Subset* pSubsetEnd = pSubset + optSubsets.size();

			while (pSubset != pSubsetEnd)
			{
				pSubset->vertices.reserve(pSubset->indexCount);
				pSubset->indices.reserve(pSubset->indexCount);

				for (uint32_t i = 0; i < pSubset->indexCount; i++)
				{
					pSubset->vertices.push_back(optVertices[pSubset->firstIndex + i]);
					pSubset->indices.push_back(i);
				}

				pSubset++;
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// 可能ならば頂点を結合し、新しいインデックスを割り振る
		// ----------------------------------------------------------------------------------------------------

		{
			std::vector<SkeletalMesh::Optimize_Subset>::iterator it_subset_begin = optSubsets.begin();
			std::vector<SkeletalMesh::Optimize_Subset>::iterator it_subset_end = optSubsets.end();
			std::vector<SkeletalMesh::Optimize_Subset>::iterator it_subset;

			std::vector<IModelSource::Vertex*> combineVertices;
			uint32_t subseMax = static_cast<uint32_t>(optSubsets.size());
			uint32_t subseCount = 1;

			for (it_subset = it_subset_begin; it_subset != it_subset_end; ++it_subset)
			{
				SkeletalMesh::Optimize_Subset* pSubset = &(*it_subset);
				size_t vertexCount = pSubset->vertices.size();

				SkeletalMesh::Optimize_Face* pFace = optFaces.data() + pSubset->firstFace;
				SkeletalMesh::Optimize_Face* pFaceEnd = pFace + pSubset->faceCount;

				logger->PrintA(Logger::TYPE_INFO, "Optimize Mesh[%d/%d] : Subset[%u/%u] FaceCount[%u]", m_ID, lastID, subseCount, subseMax, pSubset->faceCount);

				uint32_t combineVertexCount = 0;

				while (pFace != pFaceEnd)
				{
					uint32_t* pIndex = &pFace->indices[0];
					uint32_t* pIndexEnd = pIndex + 3;

					while (pIndex != pIndexEnd)
					{
						if (pSubset->indices[*pIndex] != *pIndex)
						{
							// 新しいインデックスが割り振られているので調べない
							pIndex++;
							continue;
						}

						uint32_t newIndex = static_cast<uint32_t>(pSubset->vertices.size());
						IModelSource::Vertex* pSrcVertex = &(pSubset->vertices[*pIndex]);

						combineVertices.clear();
						combineVertices.push_back(pSrcVertex);

						// 結合相手の頂点を探す
						for (size_t i = 0; i < vertexCount; i++)
						{
							if (i == *pIndex)
							{
								// 自分だった
								continue;
							}

							IModelSource::Vertex* pDstVertex = &(pSubset->vertices[i]);

							if ((pSrcVertex->pos == pDstVertex->pos) &&
								(pSrcVertex->uv == pDstVertex->uv))
							{
								if (smoosingEnable == true)
								{
									float normalCos = glm::dot(pSrcVertex->normal, pDstVertex->normal);

									if (smoosingCos <= normalCos)
									{
										// 結合相手に新規インデックスを割り振り、結合する頂点を追加
										pSubset->indices[i] = newIndex;
										combineVertices.push_back(pDstVertex);
									}
								}
								else
								{
									if (pSrcVertex->normal == pDstVertex->normal)
									{
										//結合相手に新規インデックスを割り振り、結合する頂点を追加
										pSubset->indices[i] = newIndex;
										combineVertices.push_back(pDstVertex);
									}
								}
							}
						}

						if (combineVertices.size() > 1)
						{
							// 自分と他のだれかの頂点の結合が要求されている
							IModelSource::Vertex** ppCombineVertex = combineVertices.data();

							if (smoosingEnable == true)
							{
								size_t combineVertexCount = combineVertices.size();

								IModelSource::Vertex** ppVertex = ppCombineVertex;
								IModelSource::Vertex** ppVertexEnd = ppVertex + combineVertexCount;

								glm::vec3 combineNormal;
								glm::vec3 combineTangent;
								glm::vec3 combineBinormal;

								while (ppVertex != ppVertexEnd)
								{
									combineNormal += (*ppVertex)->normal;
									combineTangent += (*ppVertex)->tangent;
									combineBinormal += (*ppVertex)->binormal;
									ppVertex++;
								}

								(*ppCombineVertex)->normal = glm::normalize(combineNormal);
								(*ppCombineVertex)->tangent = glm::normalize(combineTangent);
								(*ppCombineVertex)->binormal = glm::normalize(combineBinormal);
							}

							// 自分の新規インデックスを割り振る
							pSubset->indices[*pIndex] = newIndex;

							// 結合した頂点を追加
							pSubset->vertices.push_back(**ppCombineVertex);

							combineVertexCount += static_cast<uint32_t>(combineVertices.size() - 1);
						}

						pIndex++;
					}

					pFace++;
				}

				uint32_t afterVertexCount = static_cast<uint32_t>(vertexCount) - combineVertexCount;

				logger->PrintA(Logger::TYPE_INFO, "  VertexCount[%u/%u]", afterVertexCount, vertexCount);

				subseCount++;
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// バーテックス、インデックスのリストを作成
		// ----------------------------------------------------------------------------------------------------

		{
			uint32_t newIndex = 0;

			std::vector<SkeletalMesh::Optimize_Index> indexOpts;

			std::vector<SkeletalMesh::Optimize_Subset>::iterator it_subset_begin = optSubsets.begin();
			std::vector<SkeletalMesh::Optimize_Subset>::iterator it_subset_end = optSubsets.end();
			std::vector<SkeletalMesh::Optimize_Subset>::iterator it_subset;

			uint32_t indexOffset = static_cast<uint32_t>(vertices.size());

			for (it_subset = it_subset_begin; it_subset != it_subset_end; ++it_subset)
			{
				SkeletalMesh::Optimize_Subset* pSubset = &(*it_subset);

				indexOpts.clear();
				indexOpts.resize(pSubset->vertices.size());

				SkeletalMesh::Optimize_Face* pFace = optFaces.data() + pSubset->firstFace;
				SkeletalMesh::Optimize_Face* pFaceEnd = pFace + pSubset->faceCount;

				uint32_t firstIndex = static_cast<uint32_t>(indices.size());

				while (pFace != pFaceEnd)
				{
					uint32_t* pIndex = &pFace->indices[0];
					uint32_t* pIndexEnd = pIndex + 3;

					while (pIndex != pIndexEnd)
					{
						uint32_t oldIndex = pSubset->indices[*pIndex];
						SkeletalMesh::Optimize_Index* pOptIndex = &indexOpts[oldIndex];

						if (pOptIndex->assigned == false)
						{
							const IModelSource::Vertex& srcVertex = pSubset->vertices[oldIndex];
							SkeletalMesh::Vertex dstVertex;

							dstVertex.pos = srcVertex.pos;
							dstVertex.uv = srcVertex.uv;
							dstVertex.normal = srcVertex.normal;
							dstVertex.tangent = srcVertex.tangent;
							dstVertex.binormal = srcVertex.binormal;
							dstVertex.indices.x = srcVertex.indices[0];
							dstVertex.indices.y = srcVertex.indices[1];
							dstVertex.indices.z = srcVertex.indices[2];
							dstVertex.indices.w = srcVertex.indices[3];
							dstVertex.weights.x = srcVertex.weights[0];
							dstVertex.weights.y = srcVertex.weights[1];
							dstVertex.weights.z = srcVertex.weights[2];
							dstVertex.weights.w = srcVertex.weights[3];

							pOptIndex->index = newIndex;
							pOptIndex->assigned = true;

							vertices.push_back(dstVertex);
							indices.push_back(indexOffset + newIndex);

							newIndex++;
						}
						else
						{
							indices.push_back(indexOffset + pOptIndex->index);
						}

						pIndex++;
					}

					pFace++;
				}

				m_ColorSubsets.push_back(SkeletalMesh::ColorSubset{});

				SkeletalMesh::ColorSubset& dstSubset = m_ColorSubsets.back();
				dstSubset.materialIndex = pSubset->materialIndex;
				dstSubset.indexCount = static_cast<uint32_t>(indices.size()) - firstIndex;
				dstSubset.firstIndex = firstIndex;

				uint32_t debriPolygonCount = dstSubset.indexCount / 3;
				dstSubset.debriPolygon.resize(debriPolygonCount);

				for (uint32_t i = 0; i < debriPolygonCount; i++)
				{
					SkeletalMesh::DebriPolygon& dstDebriPolygon = dstSubset.debriPolygon[i];
					uint32_t baseIndex = dstSubset.firstIndex + i * 3;

					for (uint32_t j = 0; j < 3; j++)
					{
						const SkeletalMesh::Vertex& srcVertex = vertices[indices[baseIndex + j]];
						SkeletalMesh::DebriVertex& dstDebriVertex = dstDebriPolygon.vertices[j];

						dstDebriVertex.pos = glm::vec4(srcVertex.pos, 1.0f);
						dstDebriVertex.worldMatrixIndex = srcVertex.indices[0];
						dstDebriVertex.pWorldMatrix = &m_WorldMatrices[dstDebriVertex.worldMatrixIndex];
					}
				}
			}
		}
	}

	void SkeletalMesh::NotOptimizeVertexIndexData(
		LoggerPtr logger,
		collection::Vector<IModelSource::Polygon>& polygons, uint32_t firstPolygon, uint32_t polygonCount,
		int32_t lastID,
		collection::Vector<SkeletalMesh::Vertex>& vertices, collection::Vector<uint32_t>& indices)
	{
		const IModelSource::Polygon* pPolygon = &polygons[firstPolygon];
		const IModelSource::Polygon* pPolygonEnd = pPolygon + polygonCount;

		m_ColorSubsets.push_back(SkeletalMesh::ColorSubset{});

		SkeletalMesh::ColorSubset* pColorSubset = &m_ColorSubsets.back();
		pColorSubset->materialIndex = pPolygon->materialIndex;
		pColorSubset->firstIndex = 0;

		uint32_t index = 0;

		logger->PrintA(Logger::TYPE_INFO, "NotOptimize Mesh[%d/%d] : FaceCount[%u] VertexCount[%u]", m_ID, lastID, polygonCount, polygonCount * 3);

		while (pPolygon != pPolygonEnd)
		{
			if (pColorSubset->materialIndex != pPolygon->materialIndex)
			{
				SkeletalMesh::ColorSubset newSubset;
				newSubset.materialIndex = pPolygon->materialIndex;
				newSubset.indexCount = 0;
				newSubset.firstIndex = pColorSubset->firstIndex + pColorSubset->indexCount;

				m_ColorSubsets.push_back(newSubset);
				pColorSubset = &m_ColorSubsets.back();
			}

			pColorSubset->indexCount += 3;

			const IModelSource::Vertex* pSrcVertex = &pPolygon->vertices[0];
			const IModelSource::Vertex* pSrcVertexEnd = pSrcVertex + 3;

			pColorSubset->debriPolygon.push_back(SkeletalMesh::DebriPolygon{});
			SkeletalMesh::DebriVertex* pDebriVertex = &(pColorSubset->debriPolygon.back().vertices[0]);

			while (pSrcVertex != pSrcVertexEnd)
			{
				SkeletalMesh::Vertex vertex;

				vertex.pos = pSrcVertex->pos;
				vertex.uv = pSrcVertex->uv;
				vertex.normal = pSrcVertex->normal;
				vertex.tangent = pSrcVertex->tangent;
				vertex.binormal = pSrcVertex->binormal;
				vertex.indices.x = pSrcVertex->indices[0];
				vertex.indices.y = pSrcVertex->indices[1];
				vertex.indices.z = pSrcVertex->indices[2];
				vertex.indices.w = pSrcVertex->indices[3];
				vertex.weights.x = pSrcVertex->weights[0];
				vertex.weights.y = pSrcVertex->weights[1];
				vertex.weights.z = pSrcVertex->weights[2];
				vertex.weights.w = pSrcVertex->weights[3];

				vertices.push_back(vertex);

				pDebriVertex->pos = glm::vec4(pSrcVertex->pos, 1.0f);
				pDebriVertex->worldMatrixIndex = pSrcVertex->indices[0];
				pDebriVertex->pWorldMatrix = &m_WorldMatrices[pDebriVertex->worldMatrixIndex];

				pSrcVertex++;
				pDebriVertex++;
			}

			indices.push_back(index++);
			indices.push_back(index++);
			indices.push_back(index++);

			pPolygon++;
		}
	}
}