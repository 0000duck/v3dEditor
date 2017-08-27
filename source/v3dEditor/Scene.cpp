#include "Scene.h"
#include "DeviceContext.h"
#include "UpdatingQueue.h"
#include "DeletingQueue.h"
#include "ResourceMemoryManager.h"
#include "ImmediateContext.h"
#include "SamplerFactory.h"
#include "DynamicBuffer.h"
#include "Texture.h"
#include "Node.h"
#include "Camera.h"
#include "Light.h"
#include "IMesh.h"
#include "IModel.h"
#include "NodeSelector.h"
#include "DebugRenderer.h"

// ----------------------------------------------------------------------------------------------------
// クラス
// ----------------------------------------------------------------------------------------------------

static constexpr uint8_t VE_Scene_Ssao_NoizeImage[] =
{
#include "resource\noise_dds.inc"
};

namespace ve {

	/******************/
	/* public - Scene */
	/******************/

	ScenePtr Scene::Create(DeviceContextPtr deviceContext)
	{
		ScenePtr scene = std::make_shared<Scene>();

		if (scene->Initialize(deviceContext) == false)
		{
			return nullptr;
		}

		return scene;
	}

	Scene::Scene() :
		m_pDeletingQueue(nullptr),
		m_SimpleVertexBuffer({}),
		m_PasteStage({}),
		m_BrightPassStage({}),
		m_BlurStage({}),
		m_GeometoryStage({}),
		m_ShadowStage({}),
		m_SsaoStage({}),
		m_LightingStage({}),
		m_ForwardStage({}),
		m_GlareStage({}),
		m_ImageEffectStage({}),
		m_FinishStage({}),
		m_DefaultViewport({}),
		m_HalfViewport({}),
		m_QuarterViewport({}),
		m_DebugDrawFlags(DEBUG_DRAW_LIGHT_SHAPE),
		m_OpacityDrawSets(Scene::OpacityDrawSet_DefaultCount, Scene::OpacityDrawSet_ResizeStep),
		m_TransparencyDrawSets(Scene::TransparencyDrawSet_DefaultCount, Scene::TransparencyDrawSet_ResizeStep),
		m_ShadowDrawSets(Scene::ShadowDrawSet_DefaultCount, Scene::ShadowDrawSet_ResizeStep),
		m_pNodeSelector(nullptr),
		m_SelectBuffer({}),
		m_pDebugRenderer(nullptr)
	{
		m_Grid.enable = true;
		m_Grid.cellSize = 1.0f;
		m_Grid.halfCellCount = 10;
		m_Grid.color = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);

		m_Ssao.enable = true;
		m_Ssao.radius = 0.2f;
		m_Ssao.threshold = 0.001f;
		m_Ssao.depth = 1.0f;
		m_Ssao.intensity = 2.5f;
		m_Ssao.blurRadius = 20.0f;
		m_Ssao.blurSamples = 8;

		m_Shadow.enable = true;
		m_Shadow.resolution = Scene::SHADOW_RESOLUTION_2048;
		m_Shadow.range = 1000.0f;
		m_Shadow.fadeMargin = 100.0f;
		m_Shadow.dencity = 1.0f;
		m_Shadow.centerOffset = 250.0f;
		m_Shadow.boundsBias = 1.0f;
		m_Shadow.depthBias = 0.00001f;

		m_Glare.enable = true;
		m_Glare.bpThreshold = 0.0f;
		m_Glare.bpOffset = 1.0f;
		m_Glare.blurRadius = 10.0f;
		m_Glare.blurSamples = 8;
		m_Glare.intensity[0] = 1.0f;
		m_Glare.intensity[1] = 2.0f;

		m_ImageEffect.toneMappingEnable = false;
		m_ImageEffect.fxaaEnable = true;

		m_ImageEffectStage.selectMapping.constant.color = glm::vec4(0.9f, 0.6f, 0.0f, 1.0f);
	}

	Scene::~Scene()
	{
		Dispose();
	}

	glm::vec4 Scene::GetBackgroundColor()
	{
		return m_DeviceContext->GetGraphicsFactoryPtr()->GetBackgroundColor();
	}

	void Scene::SetBackgroundColor(const glm::vec4& color)
	{
		m_DeviceContext->GetGraphicsFactoryPtr()->SetBackgroundColor(color);
	}

	const Scene::Grid& Scene::GetGrid() const
	{
		return m_Grid;
	}

	void Scene::SetGrid(const Scene::Grid& grid)
	{
		m_Grid.enable = grid.enable;
		m_Grid.cellSize = std::max(0.01f, grid.cellSize);
		m_Grid.halfCellCount = std::max(uint32_t(1), grid.halfCellCount);
		m_Grid.color = grid.color;

		UpdateGrid();
	}

	const Scene::Ssao& Scene::GetSsao() const
	{
		return m_Ssao;
	}

	void Scene::SetSsao(const Scene::Ssao& ssao)
	{
		m_Ssao.enable = ssao.enable;
		m_Ssao.radius = std::max(0.0f, ssao.radius);
		m_Ssao.threshold = std::max(0.0f, ssao.threshold);
		m_Ssao.depth = std::max(0.0f, ssao.depth);
		m_Ssao.intensity = std::max(0.0f, ssao.intensity);
		m_Ssao.blurRadius = std::max(0.0f, ssao.blurRadius);
		m_Ssao.blurSamples = std::max(1, ssao.blurSamples);
	}

	const Scene::Shadow& Scene::GetShadow() const
	{
		return m_Shadow;
	}

	void Scene::SetShadow(const Scene::Shadow& shadow)
	{
		bool updateResolution = (m_Shadow.resolution != shadow.resolution);

		m_Shadow = shadow;

		if (updateResolution == true)
		{
			m_DeviceContext->Lost();

			switch (m_Shadow.resolution)
			{
			case Scene::SHADOW_RESOLUTION_1024:
				m_DeviceContext->GetGraphicsFactoryPtr()->SetShadowResolution(1024);
				break;
			case Scene::SHADOW_RESOLUTION_2048:
				m_DeviceContext->GetGraphicsFactoryPtr()->SetShadowResolution(2048);
				break;
			case Scene::SHADOW_RESOLUTION_4096:
				m_DeviceContext->GetGraphicsFactoryPtr()->SetShadowResolution(4096);
				break;

			default:
				m_DeviceContext->GetGraphicsFactoryPtr()->SetShadowResolution(1024);
			}

			m_DeviceContext->Restore();
		}
	}

	const Scene::Glare& Scene::GetGlare() const
	{
		return m_Glare;
	}

	void Scene::SetGlare(const Scene::Glare& glare)
	{
		m_Glare = glare;
	}

	const Scene::ImageEffect& Scene::GetImageEffect() const
	{
		return m_ImageEffect;
	}

	void Scene::SetImageEffect(const Scene::ImageEffect& imageEffect)
	{
		m_ImageEffect = imageEffect;
	}

	const glm::vec4 Scene::GetSelectColor() const
	{
		return m_ImageEffectStage.selectMapping.constant.color;
	}

	void Scene::SetSelectColor(const glm::vec4& color)
	{
		m_ImageEffectStage.selectMapping.constant.color = color;
	}

	NodePtr Scene::GetSelect()
	{
		return m_pNodeSelector->GetFound();
	}

	NodePtr Scene::Select(const glm::ivec2& pos)
	{
		const V3DImageDesc& selectImageDesc = m_DeviceContext->GetGraphicsFactoryPtr()->GetNativeAttachmentDesc(GraphicsFactory::AT_GE_SELECT);

		if ((0 > pos.x) ||
			(0 > pos.y) ||
			(static_cast<int32_t>(selectImageDesc.width) <= pos.x) ||
			(static_cast<int32_t>(selectImageDesc.height) <= pos.y))
		{
			return nullptr;
		}

		uint32_t key = 0;
		uint32_t* pMemory;

		if (m_SelectBuffer.pResource->Map(0, 0, reinterpret_cast<void**>(&pMemory)) == V3D_OK)
		{
			key = *(pMemory + selectImageDesc.width * pos.y + pos.x);

			m_SelectBuffer.pResource->Unmap();
		}

		NodePtr oldNode = m_pNodeSelector->GetFound();
		NodePtr newNode = m_pNodeSelector->Find(key);
		if (oldNode == newNode)
		{
			m_pNodeSelector->SetFound(nullptr);
		}

		return m_pNodeSelector->GetFound();
	}

	void Scene::Select(NodePtr attribute)
	{
		m_pNodeSelector->SetFound(attribute);
	}

	void Scene::Deselect()
	{
		m_pNodeSelector->SetFound(nullptr);
	}

	uint32_t Scene::GetDebugDrawFlags() const
	{
		return m_DebugDrawFlags;
	}

	void Scene::SetDebugDrawFlags(uint32_t flags)
	{
		m_DebugDrawFlags = flags;
	}

	const glm::vec4& Scene::GetDebugColor(DEBUG_COLOR_TYPE type) const
	{
		return m_pDebugRenderer->GetColor(type);
	}

	void Scene::SetDebugColor(DEBUG_COLOR_TYPE type, const glm::vec4& color)
	{
		m_pDebugRenderer->SetColor(type, color);
	}

	NodePtr Scene::GetRootNode()
	{
		return m_RootNode;
	}

	NodePtr Scene::AddNode(NodePtr parent, const wchar_t* pName, uint32_t groupFlags, ModelPtr model)
	{
		NodePtr node = Node::AddChild(parent);

		node->SetName(pName);
		node->SetGroupFlags(groupFlags);
		Node::SetAttribute(node, model);

		m_pNodeSelector->Add(model->GetRootNode());

		return node;
	}

	void Scene::RemoveNodeByGroup(NodePtr parent, uint32_t groupFlags)
	{
		Node::RemoveByGroup(m_RootNode, groupFlags);

		m_pNodeSelector->Clear();

		InternalClear();
	}

	CameraPtr Scene::GetCamera()
	{
		return m_Camera;
	}

	LightPtr Scene::GetLight()
	{
		return m_Light;
	}

	void Scene::Update()
	{
		const glm::vec3& eyePos = m_Camera->GetEye();
		glm::vec3 srcEyeDir = glm::normalize(eyePos);
		uint32_t frameIndex = m_DeviceContext->GetCurrentFrameIndex();

		// ----------------------------------------------------------------------------------------------------
		// フラスタムを更新
		// ----------------------------------------------------------------------------------------------------

		m_Frustum.Update(m_Camera->GetViewProjectionMatrix());

		// ----------------------------------------------------------------------------------------------------
		// シャドウの境界判定用のスフィアを更新
		// ----------------------------------------------------------------------------------------------------

		m_ShadowBounds.center = eyePos + srcEyeDir * m_Shadow.centerOffset;
		m_ShadowBounds.radius = m_Shadow.range;

		// ----------------------------------------------------------------------------------------------------
		// 描画セットを生成 : カラー
		// ----------------------------------------------------------------------------------------------------

		m_OpacityDrawSets.Clear();
		m_TransparencyDrawSets.Clear();

		m_RootNode->Draw(m_Frustum, frameIndex, m_OpacityDrawSets, m_TransparencyDrawSets, m_DebugDrawFlags, m_pDebugRenderer);

		if (m_OpacityDrawSets.GetCount() > 0)
		{
			Scene::SortOpacityDrawSet(m_OpacityDrawSets.GetData(), 0, static_cast<int64_t>(m_OpacityDrawSets.GetCount()) - 1);
		}

		if (m_TransparencyDrawSets.GetCount() > 0)
		{
			Scene::SortTransparencyDrawSet(m_TransparencyDrawSets.GetData(), 0, static_cast<int64_t>(m_TransparencyDrawSets.GetCount()) - 1);
		}

		NodePtr node = m_pNodeSelector->GetFound();
		if ((node != nullptr) && (node->GetAttribute() != nullptr))
		{
			node->GetAttribute()->DrawSelect(frameIndex, m_SelectDrawSet);
		}
		else
		{
			m_SelectDrawSet = {};
		}

		// ----------------------------------------------------------------------------------------------------
		// 描画セットを生成 : シャドウ
		// ----------------------------------------------------------------------------------------------------

		m_ShadowDrawSets.Clear();

		if (m_Shadow.enable == true)
		{
			AABB aabb;

			m_RootNode->DrawShadow(m_ShadowBounds, frameIndex, m_ShadowDrawSets, aabb);
			/****************************/
			/* ライトビュー行列を求める */
			/****************************/

			const glm::mat4& viewMat = m_Camera->GetViewMatrix();
			glm::vec3 eyeVec = glm::normalize(glm::vec3(viewMat[2].x, viewMat[2].y, viewMat[2].z));

			const glm::vec3& lightDir = m_Light->GetDirection();

			glm::vec3 upVec;
			upVec = glm::cross(eyeVec, lightDir);
			upVec = glm::cross(lightDir, upVec);

			aabb.UpdateCenterAndPoints();

			glm::mat4 lightViewMatrix = glm::lookAtRH(aabb.center - lightDir, aabb.center, upVec);

			/**************************************/
			/* ライトプロジェクション行列を求める */
			/**************************************/

			glm::vec3 aabbMin(+FLT_MAX, +FLT_MAX, +FLT_MAX);
			glm::vec3 aabbMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
			glm::vec3 temp;

			for (uint32_t i = 0; i < 8; i++)
			{
				temp = lightViewMatrix * glm::vec4(aabb.points[i], 1.0f);
				aabbMin = glm::min(aabbMin, temp);
				aabbMax = glm::max(aabbMax, temp);
			}

			aabbMin.z -= m_Shadow.boundsBias;
			aabbMax.z += m_Shadow.boundsBias;

			glm::mat4 lightProjMatrix = glm::mat4(1.0f);

			lightProjMatrix[0].x = 2.0f / (aabbMax.x - aabbMin.x);
			lightProjMatrix[1].y = 2.0f / (aabbMax.y - aabbMin.y);
			lightProjMatrix[2].z = 1.0f / (aabbMin.z - aabbMax.z);
			lightProjMatrix[3].x = (aabbMax.x + aabbMin.x) / (aabbMax.x - aabbMin.x);
			lightProjMatrix[3].y = (aabbMax.y + aabbMin.y) / (aabbMax.y - aabbMin.y);
			lightProjMatrix[3].z = aabbMin.z / (aabbMin.z - aabbMax.z);

			m_LightMatrix = lightProjMatrix * lightViewMatrix;
		}

		// ----------------------------------------------------------------------------------------------------
		// デバッグレンダラーを更新
		// ----------------------------------------------------------------------------------------------------

		m_pDebugRenderer->Update();

		// ----------------------------------------------------------------------------------------------------
		// SSAO を更新
		// ----------------------------------------------------------------------------------------------------

		float invFarZ = VE_FLOAT_RECIPROCAL(m_Camera->GetFarClip());

		m_SsaoStage.sampling.constant.param0.x = m_Ssao.radius * invFarZ;
		m_SsaoStage.sampling.constant.param0.y = m_Ssao.threshold * invFarZ;
		m_SsaoStage.sampling.constant.param0.z = (m_Ssao.threshold + m_Ssao.depth) * invFarZ;
		m_SsaoStage.sampling.constant.param0.w = m_Ssao.intensity;
		m_SsaoStage.sampling.constant.param1.z = m_Camera->GetNearClip();
		m_SsaoStage.sampling.constant.param1.w = m_Camera->GetFarClip();

		m_SsaoStage.blur.constantH.Set(m_Ssao.blurRadius, m_Ssao.blurSamples);
		m_SsaoStage.blur.constantV.Set(m_Ssao.blurRadius, m_Ssao.blurSamples);

		// ----------------------------------------------------------------------------------------------------
		// ライトを更新
		// ----------------------------------------------------------------------------------------------------

		const glm::vec2& shadowTexelSize = m_DeviceContext->GetGraphicsFactoryPtr()->GetShadowTexelSize();

		const glm::vec3& lightDir = m_Light->GetDirection();
		const glm::vec4& lightColor = m_Light->GetColor();

		m_LightingStage.directional.constant.eyePos = glm::vec4(eyePos, 1.0f);
		m_LightingStage.directional.constant.lightDir = glm::vec4(-lightDir, 1.0f);
		m_LightingStage.directional.constant.lightColor = lightColor;
		m_LightingStage.directional.constant.cameraParam.x = m_Camera->GetNearClip();
		m_LightingStage.directional.constant.cameraParam.y = m_Camera->GetFarClip();
		m_LightingStage.directional.constant.cameraParam.z = VE_FLOAT_DIV(m_Shadow.range - m_Shadow.fadeMargin, m_Camera->GetFarClip());
		m_LightingStage.directional.constant.cameraParam.w = VE_FLOAT_RECIPROCAL(VE_FLOAT_DIV(m_Shadow.fadeMargin, m_Camera->GetFarClip()));
		m_LightingStage.directional.constant.shadowParam.x = shadowTexelSize.x;
		m_LightingStage.directional.constant.shadowParam.y = shadowTexelSize.y;
		m_LightingStage.directional.constant.shadowParam.z = m_Shadow.depthBias;
		m_LightingStage.directional.constant.shadowParam.w = m_Shadow.dencity;
		m_LightingStage.directional.constant.invViewProjMat = m_Camera->GetInverseViewProjectionMatrix();
		m_LightingStage.directional.constant.lightMat = m_LightMatrix;

		m_ForwardStage.transparency.constant.eyePos = glm::vec4(eyePos, 1.0f);
		m_ForwardStage.transparency.constant.lightDir = glm::vec4(-lightDir, 1.0f);
		m_ForwardStage.transparency.constant.lightColor = lightColor;

		// ----------------------------------------------------------------------------------------------------
		// グレアを更新
		// ----------------------------------------------------------------------------------------------------

		m_GlareStage.brightPass.constant.threshold = m_Glare.bpThreshold;
		m_GlareStage.brightPass.constant.offset = m_Glare.bpOffset;

		for (uint32_t i = 0; i < Scene::GlarePageCount; i++)
		{
			m_GlareStage.blur[i].constantH.Set(m_Glare.blurRadius, m_Glare.blurSamples);
			m_GlareStage.blur[i].constantV.Set(m_Glare.blurRadius, m_Glare.blurSamples);
			m_GlareStage.paste[i].constant.color = glm::vec4(m_Glare.intensity[i]);
			m_GlareStage.paste[i].constant.color.a = 1.0f;
		}
	}

	void Scene::Render()
	{
		IV3DCommandBuffer* pCommandBuffer = m_DeviceContext->GetCurrentFramePtr()->pGraphicsCommandBuffer;
		uint32_t frameIndex = m_DeviceContext->GetCurrentFrameIndex();

		// ディファード : ジオメトリ
		RenderGeometry(pCommandBuffer, frameIndex);

		// ディファード : イルミネーション
		RenderIllumination(pCommandBuffer, frameIndex);

		// フォーワード
		RenderForward(pCommandBuffer, frameIndex);

		// グレア
		RenderGlare(pCommandBuffer, frameIndex);

		// イメージエフェクト
		RenderImageEffect(pCommandBuffer, frameIndex);

		// フィニッシュ
		RenderFinish(pCommandBuffer, frameIndex);
	}

	void Scene::Lost()
	{
		// 内部データを破棄
		InternalClear();

		// ステージを破棄
		LostStages();
	}

	bool Scene::Restore()
	{
		// ステージを復帰
		if (RestoreStages() == false)
		{
			return false;
		}

		m_DefaultViewport = m_DeviceContext->GetGraphicsFactoryPtr()->GetNativeViewport(GraphicsFactory::VT_DEFAULT);
		m_HalfViewport = m_DeviceContext->GetGraphicsFactoryPtr()->GetNativeViewport(GraphicsFactory::VT_HALF);
		m_QuarterViewport = m_DeviceContext->GetGraphicsFactoryPtr()->GetNativeViewport(GraphicsFactory::VT_QUARTER);

		m_Camera->SetAspect(m_DefaultViewport.rect.width, m_DefaultViewport.rect.height);

		return true;
	}

	void Scene::Dispose()
	{
		LostStages();

		if (m_pDebugRenderer != nullptr)
		{
			m_pDebugRenderer->Destroy();
			m_pDebugRenderer = nullptr;
		}

		if (m_pNodeSelector != nullptr)
		{
			m_pNodeSelector->Destroy();
			m_pNodeSelector = nullptr;
		}

		// グリッド
		if (m_GeometoryStage.grid.pVertexBuffer != nullptr)
		{
			m_GeometoryStage.grid.pVertexBuffer->Destroy();
			m_GeometoryStage.grid.pVertexBuffer = nullptr;
		}

		// シンプルバーテックス
		DeleteResource(m_pDeletingQueue, &m_SimpleVertexBuffer.pResource, &m_SimpleVertexBuffer.resourceAllocation);
	}

	/*******************/
	/* private - Scene */
	/*******************/

	bool Scene::Initialize(DeviceContextPtr deviceContext)
	{
		m_DeviceContext = deviceContext;
		m_pDeletingQueue = m_DeviceContext->GetDeletingQueuePtr();

		// ----------------------------------------------------------------------------------------------------
		// いろいろ初期化
		// ----------------------------------------------------------------------------------------------------

		// ベース
		if (InitializeBase() == false)
		{
			return false;
		}

		// ステージ
		if (InitializeStages() == false)
		{
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// リソースを作成
		// ----------------------------------------------------------------------------------------------------

		if (Restore() == false)
		{
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// ノードを更新
		// ----------------------------------------------------------------------------------------------------

		m_RootNode->Update();

		// ----------------------------------------------------------------------------------------------------

		return true;
	}

	bool Scene::InitializeBase()
	{
		IV3DDevice* pNativeDevice = m_DeviceContext->GetNativeDevicePtr();
		ResourceMemoryManager* pResourceMemoryManager = m_DeviceContext->GetResourceMemoryManagerPtr();

		// ----------------------------------------------------------------------------------------------------
		// シンプルバーテックスバッファー
		// ----------------------------------------------------------------------------------------------------

		{
			const SimpleVertex vertices[4] =
			{
				{ glm::vec4(+1.0f, -1.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f) },
				{ glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f) },
				{ glm::vec4(+1.0f, +1.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f) },
				{ glm::vec4(-1.0f, +1.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f) },
			};

			V3DBufferDesc bufferDesc;
			bufferDesc.usageFlags = V3D_BUFFER_USAGE_TRANSFER_DST | V3D_BUFFER_USAGE_VERTEX;
			bufferDesc.size = sizeof(vertices);

			if (pNativeDevice->CreateBuffer(bufferDesc, &m_SimpleVertexBuffer.pResource, VE_INTERFACE_DEBUG_NAME(L"VE_SimpleVertices")) != V3D_OK)
			{
				return false;
			}

			m_SimpleVertexBuffer.resourceAllocation = pResourceMemoryManager->Allocate(m_SimpleVertexBuffer.pResource, V3D_MEMORY_PROPERTY_DEVICE_LOCAL);
			if (m_SimpleVertexBuffer.resourceAllocation == nullptr)
			{
				return false;
			}

			IV3DCommandBuffer* pCommandBufer = m_DeviceContext->GetImmediateContextPtr()->Begin();

			V3DPipelineBarrier pipelineBarrier;
			pipelineBarrier.dependencyFlags = 0;

			V3DBufferMemoryBarrier memoryBarrier;
			memoryBarrier.srcQueueFamily = V3D_QUEUE_FAMILY_IGNORED;
			memoryBarrier.dstQueueFamily = V3D_QUEUE_FAMILY_IGNORED;
			memoryBarrier.pBuffer = m_SimpleVertexBuffer.pResource;
			memoryBarrier.offset = 0;
			memoryBarrier.size = bufferDesc.size;

			pipelineBarrier.srcStageMask = V3D_PIPELINE_STAGE_TOP_OF_PIPE;
			pipelineBarrier.dstStageMask = V3D_PIPELINE_STAGE_TRANSFER;
			memoryBarrier.srcAccessMask = 0;
			memoryBarrier.dstAccessMask = V3D_ACCESS_TRANSFER_WRITE;
			pCommandBufer->Barrier(pipelineBarrier, memoryBarrier);

			pCommandBufer->UpdateBuffer(m_SimpleVertexBuffer.pResource, 0, bufferDesc.size, vertices);

			pipelineBarrier.srcStageMask = V3D_PIPELINE_STAGE_TRANSFER;
			pipelineBarrier.dstStageMask = V3D_PIPELINE_STAGE_VERTEX_INPUT;
			memoryBarrier.srcAccessMask = V3D_ACCESS_TRANSFER_WRITE;
			memoryBarrier.dstAccessMask = V3D_ACCESS_VERTEX_READ;
			pCommandBufer->Barrier(pipelineBarrier, memoryBarrier);

			m_DeviceContext->GetImmediateContextPtr()->End();
		}

		// ----------------------------------------------------------------------------------------------------
		// SSAO : ノイズテクスチャ
		// ----------------------------------------------------------------------------------------------------

		m_SsaoStage.sampling.noiseTexture = Texture::Create(m_DeviceContext, L"v3dEditor\\private\\image\\noise.dds", sizeof(VE_Scene_Ssao_NoizeImage), VE_Scene_Ssao_NoizeImage);
		if (m_SsaoStage.sampling.noiseTexture == nullptr)
		{
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// ルートノード
		// ----------------------------------------------------------------------------------------------------

		m_RootNode = Node::Create();
		m_RootNode->SetName(L"Scene");

		// ----------------------------------------------------------------------------------------------------
		// カメラノード
		// ----------------------------------------------------------------------------------------------------

		m_Camera = Camera::Create();

		NodePtr cameraNode = Node::AddChild(m_RootNode);
		cameraNode->SetName(L"Camera");
		Node::SetAttribute(cameraNode, m_Camera);

		glm::quat cameraRotation = glm::angleAxis(glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		cameraRotation *= glm::angleAxis(glm::radians(20.0f), glm::vec3(1.0f, 0.0f, 0.0f));

		m_Camera->SetView(cameraRotation, glm::vec3(0.0f, 0.0f, 0.0f), 20.0f);

		// ----------------------------------------------------------------------------------------------------
		// ライトノード
		// ----------------------------------------------------------------------------------------------------

		m_Light = Light::Create();

		NodePtr lightNode = Node::AddChild(m_RootNode);
		lightNode->SetName(L"Light");
		Node::SetAttribute(lightNode, m_Light);

		lightNode->SetLocalTransform(glm::vec3(0.5f), glm::angleAxis(glm::radians(10.0f), glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f))), glm::vec3(10.0f, -20.0f, -10.0f));

		// ----------------------------------------------------------------------------------------------------
		// ノードセレクター
		// ----------------------------------------------------------------------------------------------------

		m_pNodeSelector = NodeSelector::Create();
		if (m_pNodeSelector == nullptr)
		{
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// デバッグレンダラー
		// ----------------------------------------------------------------------------------------------------

		m_pDebugRenderer = DebugRenderer::Create(m_DeviceContext);
		if (m_pDebugRenderer == nullptr)
		{
			return false;
		}

		// ----------------------------------------------------------------------------------------------------

		return true;
	}

	bool Scene::InitializeStages()
	{
		GraphicsFactory* pGraphicsFactory = m_DeviceContext->GetGraphicsFactoryPtr();
		uint32_t frameCount = m_DeviceContext->GetFrameCount();

		// ----------------------------------------------------------------------------------------------------
		// Paste
		// ----------------------------------------------------------------------------------------------------

		m_PasteStage.frameBufferHandle = pGraphicsFactory->GetFrameBufferHandle(GraphicsFactory::ST_SCENE_PASTE);

		m_PasteStage.sampling.pipelineHandle[0] = pGraphicsFactory->GetPipelineHandle(GraphicsFactory::ST_SCENE_PASTE, GraphicsFactory::SST_SCENE_PASTE_0_ADD);
		m_PasteStage.sampling.pipelineHandle[1] = pGraphicsFactory->GetPipelineHandle(GraphicsFactory::ST_SCENE_PASTE, GraphicsFactory::SST_SCENE_PASTE_0_SCREEN);

		// ----------------------------------------------------------------------------------------------------
		// BrightPass
		// ----------------------------------------------------------------------------------------------------

		m_BrightPassStage.frameBufferHandle = pGraphicsFactory->GetFrameBufferHandle(GraphicsFactory::ST_SCENE_BRIGHT_PASS);

		m_BrightPassStage.sampling.pipelineHandle = pGraphicsFactory->GetPipelineHandle(GraphicsFactory::ST_SCENE_BRIGHT_PASS, GraphicsFactory::SST_SCENE_BRIGHT_PASS_SAMPLING);

		// ----------------------------------------------------------------------------------------------------
		// Blur
		// ----------------------------------------------------------------------------------------------------

		m_BlurStage.frameBufferHandle[0] = pGraphicsFactory->GetFrameBufferHandle(GraphicsFactory::ST_SCENE_BLUR, 0);
		m_BlurStage.frameBufferHandle[1] = pGraphicsFactory->GetFrameBufferHandle(GraphicsFactory::ST_SCENE_BLUR, 1);

		m_BlurStage.downSampling.pipelineHandle = pGraphicsFactory->GetPipelineHandle(GraphicsFactory::ST_SCENE_BLUR, GraphicsFactory::SST_SCENE_BLUR_DOWN_SAMPLING);

		m_BlurStage.horizonal.pipelineHandle = pGraphicsFactory->GetPipelineHandle(GraphicsFactory::ST_SCENE_BLUR, GraphicsFactory::SST_SCENE_BLUR_HORIZONAL);
		m_BlurStage.horizonal.descriptorSets[0].resize(frameCount, nullptr);
		m_BlurStage.horizonal.descriptorSets[1].resize(frameCount, nullptr);

		m_BlurStage.vertical.pipelineHandle = pGraphicsFactory->GetPipelineHandle(GraphicsFactory::ST_SCENE_BLUR, GraphicsFactory::SST_SCENE_BLUR_VERTICAL);
		m_BlurStage.vertical.descriptorSets[0].resize(frameCount, nullptr);
		m_BlurStage.vertical.descriptorSets[1].resize(frameCount, nullptr);

		// ----------------------------------------------------------------------------------------------------
		// Deffered : Geometry
		// ----------------------------------------------------------------------------------------------------

		m_GeometoryStage.frameBufferHandle = pGraphicsFactory->GetFrameBufferHandle(GraphicsFactory::ST_SCENE_GEOMETRY);

		m_GeometoryStage.grid.pipelineHandle = pGraphicsFactory->GetPipelineHandle(GraphicsFactory::ST_SCENE_GEOMETRY, GraphicsFactory::SST_SCENE_GEOMETRY_GRID);

		UpdateGrid();

		// ----------------------------------------------------------------------------------------------------
		// SSAO
		// ----------------------------------------------------------------------------------------------------

		m_SsaoStage.frameBufferHandle = pGraphicsFactory->GetFrameBufferHandle(GraphicsFactory::ST_SCENE_SSAO);

		m_SsaoStage.sampling.pipelineHandle = pGraphicsFactory->GetPipelineHandle(GraphicsFactory::ST_SCENE_SSAO, GraphicsFactory::SST_SCENE_SSAO_SAMPLING);
		m_SsaoStage.sampling.descriptorSets.resize(frameCount, nullptr);

		m_SsaoStage.blur.descriptorSets.resize(frameCount, nullptr);

		// ----------------------------------------------------------------------------------------------------
		// Shadow
		// ----------------------------------------------------------------------------------------------------

		m_ShadowStage.frameBufferHandle = pGraphicsFactory->GetFrameBufferHandle(GraphicsFactory::ST_SCENE_SHADOW);

		// ----------------------------------------------------------------------------------------------------
		// Deffered : Lighting
		// ----------------------------------------------------------------------------------------------------

		m_LightingStage.frameBufferHandle = pGraphicsFactory->GetFrameBufferHandle(GraphicsFactory::ST_SCENE_LIGHTING);

		m_LightingStage.directional.pipelineHandle = pGraphicsFactory->GetPipelineHandle(GraphicsFactory::ST_SCENE_LIGHTING, GraphicsFactory::SST_SCENE_LIGHTING_DIRECTIONAL);
		m_LightingStage.directional.descriptorSets.resize(frameCount, nullptr);

		m_LightingStage.finish.pipelineHandle = pGraphicsFactory->GetPipelineHandle(GraphicsFactory::ST_SCENE_LIGHTING, GraphicsFactory::SST_SCENE_LIGHTING_FINISH);
		m_LightingStage.finish.descriptorSets.resize(frameCount, nullptr);

		// ----------------------------------------------------------------------------------------------------
		// Forward
		// ----------------------------------------------------------------------------------------------------

		m_ForwardStage.frameBufferHandle = pGraphicsFactory->GetFrameBufferHandle(GraphicsFactory::ST_SCENE_FORWARD);

		// ----------------------------------------------------------------------------------------------------
		// Glare
		// ----------------------------------------------------------------------------------------------------

		m_GlareStage.brightPass.descriptorSets.resize(frameCount, nullptr);

		for (uint32_t i = 0; i < Scene::GlarePageCount; i++)
		{
			m_GlareStage.blur[i].descriptorSets.resize(frameCount, nullptr);
			m_GlareStage.paste[i].descriptorSets.resize(frameCount, nullptr);
		}

		// ----------------------------------------------------------------------------------------------------
		// ImageEffect
		// ----------------------------------------------------------------------------------------------------

		m_ImageEffectStage.frameBufferHandle = pGraphicsFactory->GetFrameBufferHandle(GraphicsFactory::ST_SCENE_IMAGE_EFFECT);

		m_ImageEffectStage.toneMapping.pipelineHandle[0] = pGraphicsFactory->GetPipelineHandle(GraphicsFactory::ST_SCENE_IMAGE_EFFECT, GraphicsFactory::SST_SCENE_IMAGE_EFFECT_TONE_MAPPING);
		m_ImageEffectStage.toneMapping.pipelineHandle[1] = pGraphicsFactory->GetPipelineHandle(GraphicsFactory::ST_SCENE_IMAGE_EFFECT, GraphicsFactory::SST_SCENE_IMAGE_EFFECT_TONE_MAPPING_OFF);
		m_ImageEffectStage.toneMapping.descriptorSets.resize(frameCount, nullptr);

		m_ImageEffectStage.selectMapping.pipelineHandle = pGraphicsFactory->GetPipelineHandle(GraphicsFactory::ST_SCENE_IMAGE_EFFECT, GraphicsFactory::SST_SCENE_IMAGE_EFFECT_SELECT_MAPPING);
		m_ImageEffectStage.selectMapping.descriptorSets.resize(frameCount, nullptr);

		m_ImageEffectStage.debug.pipelineHandle = pGraphicsFactory->GetPipelineHandle(GraphicsFactory::ST_SCENE_IMAGE_EFFECT, GraphicsFactory::SST_SCENE_IMAGE_EFFECT_DEBUG);

		m_ImageEffectStage.fxaa.pipelineHandle[0] = pGraphicsFactory->GetPipelineHandle(GraphicsFactory::ST_SCENE_IMAGE_EFFECT, GraphicsFactory::SST_SCENE_IMAGE_EFFECT_FXAA);
		m_ImageEffectStage.fxaa.pipelineHandle[1] = pGraphicsFactory->GetPipelineHandle(GraphicsFactory::ST_SCENE_IMAGE_EFFECT, GraphicsFactory::SST_SCENE_IMAGE_EFFECT_FXAA_OFF);
		m_ImageEffectStage.fxaa.descriptorSets.resize(frameCount, nullptr);

		// ----------------------------------------------------------------------------------------------------
		// Finish
		// ----------------------------------------------------------------------------------------------------

		m_FinishStage.frameBufferHandle = pGraphicsFactory->GetFrameBufferHandle(GraphicsFactory::ST_SCENE_FINISH);

		m_FinishStage.pipelineHandle = pGraphicsFactory->GetPipelineHandle(GraphicsFactory::ST_SCENE_FINISH, GraphicsFactory::SST_SCENE_FINISH_COPY);
		m_FinishStage.descriptorSets.resize(frameCount, nullptr);

		// ----------------------------------------------------------------------------------------------------

		return true;
	}

	void Scene::LostStages()
	{
		DeleteResource(m_pDeletingQueue, &m_SelectBuffer.pResource, &m_SelectBuffer.resourceAllocation);

		DeleteDeviceChild(m_pDeletingQueue, m_GlareStage.brightPass.descriptorSets);

		for (auto i = 0; i < Scene::GlarePageCount; i++)
		{
			DeleteDeviceChild(m_pDeletingQueue, m_GlareStage.blur[i].descriptorSets);
			DeleteDeviceChild(m_pDeletingQueue, m_GlareStage.paste[i].descriptorSets);
		}

		for (auto i = 0; i < 2; i++)
		{
			DeleteDeviceChild(m_pDeletingQueue, m_BlurStage.horizonal.descriptorSets[i]);
			DeleteDeviceChild(m_pDeletingQueue, m_BlurStage.vertical.descriptorSets[i]);
		}

		DeleteDeviceChild(m_pDeletingQueue, m_SsaoStage.sampling.descriptorSets);
		DeleteDeviceChild(m_pDeletingQueue, m_SsaoStage.blur.descriptorSets);

		DeleteDeviceChild(m_pDeletingQueue, m_LightingStage.directional.descriptorSets);
		DeleteDeviceChild(m_pDeletingQueue, m_LightingStage.finish.descriptorSets);

		DeleteDeviceChild(m_pDeletingQueue, m_ImageEffectStage.fxaa.descriptorSets);
		DeleteDeviceChild(m_pDeletingQueue, m_ImageEffectStage.toneMapping.descriptorSets);
		DeleteDeviceChild(m_pDeletingQueue, m_ImageEffectStage.selectMapping.descriptorSets);

		DeleteDeviceChild(m_pDeletingQueue, m_FinishStage.descriptorSets);
	}

	bool Scene::RestoreStages()
	{
		GraphicsFactory* pGraphicsFactory = m_DeviceContext->GetGraphicsFactoryPtr();
		const V3DSwapChainDesc& nativeSwapChainDesc = m_DeviceContext->GetNativeSwapchainDesc();
		uint32_t frameCount = m_DeviceContext->GetFrameCount();

		// ----------------------------------------------------------------------------------------------------
		// Blur
		// ----------------------------------------------------------------------------------------------------

		/*************/
		/* Horizonal */
		/*************/

		{
			static constexpr GraphicsFactory::ATTACHMENT_TYPE attachmentTypes[Scene::GlarePageCount] =
			{
				GraphicsFactory::AT_BL_HALF_0,
				GraphicsFactory::AT_BL_QUARTER_0,
			};

			UniqueRefernecePtr<IV3DSampler> sampler = UniqueRefernecePtr<IV3DSampler>(m_DeviceContext->GetSamplerFactoryPtr()->Create(V3D_FILTER_LINEAR, V3D_ADDRESS_MODE_CLAMP_TO_EDGE));
			if (sampler == nullptr)
			{
				return false;
			}

			for (uint32_t i = 0; i < 2; i++)
			{
				for (uint32_t j = 0; j < frameCount; j++)
				{
					m_BlurStage.horizonal.descriptorSets[i][j] = pGraphicsFactory->CreateNativeDescriptorSet(GraphicsFactory::ST_SCENE_BLUR, GraphicsFactory::SST_SCENE_BLUR_HORIZONAL);
					if (m_BlurStage.horizonal.descriptorSets[i][j] == nullptr)
					{
						return false;
					}

					m_BlurStage.horizonal.descriptorSets[i][j]->SetImageViewAndSampler(0, pGraphicsFactory->GetNativeAttachmentPtr(attachmentTypes[i], j), sampler.get());
					m_BlurStage.horizonal.descriptorSets[i][j]->Update();
				}
			}
		}

		/************/
		/* Vertical */
		/************/

		{
			static constexpr GraphicsFactory::ATTACHMENT_TYPE attachmentTypes[Scene::GlarePageCount] =
			{
				GraphicsFactory::AT_BL_HALF_1,
				GraphicsFactory::AT_BL_QUARTER_1,
			};

			UniqueRefernecePtr<IV3DSampler> sampler = UniqueRefernecePtr<IV3DSampler>(m_DeviceContext->GetSamplerFactoryPtr()->Create(V3D_FILTER_LINEAR, V3D_ADDRESS_MODE_CLAMP_TO_EDGE));
			if (sampler == nullptr)
			{
				return false;
			}

			for (uint32_t i = 0; i < 2; i++)
			{
				for (uint32_t j = 0; j < frameCount; j++)
				{
					m_BlurStage.vertical.descriptorSets[i][j] = pGraphicsFactory->CreateNativeDescriptorSet(GraphicsFactory::ST_SCENE_BLUR, GraphicsFactory::SST_SCENE_BLUR_VERTICAL);
					if (m_BlurStage.vertical.descriptorSets[i][j] == nullptr)
					{
						return false;
					}

					m_BlurStage.vertical.descriptorSets[i][j]->SetImageViewAndSampler(0, pGraphicsFactory->GetNativeAttachmentPtr(attachmentTypes[i], j), sampler.get());
					m_BlurStage.vertical.descriptorSets[i][j]->Update();
				}
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// Defferd : Geometry
		// ----------------------------------------------------------------------------------------------------

		/****************/
		/* SelectBuffer */
		/****************/

		{
			const V3DImageDesc& imageDesc = m_DeviceContext->GetGraphicsFactoryPtr()->GetNativeAttachmentDesc(GraphicsFactory::AT_GE_SELECT);

			V3DBufferDesc bufferDesc;
			bufferDesc.size = imageDesc.width * imageDesc.height * 4;
			bufferDesc.usageFlags = V3D_BUFFER_USAGE_TRANSFER_DST;

			if (m_DeviceContext->GetNativeDevicePtr()->CreateBuffer(bufferDesc, &m_SelectBuffer.pResource, VE_INTERFACE_DEBUG_NAME(L"VE_Scene_Select")) != V3D_OK)
			{
				return false;
			}

			m_SelectBuffer.resourceAllocation = m_DeviceContext->GetResourceMemoryManagerPtr()->Allocate(m_SelectBuffer.pResource, V3D_MEMORY_PROPERTY_HOST_VISIBLE);
			if (m_SelectBuffer.resourceAllocation == nullptr)
			{
				return false;
			}

			IV3DCommandBuffer* pCommandBuffer = m_DeviceContext->GetImmediateContextPtr()->Begin();
			pCommandBuffer->FillBuffer(m_SelectBuffer.pResource, 0, m_SelectBuffer.pResource->GetResourceDesc().memorySize, 0);
			m_DeviceContext->GetImmediateContextPtr()->End();
		}

		// ----------------------------------------------------------------------------------------------------
		// SSAO
		// ----------------------------------------------------------------------------------------------------

		/************/
		/* Sampling */
		/************/

		{
			UniqueRefernecePtr<IV3DSampler> sampler0 = UniqueRefernecePtr<IV3DSampler>(m_DeviceContext->GetSamplerFactoryPtr()->Create(V3D_FILTER_NEAREST, V3D_ADDRESS_MODE_CLAMP_TO_EDGE));
			if (sampler0 == nullptr)
			{
				return false;
			}

			UniqueRefernecePtr<IV3DSampler> sampler1 = UniqueRefernecePtr<IV3DSampler>(m_DeviceContext->GetSamplerFactoryPtr()->Create(V3D_FILTER_NEAREST, V3D_ADDRESS_MODE_REPEAT));
			if (sampler1 == nullptr)
			{
				return false;
			}

			for (uint32_t i = 0; i < frameCount; i++)
			{
				m_SsaoStage.sampling.descriptorSets[i] = pGraphicsFactory->CreateNativeDescriptorSet(GraphicsFactory::ST_SCENE_SSAO, GraphicsFactory::SST_SCENE_SSAO_SAMPLING);
				if (m_SsaoStage.sampling.descriptorSets[i] == nullptr)
				{
					return false;
				}

				m_SsaoStage.sampling.descriptorSets[i]->SetImageViewAndSampler(0, pGraphicsFactory->GetNativeAttachmentPtr(GraphicsFactory::AT_GE_BUFFER_0, i), sampler0.get());
				m_SsaoStage.sampling.descriptorSets[i]->SetImageViewAndSampler(1, pGraphicsFactory->GetNativeAttachmentPtr(GraphicsFactory::AT_GE_BUFFER_1, i), sampler0.get());
				m_SsaoStage.sampling.descriptorSets[i]->SetImageViewAndSampler(2, m_SsaoStage.sampling.noiseTexture->GetNativeImageViewPtr(), sampler1.get());
				m_SsaoStage.sampling.descriptorSets[i]->Update();
			}

			const V3DImageDesc& texDesc = m_SsaoStage.sampling.noiseTexture->GetNativeDesc();

			m_SsaoStage.sampling.constant.param1.x = VE_FLOAT_DIV(static_cast<float>(nativeSwapChainDesc.imageWidth), static_cast<float>(texDesc.width));
			m_SsaoStage.sampling.constant.param1.y = VE_FLOAT_DIV(static_cast<float>(nativeSwapChainDesc.imageHeight), static_cast<float>(texDesc.height));
		}

		/********/
		/* Blur */
		/********/

		{
			// デスクリプタセット
			UniqueRefernecePtr<IV3DSampler> sampler = UniqueRefernecePtr<IV3DSampler>(m_DeviceContext->GetSamplerFactoryPtr()->Create(V3D_FILTER_LINEAR, V3D_ADDRESS_MODE_CLAMP_TO_EDGE));
			if (sampler == nullptr)
			{
				return false;
			}

			for (uint32_t i = 0; i < frameCount; i++)
			{
				m_SsaoStage.blur.descriptorSets[i] = pGraphicsFactory->CreateNativeDescriptorSet(GraphicsFactory::ST_SCENE_BLUR, GraphicsFactory::SST_SCENE_BLUR_DOWN_SAMPLING);
				if (m_SsaoStage.blur.descriptorSets[i] == nullptr)
				{
					return false;
				}

				m_SsaoStage.blur.descriptorSets[i]->SetImageViewAndSampler(0, pGraphicsFactory->GetNativeAttachmentPtr(GraphicsFactory::AT_SS_COLOR, i), sampler.get());
				m_SsaoStage.blur.descriptorSets[i]->Update();
			}

			//定数
			const V3DImageDesc& ssaoColorDesc = pGraphicsFactory->GetNativeAttachmentDesc(GraphicsFactory::AT_SS_COLOR);
			m_SsaoStage.blur.constantDS = DownSamplingConstant::Initialize(ssaoColorDesc.width, ssaoColorDesc.height);

			const V3DImageDesc& blurColorDesc = pGraphicsFactory->GetNativeAttachmentDesc(GraphicsFactory::AT_BL_HALF_0);
			m_SsaoStage.blur.constantH = GaussianBlurConstant::Initialize(blurColorDesc.width, blurColorDesc.height, glm::vec2(1.0f, 0.0f));
			m_SsaoStage.blur.constantV = GaussianBlurConstant::Initialize(blurColorDesc.width, blurColorDesc.height, glm::vec2(0.0f, 1.0f));
		}

		// ----------------------------------------------------------------------------------------------------
		// Lighting
		// ----------------------------------------------------------------------------------------------------

		/***********************/
		/* DirectionalLighting */
		/***********************/

		{
			UniqueRefernecePtr<IV3DSampler> sampler = UniqueRefernecePtr<IV3DSampler>(m_DeviceContext->GetSamplerFactoryPtr()->Create(V3D_FILTER_NEAREST, V3D_ADDRESS_MODE_CLAMP_TO_EDGE));
			if (sampler == nullptr)
			{
				return false;
			}

			for (uint32_t i = 0; i < frameCount; i++)
			{
				m_LightingStage.directional.descriptorSets[i] = pGraphicsFactory->CreateNativeDescriptorSet(GraphicsFactory::ST_SCENE_LIGHTING, GraphicsFactory::SST_SCENE_LIGHTING_DIRECTIONAL);
				if (m_LightingStage.directional.descriptorSets[i] == nullptr)
				{
					return false;
				}

				m_LightingStage.directional.descriptorSets[i]->SetImageViewAndSampler(0, pGraphicsFactory->GetNativeAttachmentPtr(GraphicsFactory::AT_GE_COLOR, i), sampler.get());
				m_LightingStage.directional.descriptorSets[i]->SetImageViewAndSampler(1, pGraphicsFactory->GetNativeAttachmentPtr(GraphicsFactory::AT_GE_BUFFER_0, i), sampler.get());
				m_LightingStage.directional.descriptorSets[i]->SetImageViewAndSampler(2, pGraphicsFactory->GetNativeAttachmentPtr(GraphicsFactory::AT_GE_BUFFER_1, i), sampler.get());
				m_LightingStage.directional.descriptorSets[i]->SetImageViewAndSampler(3, pGraphicsFactory->GetNativeAttachmentPtr(GraphicsFactory::AT_BL_HALF_0, i), sampler.get()); // SSAO
				m_LightingStage.directional.descriptorSets[i]->SetImageViewAndSampler(4, pGraphicsFactory->GetNativeAttachmentPtr(GraphicsFactory::AT_SA_BUFFER, i), sampler.get());
				m_LightingStage.directional.descriptorSets[i]->Update();
			}
		}

		/******************/
		/* FinishLighting */
		/******************/

		{
			UniqueRefernecePtr<IV3DSampler> sampler = UniqueRefernecePtr<IV3DSampler>(m_DeviceContext->GetSamplerFactoryPtr()->Create(V3D_FILTER_NEAREST, V3D_ADDRESS_MODE_CLAMP_TO_EDGE));
			if (sampler == nullptr)
			{
				return false;
			}

			for (uint32_t i = 0; i < frameCount; i++)
			{
				m_LightingStage.finish.descriptorSets[i] = pGraphicsFactory->CreateNativeDescriptorSet(GraphicsFactory::ST_SCENE_LIGHTING, GraphicsFactory::SST_SCENE_LIGHTING_FINISH);
				if (m_LightingStage.finish.descriptorSets[i] == nullptr)
				{
					return false;
				}

				m_LightingStage.finish.descriptorSets[i]->SetImageViewAndSampler(0, pGraphicsFactory->GetNativeAttachmentPtr(GraphicsFactory::AT_GE_COLOR, i), sampler.get());
				m_LightingStage.finish.descriptorSets[i]->SetImageViewAndSampler(1, pGraphicsFactory->GetNativeAttachmentPtr(GraphicsFactory::AT_LI_COLOR, i), sampler.get());
				m_LightingStage.finish.descriptorSets[i]->SetImageViewAndSampler(2, pGraphicsFactory->GetNativeAttachmentPtr(GraphicsFactory::AT_GL_COLOR, i), sampler.get());
				m_LightingStage.finish.descriptorSets[i]->Update();
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// Glare
		// ----------------------------------------------------------------------------------------------------

		{
			UniqueRefernecePtr<IV3DSampler> sampler0 = UniqueRefernecePtr<IV3DSampler>(m_DeviceContext->GetSamplerFactoryPtr()->Create(V3D_FILTER_NEAREST, V3D_ADDRESS_MODE_CLAMP_TO_EDGE));
			if (sampler0 == nullptr)
			{
				return false;
			}

			UniqueRefernecePtr<IV3DSampler> sampler1 = UniqueRefernecePtr<IV3DSampler>(m_DeviceContext->GetSamplerFactoryPtr()->Create(V3D_FILTER_LINEAR, V3D_ADDRESS_MODE_CLAMP_TO_EDGE));
			if (sampler1 == nullptr)
			{
				return false;
			}

			/**************/
			/* BrightPass */
			/**************/

			for (uint32_t i = 0; i < frameCount; i++)
			{
				m_GlareStage.brightPass.descriptorSets[i] = pGraphicsFactory->CreateNativeDescriptorSet(GraphicsFactory::ST_SCENE_BRIGHT_PASS, GraphicsFactory::SST_SCENE_BRIGHT_PASS_SAMPLING);
				if (m_GlareStage.brightPass.descriptorSets[i] == nullptr)
				{
					return false;
				}

				m_GlareStage.brightPass.descriptorSets[i]->SetImageViewAndSampler(0, pGraphicsFactory->GetNativeAttachmentPtr(GraphicsFactory::AT_GL_COLOR, i), sampler0.get());
				m_GlareStage.brightPass.descriptorSets[i]->Update();
			}

			static constexpr GraphicsFactory::ATTACHMENT_TYPE dsAttachmentTypes[Scene::GlarePageCount] =
			{
				GraphicsFactory::AT_BR_COLOR,
				GraphicsFactory::AT_BL_HALF_0,
			};

			static constexpr GraphicsFactory::ATTACHMENT_TYPE blurAttachmentTypes[Scene::GlarePageCount] =
			{
				GraphicsFactory::AT_BL_HALF_0,
				GraphicsFactory::AT_BL_QUARTER_0,
			};

			for (uint32_t i = 0; i < Scene::GlarePageCount; i++)
			{
				for (uint32_t j = 0; j < frameCount; j++)
				{
					/********/
					/* Blur */
					/********/

					m_GlareStage.blur[i].descriptorSets[j] = pGraphicsFactory->CreateNativeDescriptorSet(GraphicsFactory::ST_SCENE_BLUR, GraphicsFactory::SST_SCENE_BLUR_DOWN_SAMPLING);
					if (m_GlareStage.blur[i].descriptorSets[j] == nullptr)
					{
						return false;
					}

					m_GlareStage.blur[i].descriptorSets[j]->SetImageViewAndSampler(0, pGraphicsFactory->GetNativeAttachmentPtr(dsAttachmentTypes[i], j), sampler1.get());
					m_GlareStage.blur[i].descriptorSets[j]->Update();

					/*********/
					/* Paste */
					/*********/

					m_GlareStage.paste[i].descriptorSets[j] = pGraphicsFactory->CreateNativeDescriptorSet(GraphicsFactory::ST_SCENE_PASTE, GraphicsFactory::SST_SCENE_PASTE_0_ADD);
					if (m_GlareStage.paste[i].descriptorSets[j] == nullptr)
					{
						return false;
					}

					m_GlareStage.paste[i].descriptorSets[j]->SetImageViewAndSampler(0, pGraphicsFactory->GetNativeAttachmentPtr(blurAttachmentTypes[i], j), sampler1.get());
					m_GlareStage.paste[i].descriptorSets[j]->Update();
				}

				const V3DImageDesc& srcDsDesc = pGraphicsFactory->GetNativeAttachmentDesc(dsAttachmentTypes[i]);
				m_GlareStage.blur[i].constantDS = DownSamplingConstant::Initialize(srcDsDesc.width, srcDsDesc.height);

				const V3DImageDesc& blurColorDesc = pGraphicsFactory->GetNativeAttachmentDesc(blurAttachmentTypes[i]);
				m_GlareStage.blur[i].constantH = GaussianBlurConstant::Initialize(blurColorDesc.width, blurColorDesc.height, glm::vec2(1.0f, 0.0f));
				m_GlareStage.blur[i].constantV = GaussianBlurConstant::Initialize(blurColorDesc.width, blurColorDesc.height, glm::vec2(0.0f, 1.0f));
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// ImageEffect
		// ----------------------------------------------------------------------------------------------------

		/***************/
		/* ToneMapping */
		/***************/
		{
			UniqueRefernecePtr<IV3DSampler> sampler = UniqueRefernecePtr<IV3DSampler>(m_DeviceContext->GetSamplerFactoryPtr()->Create(V3D_FILTER_NEAREST, V3D_ADDRESS_MODE_CLAMP_TO_EDGE));
			if (sampler == nullptr)
			{
				return false;
			}

			for (uint32_t i = 0; i < frameCount; i++)
			{
				m_ImageEffectStage.toneMapping.descriptorSets[i] = pGraphicsFactory->CreateNativeDescriptorSet(GraphicsFactory::ST_SCENE_IMAGE_EFFECT, GraphicsFactory::SST_SCENE_IMAGE_EFFECT_TONE_MAPPING);
				if (m_ImageEffectStage.toneMapping.descriptorSets[i] == nullptr)
				{
					return false;
				}

				m_ImageEffectStage.toneMapping.descriptorSets[i]->SetImageViewAndSampler(0, pGraphicsFactory->GetNativeAttachmentPtr(GraphicsFactory::AT_SH_COLOR, i), sampler.get());
				m_ImageEffectStage.toneMapping.descriptorSets[i]->Update();
			}
		}

		/*****************/
		/* SelectMapping */
		/*****************/

		{
			UniqueRefernecePtr<IV3DSampler> sampler = UniqueRefernecePtr<IV3DSampler>(m_DeviceContext->GetSamplerFactoryPtr()->Create(V3D_FILTER_NEAREST, V3D_ADDRESS_MODE_CLAMP_TO_EDGE));
			if (sampler == nullptr)
			{
				return false;
			}

			for (uint32_t i = 0; i < frameCount; i++)
			{
				m_ImageEffectStage.selectMapping.descriptorSets[i] = pGraphicsFactory->CreateNativeDescriptorSet(GraphicsFactory::ST_SCENE_IMAGE_EFFECT, GraphicsFactory::SST_SCENE_IMAGE_EFFECT_SELECT_MAPPING);
				if (m_ImageEffectStage.selectMapping.descriptorSets[i] == nullptr)
				{
					return false;
				}

				m_ImageEffectStage.selectMapping.descriptorSets[i]->SetImageViewAndSampler(0, pGraphicsFactory->GetNativeAttachmentPtr(GraphicsFactory::AT_IE_SELECT_COLOR, i), sampler.get());
				m_ImageEffectStage.selectMapping.descriptorSets[i]->Update();
			}

			m_ImageEffectStage.selectMapping.constant.texelSize.x = VE_FLOAT_RECIPROCAL(static_cast<float>(nativeSwapChainDesc.imageWidth));
			m_ImageEffectStage.selectMapping.constant.texelSize.y = VE_FLOAT_RECIPROCAL(static_cast<float>(nativeSwapChainDesc.imageHeight));
		}

		/********/
		/* Fxaa */
		/********/

		{
			UniqueRefernecePtr<IV3DSampler> sampler = UniqueRefernecePtr<IV3DSampler>(m_DeviceContext->GetSamplerFactoryPtr()->Create(V3D_FILTER_NEAREST, V3D_ADDRESS_MODE_CLAMP_TO_EDGE));
			if (sampler == nullptr)
			{
				return false;
			}

			for (uint32_t i = 0; i < frameCount; i++)
			{
				m_ImageEffectStage.fxaa.descriptorSets[i] = pGraphicsFactory->CreateNativeDescriptorSet(GraphicsFactory::ST_SCENE_IMAGE_EFFECT, GraphicsFactory::SST_SCENE_IMAGE_EFFECT_FXAA);
				if (m_ImageEffectStage.fxaa.descriptorSets[i] == nullptr)
				{
					return false;
				}

				m_ImageEffectStage.fxaa.descriptorSets[i]->SetImageViewAndSampler(0, pGraphicsFactory->GetNativeAttachmentPtr(GraphicsFactory::AT_IE_LDR_COLOR_0, i), sampler.get());
				m_ImageEffectStage.fxaa.descriptorSets[i]->Update();
			}

			float invWidth = VE_FLOAT_RECIPROCAL(static_cast<float>(nativeSwapChainDesc.imageWidth));
			float invHeight = VE_FLOAT_RECIPROCAL(static_cast<float>(nativeSwapChainDesc.imageHeight));

			m_ImageEffectStage.fxaa.constant.texOffset0.x = -invWidth;
			m_ImageEffectStage.fxaa.constant.texOffset0.y = -invHeight;
			m_ImageEffectStage.fxaa.constant.texOffset0.z = +invWidth;
			m_ImageEffectStage.fxaa.constant.texOffset0.w = -invHeight;
			m_ImageEffectStage.fxaa.constant.texOffset1.x = -invWidth;
			m_ImageEffectStage.fxaa.constant.texOffset1.y = +invHeight;
			m_ImageEffectStage.fxaa.constant.texOffset1.z = +invWidth;
			m_ImageEffectStage.fxaa.constant.texOffset1.w = +invHeight;
			m_ImageEffectStage.fxaa.constant.invTexSize.x = invWidth;
			m_ImageEffectStage.fxaa.constant.invTexSize.y = invHeight;
		}

		// ----------------------------------------------------------------------------------------------------
		// フィニッシュ
		// ----------------------------------------------------------------------------------------------------

		{
			UniqueRefernecePtr<IV3DSampler> sampler = UniqueRefernecePtr<IV3DSampler>(m_DeviceContext->GetSamplerFactoryPtr()->Create(V3D_FILTER_NEAREST, V3D_ADDRESS_MODE_CLAMP_TO_EDGE));
			if (sampler == nullptr)
			{
				return false;
			}

			for (uint32_t i = 0; i < frameCount; i++)
			{
				m_FinishStage.descriptorSets[i] = pGraphicsFactory->CreateNativeDescriptorSet(GraphicsFactory::ST_SCENE_FINISH, GraphicsFactory::SST_SCENE_FINISH_COPY);
				if (m_FinishStage.descriptorSets[i] == nullptr)
				{
					return false;
				}

				m_FinishStage.descriptorSets[i]->SetImageViewAndSampler(0, pGraphicsFactory->GetNativeAttachmentPtr(GraphicsFactory::AT_IE_LDR_COLOR_1, i), sampler.get());
				m_FinishStage.descriptorSets[i]->Update();
			}
		}

		// ----------------------------------------------------------------------------------------------------

		return true;
	}

	void Scene::UpdateGrid()
	{
		// ----------------------------------------------------------------------------------------------------
		// グリッドのラインを作成
		// ----------------------------------------------------------------------------------------------------

		float base = m_Grid.cellSize * static_cast<float>(m_Grid.halfCellCount);
		uint32_t lineCount = m_Grid.halfCellCount * 2 + 1;

		collection::Vector<glm::vec4> vertices;

		for (uint32_t x = 0; x < lineCount; x++)
		{
			float pos = -base + m_Grid.cellSize * static_cast<float>(x);
			glm::vec4 p;

			p = glm::vec4(pos, 0.0f, -base, 1.0f);
			vertices.push_back(p);

			p = glm::vec4(pos, 0.0f, +base, 1.0f);
			vertices.push_back(p);
		}

		for (uint32_t z = 0; z < lineCount; z++)
		{
			float pos = -base + m_Grid.cellSize * static_cast<float>(z);
			glm::vec4 p;

			p = glm::vec4(-base, 0.0f, pos, 1.0f);
			vertices.push_back(p);

			p = glm::vec4(+base, 0.0f, pos, 1.0f);
			vertices.push_back(p);
		}

		// ----------------------------------------------------------------------------------------------------
		// グリッドのラインを保存
		// ----------------------------------------------------------------------------------------------------

		bool update = true;

		size_t memorySize = sizeof(glm::vec4) * vertices.size();
		if ((m_GeometoryStage.grid.pVertexBuffer == nullptr) || (m_GeometoryStage.grid.pVertexBuffer->GetNativeRangeSize() < memorySize))
		{
			DynamicBuffer* pNewGridBuffer = DynamicBuffer::Create(m_DeviceContext, V3D_BUFFER_USAGE_VERTEX, memorySize, V3D_PIPELINE_STAGE_VERTEX_INPUT, V3D_ACCESS_VERTEX_READ, VE_INTERFACE_DEBUG_NAME(L"Grid"));
			if (pNewGridBuffer != nullptr)
			{
				if (m_GeometoryStage.grid.pVertexBuffer != nullptr)
				{
					m_GeometoryStage.grid.pVertexBuffer->Destroy();
				}

				m_GeometoryStage.grid.pVertexBuffer = pNewGridBuffer;
			}
			else
			{
				update = false;
			}
		}

		if (update == true)
		{
			void* pMemory = m_GeometoryStage.grid.pVertexBuffer->Map();
			VE_ASSERT(pMemory != nullptr);
			memcpy_s(pMemory, memorySize, vertices.data(), memorySize);
			m_GeometoryStage.grid.pVertexBuffer->Unmap();

			m_GeometoryStage.grid.vertexCount = static_cast<uint32_t>(vertices.size());
		}
		else
		{
			m_GeometoryStage.grid.vertexCount = 0;
		}
	}

	void Scene::InternalClear()
	{
		m_OpacityDrawSets.Clear();
		m_TransparencyDrawSets.Clear();
		m_ShadowDrawSets.Clear();
		m_SelectDrawSet = SelectDrawSet{};
		m_pNodeSelector->SetFound(nullptr);
	}

	void Scene::RenderGeometry(IV3DCommandBuffer* pCommandBuffer, uint32_t frameIndex)
	{
		const glm::mat4& viewProjMatrix = m_Camera->GetViewProjectionMatrix();

		pCommandBuffer->SetViewport(0, 1, &m_DefaultViewport);
		pCommandBuffer->SetScissor(0, 1, &m_DefaultViewport.rect);

		pCommandBuffer->BeginRenderPass(m_GeometoryStage.frameBufferHandle->GetPtr(frameIndex), true);

		// ----------------------------------------------------------------------------------------------------
		// Subpass 0 - Grid
		// ----------------------------------------------------------------------------------------------------

		if ((m_Grid.enable == true) && (m_GeometoryStage.grid.vertexCount > 0))
		{
			IV3DPipeline* pNativeGridPipeline = m_GeometoryStage.grid.pipelineHandle->GetPtr();

			pCommandBuffer->BindPipeline(pNativeGridPipeline);
			pCommandBuffer->BindVertexBuffer(0, m_GeometoryStage.grid.pVertexBuffer->GetNativeBufferPtr(), m_GeometoryStage.grid.pVertexBuffer->GetNativeRangeSize() * frameIndex);
			pCommandBuffer->PushConstant(pNativeGridPipeline, 0, &viewProjMatrix);
			pCommandBuffer->PushConstant(pNativeGridPipeline, 1, &m_Grid.color);
			pCommandBuffer->Draw(m_GeometoryStage.grid.vertexCount, 1, 0, 0);
		}

		// ----------------------------------------------------------------------------------------------------
		// Subpass 1 - ModelRenderer
		// ----------------------------------------------------------------------------------------------------

		pCommandBuffer->NextSubpass();

		if (m_OpacityDrawSets.GetCount() > 0)
		{
			OpacityDrawSet** ppDrawSet = m_OpacityDrawSets.GetData();
			OpacityDrawSet** ppDrawSetEnd = ppDrawSet + m_OpacityDrawSets.GetCount();

			IV3DBuffer* pPrevVertexBuffer = nullptr;
			IV3DPipeline* pPrevPipeline = nullptr;

			while (ppDrawSet != ppDrawSetEnd)
			{
				OpacityDrawSet* pDrawSet = *ppDrawSet;

				if (pPrevVertexBuffer != pDrawSet->pVertexBuffer)
				{
					pCommandBuffer->BindVertexBuffer(0, pDrawSet->pVertexBuffer);
					pCommandBuffer->BindIndexBuffer(pDrawSet->pIndexBuffer, 0, pDrawSet->indexType);

					pPrevVertexBuffer = pDrawSet->pVertexBuffer;
				}

				if (pPrevPipeline != pDrawSet->pPipeline)
				{
					pCommandBuffer->BindPipeline(pDrawSet->pPipeline);
					pCommandBuffer->PushConstant(pDrawSet->pPipeline, 0, &viewProjMatrix);

					pPrevPipeline = pDrawSet->pPipeline;
				}

				pCommandBuffer->BindDescriptorSet(pDrawSet->pPipeline, 0, 2, pDrawSet->descriptorSet, 2, pDrawSet->dynamicOffsets);

				pCommandBuffer->DrawIndexed(pDrawSet->indexCount, 1, pDrawSet->firstIndex, 0, 0);

				ppDrawSet++;
			}
		}

		pCommandBuffer->EndRenderPass();
	}

	void Scene::RenderIllumination(IV3DCommandBuffer* pCommandBuffer, uint32_t frameIndex)
	{
		IV3DPipeline* pNativePipeline;

		// ----------------------------------------------------------------------------------------------------
		// Ssao
		// ----------------------------------------------------------------------------------------------------

		/************/
		/* Sampling */
		/************/

		pCommandBuffer->BeginRenderPass(m_SsaoStage.frameBufferHandle->GetPtr(frameIndex), true);

		if (m_Ssao.enable == true)
		{
			pCommandBuffer->SetViewport(0, 1, &m_DefaultViewport);
			pCommandBuffer->SetScissor(0, 1, &m_DefaultViewport.rect);

			pNativePipeline = m_SsaoStage.sampling.pipelineHandle->GetPtr();

			pCommandBuffer->BindVertexBuffer(0, m_SimpleVertexBuffer.pResource);
			pCommandBuffer->BindPipeline(pNativePipeline);
			pCommandBuffer->BindDescriptorSet(pNativePipeline, 0, m_SsaoStage.sampling.descriptorSets[frameIndex]);
			pCommandBuffer->PushConstant(pNativePipeline, 0, &m_SsaoStage.sampling.constant);
			pCommandBuffer->Draw(4, 1, 0, 0);
		}

		pCommandBuffer->EndRenderPass();

		/**********/
		/* ブラー */
		/**********/

		pCommandBuffer->BeginRenderPass(m_BlurStage.frameBufferHandle[0]->GetPtr(frameIndex), true);

		pCommandBuffer->SetViewport(0, 1, &m_HalfViewport);
		pCommandBuffer->SetScissor(0, 1, &m_HalfViewport.rect);
		pCommandBuffer->BindVertexBuffer(0, m_SimpleVertexBuffer.pResource);

		// DownSampling
		pNativePipeline = m_BlurStage.downSampling.pipelineHandle->GetPtr();
		pCommandBuffer->BindPipeline(pNativePipeline);
		pCommandBuffer->BindDescriptorSet(pNativePipeline, 0, m_SsaoStage.blur.descriptorSets[frameIndex]);
		pCommandBuffer->PushConstant(pNativePipeline, 0, &m_SsaoStage.blur.constantDS);
		pCommandBuffer->Draw(4, 1, 0, 0);

		// HorizonalBlur
		pNativePipeline = m_BlurStage.horizonal.pipelineHandle->GetPtr();
		pCommandBuffer->NextSubpass();
		pCommandBuffer->BindPipeline(pNativePipeline);
		pCommandBuffer->BindDescriptorSet(pNativePipeline, 0, m_BlurStage.horizonal.descriptorSets[0][frameIndex]);
		pCommandBuffer->PushConstant(pNativePipeline, 0, &m_SsaoStage.blur.constantH);
		pCommandBuffer->Draw(4, 1, 0, 0);

		// VerticalBlur
		pNativePipeline = m_BlurStage.vertical.pipelineHandle->GetPtr();
		pCommandBuffer->NextSubpass();
		pCommandBuffer->BindPipeline(pNativePipeline);
		pCommandBuffer->BindDescriptorSet(pNativePipeline, 0, m_BlurStage.vertical.descriptorSets[0][frameIndex]);
		pCommandBuffer->PushConstant(pNativePipeline, 0, &m_SsaoStage.blur.constantV);
		pCommandBuffer->Draw(4, 1, 0, 0);

		pCommandBuffer->EndRenderPass();

		// ----------------------------------------------------------------------------------------------------
		// Shadow
		// ----------------------------------------------------------------------------------------------------

		pCommandBuffer->BeginRenderPass(m_ShadowStage.frameBufferHandle->GetPtr(frameIndex), true);

		if ((m_Shadow.enable == true) && (m_ShadowDrawSets.GetCount() > 0))
		{
			ShadowDrawSet** ppDrawSet = m_ShadowDrawSets.GetData();
			ShadowDrawSet** ppDrawSetEnd = ppDrawSet + m_ShadowDrawSets.GetCount();

			IV3DBuffer* pPrevVertexBuffer = nullptr;
			IV3DPipeline* pPrevPipeline = nullptr;

			const V3DViewport& viewport = m_DeviceContext->GetGraphicsFactoryPtr()->GetNativeViewport(GraphicsFactory::VT_SHADOW);
			pCommandBuffer->SetViewport(0, 1, &viewport);
			pCommandBuffer->SetScissor(0, 1, &viewport.rect);

			while (ppDrawSet != ppDrawSetEnd)
			{
				ShadowDrawSet* pDrawSet = *ppDrawSet;

				if (pPrevVertexBuffer != pDrawSet->pVertexBuffer)
				{
					pCommandBuffer->BindVertexBuffer(0, pDrawSet->pVertexBuffer);
					pCommandBuffer->BindIndexBuffer(pDrawSet->pIndexBuffer, 0, pDrawSet->indexType);

					pPrevVertexBuffer = pDrawSet->pVertexBuffer;
				}

				if (pPrevPipeline != pDrawSet->pPipeline)
				{
					pCommandBuffer->BindPipeline(pDrawSet->pPipeline);
					pCommandBuffer->PushConstant(pDrawSet->pPipeline, 0, &m_LightMatrix);

					pPrevPipeline = pDrawSet->pPipeline;
				}

				pCommandBuffer->BindDescriptorSet(pDrawSet->pPipeline, 0, 2, pDrawSet->descriptorSet, 2, pDrawSet->dynamicOffsets);

				pCommandBuffer->DrawIndexed(pDrawSet->indexCount, 1, pDrawSet->firstIndex, 0, 0);

				ppDrawSet++;
			}
		}

		pCommandBuffer->EndRenderPass();

		// ----------------------------------------------------------------------------------------------------
		// Lighting
		// ----------------------------------------------------------------------------------------------------

		pCommandBuffer->SetViewport(0, 1, &m_DefaultViewport);
		pCommandBuffer->SetScissor(0, 1, &m_DefaultViewport.rect);

		pCommandBuffer->BeginRenderPass(m_LightingStage.frameBufferHandle->GetPtr(frameIndex), true);

		pCommandBuffer->BindVertexBuffer(0, m_SimpleVertexBuffer.pResource);

		// DirectionalLighting
		pNativePipeline = m_LightingStage.directional.pipelineHandle->GetPtr();
		pCommandBuffer->BindPipeline(pNativePipeline);
		pCommandBuffer->BindDescriptorSet(pNativePipeline, 0, m_LightingStage.directional.descriptorSets[frameIndex]);
		pCommandBuffer->PushConstant(pNativePipeline, 0, &m_LightingStage.directional.constant);
		pCommandBuffer->Draw(4, 1, 0, 0);

		// FinishLighting
		pNativePipeline = m_LightingStage.finish.pipelineHandle->GetPtr();
		pCommandBuffer->NextSubpass();
		pCommandBuffer->BindPipeline(pNativePipeline);
		pCommandBuffer->BindDescriptorSet(pNativePipeline, 0, m_LightingStage.finish.descriptorSets[frameIndex]);
		pCommandBuffer->Draw(4, 1, 0, 0);

		pCommandBuffer->EndRenderPass();

		// ----------------------------------------------------------------------------------------------------
		// Barrier
		// ----------------------------------------------------------------------------------------------------

		GraphicsFactory* pGraphicsFactory = m_DeviceContext->GetGraphicsFactoryPtr();

		V3DPipelineBarrier pipelineBarrier;
		pipelineBarrier.srcStageMask = V3D_PIPELINE_STAGE_FRAGMENT_SHADER;
		pipelineBarrier.dstStageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
		pipelineBarrier.dependencyFlags = 0;

		V3DImageViewMemoryBarrier memoryBarrier;
		memoryBarrier.srcAccessMask = V3D_ACCESS_SHADER_READ;
		memoryBarrier.dstAccessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
		memoryBarrier.srcQueueFamily = V3D_QUEUE_FAMILY_IGNORED;
		memoryBarrier.dstQueueFamily = V3D_QUEUE_FAMILY_IGNORED;
		memoryBarrier.srcLayout = V3D_IMAGE_LAYOUT_SHADER_READ_ONLY;
		memoryBarrier.dstLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
		memoryBarrier.pImageView = pGraphicsFactory->GetNativeAttachmentPtr(GraphicsFactory::AT_BL_HALF_0, frameIndex);

		pCommandBuffer->Barrier(pipelineBarrier, memoryBarrier);
	}

	void Scene::RenderForward(IV3DCommandBuffer* pCommandBuffer, uint32_t frameIndex)
	{
		pCommandBuffer->BeginRenderPass(m_ForwardStage.frameBufferHandle->GetPtr(frameIndex), true);

		// Transparency
		if (m_TransparencyDrawSets.GetCount() > 0)
		{
			TransparencyDrawSet** ppDrawSet = m_TransparencyDrawSets.GetData();
			TransparencyDrawSet** ppDrawSetEnd = ppDrawSet + m_TransparencyDrawSets.GetCount();

			IV3DBuffer* pPrevVertexBuffer = nullptr;
			IV3DPipeline* pPrevPipeline = nullptr;

			const glm::mat4& viewProjMatrix = m_Camera->GetViewProjectionMatrix();

			pCommandBuffer->SetViewport(0, 1, &m_DefaultViewport);
			pCommandBuffer->SetScissor(0, 1, &m_DefaultViewport.rect);

			while (ppDrawSet != ppDrawSetEnd)
			{
				TransparencyDrawSet* pDrawSet = *ppDrawSet;

				if (pPrevVertexBuffer != pDrawSet->pVertexBuffer)
				{
					pCommandBuffer->BindVertexBuffer(0, pDrawSet->pVertexBuffer);
					pCommandBuffer->BindIndexBuffer(pDrawSet->pIndexBuffer, 0, pDrawSet->indexType);

					pPrevVertexBuffer = pDrawSet->pVertexBuffer;
				}

				if (pPrevPipeline != pDrawSet->pPipeline)
				{
					pCommandBuffer->BindPipeline(pDrawSet->pPipeline);
					pCommandBuffer->PushConstant(pDrawSet->pPipeline, 0, &viewProjMatrix);
					pCommandBuffer->PushConstant(pDrawSet->pPipeline, 1, &m_ForwardStage.transparency.constant);

					pPrevPipeline = pDrawSet->pPipeline;
				}

				pCommandBuffer->BindDescriptorSet(pDrawSet->pPipeline, 0, 2, pDrawSet->descriptorSet, 2, pDrawSet->dynamicOffsets);

				pCommandBuffer->DrawIndexed(pDrawSet->indexCount, 1, pDrawSet->firstIndex, 0, 0);

				ppDrawSet++;
			}
		}

		pCommandBuffer->EndRenderPass();
	}

	void Scene::RenderGlare(IV3DCommandBuffer* pCommandBuffer, uint32_t frameIndex)
	{
		// ----------------------------------------------------------------------------------------------------
		// Glare
		// ----------------------------------------------------------------------------------------------------

		if (m_Glare.enable == true)
		{
			IV3DPipeline* pNativePipeline;

			pCommandBuffer->BindVertexBuffer(0, m_SimpleVertexBuffer.pResource);

			// ----------------------------------------------------------------------------------------------------
			// BrightPass
			// ----------------------------------------------------------------------------------------------------

			pCommandBuffer->SetViewport(0, 1, &m_DefaultViewport);
			pCommandBuffer->SetScissor(0, 1, &m_DefaultViewport.rect);

			pCommandBuffer->BeginRenderPass(m_BrightPassStage.frameBufferHandle->GetPtr(frameIndex), true);

			pNativePipeline = m_BrightPassStage.sampling.pipelineHandle->GetPtr();
			pCommandBuffer->BindPipeline(pNativePipeline);
			pCommandBuffer->BindDescriptorSet(pNativePipeline, 0, m_GlareStage.brightPass.descriptorSets[frameIndex]);
			pCommandBuffer->PushConstant(pNativePipeline, 0, &m_GlareStage.brightPass.constant);
			pCommandBuffer->BindVertexBuffer(0, m_SimpleVertexBuffer.pResource);
			pCommandBuffer->Draw(4, 1, 0, 0);

			pCommandBuffer->EndRenderPass();

			// ----------------------------------------------------------------------------------------------------
			// Blur
			// ----------------------------------------------------------------------------------------------------

			const V3DViewport blurViewports[Scene::GlarePageCount] =
			{
				m_HalfViewport,
				m_QuarterViewport,
			};

			pCommandBuffer->BindVertexBuffer(0, m_SimpleVertexBuffer.pResource);

			for (uint32_t i = 0; i < Scene::GlarePageCount; i++)
			{
				const V3DViewport& viewport = blurViewports[i];

				pCommandBuffer->SetViewport(0, 1, &viewport);
				pCommandBuffer->SetScissor(0, 1, &viewport.rect);

				pCommandBuffer->BeginRenderPass(m_BlurStage.frameBufferHandle[i]->GetPtr(frameIndex), true);

				// DownSampling
				pNativePipeline = m_BlurStage.downSampling.pipelineHandle->GetPtr();
				pCommandBuffer->BindPipeline(pNativePipeline);
				pCommandBuffer->BindDescriptorSet(pNativePipeline, 0, m_GlareStage.blur[i].descriptorSets[frameIndex]);
				pCommandBuffer->PushConstant(pNativePipeline, 0, &m_GlareStage.blur[i].constantDS);
				pCommandBuffer->Draw(4, 1, 0, 0);

				// HorizonalBlur
				pNativePipeline = m_BlurStage.horizonal.pipelineHandle->GetPtr();
				pCommandBuffer->NextSubpass();
				pCommandBuffer->BindPipeline(pNativePipeline);
				pCommandBuffer->BindDescriptorSet(pNativePipeline, 0, m_BlurStage.horizonal.descriptorSets[i][frameIndex]);
				pCommandBuffer->PushConstant(pNativePipeline, 0, &m_GlareStage.blur[i].constantH);
				pCommandBuffer->Draw(4, 1, 0, 0);

				// VerticalBlur
				pNativePipeline = m_BlurStage.vertical.pipelineHandle->GetPtr();
				pCommandBuffer->NextSubpass();
				pCommandBuffer->BindPipeline(pNativePipeline);
				pCommandBuffer->BindDescriptorSet(pNativePipeline, 0, m_BlurStage.vertical.descriptorSets[i][frameIndex]);
				pCommandBuffer->PushConstant(pNativePipeline, 0, &m_GlareStage.blur[i].constantV);
				pCommandBuffer->Draw(4, 1, 0, 0);

				pCommandBuffer->EndRenderPass();

				/********/
				/* 合成 */
				/********/

				pCommandBuffer->SetViewport(0, 1, &m_DefaultViewport);
				pCommandBuffer->SetScissor(0, 1, &m_DefaultViewport.rect);

				pCommandBuffer->BeginRenderPass(m_PasteStage.frameBufferHandle->GetPtr(frameIndex), true);

				pNativePipeline = m_PasteStage.sampling.pipelineHandle[0]->GetPtr();
				pCommandBuffer->BindPipeline(pNativePipeline);
				pCommandBuffer->BindDescriptorSet(pNativePipeline, 0, m_GlareStage.paste[i].descriptorSets[frameIndex]);
				pCommandBuffer->PushConstant(pNativePipeline, 0, &m_GlareStage.paste[i].constant);
				pCommandBuffer->Draw(4, 1, 0, 0);

				pCommandBuffer->EndRenderPass();
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// Finish
		// ----------------------------------------------------------------------------------------------------

		GraphicsFactory* pGraphicsFactory = m_DeviceContext->GetGraphicsFactoryPtr();

		V3DPipelineBarrier pipelineBarrier;
		pipelineBarrier.dependencyFlags = 0;

		V3DImageViewMemoryBarrier memoryBarrier;
		memoryBarrier.srcQueueFamily = V3D_QUEUE_FAMILY_IGNORED;
		memoryBarrier.dstQueueFamily = V3D_QUEUE_FAMILY_IGNORED;

		pipelineBarrier.srcStageMask = V3D_PIPELINE_STAGE_FRAGMENT_SHADER;
		pipelineBarrier.dstStageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
		memoryBarrier.srcAccessMask = V3D_ACCESS_SHADER_READ;
		memoryBarrier.dstAccessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
		memoryBarrier.srcLayout = V3D_IMAGE_LAYOUT_SHADER_READ_ONLY;
		memoryBarrier.dstLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;

		memoryBarrier.pImageView = pGraphicsFactory->GetNativeAttachmentPtr(GraphicsFactory::AT_GL_COLOR, frameIndex);
		pCommandBuffer->Barrier(pipelineBarrier, memoryBarrier);

		if (m_Glare.enable == true)
		{
			memoryBarrier.pImageView = pGraphicsFactory->GetNativeAttachmentPtr(GraphicsFactory::AT_BR_COLOR, frameIndex);
			pCommandBuffer->Barrier(pipelineBarrier, memoryBarrier);

			memoryBarrier.pImageView = pGraphicsFactory->GetNativeAttachmentPtr(GraphicsFactory::AT_BL_HALF_0, frameIndex);
			pCommandBuffer->Barrier(pipelineBarrier, memoryBarrier);

			memoryBarrier.pImageView = pGraphicsFactory->GetNativeAttachmentPtr(GraphicsFactory::AT_BL_QUARTER_0, frameIndex);
			pCommandBuffer->Barrier(pipelineBarrier, memoryBarrier);
		}

		// イメージエフェクト用に移行
		pipelineBarrier.srcStageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
		pipelineBarrier.dstStageMask = V3D_PIPELINE_STAGE_FRAGMENT_SHADER;
		memoryBarrier.srcAccessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
		memoryBarrier.dstAccessMask = V3D_ACCESS_SHADER_READ;
		memoryBarrier.srcQueueFamily = V3D_QUEUE_FAMILY_IGNORED;
		memoryBarrier.dstQueueFamily = V3D_QUEUE_FAMILY_IGNORED;
		memoryBarrier.srcLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
		memoryBarrier.dstLayout = V3D_IMAGE_LAYOUT_SHADER_READ_ONLY;
		memoryBarrier.pImageView = pGraphicsFactory->GetNativeAttachmentPtr(GraphicsFactory::AT_SH_COLOR, frameIndex);
		pCommandBuffer->Barrier(pipelineBarrier, memoryBarrier);
	}

	void Scene::RenderImageEffect(IV3DCommandBuffer* pCommandBuffer, uint32_t frameIndex)
	{
		const glm::mat4& viewProjMatrix = m_Camera->GetViewProjectionMatrix();

		IV3DPipeline* pNativePipeline;

		pCommandBuffer->SetViewport(0, 1, &m_DefaultViewport);
		pCommandBuffer->SetScissor(0, 1, &m_DefaultViewport.rect);

		pCommandBuffer->BeginRenderPass(m_ImageEffectStage.frameBufferHandle->GetPtr(frameIndex), true);

		// トーンマッピング ( HDR -> LDR )
		pNativePipeline = m_ImageEffectStage.toneMapping.pipelineHandle[(m_ImageEffect.toneMappingEnable == true)? 0 : 1]->GetPtr();
		pCommandBuffer->BindPipeline(pNativePipeline);
		pCommandBuffer->BindDescriptorSet(pNativePipeline, 0, m_ImageEffectStage.toneMapping.descriptorSets[frameIndex]);
		pCommandBuffer->BindVertexBuffer(0, m_SimpleVertexBuffer.pResource);
		pCommandBuffer->Draw(4, 1, 0, 0);

		// 選択されたメッシュの描画
		pCommandBuffer->NextSubpass();
		if (m_SelectDrawSet.pPipeline != nullptr)
		{
			pCommandBuffer->BindPipeline(m_SelectDrawSet.pPipeline);
			pCommandBuffer->BindDescriptorSet(m_SelectDrawSet.pPipeline, 0, m_SelectDrawSet.pDescriptorSet, 1, &m_SelectDrawSet.dynamicOffset);
			pCommandBuffer->BindVertexBuffer(0, m_SelectDrawSet.pVertexBuffer);
			pCommandBuffer->BindIndexBuffer(m_SelectDrawSet.pIndexBuffer, 0, m_SelectDrawSet.indexType);
			pCommandBuffer->PushConstant(m_SelectDrawSet.pPipeline, 0, &viewProjMatrix);
			pCommandBuffer->DrawIndexed(m_SelectDrawSet.indexCount, 1, m_SelectDrawSet.firstIndex, 0, 0);
		}

		// 選択されたメッシュの輪郭の描画
		pCommandBuffer->NextSubpass();
		if (m_SelectDrawSet.pPipeline != nullptr)
		{
			pNativePipeline = m_ImageEffectStage.selectMapping.pipelineHandle->GetPtr();
			pCommandBuffer->BindPipeline(pNativePipeline);
			pCommandBuffer->BindDescriptorSet(pNativePipeline, 0, m_ImageEffectStage.selectMapping.descriptorSets[frameIndex]);
			pCommandBuffer->BindVertexBuffer(0, m_SimpleVertexBuffer.pResource);
			pCommandBuffer->PushConstant(pNativePipeline, 0, &m_ImageEffectStage.selectMapping.constant);
			pCommandBuffer->Draw(4, 1, 0, 0);
		}

		// デバッグ描画
		pNativePipeline = m_ImageEffectStage.debug.pipelineHandle->GetPtr();
		pCommandBuffer->NextSubpass();
		pCommandBuffer->BindPipeline(pNativePipeline);
		pCommandBuffer->PushConstant(pNativePipeline, 0, &viewProjMatrix);
		m_pDebugRenderer->Draw(pCommandBuffer, frameIndex);

		// アンチエイリアス ( FXAA )
		pNativePipeline = m_ImageEffectStage.fxaa.pipelineHandle[(m_ImageEffect.fxaaEnable == true) ? 0 : 1]->GetPtr();
		pCommandBuffer->NextSubpass();
		pCommandBuffer->BindPipeline(pNativePipeline);
		pCommandBuffer->BindDescriptorSet(pNativePipeline, 0, m_ImageEffectStage.fxaa.descriptorSets[frameIndex]);
		pCommandBuffer->BindVertexBuffer(0, m_SimpleVertexBuffer.pResource);
		if (m_ImageEffect.fxaaEnable == true)
		{
			pCommandBuffer->PushConstant(pNativePipeline, 0, &m_ImageEffectStage.fxaa.constant);
		}
		pCommandBuffer->Draw(4, 1, 0, 0);

		pCommandBuffer->EndRenderPass();
	}

	void Scene::RenderFinish(IV3DCommandBuffer* pCommandBuffer, uint32_t frameIndex)
	{
		V3DPipelineBarrier pipelineBarrier;
		V3DImageViewMemoryBarrier memoryBarrier;

		// ----------------------------------------------------------------------------------------------------
		// バックバッファーにコピー
		// ----------------------------------------------------------------------------------------------------

		IV3DPipeline* pNativePipeline = m_FinishStage.pipelineHandle->GetPtr();

		pCommandBuffer->SetViewport(0, 1, &m_DefaultViewport);
		pCommandBuffer->SetScissor(0, 1, &m_DefaultViewport.rect);

		pCommandBuffer->BeginRenderPass(m_FinishStage.frameBufferHandle->GetPtr(frameIndex), true);

		pCommandBuffer->BindVertexBuffer(0, m_SimpleVertexBuffer.pResource);

		pCommandBuffer->BindPipeline(pNativePipeline);
		pCommandBuffer->BindDescriptorSet(pNativePipeline, 0, m_FinishStage.descriptorSets[frameIndex]);
		pCommandBuffer->Draw(4, 1, 0, 0);

		pCommandBuffer->EndRenderPass();

		// ----------------------------------------------------------------------------------------------------
		// セレクトバッファーを更新
		// ----------------------------------------------------------------------------------------------------

		IV3DImageView* pSelectImageView = m_DeviceContext->GetGraphicsFactoryPtr()->GetNativeAttachmentPtr(GraphicsFactory::AT_GE_SELECT, frameIndex);

		IV3DImage* pSelectImage;
		pSelectImageView->GetImage(&pSelectImage);

		const V3DImageDesc& selectImagheDesc = pSelectImage->GetDesc();

		pipelineBarrier.dependencyFlags = 0;

		memoryBarrier.srcQueueFamily = V3D_QUEUE_FAMILY_IGNORED;
		memoryBarrier.dstQueueFamily = V3D_QUEUE_FAMILY_IGNORED;
		memoryBarrier.pImageView = pSelectImageView;

		pipelineBarrier.srcStageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
		pipelineBarrier.dstStageMask = V3D_PIPELINE_STAGE_TRANSFER;
		memoryBarrier.srcAccessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
		memoryBarrier.dstAccessMask = V3D_ACCESS_TRANSFER_READ;
		memoryBarrier.srcLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
		memoryBarrier.dstLayout = V3D_IMAGE_LAYOUT_TRANSFER_SRC;
		pCommandBuffer->Barrier(pipelineBarrier, memoryBarrier);

		V3DSize3D copySize = { selectImagheDesc.width, selectImagheDesc.height, selectImagheDesc.depth };
		pCommandBuffer->CopyImageToBuffer(m_SelectBuffer.pResource, 0, pSelectImage, V3D_IMAGE_LAYOUT_TRANSFER_SRC, 0, frameIndex, 1, V3DPoint3D{}, copySize);

		pipelineBarrier.srcStageMask = V3D_PIPELINE_STAGE_TRANSFER;
		pipelineBarrier.dstStageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
		memoryBarrier.srcAccessMask = V3D_ACCESS_TRANSFER_READ;
		memoryBarrier.dstAccessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
		memoryBarrier.srcLayout = V3D_IMAGE_LAYOUT_TRANSFER_SRC;
		memoryBarrier.dstLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
		pCommandBuffer->Barrier(pipelineBarrier, memoryBarrier);

		pSelectImage->Release();

		// ----------------------------------------------------------------------------------------------------
		// 最後のバリア
		// ----------------------------------------------------------------------------------------------------

		GraphicsFactory* pGraphicsFactory = m_DeviceContext->GetGraphicsFactoryPtr();

		pipelineBarrier.srcStageMask = V3D_PIPELINE_STAGE_FRAGMENT_SHADER;
		pipelineBarrier.dstStageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
		pipelineBarrier.dependencyFlags = 0;

		memoryBarrier.srcAccessMask = V3D_ACCESS_SHADER_READ;
		memoryBarrier.dstAccessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
		memoryBarrier.srcQueueFamily = V3D_QUEUE_FAMILY_IGNORED;
		memoryBarrier.dstQueueFamily = V3D_QUEUE_FAMILY_IGNORED;
		memoryBarrier.srcLayout = V3D_IMAGE_LAYOUT_SHADER_READ_ONLY;
		memoryBarrier.dstLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;

		memoryBarrier.pImageView = pGraphicsFactory->GetNativeAttachmentPtr(GraphicsFactory::AT_GE_COLOR, frameIndex);
		pCommandBuffer->Barrier(pipelineBarrier, memoryBarrier);

		memoryBarrier.pImageView = pGraphicsFactory->GetNativeAttachmentPtr(GraphicsFactory::AT_GE_BUFFER_0, frameIndex);
		pCommandBuffer->Barrier(pipelineBarrier, memoryBarrier);

		memoryBarrier.pImageView = pGraphicsFactory->GetNativeAttachmentPtr(GraphicsFactory::AT_GE_BUFFER_1, frameIndex);
		pCommandBuffer->Barrier(pipelineBarrier, memoryBarrier);

		memoryBarrier.pImageView = pGraphicsFactory->GetNativeAttachmentPtr(GraphicsFactory::AT_SS_COLOR, frameIndex);
		pCommandBuffer->Barrier(pipelineBarrier, memoryBarrier);

		memoryBarrier.pImageView = pGraphicsFactory->GetNativeAttachmentPtr(GraphicsFactory::AT_SA_BUFFER, frameIndex);
		pCommandBuffer->Barrier(pipelineBarrier, memoryBarrier);

		memoryBarrier.pImageView = pGraphicsFactory->GetNativeAttachmentPtr(GraphicsFactory::AT_SH_COLOR, frameIndex);
		pCommandBuffer->Barrier(pipelineBarrier, memoryBarrier);

		memoryBarrier.pImageView = pGraphicsFactory->GetNativeAttachmentPtr(GraphicsFactory::AT_IE_LDR_COLOR_1, frameIndex);
		pCommandBuffer->Barrier(pipelineBarrier, memoryBarrier);
	}

	void Scene::SortOpacityDrawSet(OpacityDrawSet** list, int64_t first, int64_t last)
	{
		const OpacityDrawSet* pDrawSet = list[(first + last) >> 1];

		uint64_t sortKey = pDrawSet->sortKey;

		int64_t i = first;
		int64_t j = last;

		do
		{
			while (list[i]->sortKey < sortKey) { i++; }
			while (list[j]->sortKey > sortKey) { j--; }

			if (i <= j)
			{
				VE_ASSERT((i <= last) && (j >= first));

				OpacityDrawSet* pTemp = list[i];

				list[i] = list[j];
				list[j] = pTemp;

				i++;
				j--;
			}

		} while (i <= j);

		if (first < j) { SortOpacityDrawSet(list, first, j); }
		if (i < last) { SortOpacityDrawSet(list, i, last); }
	}

	void Scene::SortTransparencyDrawSet(TransparencyDrawSet** list, int64_t first, int64_t last)
	{
		const TransparencyDrawSet* pDrawSet = list[(first + last) >> 1];

		float sortKey = pDrawSet->sortKey;

		int64_t i = first;
		int64_t j = last;

		do
		{
			while (list[i]->sortKey > sortKey) { i++; }
			while (list[j]->sortKey < sortKey) { j--; }

			if (i <= j)
			{
				VE_ASSERT((i <= last) && (j >= first));

				TransparencyDrawSet* pTemp = list[i];

				list[i] = list[j];
				list[j] = pTemp;

				i++;
				j--;
			}

		} while (i <= j);

		if (first < j) { SortTransparencyDrawSet(list, first, j); }
		if (i < last) { SortTransparencyDrawSet(list, i, last); }
	}

}