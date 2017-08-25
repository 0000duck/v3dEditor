#include "GraphicsFactory.h"
#include "DeviceContext.h"
#include "ResourceMemoryManager.h"
#include "ImmediateContext.h"
#include "DeletingQueue.h"
#include "shaderCompiler\shaderCompiler.h"

// ----------------------------------------------------------------------------------------------------
// リソース
// ----------------------------------------------------------------------------------------------------

static constexpr uint8_t VE_GF_Scene_Simple_Vert[] =
{
#include "resource\simple_vert_spv.inc"
};

static constexpr uint8_t VE_GF_Scene_Simple_Frag[] =
{
#include "resource\simple_frag_spv.inc"
};

static constexpr uint8_t VE_GF_Scene_Paste_Frag[] =
{
#include "resource\paste_frag_spv.inc"
};

static constexpr uint8_t VE_GF_Scene_BrightPass_Frag[] =
{
#include "resource\brightPass_frag_spv.inc"
};

static constexpr uint8_t VE_GF_Scene_DownSampling2x2_Frag[] =
{
#include "resource\downSampling2x2_frag_spv.inc"
};

static constexpr uint8_t VE_GF_Scene_GaussianBlur_Frag[] =
{
#include "resource\gaussianBlur_frag_spv.inc"
};

static constexpr uint8_t VE_GF_Scene_Grid_Vert[] =
{
#include "resource\grid_vert_spv.inc"
};

static constexpr uint8_t VE_GF_Scene_Grid_Frag[] =
{
#include "resource\grid_frag_spv.inc"
};

static constexpr uint8_t VE_GF_Scene_Ssao_Frag[] =
{
#include "resource\ssao_frag_spv.inc"
};

static constexpr uint8_t VE_GF_Scene_DirectionalLighting_O_Frag[] =
{
#include "resource\directionalLighting_o_frag_spv.inc"
};

static constexpr uint8_t VE_GF_Scene_FinishLighting_O_Frag[] =
{
#include "resource\finishLighting_o_frag_spv.inc"
};

static constexpr uint8_t VE_GF_Scene_SelectMapping_Frag[] =
{
#include "resource\selectMapping_frag_spv.inc"
};

static constexpr uint8_t VE_GF_Scene_ToneMapping_Frag[] =
{
#include "resource\toneMapping_frag_spv.inc"
};

static constexpr uint8_t VE_GF_Scene_Fxaa_Frag[] =
{
#include "resource\fxaa_frag_spv.inc"
};

static constexpr uint8_t VE_GF_Scene_Debug_Vert[] =
{
#include "resource\debug_vert_spv.inc"
};

static constexpr uint8_t VE_GF_Scene_Debug_Frag[] =
{
#include "resource\debug_frag_spv.inc"
};

static constexpr char VE_GF_Scene_Mesh_Vert[] =
{
#include "resource\mesh_vert.inc"
};

static constexpr char VE_GF_Scene_Mesh_Frag[] =
{
#include "resource\mesh_frag.inc"
};

static constexpr char VE_GF_Scene_MeshShadow_Vert[] =
{
#include "resource\meshShadow_vert.inc"
};

static constexpr char VE_GF_Scene_MeshShadow_Frag[] =
{
#include "resource\meshShadow_frag.inc"
};

static constexpr char VE_GF_Scene_MeshSelect_Vert[] =
{
#include "resource\meshSelect_vert.inc"
};

static constexpr char VE_GF_Scene_MeshSelect_Frag[] =
{
#include "resource\meshSelect_frag.inc"
};

static constexpr uint8_t VE_GF_GUI_Vert[] =
{
#include "resource\gui_vert_spv.inc"
};

static constexpr uint8_t VE_GF_GUI_Frag[] =
{
#include "resource\gui_frag_spv.inc"
};

namespace ve {

	/****************************/
	/* public - GraphicsFactory */
	/****************************/

	glm::vec4 GraphicsFactory::GetBackgroundColor()
	{
		const V3DClearValue& clearValue = m_Stages[GraphicsFactory::ST_SCENE_GEOMETRY].pNativeRenderPass->GetAttachmentClearValue(0);
		return glm::vec4(clearValue.color.float32[0], clearValue.color.float32[1], clearValue.color.float32[2], clearValue.color.float32[3]);
	}

	void GraphicsFactory::SetBackgroundColor(const glm::vec4& color)
	{
		V3DClearValue clearValue;
		clearValue.color.float32[0] = color.r;
		clearValue.color.float32[1] = color.g;
		clearValue.color.float32[2] = color.b;
		clearValue.color.float32[3] = color.a;
		m_Stages[GraphicsFactory::ST_SCENE_GEOMETRY].pNativeRenderPass->SetAttachmentClearValue(0, clearValue);

		V3DAttachmentDesc& attachment = m_Stages[GraphicsFactory::ST_SCENE_GEOMETRY].renderPassDesc.pAttachments[0];
		attachment.clearValue.color.float32[0] = color.r;
		attachment.clearValue.color.float32[1] = color.g;
		attachment.clearValue.color.float32[2] = color.b;
		attachment.clearValue.color.float32[3] = color.a;
	}

	const V3DViewport& GraphicsFactory::GetNativeViewport(GraphicsFactory::VIEWPORT_TYPE type) const
	{
		return m_Viewports[type];
	}

	uint32_t GraphicsFactory::GetShadowResolution() const
	{
		return m_ShadowResolution;
	}

	void GraphicsFactory::SetShadowResolution(uint32_t resolution)
	{
		m_ShadowResolution = resolution;

		V3DViewport& viewport = m_Viewports[GraphicsFactory::VT_SHADOW];
		viewport.rect.x = 0;
		viewport.rect.y = 0;
		viewport.rect.width = m_ShadowResolution;
		viewport.rect.height = m_ShadowResolution;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		m_ShadowTexelSize.x = 1.0f / static_cast<float>(m_ShadowResolution);
		m_ShadowTexelSize.y = m_ShadowTexelSize.x;

		m_Attachments[GraphicsFactory::AT_SA_BUFFER].imageDesc.width = m_ShadowResolution;
		m_Attachments[GraphicsFactory::AT_SA_BUFFER].imageDesc.height = m_ShadowResolution;

		m_Attachments[GraphicsFactory::AT_SA_DEPTH].imageDesc.width = m_ShadowResolution;
		m_Attachments[GraphicsFactory::AT_SA_DEPTH].imageDesc.height = m_ShadowResolution;
	}

	const glm::vec2& GraphicsFactory::GetShadowTexelSize() const
	{
		return m_ShadowTexelSize;
	}

	IV3DImageView* GraphicsFactory::GetNativeAttachmentPtr(GraphicsFactory::ATTACHMENT_TYPE type, uint32_t frameIndex)
	{
		return m_Attachments[type].imageViews[frameIndex];
	}

	const V3DImageDesc& GraphicsFactory::GetNativeAttachmentDesc(GraphicsFactory::ATTACHMENT_TYPE type) const
	{
		IV3DImage* pImage;

		m_Attachments[type].imageViews[0]->GetImage(&pImage);

		const V3DImageDesc& desc = pImage->GetDesc();
		pImage->Release();

		return desc;
	}

	IV3DDescriptorSet* GraphicsFactory::CreateNativeDescriptorSet(GraphicsFactory::DESCRIPTOR_SET_TYPE type)
	{
		IV3DDescriptorSet* pNativeDescriptorSet;

		if (m_pDeviceContext->GetNativeDevicePtr()->CreateDescriptorSet(
			m_DescriptorSets[type].pNativeLayout,
			&pNativeDescriptorSet,
			VE_INTERFACE_DEBUG_NAME(m_DescriptorSets[type].debugName.c_str())) != V3D_OK)
		{
			return nullptr;
		}

		return pNativeDescriptorSet;
	}

	IV3DDescriptorSet* GraphicsFactory::CreateNativeDescriptorSet(GraphicsFactory::STAGE_TYPE type, GraphicsFactory::STAGE_SUBPASS_TYPE subpassType, uint32_t descriptorSetLayoutIndex)
	{
		GraphicsFactory::StageSubpass& subpass = m_Stages[type].subpasses[subpassType];

		IV3DPipelineLayout* pPipelineLayout;
		subpass.pipelineHandle->m_pPipeline->GetLayout(&pPipelineLayout);

		IV3DDescriptorSetLayout* pDescriptorSetLayout;
		pPipelineLayout->GetDescriptorSetLayout(descriptorSetLayoutIndex, &pDescriptorSetLayout);

		IV3DDescriptorSet* pNativeDescriptorSet;

		if (m_pDeviceContext->GetNativeDevicePtr()->CreateDescriptorSet(
			pDescriptorSetLayout,
			&pNativeDescriptorSet,
			VE_INTERFACE_DEBUG_NAME(subpass.debugName.c_str())) != V3D_OK)
		{
			pDescriptorSetLayout->Release();
			pPipelineLayout->Release();
			return nullptr;
		}

		pDescriptorSetLayout->Release();
		pPipelineLayout->Release();

		return pNativeDescriptorSet;
	}

	IV3DDescriptorSet* GraphicsFactory::CreateNativeDescriptorSet(MATERIAL_PIPELINE_TYPE type, uint32_t shaderFlags)
	{
		LockGuard<Mutex> lock(m_Mutex);

		uint32_t pipelineFlags = 0;

		switch (type)
		{
		case GraphicsFactory::MPT_COLOR:
			pipelineFlags = shaderFlags & MATERIAL_SHADER_PIPELINE_MASK;
			break;
		case GraphicsFactory::MPT_SHADOW:
			pipelineFlags = shaderFlags & MATERIAL_SHADER_SHADOW_PIPELINE_MASK;
			if ((pipelineFlags & MATERIAL_SHADER_SHADOW) == 0)
			{
				pipelineFlags |= MATERIAL_SHADER_SHADOW;
			}
			break;
		}

		auto it = m_MaterialPipelineMap.find(pipelineFlags);
		VE_ASSERT(it != m_MaterialPipelineMap.end());

		IV3DDescriptorSetLayout* pNativeDescriptorSetLayout;
		it->second.pNativeLayout->GetDescriptorSetLayout(1, &pNativeDescriptorSetLayout);

		IV3DDescriptorSet* pNativeDescriptorSet;
		if (m_pDeviceContext->GetNativeDevicePtr()->CreateDescriptorSet(pNativeDescriptorSetLayout, &pNativeDescriptorSet, VE_INTERFACE_DEBUG_NAME(it->second.debugName.c_str())) != V3D_OK)
		{
			pNativeDescriptorSetLayout->Release();
			return false;
		}

		pNativeDescriptorSetLayout->Release();

		return pNativeDescriptorSet;
	}

	FrameBufferHandlePtr GraphicsFactory::GetFrameBufferHandle(GraphicsFactory::STAGE_TYPE type, uint32_t index)
	{
		return m_Stages[type].frameBuffers[index].handle;
	}

	PipelineHandlePtr GraphicsFactory::GetPipelineHandle(GraphicsFactory::STAGE_TYPE type, GraphicsFactory::STAGE_SUBPASS_TYPE subpassType)
	{
		return m_Stages[type].subpasses[subpassType].pipelineHandle;
	}

	PipelineHandlePtr GraphicsFactory::GetPipelineHandle(MATERIAL_PIPELINE_TYPE type, uint32_t shaderFlags, size_t boneCount, uint32_t vertexStride, V3D_POLYGON_MODE polygonMode, V3D_CULL_MODE cullMode, BLEND_MODE blendMode)
	{
		LockGuard<Mutex> lock(m_Mutex);

		GraphicsFactory::MaterialSet* pMaterialSet;

		if (CreateMaterialSet(&pMaterialSet, shaderFlags, true, boneCount, vertexStride, polygonMode, cullMode, blendMode) == false)
		{
			return nullptr;
		}

		return pMaterialSet->pipelines[type].handle;
	}

	/*****************************/
	/* private - GraphicsFactory */
	/*****************************/

	GraphicsFactory* GraphicsFactory::Create(DeviceContext* pDeviceContext)
	{
		GraphicsFactory* pGraphicsFactory = VE_NEW_T(GraphicsFactory, pDeviceContext);
		if (pGraphicsFactory == nullptr)
		{
			return nullptr;
		}

		if (pGraphicsFactory->Initialize() == false)
		{
			VE_DELETE_T(pGraphicsFactory, GraphicsFactory);
			return nullptr;
		}

		return pGraphicsFactory;
	}

	void GraphicsFactory::Destroy()
	{
		VE_DELETE_THIS_T(this, GraphicsFactory);
	}

	GraphicsFactory::GraphicsFactory(DeviceContext* pDeviceContext) :
		m_pDeviceContext(nullptr),
		m_ShaderModules({}),
		m_DescriptorSets({}),
		m_Pipelines({}),
		m_Attachments({})
	{
		VE_ASSERT(pDeviceContext != nullptr);

		m_pDeviceContext = pDeviceContext;

		m_ShadowResolution = 2048;

		V3DViewport& shadowViewport = m_Viewports[GraphicsFactory::VT_SHADOW];
		shadowViewport.rect.x = 0;
		shadowViewport.rect.y = 0;
		shadowViewport.rect.width = m_ShadowResolution;
		shadowViewport.rect.height = m_ShadowResolution;
		shadowViewport.minDepth = 0.0f;
		shadowViewport.maxDepth = 1.0f;

		m_ShadowTexelSize.x = 1.0f / static_cast<float>(m_ShadowResolution);
		m_ShadowTexelSize.y = m_ShadowTexelSize.x;
	}

	GraphicsFactory::~GraphicsFactory()
	{
		DeletingQueue* pDeletingQueue = m_pDeviceContext->GetDeletingQueuePtr();

		if (m_MaterialSetMap.empty() == false)
		{
			auto it_begin = m_MaterialSetMap.begin();
			auto it_end = m_MaterialSetMap.end();

			for (auto it = it_begin; it != it_end; ++it)
			{
				for (auto i = 0; i < GraphicsFactory::MPT_MAX; i++)
				{
					GraphicsFactory::MaterialPipeline& pipeline = it->second.pipelines[i];

					DeleteDeviceChild(pDeletingQueue, &pipeline.handle->m_pPipeline);
					DeleteDeviceChild(pDeletingQueue, &pipeline.desc.vertexShader.pModule);
					DeleteDeviceChild(pDeletingQueue, &pipeline.desc.tessellationControlShader.pModule);
					DeleteDeviceChild(pDeletingQueue, &pipeline.desc.tessellationEvaluationShader.pModule);
					DeleteDeviceChild(pDeletingQueue, &pipeline.desc.geometryShader.pModule);
					DeleteDeviceChild(pDeletingQueue, &pipeline.desc.fragmentShader.pModule);
				}
			}
		}

		if (m_MaterialPipelineMap.empty() == false)
		{
			auto it_begin = m_MaterialPipelineMap.begin();
			auto it_end = m_MaterialPipelineMap.end();

			for (auto it = it_begin; it != it_end; ++it)
			{
				DeleteDeviceChild(pDeletingQueue, &it->second.pNativeLayout);
			}
		}

		LostStages(pDeletingQueue);
		LostAttachments(pDeletingQueue, true);

		for (GraphicsFactory::Pipeline& pipeline : m_Pipelines)
		{
			DeleteDeviceChild(pDeletingQueue, &pipeline.pNativeLayout);
		}

		for (GraphicsFactory::DescriptorSet& descriptorSet : m_DescriptorSets)
		{
			DeleteDeviceChild(pDeletingQueue, &descriptorSet.pNativeLayout);
		}

		for (auto i = 0; i < GraphicsFactory::SMT_MAX; i++)
		{
			DeleteDeviceChild(pDeletingQueue, &m_ShaderModules[i]);
		}
	}

	bool GraphicsFactory::Initialize()
	{
		if (InitializeBase() == false)
		{
			return false;
		}

		if (InitializeAttachments() == false)
		{
			return false;
		}

		if (InitializeStages() == false)
		{
			return false;
		}

		if (Restore() == false)
		{
			return false;
		}

		return true;
	}

	bool GraphicsFactory::InitializeBase()
	{
		IV3DDevice* pNativeDevice = m_pDeviceContext->GetNativeDevicePtr();
		ResourceMemoryManager* pResourceMemoryManager = m_pDeviceContext->GetResourceMemoryManagerPtr();

		// ----------------------------------------------------------------------------------------------------
		// シェーダーモジュール
		// ----------------------------------------------------------------------------------------------------

		const size_t shaderModuleSizes[] =
		{
			sizeof(VE_GF_Scene_Simple_Vert),
			sizeof(VE_GF_Scene_Simple_Frag),
			sizeof(VE_GF_Scene_Paste_Frag),
			sizeof(VE_GF_Scene_BrightPass_Frag),
			sizeof(VE_GF_Scene_DownSampling2x2_Frag),
			sizeof(VE_GF_Scene_GaussianBlur_Frag),
			sizeof(VE_GF_Scene_Grid_Vert),
			sizeof(VE_GF_Scene_Grid_Frag),
			sizeof(VE_GF_Scene_Ssao_Frag),
			sizeof(VE_GF_Scene_DirectionalLighting_O_Frag),
			sizeof(VE_GF_Scene_FinishLighting_O_Frag),
			sizeof(VE_GF_Scene_SelectMapping_Frag),
			sizeof(VE_GF_Scene_ToneMapping_Frag),
			sizeof(VE_GF_Scene_Fxaa_Frag),
			sizeof(VE_GF_Scene_Debug_Vert),
			sizeof(VE_GF_Scene_Debug_Frag),
			sizeof(VE_GF_GUI_Vert),
			sizeof(VE_GF_GUI_Frag),
		};

		const uint8_t* shaderModuleBinaries[] =
		{
			VE_GF_Scene_Simple_Vert,
			VE_GF_Scene_Simple_Frag,
			VE_GF_Scene_Paste_Frag,
			VE_GF_Scene_BrightPass_Frag,
			VE_GF_Scene_DownSampling2x2_Frag,
			VE_GF_Scene_GaussianBlur_Frag,
			VE_GF_Scene_Grid_Vert,
			VE_GF_Scene_Grid_Frag,
			VE_GF_Scene_Ssao_Frag,
			VE_GF_Scene_DirectionalLighting_O_Frag,
			VE_GF_Scene_FinishLighting_O_Frag,
			VE_GF_Scene_SelectMapping_Frag,
			VE_GF_Scene_ToneMapping_Frag,
			VE_GF_Scene_Fxaa_Frag,
			VE_GF_Scene_Debug_Vert,
			VE_GF_Scene_Debug_Frag,
			VE_GF_GUI_Vert,
			VE_GF_GUI_Frag,
		};

#ifdef _DEBUG
		const wchar_t* shaderModuleNames[] =
		{
			L"Scene_Simple_Vert",
			L"Scene_Simple_Frag",
			L"Scene_Paste_Frag",
			L"Scene_BrightPass_Frag",
			L"Scene_DownSampling2x2_Frag",
			L"Scene_GaussianBlur_Frag",
			L"Scene_Grid_Vert",
			L"Scene_Grid_Frag",
			L"Scene_Ssao_Frag",
			L"Scene_DirectionalLighting_O_Frag",
			L"Scene_SelectMapping_Frag",
			L"Scene_FinishLighting_O_Frag",
			L"Scene_ToneMapping_Frag",
			L"Scene_Fxaa_Frag",
			L"Scene_Debug_Vert",
			L"Scene_Debug_Frag",
			L"Scene_Gui_Vert",
			L"Scene_Gui_Frag",
		};
#endif //_DEBUG

		for (auto i = 0; i < _countof(shaderModuleBinaries); i++)
		{
			if (pNativeDevice->CreateShaderModule(shaderModuleSizes[i], shaderModuleBinaries[i], &m_ShaderModules[i], VE_INTERFACE_DEBUG_NAME(shaderModuleNames[i])) != V3D_OK)
			{
				return false;
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// デスクリプタセットレイアウト
		// ----------------------------------------------------------------------------------------------------

		/************/
		/* シンプル */
		/************/

		{
			V3DDescriptorDesc descriptors[1];
			descriptors[0].binding = 0;
			descriptors[0].type = V3D_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptors[0].stageFlags = V3D_SHADER_STAGE_FRAGMENT;

			GraphicsFactory::DescriptorSet& descriptorSet = m_DescriptorSets[GraphicsFactory::DST_SIMPLE];
			VE_DEBUG_CODE(descriptorSet.debugName = L"Scene_Simple");

			if (pNativeDevice->CreateDescriptorSetLayout(
				_countof(descriptors), descriptors,	2, 1,
				&descriptorSet.pNativeLayout,
				VE_INTERFACE_DEBUG_NAME(descriptorSet.debugName.c_str())) != V3D_OK)
			{
				return false;
			}
		}

		/************/
		/* メッシュ */
		/************/

		{
			V3DDescriptorDesc descriptors[1];
			descriptors[0].binding = 0;
			descriptors[0].type = V3D_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			descriptors[0].stageFlags = V3D_SHADER_STAGE_VERTEX | V3D_SHADER_STAGE_FRAGMENT;

			GraphicsFactory::DescriptorSet& descriptorSet = m_DescriptorSets[GraphicsFactory::DST_MESH];
			VE_DEBUG_CODE(descriptorSet.debugName = L"Scene_Mesh");

			if (pNativeDevice->CreateDescriptorSetLayout(
				_countof(descriptors), descriptors, 256, 256,
				&descriptorSet.pNativeLayout,
				VE_INTERFACE_DEBUG_NAME(descriptorSet.debugName.c_str())) != V3D_OK)
			{
				return false;
			}
		}

		/***********************/
		/* メッシュ - シャドウ */
		/***********************/

		{
			V3DDescriptorDesc descriptors[1];
			descriptors[0].binding = 0;
			descriptors[0].type = V3D_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			descriptors[0].stageFlags = V3D_SHADER_STAGE_VERTEX;

			GraphicsFactory::DescriptorSet& descriptorSet = m_DescriptorSets[GraphicsFactory::DST_MESH_SHADOW];
			VE_DEBUG_CODE(descriptorSet.debugName = L"Scene_MeshShadow");

			if (pNativeDevice->CreateDescriptorSetLayout(
				_countof(descriptors), descriptors, 256, 256,
				&descriptorSet.pNativeLayout,
				VE_INTERFACE_DEBUG_NAME(descriptorSet.debugName.c_str())) != V3D_OK)
			{
				return false;
			}
		}

		/************************************************/
		/* スクリーンスペースアンビエントオクルージョン */
		/************************************************/

		{
			V3DDescriptorDesc descriptors[3];
			descriptors[0].binding = 0;
			descriptors[0].type = V3D_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptors[0].stageFlags = V3D_SHADER_STAGE_FRAGMENT;
			descriptors[1].binding = 1;
			descriptors[1].type = V3D_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptors[1].stageFlags = V3D_SHADER_STAGE_FRAGMENT;
			descriptors[2].binding = 2;
			descriptors[2].type = V3D_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptors[2].stageFlags = V3D_SHADER_STAGE_FRAGMENT;

			GraphicsFactory::DescriptorSet& descriptorSet = m_DescriptorSets[GraphicsFactory::DST_SSAO];
			VE_DEBUG_CODE(descriptorSet.debugName = L"Scene_Ssao");

			if (pNativeDevice->CreateDescriptorSetLayout(
				_countof(descriptors), descriptors, 1, 1,
				&descriptorSet.pNativeLayout,
				VE_INTERFACE_DEBUG_NAME(descriptorSet.debugName.c_str())) != V3D_OK)
			{
				return false;
			}
		}

		/********************************/
		/* ディレクショナルライティング */
		/********************************/

		{
			V3DDescriptorDesc descriptors[5];
			descriptors[0].binding = 0;
			descriptors[0].type = V3D_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptors[0].stageFlags = V3D_SHADER_STAGE_FRAGMENT;
			descriptors[1].binding = 1;
			descriptors[1].type = V3D_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptors[1].stageFlags = V3D_SHADER_STAGE_FRAGMENT;
			descriptors[2].binding = 2;
			descriptors[2].type = V3D_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptors[2].stageFlags = V3D_SHADER_STAGE_FRAGMENT;
			descriptors[3].binding = 3;
			descriptors[3].type = V3D_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptors[3].stageFlags = V3D_SHADER_STAGE_FRAGMENT;
			descriptors[4].binding = 4;
			descriptors[4].type = V3D_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptors[4].stageFlags = V3D_SHADER_STAGE_FRAGMENT;

			GraphicsFactory::DescriptorSet& descriptorSet = m_DescriptorSets[GraphicsFactory::DST_DIRECTIONAL_LIGHTING_O];
			VE_DEBUG_CODE(descriptorSet.debugName = L"Scene_DirectionalLighting_O");

			if (pNativeDevice->CreateDescriptorSetLayout(
				_countof(descriptors), descriptors, 1, 1,
				&descriptorSet.pNativeLayout,
				VE_INTERFACE_DEBUG_NAME(descriptorSet.debugName.c_str())) != V3D_OK)
			{
				return false;
			}
		}

		/****************************/
		/* フィニッシュライティング */
		/****************************/

		{
			V3DDescriptorDesc descriptors[3];
			descriptors[0].binding = 0;
			descriptors[0].type = V3D_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptors[0].stageFlags = V3D_SHADER_STAGE_FRAGMENT;
			descriptors[1].binding = 1;
			descriptors[1].type = V3D_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptors[1].stageFlags = V3D_SHADER_STAGE_FRAGMENT;
			descriptors[2].binding = 2;
			descriptors[2].type = V3D_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptors[2].stageFlags = V3D_SHADER_STAGE_FRAGMENT;

			GraphicsFactory::DescriptorSet& descriptorSet = m_DescriptorSets[GraphicsFactory::DST_FINISH_LIGHTING];
			VE_DEBUG_CODE(descriptorSet.debugName = L"Scene_FinishLighting");

			if (pNativeDevice->CreateDescriptorSetLayout(
				_countof(descriptors), descriptors, 1, 1,
				&descriptorSet.pNativeLayout,
				VE_INTERFACE_DEBUG_NAME(descriptorSet.debugName.c_str())) != V3D_OK)
			{
				return false;
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// パイプラインレイアウト
		// ----------------------------------------------------------------------------------------------------

		/**********/
		/* Simple */
		/**********/

		{
			GraphicsFactory::Pipeline& pipeline = m_Pipelines[GraphicsFactory::PT_SIMPLE];
			VE_DEBUG_CODE(pipeline.debugName = L"Scene_Simple");

			if (pNativeDevice->CreatePipelineLayout(
				0, nullptr,
				1, &m_DescriptorSets[GraphicsFactory::DST_SIMPLE].pNativeLayout,
				&pipeline.pNativeLayout,
				VE_INTERFACE_DEBUG_NAME(pipeline.debugName.c_str())) != V3D_OK)
			{
				return false;
			}
		}

		/*********/
		/* Paste */
		/*********/

		{
			V3DConstantDesc constants[1];
			constants[0].shaderStageFlags = V3D_SHADER_STAGE_FRAGMENT;
			constants[0].offset = 0;
			constants[0].size = sizeof(PasteConstant);

			GraphicsFactory::Pipeline& pipeline = m_Pipelines[GraphicsFactory::PT_PASTE];
			VE_DEBUG_CODE(pipeline.debugName = L"Scene_Paste");

			if (pNativeDevice->CreatePipelineLayout(
				_countof(constants), constants,
				1, &m_DescriptorSets[GraphicsFactory::DST_SIMPLE].pNativeLayout,
				&pipeline.pNativeLayout,
				VE_INTERFACE_DEBUG_NAME(pipeline.debugName.c_str())) != V3D_OK)
			{
				return false;
			}
		}

		/**************/
		/* BrightPass */
		/**************/

		{
			V3DConstantDesc constants[1];
			constants[0].shaderStageFlags = V3D_SHADER_STAGE_FRAGMENT;
			constants[0].offset = 0;
			constants[0].size = sizeof(BrightPassConstant);

			GraphicsFactory::Pipeline& pipeline = m_Pipelines[GraphicsFactory::PT_BRIGHT_PASS];
			VE_DEBUG_CODE(pipeline.debugName = L"Scene_BrightPass");

			if (pNativeDevice->CreatePipelineLayout(
				_countof(constants), constants,
				1, &m_DescriptorSets[GraphicsFactory::DST_SIMPLE].pNativeLayout,
				&pipeline.pNativeLayout,
				VE_INTERFACE_DEBUG_NAME(pipeline.debugName.c_str())) != V3D_OK)
			{
				return false;
			}
		}

		/****************/
		/* DownSampling */
		/****************/

		{
			V3DConstantDesc constants[1];
			constants[0].shaderStageFlags = V3D_SHADER_STAGE_FRAGMENT;
			constants[0].offset = 0;
			constants[0].size = sizeof(DownSamplingConstant);

			GraphicsFactory::Pipeline& pipeline = m_Pipelines[GraphicsFactory::PT_DOWN_SAMPLING];
			VE_DEBUG_CODE(pipeline.debugName = L"Scene_DownSampling");

			if (pNativeDevice->CreatePipelineLayout(
				_countof(constants), constants,
				1, &m_DescriptorSets[GraphicsFactory::DST_SIMPLE].pNativeLayout,
				&pipeline.pNativeLayout,
				VE_INTERFACE_DEBUG_NAME(pipeline.debugName.c_str())) != V3D_OK)
			{
				return false;
			}
		}

		/****************/
		/* GaussianBlur */
		/****************/

		{
			V3DConstantDesc constants[1];
			constants[0].shaderStageFlags = V3D_SHADER_STAGE_FRAGMENT;
			constants[0].offset = 0;
			constants[0].size = sizeof(GaussianBlurConstant);

			GraphicsFactory::Pipeline& pipeline = m_Pipelines[GraphicsFactory::PT_GAUSSIAN_BLUR];
			VE_DEBUG_CODE(pipeline.debugName = L"Scene_GaussianBlur");

			if (pNativeDevice->CreatePipelineLayout(
				_countof(constants), constants,
				1, &m_DescriptorSets[GraphicsFactory::DST_SIMPLE].pNativeLayout,
				&pipeline.pNativeLayout,
				VE_INTERFACE_DEBUG_NAME(pipeline.debugName.c_str())) != V3D_OK)
			{
				return false;
			}
		}

		/********/
		/* Grid */
		/********/

		{
			V3DConstantDesc constants[2];
			constants[0].shaderStageFlags = V3D_SHADER_STAGE_VERTEX;
			constants[0].offset = 0;
			constants[0].size = sizeof(glm::mat4);
			constants[1].shaderStageFlags = V3D_SHADER_STAGE_FRAGMENT;
			constants[1].offset = 64;
			constants[1].size = sizeof(glm::vec4);

			GraphicsFactory::Pipeline& pipeline = m_Pipelines[GraphicsFactory::PT_GRID];
			VE_DEBUG_CODE(pipeline.debugName = L"Scene_Grid");

			if (pNativeDevice->CreatePipelineLayout(
				_countof(constants), constants,
				0, nullptr,
				&pipeline.pNativeLayout,
				VE_INTERFACE_DEBUG_NAME(pipeline.debugName.c_str())) != V3D_OK)
			{
				return false;
			}
		}

		/********/
		/* Ssao */
		/********/

		{
			V3DConstantDesc constants[1];
			constants[0].shaderStageFlags = V3D_SHADER_STAGE_FRAGMENT;
			constants[0].offset = 0;
			constants[0].size = sizeof(SsaoConstant);

			GraphicsFactory::Pipeline& pipeline = m_Pipelines[GraphicsFactory::PT_SSAO];
			VE_DEBUG_CODE(pipeline.debugName = L"Scene_Ssao");

			if (pNativeDevice->CreatePipelineLayout(
				_countof(constants), constants,
				1, &m_DescriptorSets[GraphicsFactory::DST_SSAO].pNativeLayout,
				&pipeline.pNativeLayout, VE_INTERFACE_DEBUG_NAME(pipeline.debugName.c_str())) != V3D_OK)
			{
				return false;
			}
		}

		/***********************/
		/* DirectionalLighting */
		/***********************/

		{
			V3DConstantDesc constants[1];
			constants[0].shaderStageFlags = V3D_SHADER_STAGE_FRAGMENT;
			constants[0].offset = 0;
			constants[0].size = sizeof(DirectionalLightingConstant);

			GraphicsFactory::Pipeline& pipeline = m_Pipelines[GraphicsFactory::PT_DIRECTIONAL_LIGHTING];
			VE_DEBUG_CODE(pipeline.debugName = L"Scene_DirectionalLighting_O");

			if (pNativeDevice->CreatePipelineLayout(
				_countof(constants), constants,
				1, &m_DescriptorSets[GraphicsFactory::DST_DIRECTIONAL_LIGHTING_O].pNativeLayout,
				&pipeline.pNativeLayout, VE_INTERFACE_DEBUG_NAME(pipeline.debugName.c_str())) != V3D_OK)
			{
				return false;
			}
		}

		/******************/
		/* FinishLighting */
		/******************/

		{
			GraphicsFactory::Pipeline& pipeline = m_Pipelines[GraphicsFactory::PT_FINISH_LIGHTING];
			VE_DEBUG_CODE(pipeline.debugName = L"Scene_DirectionalLighting_O");

			if (pNativeDevice->CreatePipelineLayout(
				0, nullptr,
				1, &m_DescriptorSets[GraphicsFactory::DST_FINISH_LIGHTING].pNativeLayout,
				&pipeline.pNativeLayout, VE_INTERFACE_DEBUG_NAME(pipeline.debugName.c_str())) != V3D_OK)
			{
				return false;
			}
		}

		/**************/
		/* MeshSelect */
		/**************/

		{
			V3DConstantDesc constants[1];
			constants[0].shaderStageFlags = V3D_SHADER_STAGE_VERTEX;
			constants[0].offset = 0;
			constants[0].size = sizeof(glm::mat4);

			GraphicsFactory::Pipeline& pipeline = m_Pipelines[GraphicsFactory::PT_MESH_SELECT];
			VE_DEBUG_CODE(pipeline.debugName = L"Scene_MeshSelect");

			if (pNativeDevice->CreatePipelineLayout(
				_countof(constants), constants,
				1, &m_DescriptorSets[GraphicsFactory::DST_MESH].pNativeLayout,
				&pipeline.pNativeLayout, VE_INTERFACE_DEBUG_NAME(pipeline.debugName.c_str())) != V3D_OK)
			{
				return false;
			}
		}

		/*****************/
		/* SelectMapping */
		/*****************/

		{
			V3DConstantDesc constants[1];
			constants[0].shaderStageFlags = V3D_SHADER_STAGE_FRAGMENT;
			constants[0].offset = 0;
			constants[0].size = sizeof(SelectMappingConstant);

			GraphicsFactory::Pipeline& pipeline = m_Pipelines[GraphicsFactory::PT_SELECT_MAPPING];
			VE_DEBUG_CODE(pipeline.debugName = L"Scene_SelectMapping");

			if (pNativeDevice->CreatePipelineLayout(
				_countof(constants), constants,
				1, &m_DescriptorSets[GraphicsFactory::DST_SIMPLE].pNativeLayout,
				&pipeline.pNativeLayout, VE_INTERFACE_DEBUG_NAME(pipeline.debugName.c_str())) != V3D_OK)
			{
				return false;
			}
		}

		/*********/
		/* Debug */
		/*********/

		{
			V3DConstantDesc constants[1];
			constants[0].shaderStageFlags = V3D_SHADER_STAGE_VERTEX;
			constants[0].offset = 0;
			constants[0].size = sizeof(glm::mat4);

			GraphicsFactory::Pipeline& pipeline = m_Pipelines[GraphicsFactory::PT_DEBUG];
			VE_DEBUG_CODE(pipeline.debugName = L"Scene_Debug");

			if (pNativeDevice->CreatePipelineLayout(
				_countof(constants), constants,
				0, nullptr,
				&pipeline.pNativeLayout, VE_INTERFACE_DEBUG_NAME(pipeline.debugName.c_str())) != V3D_OK)
			{
				return false;
			}
		}

		/********/
		/* FXAA */
		/********/

		{
			V3DConstantDesc constants[1];
			constants[0].shaderStageFlags = V3D_SHADER_STAGE_FRAGMENT;
			constants[0].offset = 0;
			constants[0].size = sizeof(FxaaConstant);

			GraphicsFactory::Pipeline& pipeline = m_Pipelines[GraphicsFactory::PT_FXAA];
			VE_DEBUG_CODE(pipeline.debugName = L"Scene_Fxaa");

			if (pNativeDevice->CreatePipelineLayout(
				_countof(constants), constants,
				1, &m_DescriptorSets[GraphicsFactory::DST_SIMPLE].pNativeLayout,
				&pipeline.pNativeLayout, VE_INTERFACE_DEBUG_NAME(pipeline.debugName.c_str())) != V3D_OK)
			{
				return false;
			}
		}

		/*******/
		/* GUI */
		/*******/

		{
			V3DConstantDesc constantDesc[1];
			constantDesc[0].shaderStageFlags = V3D_SHADER_STAGE_VERTEX;
			constantDesc[0].offset = 0;
			constantDesc[0].size = sizeof(GuiConstant);

			GraphicsFactory::Pipeline& pipeline = m_Pipelines[GraphicsFactory::PT_GUI];
			VE_DEBUG_CODE(pipeline.debugName = L"VE_Gui");

			if (pNativeDevice->CreatePipelineLayout(
				_countof(constantDesc), constantDesc,
				1, &m_DescriptorSets[GraphicsFactory::DST_SIMPLE].pNativeLayout,
				&pipeline.pNativeLayout, VE_INTERFACE_DEBUG_NAME(pipeline.debugName.c_str())) != V3D_OK)
			{
				return false;
			}
		}

		// ----------------------------------------------------------------------------------------------------

		return true;
	}

	bool GraphicsFactory::InitializeAttachments()
	{
		IV3DDevice* pNativeDevice = m_pDeviceContext->GetNativeDevicePtr();

		// ----------------------------------------------------------------------------------------------------
		// フォーマットを決定
		// ----------------------------------------------------------------------------------------------------

		V3DFlags formatFeatureFlags;
		V3D_FORMAT unorm8BitFormat;
		V3D_FORMAT colorFormat;
		V3D_FORMAT highColorFormat;
		V3D_FORMAT selectFormat;
		V3D_FORMAT depthStencilFormat;

		formatFeatureFlags = V3D_IMAGE_FORMAT_FEATURE_SAMPLED | V3D_IMAGE_FORMAT_FEATURE_COLOR_ATTACHMENT | V3D_IMAGE_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND;
		unorm8BitFormat = V3D_FORMAT_B8G8R8A8_UNORM;
		if (pNativeDevice->CheckImageFormatFeature(unorm8BitFormat, V3D_IMAGE_TILING_OPTIMAL, formatFeatureFlags) != V3D_OK)
		{
			unorm8BitFormat = V3D_FORMAT_A8B8G8R8_UNORM;
			if (pNativeDevice->CheckImageFormatFeature(unorm8BitFormat, V3D_IMAGE_TILING_OPTIMAL, formatFeatureFlags) != V3D_OK)
			{
				unorm8BitFormat = V3D_FORMAT_R8G8B8A8_UNORM;
				if (pNativeDevice->CheckImageFormatFeature(unorm8BitFormat, V3D_IMAGE_TILING_OPTIMAL, formatFeatureFlags) != V3D_OK)
				{
					return false;
				}
			}
		}

		formatFeatureFlags = V3D_IMAGE_FORMAT_FEATURE_SAMPLED | V3D_IMAGE_FORMAT_FEATURE_COLOR_ATTACHMENT | V3D_IMAGE_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND;
		colorFormat = V3D_FORMAT_R16G16B16A16_SFLOAT;
		if (pNativeDevice->CheckImageFormatFeature(colorFormat, V3D_IMAGE_TILING_OPTIMAL, formatFeatureFlags) != V3D_OK)
		{
			colorFormat = V3D_FORMAT_R32G32B32A32_SFLOAT;
			if (pNativeDevice->CheckImageFormatFeature(colorFormat, V3D_IMAGE_TILING_OPTIMAL, formatFeatureFlags) != V3D_OK)
			{
				return false;
			}
		}

		formatFeatureFlags = V3D_IMAGE_FORMAT_FEATURE_SAMPLED | V3D_IMAGE_FORMAT_FEATURE_COLOR_ATTACHMENT | V3D_IMAGE_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND;
		highColorFormat = V3D_FORMAT_R32G32B32A32_SFLOAT;
		if (pNativeDevice->CheckImageFormatFeature(highColorFormat, V3D_IMAGE_TILING_OPTIMAL, formatFeatureFlags) != V3D_OK)
		{
			highColorFormat = V3D_FORMAT_R16G16B16A16_SFLOAT;
			if (pNativeDevice->CheckImageFormatFeature(highColorFormat, V3D_IMAGE_TILING_OPTIMAL, formatFeatureFlags) != V3D_OK)
			{
				return false;
			}
		}

		formatFeatureFlags = V3D_IMAGE_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT;
		depthStencilFormat = V3D_FORMAT_D24_UNORM_S8_UINT;
		if (pNativeDevice->CheckImageFormatFeature(depthStencilFormat, V3D_IMAGE_TILING_OPTIMAL, formatFeatureFlags) != V3D_OK)
		{
			depthStencilFormat = V3D_FORMAT_D16_UNORM;
			if (pNativeDevice->CheckImageFormatFeature(depthStencilFormat, V3D_IMAGE_TILING_OPTIMAL, formatFeatureFlags) != V3D_OK)
			{
				return false;
			}
		}

		formatFeatureFlags = V3D_IMAGE_FORMAT_FEATURE_SAMPLED | V3D_IMAGE_FORMAT_FEATURE_COLOR_ATTACHMENT;
		selectFormat = V3D_FORMAT_R8G8B8A8_UNORM;
		if (pNativeDevice->CheckImageFormatFeature(selectFormat, V3D_IMAGE_TILING_OPTIMAL, formatFeatureFlags) != V3D_OK)
		{
			selectFormat = V3D_FORMAT_A8B8G8R8_UNORM;
			if (pNativeDevice->CheckImageFormatFeature(selectFormat, V3D_IMAGE_TILING_OPTIMAL, formatFeatureFlags) != V3D_OK)
			{
				selectFormat = V3D_FORMAT_B8G8R8A8_UNORM;
				if (pNativeDevice->CheckImageFormatFeature(selectFormat, V3D_IMAGE_TILING_OPTIMAL, formatFeatureFlags) != V3D_OK)
				{
					return false;
				}
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// 記述を作成
		// ----------------------------------------------------------------------------------------------------

		uint32_t frameCount = m_pDeviceContext->GetFrameCount();

		V3DImageDesc defImageDesc;
		defImageDesc.type = V3D_IMAGE_TYPE_2D;
		defImageDesc.format = V3D_FORMAT_UNDEFINED;
		defImageDesc.width = 0; // ※1
		defImageDesc.height = 0; // ※1
		defImageDesc.depth = 1;
		defImageDesc.levelCount = 1;
		defImageDesc.layerCount = 0; // ※2
		defImageDesc.samples = V3D_SAMPLE_COUNT_1;
		defImageDesc.tiling = V3D_IMAGE_TILING_OPTIMAL;
		defImageDesc.usageFlags = 0;  // ※3

		V3DImageViewDesc defImageViewDesc;
		defImageViewDesc.type = V3D_IMAGE_VIEW_TYPE_2D;
		defImageViewDesc.baseLevel = 0;
		defImageViewDesc.levelCount = 1;
		defImageViewDesc.baseLayer = 0; // ※2
		defImageViewDesc.layerCount = 0; // ※2

		// ※1 Restore 時にフレームのサイズが指定される
		// ※2 Restore 時にフレーム数分のレイヤーが指定される
		// ※3 任意

		/**************/
		/* BrightPass */
		/**************/

		// Color
		m_Attachments[GraphicsFactory::AT_BR_COLOR].sizeType = GraphicsFactory::AST_DEFAULT;
		m_Attachments[GraphicsFactory::AT_BR_COLOR].imageDesc = defImageDesc;
		m_Attachments[GraphicsFactory::AT_BR_COLOR].imageDesc.format = unorm8BitFormat;
		m_Attachments[GraphicsFactory::AT_BR_COLOR].imageDesc.usageFlags = V3D_IMAGE_USAGE_SAMPLED | V3D_IMAGE_USAGE_COLOR_ATTACHMENT;
		m_Attachments[GraphicsFactory::AT_BR_COLOR].pImage = nullptr;
		m_Attachments[GraphicsFactory::AT_BR_COLOR].imageAllocation = nullptr;
		m_Attachments[GraphicsFactory::AT_BR_COLOR].imageViewDesc = defImageViewDesc;
		m_Attachments[GraphicsFactory::AT_BR_COLOR].imageViews.resize(frameCount, nullptr);
		m_Attachments[GraphicsFactory::AT_BR_COLOR].stageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
		m_Attachments[GraphicsFactory::AT_BR_COLOR].accessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
		m_Attachments[GraphicsFactory::AT_BR_COLOR].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
		VE_DEBUG_CODE(m_Attachments[GraphicsFactory::AT_BR_COLOR].debugName = L"BrightPass");

		/********/
		/* Blur */
		/********/

		// Half 0
		m_Attachments[GraphicsFactory::AT_BL_HALF_0].sizeType = GraphicsFactory::AST_HALF;
		m_Attachments[GraphicsFactory::AT_BL_HALF_0].imageDesc = defImageDesc;
		m_Attachments[GraphicsFactory::AT_BL_HALF_0].imageDesc.format = unorm8BitFormat;
		m_Attachments[GraphicsFactory::AT_BL_HALF_0].imageDesc.usageFlags = V3D_IMAGE_USAGE_SAMPLED | V3D_IMAGE_USAGE_COLOR_ATTACHMENT | V3D_IMAGE_USAGE_INPUT_ATTACHMENT;
		m_Attachments[GraphicsFactory::AT_BL_HALF_0].pImage = nullptr;
		m_Attachments[GraphicsFactory::AT_BL_HALF_0].imageAllocation = nullptr;
		m_Attachments[GraphicsFactory::AT_BL_HALF_0].imageViewDesc = defImageViewDesc;
		m_Attachments[GraphicsFactory::AT_BL_HALF_0].imageViews.resize(frameCount, nullptr);
		m_Attachments[GraphicsFactory::AT_BL_HALF_0].stageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
		m_Attachments[GraphicsFactory::AT_BL_HALF_0].accessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
		m_Attachments[GraphicsFactory::AT_BL_HALF_0].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
		VE_DEBUG_CODE(m_Attachments[GraphicsFactory::AT_BL_HALF_0].debugName = L"Blur_Half_0");

		// Half 1
		m_Attachments[GraphicsFactory::AT_BL_HALF_1].sizeType = GraphicsFactory::AST_HALF;
		m_Attachments[GraphicsFactory::AT_BL_HALF_1].imageDesc = defImageDesc;
		m_Attachments[GraphicsFactory::AT_BL_HALF_1].imageDesc.format = unorm8BitFormat;
		m_Attachments[GraphicsFactory::AT_BL_HALF_1].imageDesc.usageFlags = V3D_IMAGE_USAGE_SAMPLED | V3D_IMAGE_USAGE_COLOR_ATTACHMENT | V3D_IMAGE_USAGE_INPUT_ATTACHMENT;
		m_Attachments[GraphicsFactory::AT_BL_HALF_1].pImage = nullptr;
		m_Attachments[GraphicsFactory::AT_BL_HALF_1].imageAllocation = nullptr;
		m_Attachments[GraphicsFactory::AT_BL_HALF_1].imageViewDesc = defImageViewDesc;
		m_Attachments[GraphicsFactory::AT_BL_HALF_1].imageViews.resize(frameCount, nullptr);
		m_Attachments[GraphicsFactory::AT_BL_HALF_1].stageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
		m_Attachments[GraphicsFactory::AT_BL_HALF_1].accessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
		m_Attachments[GraphicsFactory::AT_BL_HALF_1].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
		VE_DEBUG_CODE(m_Attachments[GraphicsFactory::AT_BL_HALF_1].debugName = L"Blur_Half_1");

		// Quarter 0
		m_Attachments[GraphicsFactory::AT_BL_QUARTER_0].sizeType = GraphicsFactory::AST_QUARTER;
		m_Attachments[GraphicsFactory::AT_BL_QUARTER_0].imageDesc = defImageDesc;
		m_Attachments[GraphicsFactory::AT_BL_QUARTER_0].imageDesc.format = unorm8BitFormat;
		m_Attachments[GraphicsFactory::AT_BL_QUARTER_0].imageDesc.usageFlags = V3D_IMAGE_USAGE_SAMPLED | V3D_IMAGE_USAGE_COLOR_ATTACHMENT | V3D_IMAGE_USAGE_INPUT_ATTACHMENT;
		m_Attachments[GraphicsFactory::AT_BL_QUARTER_0].pImage = nullptr;
		m_Attachments[GraphicsFactory::AT_BL_QUARTER_0].imageAllocation = nullptr;
		m_Attachments[GraphicsFactory::AT_BL_QUARTER_0].imageViewDesc = defImageViewDesc;
		m_Attachments[GraphicsFactory::AT_BL_QUARTER_0].imageViews.resize(frameCount, nullptr);
		m_Attachments[GraphicsFactory::AT_BL_QUARTER_0].stageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
		m_Attachments[GraphicsFactory::AT_BL_QUARTER_0].accessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
		m_Attachments[GraphicsFactory::AT_BL_QUARTER_0].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
		VE_DEBUG_CODE(m_Attachments[GraphicsFactory::AT_BL_QUARTER_0].debugName = L"Blur_Quarter_0");

		// Quarter 1
		m_Attachments[GraphicsFactory::AT_BL_QUARTER_1].sizeType = GraphicsFactory::AST_QUARTER;
		m_Attachments[GraphicsFactory::AT_BL_QUARTER_1].imageDesc = defImageDesc;
		m_Attachments[GraphicsFactory::AT_BL_QUARTER_1].imageDesc.format = unorm8BitFormat;
		m_Attachments[GraphicsFactory::AT_BL_QUARTER_1].imageDesc.usageFlags = V3D_IMAGE_USAGE_SAMPLED | V3D_IMAGE_USAGE_COLOR_ATTACHMENT | V3D_IMAGE_USAGE_INPUT_ATTACHMENT;
		m_Attachments[GraphicsFactory::AT_BL_QUARTER_1].pImage = nullptr;
		m_Attachments[GraphicsFactory::AT_BL_QUARTER_1].imageAllocation = nullptr;
		m_Attachments[GraphicsFactory::AT_BL_QUARTER_1].imageViewDesc = defImageViewDesc;
		m_Attachments[GraphicsFactory::AT_BL_QUARTER_1].imageViews.resize(frameCount, nullptr);
		m_Attachments[GraphicsFactory::AT_BL_QUARTER_1].stageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
		m_Attachments[GraphicsFactory::AT_BL_QUARTER_1].accessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
		m_Attachments[GraphicsFactory::AT_BL_QUARTER_1].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
		VE_DEBUG_CODE(m_Attachments[GraphicsFactory::AT_BL_QUARTER_1].debugName = L"Blur_Quarter_1");

		/************/
		/* Geometry */
		/************/

		// カラー
		m_Attachments[GraphicsFactory::AT_GE_COLOR].sizeType = GraphicsFactory::AST_DEFAULT;
		m_Attachments[GraphicsFactory::AT_GE_COLOR].imageDesc = defImageDesc;
		m_Attachments[GraphicsFactory::AT_GE_COLOR].imageDesc.format = colorFormat;
		m_Attachments[GraphicsFactory::AT_GE_COLOR].imageDesc.usageFlags = V3D_IMAGE_USAGE_SAMPLED | V3D_IMAGE_USAGE_COLOR_ATTACHMENT;
		m_Attachments[GraphicsFactory::AT_GE_COLOR].pImage = nullptr;
		m_Attachments[GraphicsFactory::AT_GE_COLOR].imageAllocation = nullptr;
		m_Attachments[GraphicsFactory::AT_GE_COLOR].imageViewDesc = defImageViewDesc;
		m_Attachments[GraphicsFactory::AT_GE_COLOR].imageViews.resize(frameCount, nullptr);
		m_Attachments[GraphicsFactory::AT_GE_COLOR].stageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
		m_Attachments[GraphicsFactory::AT_GE_COLOR].accessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
		m_Attachments[GraphicsFactory::AT_GE_COLOR].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
		VE_DEBUG_CODE(m_Attachments[GraphicsFactory::AT_GE_COLOR].debugName = L"Geometry_Color");

		// バッファー 0
		m_Attachments[GraphicsFactory::AT_GE_BUFFER_0].sizeType = GraphicsFactory::AST_DEFAULT;
		m_Attachments[GraphicsFactory::AT_GE_BUFFER_0].imageDesc = defImageDesc;
		m_Attachments[GraphicsFactory::AT_GE_BUFFER_0].imageDesc.format = unorm8BitFormat;
		m_Attachments[GraphicsFactory::AT_GE_BUFFER_0].imageDesc.usageFlags = V3D_IMAGE_USAGE_SAMPLED | V3D_IMAGE_USAGE_COLOR_ATTACHMENT;
		m_Attachments[GraphicsFactory::AT_GE_BUFFER_0].pImage = nullptr;
		m_Attachments[GraphicsFactory::AT_GE_BUFFER_0].imageAllocation = nullptr;
		m_Attachments[GraphicsFactory::AT_GE_BUFFER_0].imageViewDesc = defImageViewDesc;
		m_Attachments[GraphicsFactory::AT_GE_BUFFER_0].imageViews.resize(frameCount, nullptr);
		m_Attachments[GraphicsFactory::AT_GE_BUFFER_0].stageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
		m_Attachments[GraphicsFactory::AT_GE_BUFFER_0].accessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
		m_Attachments[GraphicsFactory::AT_GE_BUFFER_0].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
		VE_DEBUG_CODE(m_Attachments[GraphicsFactory::AT_GE_BUFFER_0].debugName = L"Geometry_Buffer_0");

		// バッファー 1
		m_Attachments[GraphicsFactory::AT_GE_BUFFER_1].sizeType = GraphicsFactory::AST_DEFAULT;
		m_Attachments[GraphicsFactory::AT_GE_BUFFER_1].imageDesc = defImageDesc;
		m_Attachments[GraphicsFactory::AT_GE_BUFFER_1].imageDesc.format = highColorFormat;
		m_Attachments[GraphicsFactory::AT_GE_BUFFER_1].imageDesc.usageFlags = V3D_IMAGE_USAGE_SAMPLED | V3D_IMAGE_USAGE_COLOR_ATTACHMENT;
		m_Attachments[GraphicsFactory::AT_GE_BUFFER_1].pImage = nullptr;
		m_Attachments[GraphicsFactory::AT_GE_BUFFER_1].imageAllocation = nullptr;
		m_Attachments[GraphicsFactory::AT_GE_BUFFER_1].imageViewDesc = defImageViewDesc;
		m_Attachments[GraphicsFactory::AT_GE_BUFFER_1].imageViews.resize(frameCount, nullptr);
		m_Attachments[GraphicsFactory::AT_GE_BUFFER_1].stageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
		m_Attachments[GraphicsFactory::AT_GE_BUFFER_1].accessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
		m_Attachments[GraphicsFactory::AT_GE_BUFFER_1].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
		VE_DEBUG_CODE(m_Attachments[GraphicsFactory::AT_GE_BUFFER_1].debugName = L"Geometry_Buffer_1");

		// セレクト
		m_Attachments[GraphicsFactory::AT_GE_SELECT].sizeType = GraphicsFactory::AST_DEFAULT;
		m_Attachments[GraphicsFactory::AT_GE_SELECT].imageDesc = defImageDesc;
		m_Attachments[GraphicsFactory::AT_GE_SELECT].imageDesc.format = selectFormat;
		m_Attachments[GraphicsFactory::AT_GE_SELECT].imageDesc.usageFlags = V3D_IMAGE_USAGE_TRANSFER_SRC | V3D_IMAGE_USAGE_COLOR_ATTACHMENT;
		m_Attachments[GraphicsFactory::AT_GE_SELECT].pImage = nullptr;
		m_Attachments[GraphicsFactory::AT_GE_SELECT].imageAllocation = nullptr;
		m_Attachments[GraphicsFactory::AT_GE_SELECT].imageViewDesc = defImageViewDesc;
		m_Attachments[GraphicsFactory::AT_GE_SELECT].imageViews.resize(frameCount, nullptr);
		m_Attachments[GraphicsFactory::AT_GE_SELECT].stageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
		m_Attachments[GraphicsFactory::AT_GE_SELECT].accessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
		m_Attachments[GraphicsFactory::AT_GE_SELECT].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
		VE_DEBUG_CODE(m_Attachments[GraphicsFactory::AT_GE_SELECT].debugName = L"Geometry_Select");

		// デプス
		m_Attachments[GraphicsFactory::AT_GE_DEPTH].sizeType = GraphicsFactory::AST_DEFAULT;
		m_Attachments[GraphicsFactory::AT_GE_DEPTH].imageDesc = defImageDesc;
		m_Attachments[GraphicsFactory::AT_GE_DEPTH].imageDesc.format = depthStencilFormat;
		m_Attachments[GraphicsFactory::AT_GE_DEPTH].imageDesc.usageFlags = V3D_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT;
		m_Attachments[GraphicsFactory::AT_GE_DEPTH].pImage = nullptr;
		m_Attachments[GraphicsFactory::AT_GE_DEPTH].imageAllocation = nullptr;
		m_Attachments[GraphicsFactory::AT_GE_DEPTH].imageViewDesc = defImageViewDesc;
		m_Attachments[GraphicsFactory::AT_GE_DEPTH].imageViews.resize(frameCount, nullptr);
		m_Attachments[GraphicsFactory::AT_GE_DEPTH].stageMask = V3D_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS | V3D_PIPELINE_STAGE_LATE_FRAGMENT_TESTS;
		m_Attachments[GraphicsFactory::AT_GE_DEPTH].accessMask = V3D_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ | V3D_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE;
		m_Attachments[GraphicsFactory::AT_GE_DEPTH].layout = V3D_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT;
		VE_DEBUG_CODE(m_Attachments[GraphicsFactory::AT_GE_DEPTH].debugName = L"Geometry_Depth");

		/********/
		/* SSAO */
		/********/

		m_Attachments[GraphicsFactory::AT_SS_COLOR].sizeType = GraphicsFactory::AST_DEFAULT;
		m_Attachments[GraphicsFactory::AT_SS_COLOR].imageDesc = defImageDesc;
		m_Attachments[GraphicsFactory::AT_SS_COLOR].imageDesc.format = unorm8BitFormat;
		m_Attachments[GraphicsFactory::AT_SS_COLOR].imageDesc.usageFlags = V3D_IMAGE_USAGE_SAMPLED | V3D_IMAGE_USAGE_COLOR_ATTACHMENT;
		m_Attachments[GraphicsFactory::AT_SS_COLOR].pImage = nullptr;
		m_Attachments[GraphicsFactory::AT_SS_COLOR].imageAllocation = nullptr;
		m_Attachments[GraphicsFactory::AT_SS_COLOR].imageViewDesc = defImageViewDesc;
		m_Attachments[GraphicsFactory::AT_SS_COLOR].imageViews.resize(frameCount, nullptr);
		m_Attachments[GraphicsFactory::AT_SS_COLOR].stageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
		m_Attachments[GraphicsFactory::AT_SS_COLOR].accessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
		m_Attachments[GraphicsFactory::AT_SS_COLOR].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
		VE_DEBUG_CODE(m_Attachments[GraphicsFactory::AT_SS_COLOR].debugName = L"Ssao_Color");

		/**********/
		/* Shadow */
		/**********/

		// バッファー 
		m_Attachments[GraphicsFactory::AT_SA_BUFFER].sizeType = GraphicsFactory::AST_FIXED;
		m_Attachments[GraphicsFactory::AT_SA_BUFFER].imageDesc = defImageDesc;
		m_Attachments[GraphicsFactory::AT_SA_BUFFER].imageDesc.format = highColorFormat;
		m_Attachments[GraphicsFactory::AT_SA_BUFFER].imageDesc.width = m_ShadowResolution;
		m_Attachments[GraphicsFactory::AT_SA_BUFFER].imageDesc.height = m_ShadowResolution;
		m_Attachments[GraphicsFactory::AT_SA_BUFFER].imageDesc.usageFlags = V3D_IMAGE_USAGE_SAMPLED | V3D_IMAGE_USAGE_COLOR_ATTACHMENT;
		m_Attachments[GraphicsFactory::AT_SA_BUFFER].pImage = nullptr;
		m_Attachments[GraphicsFactory::AT_SA_BUFFER].imageAllocation = nullptr;
		m_Attachments[GraphicsFactory::AT_SA_BUFFER].imageViewDesc = defImageViewDesc;
		m_Attachments[GraphicsFactory::AT_SA_BUFFER].imageViews.resize(frameCount, nullptr);
		m_Attachments[GraphicsFactory::AT_SA_BUFFER].stageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
		m_Attachments[GraphicsFactory::AT_SA_BUFFER].accessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
		m_Attachments[GraphicsFactory::AT_SA_BUFFER].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
		VE_DEBUG_CODE(m_Attachments[GraphicsFactory::AT_SA_BUFFER].debugName = L"Shadow_Buffer");

		// デプス
		m_Attachments[GraphicsFactory::AT_SA_DEPTH].sizeType = GraphicsFactory::AST_FIXED;
		m_Attachments[GraphicsFactory::AT_SA_DEPTH].imageDesc = defImageDesc;
		m_Attachments[GraphicsFactory::AT_SA_DEPTH].imageDesc.format = depthStencilFormat;
		m_Attachments[GraphicsFactory::AT_SA_DEPTH].imageDesc.width = m_ShadowResolution;
		m_Attachments[GraphicsFactory::AT_SA_DEPTH].imageDesc.height = m_ShadowResolution;
		m_Attachments[GraphicsFactory::AT_SA_DEPTH].imageDesc.usageFlags = V3D_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT;
		m_Attachments[GraphicsFactory::AT_SA_DEPTH].pImage = nullptr;
		m_Attachments[GraphicsFactory::AT_SA_DEPTH].imageAllocation = nullptr;
		m_Attachments[GraphicsFactory::AT_SA_DEPTH].imageViewDesc = defImageViewDesc;
		m_Attachments[GraphicsFactory::AT_SA_DEPTH].imageViews.resize(frameCount, nullptr);
		m_Attachments[GraphicsFactory::AT_SA_DEPTH].stageMask = V3D_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS | V3D_PIPELINE_STAGE_LATE_FRAGMENT_TESTS;
		m_Attachments[GraphicsFactory::AT_SA_DEPTH].accessMask = V3D_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ | V3D_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE;
		m_Attachments[GraphicsFactory::AT_SA_DEPTH].layout = V3D_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT;
		VE_DEBUG_CODE(m_Attachments[GraphicsFactory::AT_SA_DEPTH].debugName = L"Shadow_Depth");

		/************/
		/* Lighting */
		/************/

		m_Attachments[GraphicsFactory::AT_LI_COLOR].sizeType = GraphicsFactory::AST_DEFAULT;
		m_Attachments[GraphicsFactory::AT_LI_COLOR].imageDesc = defImageDesc;
		m_Attachments[GraphicsFactory::AT_LI_COLOR].imageDesc.format = colorFormat;
		m_Attachments[GraphicsFactory::AT_LI_COLOR].imageDesc.usageFlags = V3D_IMAGE_USAGE_SAMPLED | V3D_IMAGE_USAGE_COLOR_ATTACHMENT | V3D_IMAGE_USAGE_INPUT_ATTACHMENT;
		m_Attachments[GraphicsFactory::AT_LI_COLOR].pImage = nullptr;
		m_Attachments[GraphicsFactory::AT_LI_COLOR].imageAllocation = nullptr;
		m_Attachments[GraphicsFactory::AT_LI_COLOR].imageViewDesc = defImageViewDesc;
		m_Attachments[GraphicsFactory::AT_LI_COLOR].imageViews.resize(frameCount, nullptr);
		m_Attachments[GraphicsFactory::AT_LI_COLOR].stageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
		m_Attachments[GraphicsFactory::AT_LI_COLOR].accessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
		m_Attachments[GraphicsFactory::AT_LI_COLOR].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
		VE_DEBUG_CODE(m_Attachments[GraphicsFactory::AT_LI_COLOR].debugName = L"Light_Color");

		m_Attachments[GraphicsFactory::AT_GL_COLOR].sizeType = GraphicsFactory::AST_DEFAULT;
		m_Attachments[GraphicsFactory::AT_GL_COLOR].imageDesc = defImageDesc;
		m_Attachments[GraphicsFactory::AT_GL_COLOR].imageDesc.format = colorFormat;
		m_Attachments[GraphicsFactory::AT_GL_COLOR].imageDesc.usageFlags = V3D_IMAGE_USAGE_SAMPLED | V3D_IMAGE_USAGE_COLOR_ATTACHMENT | V3D_IMAGE_USAGE_INPUT_ATTACHMENT;
		m_Attachments[GraphicsFactory::AT_GL_COLOR].pImage = nullptr;
		m_Attachments[GraphicsFactory::AT_GL_COLOR].imageAllocation = nullptr;
		m_Attachments[GraphicsFactory::AT_GL_COLOR].imageViewDesc = defImageViewDesc;
		m_Attachments[GraphicsFactory::AT_GL_COLOR].imageViews.resize(frameCount, nullptr);
		m_Attachments[GraphicsFactory::AT_GL_COLOR].stageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
		m_Attachments[GraphicsFactory::AT_GL_COLOR].accessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
		m_Attachments[GraphicsFactory::AT_GL_COLOR].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
		VE_DEBUG_CODE(m_Attachments[GraphicsFactory::AT_GL_COLOR].debugName = L"Glare_Color");

		m_Attachments[GraphicsFactory::AT_SH_COLOR].sizeType = GraphicsFactory::AST_DEFAULT;
		m_Attachments[GraphicsFactory::AT_SH_COLOR].imageDesc = defImageDesc;
		m_Attachments[GraphicsFactory::AT_SH_COLOR].imageDesc.format = colorFormat;
		m_Attachments[GraphicsFactory::AT_SH_COLOR].imageDesc.usageFlags = V3D_IMAGE_USAGE_SAMPLED | V3D_IMAGE_USAGE_COLOR_ATTACHMENT;
		m_Attachments[GraphicsFactory::AT_SH_COLOR].pImage = nullptr;
		m_Attachments[GraphicsFactory::AT_SH_COLOR].imageAllocation = nullptr;
		m_Attachments[GraphicsFactory::AT_SH_COLOR].imageViewDesc = defImageViewDesc;
		m_Attachments[GraphicsFactory::AT_SH_COLOR].imageViews.resize(frameCount, nullptr);
		m_Attachments[GraphicsFactory::AT_SH_COLOR].stageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
		m_Attachments[GraphicsFactory::AT_SH_COLOR].accessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
		m_Attachments[GraphicsFactory::AT_SH_COLOR].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
		VE_DEBUG_CODE(m_Attachments[GraphicsFactory::AT_SH_COLOR].debugName = L"Shade_Color");

		/***************/
		/* ImageEffect */
		/***************/

		m_Attachments[GraphicsFactory::AT_IE_LDR_COLOR_0].sizeType = GraphicsFactory::AST_DEFAULT;
		m_Attachments[GraphicsFactory::AT_IE_LDR_COLOR_0].imageDesc = defImageDesc;
		m_Attachments[GraphicsFactory::AT_IE_LDR_COLOR_0].imageDesc.format = unorm8BitFormat;
		m_Attachments[GraphicsFactory::AT_IE_LDR_COLOR_0].imageDesc.usageFlags = V3D_IMAGE_USAGE_SAMPLED | V3D_IMAGE_USAGE_COLOR_ATTACHMENT | V3D_IMAGE_USAGE_INPUT_ATTACHMENT;
		m_Attachments[GraphicsFactory::AT_IE_LDR_COLOR_0].pImage = nullptr;
		m_Attachments[GraphicsFactory::AT_IE_LDR_COLOR_0].imageAllocation = nullptr;
		m_Attachments[GraphicsFactory::AT_IE_LDR_COLOR_0].imageViewDesc = defImageViewDesc;
		m_Attachments[GraphicsFactory::AT_IE_LDR_COLOR_0].imageViews.resize(frameCount, nullptr);
		m_Attachments[GraphicsFactory::AT_IE_LDR_COLOR_0].stageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
		m_Attachments[GraphicsFactory::AT_IE_LDR_COLOR_0].accessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
		m_Attachments[GraphicsFactory::AT_IE_LDR_COLOR_0].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
		VE_DEBUG_CODE(m_Attachments[GraphicsFactory::AT_IE_LDR_COLOR_0].debugName = L"ImageEffect_LDR_Color_0");

		m_Attachments[GraphicsFactory::AT_IE_LDR_COLOR_1].sizeType = GraphicsFactory::AST_DEFAULT;
		m_Attachments[GraphicsFactory::AT_IE_LDR_COLOR_1].imageDesc = defImageDesc;
		m_Attachments[GraphicsFactory::AT_IE_LDR_COLOR_1].imageDesc.format = unorm8BitFormat;
		m_Attachments[GraphicsFactory::AT_IE_LDR_COLOR_1].imageDesc.usageFlags = V3D_IMAGE_USAGE_SAMPLED | V3D_IMAGE_USAGE_COLOR_ATTACHMENT | V3D_IMAGE_USAGE_INPUT_ATTACHMENT;
		m_Attachments[GraphicsFactory::AT_IE_LDR_COLOR_1].pImage = nullptr;
		m_Attachments[GraphicsFactory::AT_IE_LDR_COLOR_1].imageAllocation = nullptr;
		m_Attachments[GraphicsFactory::AT_IE_LDR_COLOR_1].imageViewDesc = defImageViewDesc;
		m_Attachments[GraphicsFactory::AT_IE_LDR_COLOR_1].imageViews.resize(frameCount, nullptr);
		m_Attachments[GraphicsFactory::AT_IE_LDR_COLOR_1].stageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
		m_Attachments[GraphicsFactory::AT_IE_LDR_COLOR_1].accessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
		m_Attachments[GraphicsFactory::AT_IE_LDR_COLOR_1].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
		VE_DEBUG_CODE(m_Attachments[GraphicsFactory::AT_IE_LDR_COLOR_1].debugName = L"ImageEffect_LDR_Color_1");

		m_Attachments[GraphicsFactory::AT_IE_SELECT_COLOR].sizeType = GraphicsFactory::AST_DEFAULT;
		m_Attachments[GraphicsFactory::AT_IE_SELECT_COLOR].imageDesc = defImageDesc;
		m_Attachments[GraphicsFactory::AT_IE_SELECT_COLOR].imageDesc.format = unorm8BitFormat;
		m_Attachments[GraphicsFactory::AT_IE_SELECT_COLOR].imageDesc.usageFlags = V3D_IMAGE_USAGE_SAMPLED | V3D_IMAGE_USAGE_COLOR_ATTACHMENT | V3D_IMAGE_USAGE_INPUT_ATTACHMENT;
		m_Attachments[GraphicsFactory::AT_IE_SELECT_COLOR].pImage = nullptr;
		m_Attachments[GraphicsFactory::AT_IE_SELECT_COLOR].imageAllocation = nullptr;
		m_Attachments[GraphicsFactory::AT_IE_SELECT_COLOR].imageViewDesc = defImageViewDesc;
		m_Attachments[GraphicsFactory::AT_IE_SELECT_COLOR].imageViews.resize(frameCount, nullptr);
		m_Attachments[GraphicsFactory::AT_IE_SELECT_COLOR].stageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
		m_Attachments[GraphicsFactory::AT_IE_SELECT_COLOR].accessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
		m_Attachments[GraphicsFactory::AT_IE_SELECT_COLOR].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
		VE_DEBUG_CODE(m_Attachments[GraphicsFactory::AT_IE_SELECT_COLOR].debugName = L"ImageEffect_Select_Color");

		// ----------------------------------------------------------------------------------------------------

		return true;
	}

	void GraphicsFactory::LostAttachments(DeletingQueue* pDeletingQueue, bool force)
	{
		for (GraphicsFactory::Attachment& attachment : m_Attachments)
		{
//			if ((force == true) || (attachment.sizeType != GraphicsFactory::AST_FIXED))
//			{
				DeleteDeviceChild(pDeletingQueue, attachment.imageViews);
				DeleteResource(pDeletingQueue, &attachment.pImage, &attachment.imageAllocation);
//			}
		}
	}

	bool GraphicsFactory::RestoreAttachments()
	{
		const V3DSwapChainDesc& swapChainDesc = m_pDeviceContext->GetNativeSwapchainDesc();
		IV3DDevice* pNativeDevice = m_pDeviceContext->GetNativeDevicePtr();
		ResourceMemoryManager* pResourceMemoryManager = m_pDeviceContext->GetResourceMemoryManagerPtr();

		V3DViewport& defaultViewport = m_Viewports[GraphicsFactory::VT_DEFAULT];
		defaultViewport.rect.x = 0;
		defaultViewport.rect.y = 0;
		defaultViewport.rect.width = swapChainDesc.imageWidth;
		defaultViewport.rect.height = swapChainDesc.imageHeight;
		defaultViewport.minDepth = 0.0f;
		defaultViewport.maxDepth = 1.0f;

		V3DViewport& halfViewport = m_Viewports[GraphicsFactory::VT_HALF];
		halfViewport.rect.x = 0;
		halfViewport.rect.y = 0;
		halfViewport.rect.width = std::max(uint32_t(1), swapChainDesc.imageWidth / 2);
		halfViewport.rect.height = std::max(uint32_t(1), swapChainDesc.imageHeight / 2);
		halfViewport.minDepth = 0.0f;
		halfViewport.maxDepth = 1.0f;

		V3DViewport& quarterViewport = m_Viewports[GraphicsFactory::VT_QUARTER];
		quarterViewport.rect.x = 0;
		quarterViewport.rect.y = 0;
		quarterViewport.rect.width = std::max(uint32_t(1), swapChainDesc.imageWidth / 4);
		quarterViewport.rect.height = std::max(uint32_t(1), swapChainDesc.imageHeight / 4);
		quarterViewport.minDepth = 0.0f;
		quarterViewport.maxDepth = 1.0f;

		IV3DCommandBuffer* pCommandBuffer = m_pDeviceContext->GetImmediateContextPtr()->Begin();

		for (GraphicsFactory::Attachment& attachment : m_Attachments)
		{
			/******************/
			/* イメージを作成 */
			/******************/
			
//			if (attachment.pImage == nullptr)
			{
				V3DImageDesc imageDesc = attachment.imageDesc;
				imageDesc.layerCount = swapChainDesc.imageCount;

				/****************************/
				/* イメージのサイズを決める */
				/****************************/

				switch (attachment.sizeType)
				{
				case GraphicsFactory::AST_DEFAULT:
					imageDesc.width = swapChainDesc.imageWidth;
					imageDesc.height = swapChainDesc.imageHeight;
					break;
				case GraphicsFactory::AST_HALF:
					imageDesc.width = std::max(uint32_t(1), swapChainDesc.imageWidth / 2);
					imageDesc.height = std::max(uint32_t(1), swapChainDesc.imageHeight / 2);
					break;
				case GraphicsFactory::AST_QUARTER:
					imageDesc.width = std::max(uint32_t(1), swapChainDesc.imageWidth / 4);
					imageDesc.height = std::max(uint32_t(1), swapChainDesc.imageHeight / 4);
					break;
				case GraphicsFactory::AST_FIXED:
					imageDesc.width = attachment.imageDesc.width;
					imageDesc.height = attachment.imageDesc.height;
					break;
				}

				if (pNativeDevice->CreateImage(imageDesc, V3D_IMAGE_LAYOUT_UNDEFINED, &attachment.pImage, VE_INTERFACE_DEBUG_NAME(attachment.debugName.c_str())) != V3D_OK)
				{
					m_pDeviceContext->GetImmediateContextPtr()->End();
					return false;
				}

				attachment.imageAllocation = pResourceMemoryManager->Allocate(attachment.pImage, V3D_MEMORY_PROPERTY_DEVICE_LOCAL);
				if (attachment.imageAllocation == nullptr)
				{
					m_pDeviceContext->GetImmediateContextPtr()->End();
					return false;
				}

				V3DPipelineBarrier pipelineBarrier;
				pipelineBarrier.srcStageMask = V3D_PIPELINE_STAGE_TOP_OF_PIPE;
				pipelineBarrier.dstStageMask = attachment.stageMask;
				pipelineBarrier.dependencyFlags = 0;

				V3DImageMemoryBarrier memoryBarrier;
				memoryBarrier.srcAccessMask = 0;
				memoryBarrier.dstAccessMask = attachment.accessMask;
				memoryBarrier.srcQueueFamily = V3D_QUEUE_FAMILY_IGNORED;
				memoryBarrier.dstQueueFamily = V3D_QUEUE_FAMILY_IGNORED;
				memoryBarrier.srcLayout = V3D_IMAGE_LAYOUT_UNDEFINED;
				memoryBarrier.dstLayout = attachment.layout;
				memoryBarrier.pImage = attachment.pImage;
				memoryBarrier.baseLevel = 0;
				memoryBarrier.levelCount = imageDesc.levelCount;
				memoryBarrier.baseLayer = 0;
				memoryBarrier.layerCount = imageDesc.layerCount;

				pCommandBuffer->Barrier(pipelineBarrier, memoryBarrier);

				for (uint32_t i = 0; i < swapChainDesc.imageCount; i++)
				{
					V3DImageViewDesc imageViewDesc = attachment.imageViewDesc;
					imageViewDesc.baseLayer = i;
					imageViewDesc.layerCount = 1;

					if (pNativeDevice->CreateImageView(attachment.pImage, imageViewDesc, &attachment.imageViews[i], VE_INTERFACE_DEBUG_NAME(attachment.debugName.c_str())) != V3D_OK)
					{
						m_pDeviceContext->GetImmediateContextPtr()->End();
						return false;
					}
				}
			}
		}

		m_pDeviceContext->GetImmediateContextPtr()->End();

		return true;
	}

	bool GraphicsFactory::InitializeStages()
	{
		uint32_t frameCount = m_pDeviceContext->GetFrameCount();

		// ----------------------------------------------------------------------------------------------------
		// Paste
		// ----------------------------------------------------------------------------------------------------

		{
			GraphicsFactory::Stage& stage = m_Stages[GraphicsFactory::ST_SCENE_PASTE];
			VE_DEBUG_CODE(stage.debugName = L"PasteStage");

			/****************/
			/* レンダーパス */
			/****************/

			RenderPassDesc& renderPassDesc = stage.renderPassDesc;
			renderPassDesc.Allocate(1, 1, 0);

			renderPassDesc.pAttachments[0].format = m_Attachments[GraphicsFactory::AT_SH_COLOR].imageDesc.format;
			renderPassDesc.pAttachments[0].samples = m_Attachments[GraphicsFactory::AT_SH_COLOR].imageDesc.samples;
			renderPassDesc.pAttachments[0].loadOp = V3D_ATTACHMENT_LOAD_OP_LOAD;
			renderPassDesc.pAttachments[0].storeOp = V3D_ATTACHMENT_STORE_OP_STORE;
			renderPassDesc.pAttachments[0].stencilLoadOp = V3D_ATTACHMENT_LOAD_OP_UNDEFINED;
			renderPassDesc.pAttachments[0].stencilStoreOp = V3D_ATTACHMENT_STORE_OP_UNDEFINED;
			renderPassDesc.pAttachments[0].initialLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			renderPassDesc.pAttachments[0].finalLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;

			// サブパス 0
			RenderPassDesc::Subpass* pSubpass = renderPassDesc.AllocateSubpass(0, 0, 1, false, 0);
			pSubpass->colorAttachments[0].attachment = 0;
			pSubpass->colorAttachments[0].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;

			/**********************/
			/* フレームバッファー */
			/**********************/

			stage.frameBuffers.resize(1);

			stage.frameBuffers[0].attachmentIndices.resize(1);
			stage.frameBuffers[0].attachmentIndices[0] = GraphicsFactory::AT_SH_COLOR;
			stage.frameBuffers[0].handle = std::make_shared<FrameBufferHandle>();
			stage.frameBuffers[0].handle->m_FrameBuffers.resize(frameCount, nullptr);

			/****************/
			/* パイプライン */
			/****************/

			stage.subpasses.resize(GraphicsFactory::SST_SCENE_PASTE_MAX);
		}

		/**********************/
		/* パイプライン : Add */
		/**********************/

		{
			GraphicsFactory::Stage& stage = m_Stages[GraphicsFactory::ST_SCENE_PASTE];
			GraphicsFactory::StageSubpass& subpass = stage.subpasses[GraphicsFactory::SST_SCENE_PASTE_0_ADD];

			subpass.pipelineType = GraphicsFactory::PT_PASTE;

			GraphicsPipelineDesc& pipelineDesc = subpass.pipelineDesc;
			pipelineDesc.Allocate(2, 1, 1);
			pipelineDesc.vertexShader.pModule = m_ShaderModules[GraphicsFactory::SMT_SIMPLE_VERT];
			pipelineDesc.vertexShader.pEntryPointName = "main";
			pipelineDesc.fragmentShader.pModule = m_ShaderModules[GraphicsFactory::SMT_PASTE_FRAG];
			pipelineDesc.fragmentShader.pEntryPointName = "main";
			pipelineDesc.vertexInput.pElements[0].format = V3D_FORMAT_R32G32B32A32_SFLOAT;
			pipelineDesc.vertexInput.pElements[0].location = 0;
			pipelineDesc.vertexInput.pElements[0].offset = 0;
			pipelineDesc.vertexInput.pElements[1].format = V3D_FORMAT_R32G32_SFLOAT;
			pipelineDesc.vertexInput.pElements[1].location = 1;
			pipelineDesc.vertexInput.pElements[1].offset = 16;
			pipelineDesc.vertexInput.pLayouts[0].binding = 0;
			pipelineDesc.vertexInput.pLayouts[0].firstElement = 0;
			pipelineDesc.vertexInput.pLayouts[0].elementCount = 2;
			pipelineDesc.vertexInput.pLayouts[0].stride = sizeof(SimpleVertex);
			pipelineDesc.primitiveTopology = V3D_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			pipelineDesc.rasterization.polygonMode = V3D_POLYGON_MODE_FILL;
			pipelineDesc.rasterization.cullMode = V3D_CULL_MODE_NONE;
			pipelineDesc.depthStencil.depthTestEnable = false;
			pipelineDesc.depthStencil.depthWriteEnable = false;
			pipelineDesc.depthStencil.depthCompareOp = V3D_COMPARE_OP_ALWAYS;
			pipelineDesc.colorBlend.pAttachments[0] = InitializeColorBlendAttachment(BLEND_MODE_ADD);
			pipelineDesc.subpass = 0;

			subpass.pipelineHandle = std::make_shared<PipelineHandle>();
		}

		/*************************/
		/* パイプライン : Screen */
		/*************************/

		{
			GraphicsFactory::Stage& stage = m_Stages[GraphicsFactory::ST_SCENE_PASTE];
			GraphicsFactory::StageSubpass& subpass = stage.subpasses[GraphicsFactory::SST_SCENE_PASTE_0_SCREEN];

			subpass.pipelineType = GraphicsFactory::PT_PASTE;

			GraphicsPipelineDesc& pipelineDesc = subpass.pipelineDesc;
			pipelineDesc.Allocate(2, 1, 1);
			pipelineDesc.vertexShader.pModule = m_ShaderModules[GraphicsFactory::SMT_SIMPLE_VERT];
			pipelineDesc.vertexShader.pEntryPointName = "main";
			pipelineDesc.fragmentShader.pModule = m_ShaderModules[GraphicsFactory::SMT_PASTE_FRAG];
			pipelineDesc.fragmentShader.pEntryPointName = "main";
			pipelineDesc.vertexInput.pElements[0].format = V3D_FORMAT_R32G32B32A32_SFLOAT;
			pipelineDesc.vertexInput.pElements[0].location = 0;
			pipelineDesc.vertexInput.pElements[0].offset = 0;
			pipelineDesc.vertexInput.pElements[1].format = V3D_FORMAT_R32G32_SFLOAT;
			pipelineDesc.vertexInput.pElements[1].location = 1;
			pipelineDesc.vertexInput.pElements[1].offset = 16;
			pipelineDesc.vertexInput.pLayouts[0].binding = 0;
			pipelineDesc.vertexInput.pLayouts[0].firstElement = 0;
			pipelineDesc.vertexInput.pLayouts[0].elementCount = 2;
			pipelineDesc.vertexInput.pLayouts[0].stride = sizeof(SimpleVertex);
			pipelineDesc.primitiveTopology = V3D_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			pipelineDesc.rasterization.polygonMode = V3D_POLYGON_MODE_FILL;
			pipelineDesc.rasterization.cullMode = V3D_CULL_MODE_NONE;
			pipelineDesc.depthStencil.depthTestEnable = false;
			pipelineDesc.depthStencil.depthWriteEnable = false;
			pipelineDesc.depthStencil.depthCompareOp = V3D_COMPARE_OP_ALWAYS;
			pipelineDesc.colorBlend.pAttachments[0] = InitializeColorBlendAttachment(BLEND_MODE_SCREEN);
			pipelineDesc.subpass = 0;

			subpass.pipelineHandle = std::make_shared<PipelineHandle>();
		}

		// ----------------------------------------------------------------------------------------------------
		// BrightPass
		// ----------------------------------------------------------------------------------------------------

		{
			GraphicsFactory::Stage& stage = m_Stages[GraphicsFactory::ST_SCENE_BRIGHT_PASS];
			VE_DEBUG_CODE(stage.debugName = L"BrightPassStage");

			/****************/
			/* レンダーパス */
			/****************/

			RenderPassDesc& renderPassDesc = stage.renderPassDesc;
			renderPassDesc.Allocate(1, 1, 1);

			renderPassDesc.pAttachments[0].format = m_Attachments[GraphicsFactory::AT_BR_COLOR].imageDesc.format;
			renderPassDesc.pAttachments[0].samples = m_Attachments[GraphicsFactory::AT_BR_COLOR].imageDesc.samples;
			renderPassDesc.pAttachments[0].loadOp = V3D_ATTACHMENT_LOAD_OP_CLEAR;
			renderPassDesc.pAttachments[0].storeOp = V3D_ATTACHMENT_STORE_OP_STORE;
			renderPassDesc.pAttachments[0].stencilLoadOp = V3D_ATTACHMENT_LOAD_OP_UNDEFINED;
			renderPassDesc.pAttachments[0].stencilStoreOp = V3D_ATTACHMENT_STORE_OP_UNDEFINED;
			renderPassDesc.pAttachments[0].initialLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			renderPassDesc.pAttachments[0].finalLayout = V3D_IMAGE_LAYOUT_SHADER_READ_ONLY;
			renderPassDesc.pAttachments[0].clearValue = V3DClearValue{};

			// サブパス 0
			RenderPassDesc::Subpass* pSubpass = renderPassDesc.AllocateSubpass(0, 0, 1, false, 0);
			pSubpass->colorAttachments[0].attachment = 0;
			pSubpass->colorAttachments[0].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;

			renderPassDesc.pSubpassDependencies[0].srcSubpass = 0;
			renderPassDesc.pSubpassDependencies[0].dstSubpass = V3D_SUBPASS_EXTERNAL;
			renderPassDesc.pSubpassDependencies[0].srcStageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
			renderPassDesc.pSubpassDependencies[0].dstStageMask = V3D_PIPELINE_STAGE_FRAGMENT_SHADER;
			renderPassDesc.pSubpassDependencies[0].srcAccessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
			renderPassDesc.pSubpassDependencies[0].dstAccessMask = V3D_ACCESS_SHADER_READ;
			renderPassDesc.pSubpassDependencies[0].dependencyFlags = 0;

			/**********************/
			/* フレームバッファー */
			/**********************/

			stage.frameBuffers.resize(1);

			stage.frameBuffers[0].attachmentIndices.resize(1);
			stage.frameBuffers[0].attachmentIndices[0] = GraphicsFactory::AT_BR_COLOR;
			stage.frameBuffers[0].handle = std::make_shared<FrameBufferHandle>();
			stage.frameBuffers[0].handle->m_FrameBuffers.resize(frameCount, nullptr);

			/****************/
			/* パイプライン */
			/****************/

			stage.subpasses.resize(GraphicsFactory::SST_SCENE_BRIGHT_PASS_MAX);
		}

		/***************************/
		/* パイプライン : Sampling */
		/***************************/

		{
			GraphicsFactory::Stage& stage = m_Stages[GraphicsFactory::ST_SCENE_BRIGHT_PASS];
			GraphicsFactory::StageSubpass& subpass = stage.subpasses[GraphicsFactory::SST_SCENE_BRIGHT_PASS_SAMPLING];

			subpass.pipelineType = GraphicsFactory::PT_BRIGHT_PASS;

			GraphicsPipelineDesc& pipelineDesc = subpass.pipelineDesc;
			pipelineDesc.Allocate(2, 1, 1);
			pipelineDesc.vertexShader.pModule = m_ShaderModules[GraphicsFactory::SMT_SIMPLE_VERT];
			pipelineDesc.vertexShader.pEntryPointName = "main";
			pipelineDesc.fragmentShader.pModule = m_ShaderModules[GraphicsFactory::SMT_BRIGHT_PASS_FRAG];
			pipelineDesc.fragmentShader.pEntryPointName = "main";
			pipelineDesc.vertexInput.pElements[0].format = V3D_FORMAT_R32G32B32A32_SFLOAT;
			pipelineDesc.vertexInput.pElements[0].location = 0;
			pipelineDesc.vertexInput.pElements[0].offset = 0;
			pipelineDesc.vertexInput.pElements[1].format = V3D_FORMAT_R32G32_SFLOAT;
			pipelineDesc.vertexInput.pElements[1].location = 1;
			pipelineDesc.vertexInput.pElements[1].offset = 16;
			pipelineDesc.vertexInput.pLayouts[0].binding = 0;
			pipelineDesc.vertexInput.pLayouts[0].firstElement = 0;
			pipelineDesc.vertexInput.pLayouts[0].elementCount = 2;
			pipelineDesc.vertexInput.pLayouts[0].stride = sizeof(SimpleVertex);
			pipelineDesc.primitiveTopology = V3D_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			pipelineDesc.rasterization.polygonMode = V3D_POLYGON_MODE_FILL;
			pipelineDesc.rasterization.cullMode = V3D_CULL_MODE_NONE;
			pipelineDesc.depthStencil.depthTestEnable = false;
			pipelineDesc.depthStencil.depthWriteEnable = false;
			pipelineDesc.depthStencil.depthCompareOp = V3D_COMPARE_OP_ALWAYS;
			pipelineDesc.colorBlend.pAttachments[0] = InitializeColorBlendAttachment(BLEND_MODE_COPY);
			pipelineDesc.subpass = 0;

			subpass.pipelineHandle = std::make_shared<PipelineHandle>();
		}

		// ----------------------------------------------------------------------------------------------------
		// Blur
		// ----------------------------------------------------------------------------------------------------

		{
			GraphicsFactory::Stage& stage = m_Stages[GraphicsFactory::ST_SCENE_BLUR];
			VE_DEBUG_CODE(stage.debugName = L"BlurStage");

			/****************/
			/* レンダーパス */
			/****************/

			RenderPassDesc& renderPassDesc = stage.renderPassDesc;
			renderPassDesc.Allocate(2, 3, 3);

			renderPassDesc.pAttachments[0].format = m_Attachments[GraphicsFactory::AT_BL_HALF_0].imageDesc.format;
			renderPassDesc.pAttachments[0].samples = m_Attachments[GraphicsFactory::AT_BL_HALF_0].imageDesc.samples;
			renderPassDesc.pAttachments[0].loadOp = V3D_ATTACHMENT_LOAD_OP_UNDEFINED;
			renderPassDesc.pAttachments[0].storeOp = V3D_ATTACHMENT_STORE_OP_STORE;
			renderPassDesc.pAttachments[0].stencilLoadOp = V3D_ATTACHMENT_LOAD_OP_UNDEFINED;
			renderPassDesc.pAttachments[0].stencilStoreOp = V3D_ATTACHMENT_STORE_OP_UNDEFINED;
			renderPassDesc.pAttachments[0].initialLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			renderPassDesc.pAttachments[0].finalLayout = V3D_IMAGE_LAYOUT_SHADER_READ_ONLY;

			renderPassDesc.pAttachments[1].format = m_Attachments[GraphicsFactory::AT_BL_HALF_1].imageDesc.format;
			renderPassDesc.pAttachments[1].samples = m_Attachments[GraphicsFactory::AT_BL_HALF_1].imageDesc.samples;
			renderPassDesc.pAttachments[1].loadOp = V3D_ATTACHMENT_LOAD_OP_UNDEFINED;
			renderPassDesc.pAttachments[1].storeOp = V3D_ATTACHMENT_STORE_OP_UNDEFINED;
			renderPassDesc.pAttachments[1].stencilLoadOp = V3D_ATTACHMENT_LOAD_OP_UNDEFINED;
			renderPassDesc.pAttachments[1].stencilStoreOp = V3D_ATTACHMENT_STORE_OP_UNDEFINED;
			renderPassDesc.pAttachments[1].initialLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			renderPassDesc.pAttachments[1].finalLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;

			// サブパス 0 : DownSampling
			RenderPassDesc::Subpass* pSubpass = renderPassDesc.AllocateSubpass(0, 0, 1, false, 0);
			pSubpass->colorAttachments[0].attachment = 0;
			pSubpass->colorAttachments[0].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;

			// サブパス 0 : NorizonalBlur
			pSubpass = renderPassDesc.AllocateSubpass(1, 1, 1, false, 0);
			pSubpass->inputAttachments[0].attachment = 0;
			pSubpass->inputAttachments[0].layout = V3D_IMAGE_LAYOUT_SHADER_READ_ONLY;
			pSubpass->colorAttachments[0].attachment = 1;
			pSubpass->colorAttachments[0].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;

			// サブパス 0 : VerticalBlur
			pSubpass = renderPassDesc.AllocateSubpass(2, 1, 1, false, 0);
			pSubpass->inputAttachments[0].attachment = 1;
			pSubpass->inputAttachments[0].layout = V3D_IMAGE_LAYOUT_SHADER_READ_ONLY;
			pSubpass->colorAttachments[0].attachment = 0;
			pSubpass->colorAttachments[0].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;

			renderPassDesc.pSubpassDependencies[0].srcSubpass = 0;
			renderPassDesc.pSubpassDependencies[0].dstSubpass = 1;
			renderPassDesc.pSubpassDependencies[0].srcStageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
			renderPassDesc.pSubpassDependencies[0].dstStageMask = V3D_PIPELINE_STAGE_FRAGMENT_SHADER;
			renderPassDesc.pSubpassDependencies[0].srcAccessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
			renderPassDesc.pSubpassDependencies[0].dstAccessMask = V3D_ACCESS_SHADER_READ;
			renderPassDesc.pSubpassDependencies[0].dependencyFlags = 0;

			renderPassDesc.pSubpassDependencies[1].srcSubpass = 1;
			renderPassDesc.pSubpassDependencies[1].dstSubpass = 2;
			renderPassDesc.pSubpassDependencies[1].srcStageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
			renderPassDesc.pSubpassDependencies[1].dstStageMask = V3D_PIPELINE_STAGE_FRAGMENT_SHADER;
			renderPassDesc.pSubpassDependencies[1].srcAccessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
			renderPassDesc.pSubpassDependencies[1].dstAccessMask = V3D_ACCESS_SHADER_READ;
			renderPassDesc.pSubpassDependencies[1].dependencyFlags = 0;

			renderPassDesc.pSubpassDependencies[2].srcSubpass = 2;
			renderPassDesc.pSubpassDependencies[2].dstSubpass = V3D_SUBPASS_EXTERNAL;
			renderPassDesc.pSubpassDependencies[2].srcStageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
			renderPassDesc.pSubpassDependencies[2].dstStageMask = V3D_PIPELINE_STAGE_FRAGMENT_SHADER;
			renderPassDesc.pSubpassDependencies[2].srcAccessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
			renderPassDesc.pSubpassDependencies[2].dstAccessMask = V3D_ACCESS_SHADER_READ;
			renderPassDesc.pSubpassDependencies[2].dependencyFlags = 0;

			/**********************/
			/* フレームバッファー */
			/**********************/

			stage.frameBuffers.resize(2);

			// Half
			stage.frameBuffers[0].attachmentIndices.resize(2);
			stage.frameBuffers[0].attachmentIndices[0] = GraphicsFactory::AT_BL_HALF_0;
			stage.frameBuffers[0].attachmentIndices[1] = GraphicsFactory::AT_BL_HALF_1;
			stage.frameBuffers[0].handle = std::make_shared<FrameBufferHandle>();
			stage.frameBuffers[0].handle->m_FrameBuffers.resize(frameCount, nullptr);

			// Quarter
			stage.frameBuffers[1].attachmentIndices.resize(2);
			stage.frameBuffers[1].attachmentIndices[0] = GraphicsFactory::AT_BL_QUARTER_0;
			stage.frameBuffers[1].attachmentIndices[1] = GraphicsFactory::AT_BL_QUARTER_1;
			stage.frameBuffers[1].handle = std::make_shared<FrameBufferHandle>();
			stage.frameBuffers[1].handle->m_FrameBuffers.resize(frameCount, nullptr);

			/****************/
			/* パイプライン */
			/****************/

			stage.subpasses.resize(GraphicsFactory::SST_SCENE_BLUR_MAX);
		}

		/*******************************/
		/* パイプライン : DownSampling */
		/*******************************/

		{
			GraphicsFactory::Stage& stage = m_Stages[GraphicsFactory::ST_SCENE_BLUR];
			GraphicsFactory::StageSubpass& subpass = stage.subpasses[GraphicsFactory::SST_SCENE_BLUR_DOWN_SAMPLING];

			subpass.pipelineType = GraphicsFactory::PT_DOWN_SAMPLING;

			GraphicsPipelineDesc& pipelineDesc = subpass.pipelineDesc;
			pipelineDesc.Allocate(2, 1, 1);
			pipelineDesc.vertexShader.pModule = m_ShaderModules[GraphicsFactory::SMT_SIMPLE_VERT];
			pipelineDesc.vertexShader.pEntryPointName = "main";
			pipelineDesc.fragmentShader.pModule = m_ShaderModules[GraphicsFactory::SMT_DOWN_SAMPLING_2x2];
			pipelineDesc.fragmentShader.pEntryPointName = "main";
			pipelineDesc.vertexInput.pElements[0].format = V3D_FORMAT_R32G32B32A32_SFLOAT;
			pipelineDesc.vertexInput.pElements[0].location = 0;
			pipelineDesc.vertexInput.pElements[0].offset = 0;
			pipelineDesc.vertexInput.pElements[1].format = V3D_FORMAT_R32G32_SFLOAT;
			pipelineDesc.vertexInput.pElements[1].location = 1;
			pipelineDesc.vertexInput.pElements[1].offset = 16;
			pipelineDesc.vertexInput.pLayouts[0].binding = 0;
			pipelineDesc.vertexInput.pLayouts[0].firstElement = 0;
			pipelineDesc.vertexInput.pLayouts[0].elementCount = 2;
			pipelineDesc.vertexInput.pLayouts[0].stride = sizeof(SimpleVertex);
			pipelineDesc.primitiveTopology = V3D_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			pipelineDesc.rasterization.polygonMode = V3D_POLYGON_MODE_FILL;
			pipelineDesc.rasterization.cullMode = V3D_CULL_MODE_NONE;
			pipelineDesc.depthStencil.depthTestEnable = false;
			pipelineDesc.depthStencil.depthWriteEnable = false;
			pipelineDesc.depthStencil.depthCompareOp = V3D_COMPARE_OP_ALWAYS;
			pipelineDesc.colorBlend.pAttachments[0] = InitializeColorBlendAttachment(BLEND_MODE_COPY);
			pipelineDesc.subpass = 0;

			subpass.pipelineHandle = std::make_shared<PipelineHandle>();
		}

		/****************************/
		/* パイプライン : Horizonal */
		/****************************/

		{
			GraphicsFactory::Stage& stage = m_Stages[GraphicsFactory::ST_SCENE_BLUR];
			GraphicsFactory::StageSubpass& subpass = stage.subpasses[GraphicsFactory::SST_SCENE_BLUR_HORIZONAL];

			subpass.pipelineType = GraphicsFactory::PT_GAUSSIAN_BLUR;

			GraphicsPipelineDesc& pipelineDesc = subpass.pipelineDesc;
			pipelineDesc.Allocate(2, 1, 1);
			pipelineDesc.vertexShader.pModule = m_ShaderModules[GraphicsFactory::SMT_SIMPLE_VERT];
			pipelineDesc.vertexShader.pEntryPointName = "main";
			pipelineDesc.fragmentShader.pModule = m_ShaderModules[GraphicsFactory::SMT_GAUSSIAN_BLUR];
			pipelineDesc.fragmentShader.pEntryPointName = "main";
			pipelineDesc.vertexInput.pElements[0].format = V3D_FORMAT_R32G32B32A32_SFLOAT;
			pipelineDesc.vertexInput.pElements[0].location = 0;
			pipelineDesc.vertexInput.pElements[0].offset = 0;
			pipelineDesc.vertexInput.pElements[1].format = V3D_FORMAT_R32G32_SFLOAT;
			pipelineDesc.vertexInput.pElements[1].location = 1;
			pipelineDesc.vertexInput.pElements[1].offset = 16;
			pipelineDesc.vertexInput.pLayouts[0].binding = 0;
			pipelineDesc.vertexInput.pLayouts[0].firstElement = 0;
			pipelineDesc.vertexInput.pLayouts[0].elementCount = 2;
			pipelineDesc.vertexInput.pLayouts[0].stride = sizeof(SimpleVertex);
			pipelineDesc.primitiveTopology = V3D_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			pipelineDesc.rasterization.polygonMode = V3D_POLYGON_MODE_FILL;
			pipelineDesc.rasterization.cullMode = V3D_CULL_MODE_NONE;
			pipelineDesc.depthStencil.depthTestEnable = false;
			pipelineDesc.depthStencil.depthWriteEnable = false;
			pipelineDesc.depthStencil.depthCompareOp = V3D_COMPARE_OP_ALWAYS;
			pipelineDesc.colorBlend.pAttachments[0] = InitializeColorBlendAttachment(BLEND_MODE_COPY);
			pipelineDesc.subpass = 1;

			subpass.pipelineHandle = std::make_shared<PipelineHandle>();
		}

		/***************************/
		/* パイプライン : Vertical */
		/***************************/

		{
			GraphicsFactory::Stage& stage = m_Stages[GraphicsFactory::ST_SCENE_BLUR];
			GraphicsFactory::StageSubpass& subpass = stage.subpasses[GraphicsFactory::SST_SCENE_BLUR_VERTICAL];

			subpass.pipelineType = GraphicsFactory::PT_GAUSSIAN_BLUR;

			GraphicsPipelineDesc& pipelineDesc = subpass.pipelineDesc;
			pipelineDesc.Allocate(2, 1, 1);
			pipelineDesc.vertexShader.pModule = m_ShaderModules[GraphicsFactory::SMT_SIMPLE_VERT];
			pipelineDesc.vertexShader.pEntryPointName = "main";
			pipelineDesc.fragmentShader.pModule = m_ShaderModules[GraphicsFactory::SMT_GAUSSIAN_BLUR];
			pipelineDesc.fragmentShader.pEntryPointName = "main";
			pipelineDesc.vertexInput.pElements[0].format = V3D_FORMAT_R32G32B32A32_SFLOAT;
			pipelineDesc.vertexInput.pElements[0].location = 0;
			pipelineDesc.vertexInput.pElements[0].offset = 0;
			pipelineDesc.vertexInput.pElements[1].format = V3D_FORMAT_R32G32_SFLOAT;
			pipelineDesc.vertexInput.pElements[1].location = 1;
			pipelineDesc.vertexInput.pElements[1].offset = 16;
			pipelineDesc.vertexInput.pLayouts[0].binding = 0;
			pipelineDesc.vertexInput.pLayouts[0].firstElement = 0;
			pipelineDesc.vertexInput.pLayouts[0].elementCount = 2;
			pipelineDesc.vertexInput.pLayouts[0].stride = sizeof(SimpleVertex);
			pipelineDesc.primitiveTopology = V3D_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			pipelineDesc.rasterization.polygonMode = V3D_POLYGON_MODE_FILL;
			pipelineDesc.rasterization.cullMode = V3D_CULL_MODE_NONE;
			pipelineDesc.depthStencil.depthTestEnable = false;
			pipelineDesc.depthStencil.depthWriteEnable = false;
			pipelineDesc.depthStencil.depthCompareOp = V3D_COMPARE_OP_ALWAYS;
			pipelineDesc.colorBlend.pAttachments[0] = InitializeColorBlendAttachment(BLEND_MODE_COPY);
			pipelineDesc.subpass = 2;

			subpass.pipelineHandle = std::make_shared<PipelineHandle>();
		}

		// ----------------------------------------------------------------------------------------------------
		// Geometry
		// ----------------------------------------------------------------------------------------------------

		{
			GraphicsFactory::Stage& stage = m_Stages[GraphicsFactory::ST_SCENE_GEOMETRY];
			VE_DEBUG_CODE(stage.debugName = L"GeometryStage");

			/****************/
			/* レンダーパス */
			/****************/

			RenderPassDesc& renderPassDesc = stage.renderPassDesc;
			renderPassDesc.Allocate(5, 2, 2);

			renderPassDesc.pAttachments[0].format = m_Attachments[GraphicsFactory::AT_GE_COLOR].imageDesc.format;
			renderPassDesc.pAttachments[0].samples = m_Attachments[GraphicsFactory::AT_GE_COLOR].imageDesc.samples;
			renderPassDesc.pAttachments[0].loadOp = V3D_ATTACHMENT_LOAD_OP_CLEAR;
			renderPassDesc.pAttachments[0].storeOp = V3D_ATTACHMENT_STORE_OP_STORE;
			renderPassDesc.pAttachments[0].stencilLoadOp = V3D_ATTACHMENT_LOAD_OP_UNDEFINED;
			renderPassDesc.pAttachments[0].stencilStoreOp = V3D_ATTACHMENT_STORE_OP_UNDEFINED;
			renderPassDesc.pAttachments[0].initialLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			renderPassDesc.pAttachments[0].finalLayout = V3D_IMAGE_LAYOUT_SHADER_READ_ONLY;
			renderPassDesc.pAttachments[0].clearValue.color.float32[0] = 0.1f;
			renderPassDesc.pAttachments[0].clearValue.color.float32[1] = 0.1f;
			renderPassDesc.pAttachments[0].clearValue.color.float32[2] = 0.1f;
			renderPassDesc.pAttachments[0].clearValue.color.float32[3] = 1.0f;

			renderPassDesc.pAttachments[1].format = m_Attachments[GraphicsFactory::AT_GE_BUFFER_0].imageDesc.format;
			renderPassDesc.pAttachments[1].samples = m_Attachments[GraphicsFactory::AT_GE_BUFFER_0].imageDesc.samples;
			renderPassDesc.pAttachments[1].loadOp = V3D_ATTACHMENT_LOAD_OP_CLEAR;
			renderPassDesc.pAttachments[1].storeOp = V3D_ATTACHMENT_STORE_OP_STORE;
			renderPassDesc.pAttachments[1].stencilLoadOp = V3D_ATTACHMENT_LOAD_OP_UNDEFINED;
			renderPassDesc.pAttachments[1].stencilStoreOp = V3D_ATTACHMENT_STORE_OP_UNDEFINED;
			renderPassDesc.pAttachments[1].initialLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			renderPassDesc.pAttachments[1].finalLayout = V3D_IMAGE_LAYOUT_SHADER_READ_ONLY;
			renderPassDesc.pAttachments[1].clearValue = V3DClearValue{};

			renderPassDesc.pAttachments[2].format = m_Attachments[GraphicsFactory::AT_GE_BUFFER_1].imageDesc.format;
			renderPassDesc.pAttachments[2].samples = m_Attachments[GraphicsFactory::AT_GE_BUFFER_1].imageDesc.samples;
			renderPassDesc.pAttachments[2].loadOp = V3D_ATTACHMENT_LOAD_OP_CLEAR;
			renderPassDesc.pAttachments[2].storeOp = V3D_ATTACHMENT_STORE_OP_STORE;
			renderPassDesc.pAttachments[2].stencilLoadOp = V3D_ATTACHMENT_LOAD_OP_UNDEFINED;
			renderPassDesc.pAttachments[2].stencilStoreOp = V3D_ATTACHMENT_STORE_OP_UNDEFINED;
			renderPassDesc.pAttachments[2].initialLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			renderPassDesc.pAttachments[2].finalLayout = V3D_IMAGE_LAYOUT_SHADER_READ_ONLY;
			renderPassDesc.pAttachments[2].clearValue = V3DClearValue{};

			renderPassDesc.pAttachments[3].format = m_Attachments[GraphicsFactory::AT_GE_SELECT].imageDesc.format;
			renderPassDesc.pAttachments[3].samples = m_Attachments[GraphicsFactory::AT_GE_SELECT].imageDesc.samples;
			renderPassDesc.pAttachments[3].loadOp = V3D_ATTACHMENT_LOAD_OP_CLEAR;
			renderPassDesc.pAttachments[3].storeOp = V3D_ATTACHMENT_STORE_OP_STORE;
			renderPassDesc.pAttachments[3].stencilLoadOp = V3D_ATTACHMENT_LOAD_OP_UNDEFINED;
			renderPassDesc.pAttachments[3].stencilStoreOp = V3D_ATTACHMENT_STORE_OP_UNDEFINED;
			renderPassDesc.pAttachments[3].initialLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			renderPassDesc.pAttachments[3].finalLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			renderPassDesc.pAttachments[3].clearValue = V3DClearValue{};

			renderPassDesc.pAttachments[4].format = m_Attachments[GraphicsFactory::AT_GE_DEPTH].imageDesc.format;
			renderPassDesc.pAttachments[4].samples = m_Attachments[GraphicsFactory::AT_GE_DEPTH].imageDesc.samples;
			renderPassDesc.pAttachments[4].loadOp = V3D_ATTACHMENT_LOAD_OP_CLEAR;
			renderPassDesc.pAttachments[4].storeOp = V3D_ATTACHMENT_STORE_OP_STORE;
			renderPassDesc.pAttachments[4].stencilLoadOp = V3D_ATTACHMENT_LOAD_OP_UNDEFINED;
			renderPassDesc.pAttachments[4].stencilStoreOp = V3D_ATTACHMENT_STORE_OP_UNDEFINED;
			renderPassDesc.pAttachments[4].initialLayout = V3D_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT;
			renderPassDesc.pAttachments[4].finalLayout = V3D_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT;
			renderPassDesc.pAttachments[4].clearValue.depthStencil.depth = 1.0f;
			renderPassDesc.pAttachments[4].clearValue.depthStencil.stencil = 0;

			// サブパス 0 - Grid
			RenderPassDesc::Subpass* pSubpass = renderPassDesc.AllocateSubpass(0, 0, 1, true, 0);
			pSubpass->colorAttachments[0].attachment = 0;
			pSubpass->colorAttachments[0].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			pSubpass->depthStencilAttachment.attachment = 4;
			pSubpass->depthStencilAttachment.layout = V3D_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT;

			// サブパス 1 - Model
			pSubpass = renderPassDesc.AllocateSubpass(1, 0, 4, true, 0);
			pSubpass->colorAttachments[0].attachment = 0;
			pSubpass->colorAttachments[0].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			pSubpass->colorAttachments[1].attachment = 1;
			pSubpass->colorAttachments[1].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			pSubpass->colorAttachments[2].attachment = 2;
			pSubpass->colorAttachments[2].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			pSubpass->colorAttachments[3].attachment = 3;
			pSubpass->colorAttachments[3].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			pSubpass->depthStencilAttachment.attachment = 4;
			pSubpass->depthStencilAttachment.layout = V3D_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT;

			renderPassDesc.pSubpassDependencies[0].srcSubpass = 0;
			renderPassDesc.pSubpassDependencies[0].dstSubpass = 1;
			renderPassDesc.pSubpassDependencies[0].srcStageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
			renderPassDesc.pSubpassDependencies[0].dstStageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
			renderPassDesc.pSubpassDependencies[0].srcAccessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
			renderPassDesc.pSubpassDependencies[0].dstAccessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
			renderPassDesc.pSubpassDependencies[0].dependencyFlags = V3D_DEPENDENCY_BY_REGION;

			renderPassDesc.pSubpassDependencies[1].srcSubpass = 1;
			renderPassDesc.pSubpassDependencies[1].dstSubpass = V3D_SUBPASS_EXTERNAL;
			renderPassDesc.pSubpassDependencies[1].srcStageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
			renderPassDesc.pSubpassDependencies[1].dstStageMask = V3D_PIPELINE_STAGE_FRAGMENT_SHADER;
			renderPassDesc.pSubpassDependencies[1].srcAccessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
			renderPassDesc.pSubpassDependencies[1].dstAccessMask = V3D_ACCESS_SHADER_READ;
			renderPassDesc.pSubpassDependencies[1].dependencyFlags = 0;

			/**********************/
			/* フレームバッファー */
			/**********************/

			stage.frameBuffers.resize(1);
			stage.frameBuffers[0].attachmentIndices.resize(5);
			stage.frameBuffers[0].attachmentIndices[0] = GraphicsFactory::AT_GE_COLOR;
			stage.frameBuffers[0].attachmentIndices[1] = GraphicsFactory::AT_GE_BUFFER_0;
			stage.frameBuffers[0].attachmentIndices[2] = GraphicsFactory::AT_GE_BUFFER_1;
			stage.frameBuffers[0].attachmentIndices[3] = GraphicsFactory::AT_GE_SELECT;
			stage.frameBuffers[0].attachmentIndices[4] = GraphicsFactory::AT_GE_DEPTH;

			stage.frameBuffers[0].handle = std::make_shared<FrameBufferHandle>();
			stage.frameBuffers[0].handle->m_FrameBuffers.resize(frameCount, nullptr);

			/****************/
			/* パイプライン */
			/****************/

			stage.subpasses.resize(GraphicsFactory::SST_SCENE_GEOMETRY_MAX);
		}

		/***********************/
		/* パイプライン - Grid */
		/***********************/

		{
			GraphicsFactory::Stage& stage = m_Stages[GraphicsFactory::ST_SCENE_GEOMETRY];
			GraphicsFactory::StageSubpass& subpass = stage.subpasses[GraphicsFactory::SST_SCENE_GEOMETRY_GRID];

			subpass.pipelineType = GraphicsFactory::PT_GRID;
		
			GraphicsPipelineDesc& pipelineDesc = subpass.pipelineDesc;
			pipelineDesc.Allocate(1, 1, 1);
			pipelineDesc.vertexShader.pModule = m_ShaderModules[GraphicsFactory::SMT_GRID_VERT];
			pipelineDesc.vertexShader.pEntryPointName = "main";
			pipelineDesc.fragmentShader.pModule = m_ShaderModules[GraphicsFactory::SMT_GRID_FRAG];
			pipelineDesc.fragmentShader.pEntryPointName = "main";
			pipelineDesc.vertexInput.pElements[0].format = V3D_FORMAT_R32G32B32A32_SFLOAT;
			pipelineDesc.vertexInput.pElements[0].location = 0;
			pipelineDesc.vertexInput.pElements[0].offset = 0;
			pipelineDesc.vertexInput.pLayouts[0].binding = 0;
			pipelineDesc.vertexInput.pLayouts[0].firstElement = 0;
			pipelineDesc.vertexInput.pLayouts[0].elementCount = 1;
			pipelineDesc.vertexInput.pLayouts[0].stride = sizeof(glm::vec4);
			pipelineDesc.primitiveTopology = V3D_PRIMITIVE_TOPOLOGY_LINE_LIST;
			pipelineDesc.rasterization.polygonMode = V3D_POLYGON_MODE_LINE;
			pipelineDesc.rasterization.cullMode = V3D_CULL_MODE_NONE;
			pipelineDesc.depthStencil.depthTestEnable = false;
			pipelineDesc.depthStencil.depthWriteEnable = true;
			pipelineDesc.depthStencil.depthCompareOp = V3D_COMPARE_OP_LESS;
			pipelineDesc.colorBlend.pAttachments[0] = InitializeColorBlendAttachment(BLEND_MODE_COPY);
			pipelineDesc.subpass = 0;

			subpass.pipelineHandle = std::make_shared<PipelineHandle>();
		}

		// ----------------------------------------------------------------------------------------------------
		// SSAO
		// ----------------------------------------------------------------------------------------------------

		{
			GraphicsFactory::Stage& stage = m_Stages[GraphicsFactory::ST_SCENE_SSAO];
			VE_DEBUG_CODE(stage.debugName = L"SsaoStage");

			/****************/
			/* レンダーパス */
			/****************/

			RenderPassDesc& renderPassDesc = stage.renderPassDesc;
			renderPassDesc.Allocate(1, 1, 1);

			renderPassDesc.pAttachments[0].format = m_Attachments[GraphicsFactory::AT_SS_COLOR].imageDesc.format;
			renderPassDesc.pAttachments[0].samples = m_Attachments[GraphicsFactory::AT_SS_COLOR].imageDesc.samples;
			renderPassDesc.pAttachments[0].loadOp = V3D_ATTACHMENT_LOAD_OP_CLEAR;
			renderPassDesc.pAttachments[0].storeOp = V3D_ATTACHMENT_STORE_OP_STORE;
			renderPassDesc.pAttachments[0].stencilLoadOp = V3D_ATTACHMENT_LOAD_OP_UNDEFINED;
			renderPassDesc.pAttachments[0].stencilStoreOp = V3D_ATTACHMENT_STORE_OP_UNDEFINED;
			renderPassDesc.pAttachments[0].initialLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			renderPassDesc.pAttachments[0].finalLayout = V3D_IMAGE_LAYOUT_SHADER_READ_ONLY;
			renderPassDesc.pAttachments[0].clearValue.color.float32[0] = 1.0f;
			renderPassDesc.pAttachments[0].clearValue.color.float32[1] = 1.0f;
			renderPassDesc.pAttachments[0].clearValue.color.float32[2] = 1.0f;
			renderPassDesc.pAttachments[0].clearValue.color.float32[3] = 1.0f;

			// サブパス 0
			RenderPassDesc::Subpass* pSubpass = renderPassDesc.AllocateSubpass(0, 0, 1, false, 0);
			pSubpass->colorAttachments[0].attachment = 0;
			pSubpass->colorAttachments[0].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;

			renderPassDesc.pSubpassDependencies[0].srcSubpass = 0;
			renderPassDesc.pSubpassDependencies[0].dstSubpass = V3D_SUBPASS_EXTERNAL;
			renderPassDesc.pSubpassDependencies[0].srcStageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
			renderPassDesc.pSubpassDependencies[0].dstStageMask = V3D_PIPELINE_STAGE_FRAGMENT_SHADER;
			renderPassDesc.pSubpassDependencies[0].srcAccessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
			renderPassDesc.pSubpassDependencies[0].dstAccessMask = V3D_ACCESS_SHADER_READ;
			renderPassDesc.pSubpassDependencies[0].dependencyFlags = 0;

			/**********************/
			/* フレームバッファー */
			/**********************/

			stage.frameBuffers.resize(1);
			stage.frameBuffers[0].attachmentIndices.resize(1);
			stage.frameBuffers[0].attachmentIndices[0] = GraphicsFactory::AT_SS_COLOR;

			stage.frameBuffers[0].handle = std::make_shared<FrameBufferHandle>();
			stage.frameBuffers[0].handle->m_FrameBuffers.resize(frameCount, nullptr);

			/****************/
			/* パイプライン */
			/****************/

			stage.subpasses.resize(GraphicsFactory::SST_SCENE_SSAO_MAX);
		}

		/***************************/
		/* パイプライン - Sampling */
		/***************************/

		{
			GraphicsFactory::Stage& stage = m_Stages[GraphicsFactory::ST_SCENE_SSAO];
			GraphicsFactory::StageSubpass& subpass = stage.subpasses[GraphicsFactory::SST_SCENE_SSAO_SAMPLING];

			subpass.pipelineType = GraphicsFactory::PT_SSAO;

			GraphicsPipelineDesc& pipelineDesc = subpass.pipelineDesc;
			pipelineDesc.Allocate(2, 1, 1);
			pipelineDesc.vertexShader.pModule = m_ShaderModules[GraphicsFactory::SMT_SIMPLE_VERT];
			pipelineDesc.vertexShader.pEntryPointName = "main";
			pipelineDesc.fragmentShader.pModule = m_ShaderModules[GraphicsFactory::SMT_SSAO_FRAG];
			pipelineDesc.fragmentShader.pEntryPointName = "main";
			pipelineDesc.vertexInput.pElements[0].format = V3D_FORMAT_R32G32B32A32_SFLOAT;
			pipelineDesc.vertexInput.pElements[0].location = 0;
			pipelineDesc.vertexInput.pElements[0].offset = 0;
			pipelineDesc.vertexInput.pElements[1].format = V3D_FORMAT_R32G32_SFLOAT;
			pipelineDesc.vertexInput.pElements[1].location = 1;
			pipelineDesc.vertexInput.pElements[1].offset = 16;
			pipelineDesc.vertexInput.pLayouts[0].binding = 0;
			pipelineDesc.vertexInput.pLayouts[0].firstElement = 0;
			pipelineDesc.vertexInput.pLayouts[0].elementCount = 2;
			pipelineDesc.vertexInput.pLayouts[0].stride = sizeof(SimpleVertex);
			pipelineDesc.primitiveTopology = V3D_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			pipelineDesc.rasterization.polygonMode = V3D_POLYGON_MODE_FILL;
			pipelineDesc.rasterization.cullMode = V3D_CULL_MODE_NONE;
			pipelineDesc.depthStencil.depthTestEnable = false;
			pipelineDesc.depthStencil.depthWriteEnable = false;
			pipelineDesc.depthStencil.depthCompareOp = V3D_COMPARE_OP_ALWAYS;
			pipelineDesc.colorBlend.pAttachments[0] = InitializeColorBlendAttachment(BLEND_MODE_COPY);
			pipelineDesc.subpass = 0;

			subpass.pipelineHandle = std::make_shared<PipelineHandle>();
		}

		// ----------------------------------------------------------------------------------------------------
		// Shadow
		// ----------------------------------------------------------------------------------------------------

		{
			GraphicsFactory::Stage& stage = m_Stages[GraphicsFactory::ST_SCENE_SHADOW];
			VE_DEBUG_CODE(stage.debugName = L"ShadowStage");

			/****************/
			/* レンダーパス */
			/****************/

			RenderPassDesc& renderPassDesc = stage.renderPassDesc;
			renderPassDesc.Allocate(2, 1, 1);

			renderPassDesc.pAttachments[0].format = m_Attachments[GraphicsFactory::AT_SA_BUFFER].imageDesc.format;
			renderPassDesc.pAttachments[0].samples = m_Attachments[GraphicsFactory::AT_SA_BUFFER].imageDesc.samples;
			renderPassDesc.pAttachments[0].loadOp = V3D_ATTACHMENT_LOAD_OP_CLEAR;
			renderPassDesc.pAttachments[0].storeOp = V3D_ATTACHMENT_STORE_OP_STORE;
			renderPassDesc.pAttachments[0].stencilLoadOp = V3D_ATTACHMENT_LOAD_OP_UNDEFINED;
			renderPassDesc.pAttachments[0].stencilStoreOp = V3D_ATTACHMENT_STORE_OP_UNDEFINED;
			renderPassDesc.pAttachments[0].initialLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			renderPassDesc.pAttachments[0].finalLayout = V3D_IMAGE_LAYOUT_SHADER_READ_ONLY;
			renderPassDesc.pAttachments[0].clearValue.color.float32[0] = 1.0f;
			renderPassDesc.pAttachments[0].clearValue.color.float32[1] = 1.0f;
			renderPassDesc.pAttachments[0].clearValue.color.float32[2] = 1.0f;
			renderPassDesc.pAttachments[0].clearValue.color.float32[3] = 1.0f;

			renderPassDesc.pAttachments[1].format = m_Attachments[GraphicsFactory::AT_SA_DEPTH].imageDesc.format;
			renderPassDesc.pAttachments[1].samples = m_Attachments[GraphicsFactory::AT_SA_DEPTH].imageDesc.samples;
			renderPassDesc.pAttachments[1].loadOp = V3D_ATTACHMENT_LOAD_OP_CLEAR;
			renderPassDesc.pAttachments[1].storeOp = V3D_ATTACHMENT_STORE_OP_UNDEFINED;
			renderPassDesc.pAttachments[1].stencilLoadOp = V3D_ATTACHMENT_LOAD_OP_UNDEFINED;
			renderPassDesc.pAttachments[1].stencilStoreOp = V3D_ATTACHMENT_STORE_OP_UNDEFINED;
			renderPassDesc.pAttachments[1].initialLayout = V3D_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT;
			renderPassDesc.pAttachments[1].finalLayout = V3D_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT;
			renderPassDesc.pAttachments[1].clearValue.depthStencil.depth = 1.0f;
			renderPassDesc.pAttachments[1].clearValue.depthStencil.stencil = 0;

			// サブパス 0
			RenderPassDesc::Subpass* pSubpass = renderPassDesc.AllocateSubpass(0, 0, 1, true, 0);
			pSubpass->colorAttachments[0].attachment = 0;
			pSubpass->colorAttachments[0].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			pSubpass->depthStencilAttachment.attachment = 1;
			pSubpass->depthStencilAttachment.layout = V3D_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT;

			renderPassDesc.pSubpassDependencies[0].srcSubpass = 0;
			renderPassDesc.pSubpassDependencies[0].dstSubpass = V3D_SUBPASS_EXTERNAL;
			renderPassDesc.pSubpassDependencies[0].srcStageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
			renderPassDesc.pSubpassDependencies[0].dstStageMask = V3D_PIPELINE_STAGE_FRAGMENT_SHADER;
			renderPassDesc.pSubpassDependencies[0].srcAccessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
			renderPassDesc.pSubpassDependencies[0].dstAccessMask = V3D_ACCESS_SHADER_READ;
			renderPassDesc.pSubpassDependencies[0].dependencyFlags = 0;

			/**********************/
			/* フレームバッファー */
			/**********************/

			stage.frameBuffers.resize(1);
			stage.frameBuffers[0].attachmentIndices.resize(2);
			stage.frameBuffers[0].attachmentIndices[0] = GraphicsFactory::AT_SA_BUFFER;
			stage.frameBuffers[0].attachmentIndices[1] = GraphicsFactory::AT_SA_DEPTH;

			stage.frameBuffers[0].handle = std::make_shared<FrameBufferHandle>();
			stage.frameBuffers[0].handle->m_FrameBuffers.resize(frameCount, nullptr);

			/****************/
			/* パイプライン */
			/****************/

			stage.subpasses.resize(GraphicsFactory::SST_SCENE_SHADOW_MAX);
		}

		// ----------------------------------------------------------------------------------------------------
		// Lighting
		// ----------------------------------------------------------------------------------------------------

		{
			GraphicsFactory::Stage& stage = m_Stages[GraphicsFactory::ST_SCENE_LIGHTING];
			VE_DEBUG_CODE(stage.debugName = L"LightingStage");

			/****************/
			/* レンダーパス */
			/****************/

			RenderPassDesc& renderPassDesc = stage.renderPassDesc;
			renderPassDesc.Allocate(3, 2, 1);

			renderPassDesc.pAttachments[0].format = m_Attachments[GraphicsFactory::AT_LI_COLOR].imageDesc.format;
			renderPassDesc.pAttachments[0].samples = m_Attachments[GraphicsFactory::AT_LI_COLOR].imageDesc.samples;
			renderPassDesc.pAttachments[0].loadOp = V3D_ATTACHMENT_LOAD_OP_CLEAR;
			renderPassDesc.pAttachments[0].storeOp = V3D_ATTACHMENT_STORE_OP_UNDEFINED;
			renderPassDesc.pAttachments[0].stencilLoadOp = V3D_ATTACHMENT_LOAD_OP_UNDEFINED;
			renderPassDesc.pAttachments[0].stencilStoreOp = V3D_ATTACHMENT_STORE_OP_UNDEFINED;
			renderPassDesc.pAttachments[0].initialLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			renderPassDesc.pAttachments[0].finalLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			renderPassDesc.pAttachments[0].clearValue.color.float32[0] = 0.0f;
			renderPassDesc.pAttachments[0].clearValue.color.float32[1] = 0.0f;
			renderPassDesc.pAttachments[0].clearValue.color.float32[2] = 0.0f;
			renderPassDesc.pAttachments[0].clearValue.color.float32[3] = 0.0f;

			renderPassDesc.pAttachments[1].format = m_Attachments[GraphicsFactory::AT_GL_COLOR].imageDesc.format;
			renderPassDesc.pAttachments[1].samples = m_Attachments[GraphicsFactory::AT_GL_COLOR].imageDesc.samples;
			renderPassDesc.pAttachments[1].loadOp = V3D_ATTACHMENT_LOAD_OP_CLEAR;
			renderPassDesc.pAttachments[1].storeOp = V3D_ATTACHMENT_STORE_OP_STORE;
			renderPassDesc.pAttachments[1].stencilLoadOp = V3D_ATTACHMENT_LOAD_OP_UNDEFINED;
			renderPassDesc.pAttachments[1].stencilStoreOp = V3D_ATTACHMENT_STORE_OP_UNDEFINED;
			renderPassDesc.pAttachments[1].initialLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			renderPassDesc.pAttachments[1].finalLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			renderPassDesc.pAttachments[1].clearValue.color.float32[0] = 0.0f;
			renderPassDesc.pAttachments[1].clearValue.color.float32[1] = 0.0f;
			renderPassDesc.pAttachments[1].clearValue.color.float32[2] = 0.0f;
			renderPassDesc.pAttachments[1].clearValue.color.float32[3] = 0.0f;

			renderPassDesc.pAttachments[2].format = m_Attachments[GraphicsFactory::AT_SH_COLOR].imageDesc.format;
			renderPassDesc.pAttachments[2].samples = m_Attachments[GraphicsFactory::AT_SH_COLOR].imageDesc.samples;
			renderPassDesc.pAttachments[2].loadOp = V3D_ATTACHMENT_LOAD_OP_UNDEFINED;
			renderPassDesc.pAttachments[2].storeOp = V3D_ATTACHMENT_STORE_OP_STORE;
			renderPassDesc.pAttachments[2].stencilLoadOp = V3D_ATTACHMENT_LOAD_OP_UNDEFINED;
			renderPassDesc.pAttachments[2].stencilStoreOp = V3D_ATTACHMENT_STORE_OP_UNDEFINED;
			renderPassDesc.pAttachments[2].initialLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			renderPassDesc.pAttachments[2].finalLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			renderPassDesc.pAttachments[2].clearValue = V3DClearValue{};

			// サブパス 0 - DirectionalLighting
			RenderPassDesc::Subpass* pSubpass = renderPassDesc.AllocateSubpass(0, 0, 2, false, 0);
			pSubpass->colorAttachments[0].attachment = 0;
			pSubpass->colorAttachments[0].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			pSubpass->colorAttachments[1].attachment = 1;
			pSubpass->colorAttachments[1].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;

			// サブパス 1 - FinishLighting
			pSubpass = renderPassDesc.AllocateSubpass(1, 2, 1, false, 0);
			pSubpass->inputAttachments[0].attachment = 0;
			pSubpass->inputAttachments[0].layout = V3D_IMAGE_LAYOUT_SHADER_READ_ONLY;
			pSubpass->inputAttachments[1].attachment = 1;
			pSubpass->inputAttachments[1].layout = V3D_IMAGE_LAYOUT_SHADER_READ_ONLY;
			pSubpass->colorAttachments[0].attachment = 2;
			pSubpass->colorAttachments[0].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;

			renderPassDesc.pSubpassDependencies[0].srcSubpass = 0;
			renderPassDesc.pSubpassDependencies[0].dstSubpass = 1;
			renderPassDesc.pSubpassDependencies[0].srcStageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
			renderPassDesc.pSubpassDependencies[0].dstStageMask = V3D_PIPELINE_STAGE_FRAGMENT_SHADER;
			renderPassDesc.pSubpassDependencies[0].srcAccessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
			renderPassDesc.pSubpassDependencies[0].dstAccessMask = V3D_ACCESS_SHADER_READ;
			renderPassDesc.pSubpassDependencies[0].dependencyFlags = V3D_DEPENDENCY_BY_REGION;

			/**********************/
			/* フレームバッファー */
			/**********************/

			stage.frameBuffers.resize(1);
			stage.frameBuffers[0].attachmentIndices.resize(3);
			stage.frameBuffers[0].attachmentIndices[0] = GraphicsFactory::AT_LI_COLOR;
			stage.frameBuffers[0].attachmentIndices[1] = GraphicsFactory::AT_GL_COLOR;
			stage.frameBuffers[0].attachmentIndices[2] = GraphicsFactory::AT_SH_COLOR;

			stage.frameBuffers[0].handle = std::make_shared<FrameBufferHandle>();
			stage.frameBuffers[0].handle->m_FrameBuffers.resize(frameCount, nullptr);

			/****************/
			/* パイプライン */
			/****************/

			stage.subpasses.resize(GraphicsFactory::SST_SCENE_LIGHTING_MAX);
		}

		/**************************************/
		/* パイプライン - DirectionalLighting */
		/**************************************/

		{
			GraphicsFactory::Stage& stage = m_Stages[GraphicsFactory::ST_SCENE_LIGHTING];
			GraphicsFactory::StageSubpass& subpass = stage.subpasses[SST_SCENE_LIGHTING_DIRECTIONAL];

			subpass.pipelineType = GraphicsFactory::PT_DIRECTIONAL_LIGHTING;

			GraphicsPipelineDesc& pipelineDesc = subpass.pipelineDesc;
			pipelineDesc.Allocate(2, 1, 2);
			pipelineDesc.vertexShader.pModule = m_ShaderModules[GraphicsFactory::SMT_SIMPLE_VERT];
			pipelineDesc.vertexShader.pEntryPointName = "main";
			pipelineDesc.fragmentShader.pModule = m_ShaderModules[GraphicsFactory::SMT_DIRECTIONAL_LIGHTING];
			pipelineDesc.fragmentShader.pEntryPointName = "main";
			pipelineDesc.vertexInput.pElements[0].format = V3D_FORMAT_R32G32B32A32_SFLOAT;
			pipelineDesc.vertexInput.pElements[0].location = 0;
			pipelineDesc.vertexInput.pElements[0].offset = 0;
			pipelineDesc.vertexInput.pElements[1].format = V3D_FORMAT_R32G32_SFLOAT;
			pipelineDesc.vertexInput.pElements[1].location = 1;
			pipelineDesc.vertexInput.pElements[1].offset = 16;
			pipelineDesc.vertexInput.pLayouts[0].binding = 0;
			pipelineDesc.vertexInput.pLayouts[0].firstElement = 0;
			pipelineDesc.vertexInput.pLayouts[0].elementCount = 2;
			pipelineDesc.vertexInput.pLayouts[0].stride = sizeof(SimpleVertex);
			pipelineDesc.primitiveTopology = V3D_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			pipelineDesc.rasterization.polygonMode = V3D_POLYGON_MODE_FILL;
			pipelineDesc.rasterization.cullMode = V3D_CULL_MODE_NONE;
			pipelineDesc.depthStencil.depthTestEnable = false;
			pipelineDesc.depthStencil.depthWriteEnable = false;
			pipelineDesc.depthStencil.depthCompareOp = V3D_COMPARE_OP_ALWAYS;
			pipelineDesc.colorBlend.pAttachments[0] = InitializeColorBlendAttachment(BLEND_MODE_COPY);
			pipelineDesc.colorBlend.pAttachments[1] = InitializeColorBlendAttachment(BLEND_MODE_COPY);
			pipelineDesc.subpass = 0;

			subpass.pipelineHandle = std::make_shared<PipelineHandle>();
		}

		/*********************************/
		/* パイプライン - FinishLighting */
		/*********************************/

		{
			GraphicsFactory::Stage& stage = m_Stages[GraphicsFactory::ST_SCENE_LIGHTING];
			GraphicsFactory::StageSubpass& subpass = stage.subpasses[SST_SCENE_LIGHTING_FINISH];

			subpass.pipelineType = GraphicsFactory::PT_FINISH_LIGHTING;

			GraphicsPipelineDesc& pipelineDesc = subpass.pipelineDesc;
			pipelineDesc.Allocate(2, 1, 1);
			pipelineDesc.vertexShader.pModule = m_ShaderModules[GraphicsFactory::SMT_SIMPLE_VERT];
			pipelineDesc.vertexShader.pEntryPointName = "main";
			pipelineDesc.fragmentShader.pModule = m_ShaderModules[GraphicsFactory::SMT_FINISH_LIGHTING];
			pipelineDesc.fragmentShader.pEntryPointName = "main";
			pipelineDesc.vertexInput.pElements[0].format = V3D_FORMAT_R32G32B32A32_SFLOAT;
			pipelineDesc.vertexInput.pElements[0].location = 0;
			pipelineDesc.vertexInput.pElements[0].offset = 0;
			pipelineDesc.vertexInput.pElements[1].format = V3D_FORMAT_R32G32_SFLOAT;
			pipelineDesc.vertexInput.pElements[1].location = 1;
			pipelineDesc.vertexInput.pElements[1].offset = 16;
			pipelineDesc.vertexInput.pLayouts[0].binding = 0;
			pipelineDesc.vertexInput.pLayouts[0].firstElement = 0;
			pipelineDesc.vertexInput.pLayouts[0].elementCount = 2;
			pipelineDesc.vertexInput.pLayouts[0].stride = sizeof(SimpleVertex);
			pipelineDesc.primitiveTopology = V3D_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			pipelineDesc.rasterization.polygonMode = V3D_POLYGON_MODE_FILL;
			pipelineDesc.rasterization.cullMode = V3D_CULL_MODE_NONE;
			pipelineDesc.depthStencil.depthTestEnable = false;
			pipelineDesc.depthStencil.depthWriteEnable = false;
			pipelineDesc.depthStencil.depthCompareOp = V3D_COMPARE_OP_ALWAYS;
			pipelineDesc.colorBlend.pAttachments[0] = InitializeColorBlendAttachment(BLEND_MODE_COPY);
			pipelineDesc.subpass = 1;

			subpass.pipelineHandle = std::make_shared<PipelineHandle>();
		}

		// ----------------------------------------------------------------------------------------------------
		// フォーワード
		// ----------------------------------------------------------------------------------------------------

		{
			GraphicsFactory::Stage& stage = m_Stages[GraphicsFactory::ST_SCENE_FORWARD];
			VE_DEBUG_CODE(stage.debugName = L"ForwardStage");

			/****************/
			/* レンダーパス */
			/****************/

			RenderPassDesc& renderPassDesc = stage.renderPassDesc;
			renderPassDesc.Allocate(4, 1, 1);

			renderPassDesc.pAttachments[0].format = m_Attachments[GraphicsFactory::AT_SH_COLOR].imageDesc.format;
			renderPassDesc.pAttachments[0].samples = m_Attachments[GraphicsFactory::AT_SH_COLOR].imageDesc.samples;
			renderPassDesc.pAttachments[0].loadOp = V3D_ATTACHMENT_LOAD_OP_LOAD;
			renderPassDesc.pAttachments[0].storeOp = V3D_ATTACHMENT_STORE_OP_STORE;
			renderPassDesc.pAttachments[0].stencilLoadOp = V3D_ATTACHMENT_LOAD_OP_UNDEFINED;
			renderPassDesc.pAttachments[0].stencilStoreOp = V3D_ATTACHMENT_STORE_OP_UNDEFINED;
			renderPassDesc.pAttachments[0].initialLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			renderPassDesc.pAttachments[0].finalLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			renderPassDesc.pAttachments[0].clearValue.color.float32[0] = 0.1f;
			renderPassDesc.pAttachments[0].clearValue.color.float32[1] = 0.1f;
			renderPassDesc.pAttachments[0].clearValue.color.float32[2] = 0.1f;
			renderPassDesc.pAttachments[0].clearValue.color.float32[3] = 1.0f;

			renderPassDesc.pAttachments[1].format = m_Attachments[GraphicsFactory::AT_GL_COLOR].imageDesc.format;
			renderPassDesc.pAttachments[1].samples = m_Attachments[GraphicsFactory::AT_GL_COLOR].imageDesc.samples;
			renderPassDesc.pAttachments[1].loadOp = V3D_ATTACHMENT_LOAD_OP_LOAD;
			renderPassDesc.pAttachments[1].storeOp = V3D_ATTACHMENT_STORE_OP_STORE;
			renderPassDesc.pAttachments[1].stencilLoadOp = V3D_ATTACHMENT_LOAD_OP_UNDEFINED;
			renderPassDesc.pAttachments[1].stencilStoreOp = V3D_ATTACHMENT_STORE_OP_UNDEFINED;
			renderPassDesc.pAttachments[1].initialLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			renderPassDesc.pAttachments[1].finalLayout = V3D_IMAGE_LAYOUT_SHADER_READ_ONLY;

			renderPassDesc.pAttachments[2].format = m_Attachments[GraphicsFactory::AT_GE_SELECT].imageDesc.format;
			renderPassDesc.pAttachments[2].samples = m_Attachments[GraphicsFactory::AT_GE_SELECT].imageDesc.samples;
			renderPassDesc.pAttachments[2].loadOp = V3D_ATTACHMENT_LOAD_OP_LOAD;
			renderPassDesc.pAttachments[2].storeOp = V3D_ATTACHMENT_STORE_OP_STORE;
			renderPassDesc.pAttachments[2].stencilLoadOp = V3D_ATTACHMENT_LOAD_OP_UNDEFINED;
			renderPassDesc.pAttachments[2].stencilStoreOp = V3D_ATTACHMENT_STORE_OP_UNDEFINED;
			renderPassDesc.pAttachments[2].initialLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			renderPassDesc.pAttachments[2].finalLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;

			renderPassDesc.pAttachments[3].format = m_Attachments[GraphicsFactory::AT_GE_DEPTH].imageDesc.format;
			renderPassDesc.pAttachments[3].samples = m_Attachments[GraphicsFactory::AT_GE_DEPTH].imageDesc.samples;
			renderPassDesc.pAttachments[3].loadOp = V3D_ATTACHMENT_LOAD_OP_LOAD;
			renderPassDesc.pAttachments[3].storeOp = V3D_ATTACHMENT_STORE_OP_STORE;
			renderPassDesc.pAttachments[3].stencilLoadOp = V3D_ATTACHMENT_LOAD_OP_LOAD;
			renderPassDesc.pAttachments[3].stencilStoreOp = V3D_ATTACHMENT_STORE_OP_STORE;
			renderPassDesc.pAttachments[3].initialLayout = V3D_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT;
			renderPassDesc.pAttachments[3].finalLayout = V3D_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT;

			// サブパス 0 - Transparency
			RenderPassDesc::Subpass* pSubpass = renderPassDesc.AllocateSubpass(0, 0, 3, true, 0);
			pSubpass->colorAttachments[0].attachment = 0;
			pSubpass->colorAttachments[0].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			pSubpass->colorAttachments[1].attachment = 1;
			pSubpass->colorAttachments[1].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			pSubpass->colorAttachments[2].attachment = 2;
			pSubpass->colorAttachments[2].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			pSubpass->depthStencilAttachment.attachment = 3;
			pSubpass->depthStencilAttachment.layout = V3D_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT;

			renderPassDesc.pSubpassDependencies[0].srcSubpass = 0;
			renderPassDesc.pSubpassDependencies[0].dstSubpass = V3D_SUBPASS_EXTERNAL;
			renderPassDesc.pSubpassDependencies[0].srcStageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
			renderPassDesc.pSubpassDependencies[0].dstStageMask = V3D_PIPELINE_STAGE_FRAGMENT_SHADER;
			renderPassDesc.pSubpassDependencies[0].srcAccessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
			renderPassDesc.pSubpassDependencies[0].dstAccessMask = V3D_ACCESS_SHADER_READ;
			renderPassDesc.pSubpassDependencies[0].dependencyFlags = 9;

			/**********************/
			/* フレームバッファー */
			/**********************/

			stage.frameBuffers.resize(1);
			stage.frameBuffers[0].attachmentIndices.resize(4);
			stage.frameBuffers[0].attachmentIndices[0] = GraphicsFactory::AT_SH_COLOR;
			stage.frameBuffers[0].attachmentIndices[1] = GraphicsFactory::AT_GL_COLOR;
			stage.frameBuffers[0].attachmentIndices[2] = GraphicsFactory::AT_GE_SELECT;
			stage.frameBuffers[0].attachmentIndices[3] = GraphicsFactory::AT_GE_DEPTH;

			stage.frameBuffers[0].handle = std::make_shared<FrameBufferHandle>();
			stage.frameBuffers[0].handle->m_FrameBuffers.resize(frameCount, nullptr);

			/****************/
			/* パイプライン */
			/****************/

			stage.subpasses.resize(GraphicsFactory::SST_SCENE_FORWARD_MAX);
		}

		// ----------------------------------------------------------------------------------------------------
		// イメージエフェクト
		// ----------------------------------------------------------------------------------------------------

		{
			GraphicsFactory::Stage& stage = m_Stages[GraphicsFactory::ST_SCENE_IMAGE_EFFECT];
			VE_DEBUG_CODE(stage.debugName = L"ImageEffectStage");

			/****************/
			/* レンダーパス */
			/****************/

			RenderPassDesc& renderPassDesc = stage.renderPassDesc;
			renderPassDesc.Allocate(4, 5, 5);

			renderPassDesc.pAttachments[0].format = m_Attachments[GraphicsFactory::AT_IE_LDR_COLOR_0].imageDesc.format;
			renderPassDesc.pAttachments[0].samples = m_Attachments[GraphicsFactory::AT_IE_LDR_COLOR_0].imageDesc.samples;
			renderPassDesc.pAttachments[0].loadOp = V3D_ATTACHMENT_LOAD_OP_UNDEFINED;
			renderPassDesc.pAttachments[0].storeOp = V3D_ATTACHMENT_STORE_OP_STORE;
			renderPassDesc.pAttachments[0].stencilLoadOp = V3D_ATTACHMENT_LOAD_OP_UNDEFINED;
			renderPassDesc.pAttachments[0].stencilStoreOp = V3D_ATTACHMENT_STORE_OP_UNDEFINED;
			renderPassDesc.pAttachments[0].initialLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			renderPassDesc.pAttachments[0].finalLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;

			renderPassDesc.pAttachments[1].format = m_Attachments[GraphicsFactory::AT_IE_LDR_COLOR_1].imageDesc.format;
			renderPassDesc.pAttachments[1].samples = m_Attachments[GraphicsFactory::AT_IE_LDR_COLOR_1].imageDesc.samples;
			renderPassDesc.pAttachments[1].loadOp = V3D_ATTACHMENT_LOAD_OP_UNDEFINED;
			renderPassDesc.pAttachments[1].storeOp = V3D_ATTACHMENT_STORE_OP_STORE;
			renderPassDesc.pAttachments[1].stencilLoadOp = V3D_ATTACHMENT_LOAD_OP_UNDEFINED;
			renderPassDesc.pAttachments[1].stencilStoreOp = V3D_ATTACHMENT_STORE_OP_UNDEFINED;
			renderPassDesc.pAttachments[1].initialLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			renderPassDesc.pAttachments[1].finalLayout = V3D_IMAGE_LAYOUT_SHADER_READ_ONLY;

			renderPassDesc.pAttachments[2].format = m_Attachments[GraphicsFactory::AT_IE_SELECT_COLOR].imageDesc.format;
			renderPassDesc.pAttachments[2].samples = m_Attachments[GraphicsFactory::AT_IE_SELECT_COLOR].imageDesc.samples;
			renderPassDesc.pAttachments[2].loadOp = V3D_ATTACHMENT_LOAD_OP_CLEAR;
			renderPassDesc.pAttachments[2].storeOp = V3D_ATTACHMENT_STORE_OP_UNDEFINED;
			renderPassDesc.pAttachments[2].stencilLoadOp = V3D_ATTACHMENT_LOAD_OP_UNDEFINED;
			renderPassDesc.pAttachments[2].stencilStoreOp = V3D_ATTACHMENT_STORE_OP_UNDEFINED;
			renderPassDesc.pAttachments[2].initialLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			renderPassDesc.pAttachments[2].finalLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			renderPassDesc.pAttachments[2].clearValue = V3DClearValue{};

			renderPassDesc.pAttachments[3].format = m_Attachments[GraphicsFactory::AT_GE_DEPTH].imageDesc.format;
			renderPassDesc.pAttachments[3].samples = m_Attachments[GraphicsFactory::AT_GE_DEPTH].imageDesc.samples;
			renderPassDesc.pAttachments[3].loadOp = V3D_ATTACHMENT_LOAD_OP_LOAD;
			renderPassDesc.pAttachments[3].storeOp = V3D_ATTACHMENT_STORE_OP_UNDEFINED;
			renderPassDesc.pAttachments[3].stencilLoadOp = V3D_ATTACHMENT_LOAD_OP_UNDEFINED;
			renderPassDesc.pAttachments[3].stencilStoreOp = V3D_ATTACHMENT_STORE_OP_UNDEFINED;
			renderPassDesc.pAttachments[3].initialLayout = V3D_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT;
			renderPassDesc.pAttachments[3].finalLayout = V3D_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT;

			// Subpass 0 - Tone mapping
			RenderPassDesc::Subpass* pSubpass = renderPassDesc.AllocateSubpass(0, 0, 1, false, 0);
			pSubpass->colorAttachments[0].attachment = 0;
			pSubpass->colorAttachments[0].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;

			// Subpass 1 - Draw select mesh
			pSubpass = renderPassDesc.AllocateSubpass(1, 0, 1, false, 0);
			pSubpass->colorAttachments[0].attachment = 2;
			pSubpass->colorAttachments[0].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;

			// Subpass 2 - Select mapping
			pSubpass = renderPassDesc.AllocateSubpass(2, 1, 1, false, 0);
			pSubpass->inputAttachments[0].attachment = 2;
			pSubpass->inputAttachments[0].layout = V3D_IMAGE_LAYOUT_SHADER_READ_ONLY;
			pSubpass->colorAttachments[0].attachment = 0;
			pSubpass->colorAttachments[0].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;

			// Subpass 3 - Debug
			pSubpass = renderPassDesc.AllocateSubpass(3, 0, 1, true, 0);
			pSubpass->colorAttachments[0].attachment = 0;
			pSubpass->colorAttachments[0].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			pSubpass->depthStencilAttachment.attachment = 3;
			pSubpass->depthStencilAttachment.layout = V3D_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT;

			// Subpass 4 - Fxaa
			pSubpass = renderPassDesc.AllocateSubpass(4, 1, 1, false, 0);
			pSubpass->inputAttachments[0].attachment = 0;
			pSubpass->inputAttachments[0].layout = V3D_IMAGE_LAYOUT_SHADER_READ_ONLY;
			pSubpass->colorAttachments[0].attachment = 1;
			pSubpass->colorAttachments[0].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;

			renderPassDesc.pSubpassDependencies[0].srcSubpass = 0;
			renderPassDesc.pSubpassDependencies[0].dstSubpass = 2;
			renderPassDesc.pSubpassDependencies[0].srcStageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
			renderPassDesc.pSubpassDependencies[0].dstStageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
			renderPassDesc.pSubpassDependencies[0].srcAccessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
			renderPassDesc.pSubpassDependencies[0].dstAccessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
			renderPassDesc.pSubpassDependencies[0].dependencyFlags = V3D_DEPENDENCY_BY_REGION;

			renderPassDesc.pSubpassDependencies[1].srcSubpass = 1;
			renderPassDesc.pSubpassDependencies[1].dstSubpass = 2;
			renderPassDesc.pSubpassDependencies[1].srcStageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
			renderPassDesc.pSubpassDependencies[1].dstStageMask = V3D_PIPELINE_STAGE_FRAGMENT_SHADER;
			renderPassDesc.pSubpassDependencies[1].srcAccessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
			renderPassDesc.pSubpassDependencies[1].dstAccessMask = V3D_ACCESS_SHADER_READ;
			renderPassDesc.pSubpassDependencies[1].dependencyFlags = V3D_DEPENDENCY_BY_REGION;

			renderPassDesc.pSubpassDependencies[2].srcSubpass = 2;
			renderPassDesc.pSubpassDependencies[2].dstSubpass = 3;
			renderPassDesc.pSubpassDependencies[2].srcStageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
			renderPassDesc.pSubpassDependencies[2].dstStageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
			renderPassDesc.pSubpassDependencies[2].srcAccessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
			renderPassDesc.pSubpassDependencies[2].dstAccessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
			renderPassDesc.pSubpassDependencies[2].dependencyFlags = V3D_DEPENDENCY_BY_REGION;

			renderPassDesc.pSubpassDependencies[3].srcSubpass = 3;
			renderPassDesc.pSubpassDependencies[3].dstSubpass = 4;
			renderPassDesc.pSubpassDependencies[3].srcStageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
			renderPassDesc.pSubpassDependencies[3].dstStageMask = V3D_PIPELINE_STAGE_FRAGMENT_SHADER;
			renderPassDesc.pSubpassDependencies[3].srcAccessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
			renderPassDesc.pSubpassDependencies[3].dstAccessMask = V3D_ACCESS_SHADER_READ;
			renderPassDesc.pSubpassDependencies[3].dependencyFlags = V3D_DEPENDENCY_BY_REGION;

			renderPassDesc.pSubpassDependencies[4].srcSubpass = 4;
			renderPassDesc.pSubpassDependencies[4].dstSubpass = V3D_SUBPASS_EXTERNAL;
			renderPassDesc.pSubpassDependencies[4].srcStageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
			renderPassDesc.pSubpassDependencies[4].dstStageMask = V3D_PIPELINE_STAGE_FRAGMENT_SHADER;
			renderPassDesc.pSubpassDependencies[4].srcAccessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
			renderPassDesc.pSubpassDependencies[4].dstAccessMask = V3D_ACCESS_SHADER_READ;
			renderPassDesc.pSubpassDependencies[4].dependencyFlags = 0;

			/**********************/
			/* フレームバッファー */
			/**********************/

			stage.frameBuffers.resize(1);
			stage.frameBuffers[0].attachmentIndices.resize(4);
			stage.frameBuffers[0].attachmentIndices[0] = GraphicsFactory::AT_IE_LDR_COLOR_0;
			stage.frameBuffers[0].attachmentIndices[1] = GraphicsFactory::AT_IE_LDR_COLOR_1;
			stage.frameBuffers[0].attachmentIndices[2] = GraphicsFactory::AT_IE_SELECT_COLOR;
			stage.frameBuffers[0].attachmentIndices[3] = GraphicsFactory::AT_GE_DEPTH;

			stage.frameBuffers[0].handle = std::make_shared<FrameBufferHandle>();
			stage.frameBuffers[0].handle->m_FrameBuffers.resize(frameCount, nullptr);

			/****************/
			/* パイプライン */
			/****************/

			stage.subpasses.resize(GraphicsFactory::SST_SCENE_IMAGE_EFFECT_MAX);
		}

		/******************************/
		/* パイプライン - ToneMapping */
		/******************************/

		// On
		{
			GraphicsFactory::Stage& stage = m_Stages[GraphicsFactory::ST_SCENE_IMAGE_EFFECT];
			GraphicsFactory::StageSubpass& subpass = stage.subpasses[SST_SCENE_IMAGE_EFFECT_TONE_MAPPING];

			subpass.pipelineType = GraphicsFactory::PT_SIMPLE;

			GraphicsPipelineDesc& pipelineDesc = subpass.pipelineDesc;
			pipelineDesc.Allocate(2, 1, 1);
			pipelineDesc.vertexShader.pModule = m_ShaderModules[GraphicsFactory::SMT_SIMPLE_VERT];
			pipelineDesc.vertexShader.pEntryPointName = "main";
			pipelineDesc.fragmentShader.pModule = m_ShaderModules[GraphicsFactory::SMT_TONEMAPPING_FRAG];
			pipelineDesc.fragmentShader.pEntryPointName = "main";
			pipelineDesc.vertexInput.pElements[0].format = V3D_FORMAT_R32G32B32A32_SFLOAT;
			pipelineDesc.vertexInput.pElements[0].location = 0;
			pipelineDesc.vertexInput.pElements[0].offset = 0;
			pipelineDesc.vertexInput.pElements[1].format = V3D_FORMAT_R32G32_SFLOAT;
			pipelineDesc.vertexInput.pElements[1].location = 1;
			pipelineDesc.vertexInput.pElements[1].offset = 16;
			pipelineDesc.vertexInput.pLayouts[0].binding = 0;
			pipelineDesc.vertexInput.pLayouts[0].firstElement = 0;
			pipelineDesc.vertexInput.pLayouts[0].elementCount = 2;
			pipelineDesc.vertexInput.pLayouts[0].stride = sizeof(SimpleVertex);
			pipelineDesc.primitiveTopology = V3D_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			pipelineDesc.rasterization.polygonMode = V3D_POLYGON_MODE_FILL;
			pipelineDesc.rasterization.cullMode = V3D_CULL_MODE_NONE;
			pipelineDesc.depthStencil.depthTestEnable = false;
			pipelineDesc.depthStencil.depthWriteEnable = false;
			pipelineDesc.depthStencil.depthCompareOp = V3D_COMPARE_OP_ALWAYS;
			pipelineDesc.colorBlend.pAttachments[0] = InitializeColorBlendAttachment(BLEND_MODE_COPY);
			pipelineDesc.subpass = 0;

			subpass.pipelineHandle = std::make_shared<PipelineHandle>();
		}

		// Off
		{
			GraphicsFactory::Stage& stage = m_Stages[GraphicsFactory::ST_SCENE_IMAGE_EFFECT];
			GraphicsFactory::StageSubpass& subpass = stage.subpasses[SST_SCENE_IMAGE_EFFECT_TONE_MAPPING_OFF];

			subpass.pipelineType = GraphicsFactory::PT_SIMPLE;

			GraphicsPipelineDesc& pipelineDesc = subpass.pipelineDesc;
			pipelineDesc.Allocate(2, 1, 1);
			pipelineDesc.vertexShader.pModule = m_ShaderModules[GraphicsFactory::SMT_SIMPLE_VERT];
			pipelineDesc.vertexShader.pEntryPointName = "main";
			pipelineDesc.fragmentShader.pModule = m_ShaderModules[GraphicsFactory::SMT_SIMPLE_FRAG];
			pipelineDesc.fragmentShader.pEntryPointName = "main";
			pipelineDesc.vertexInput.pElements[0].format = V3D_FORMAT_R32G32B32A32_SFLOAT;
			pipelineDesc.vertexInput.pElements[0].location = 0;
			pipelineDesc.vertexInput.pElements[0].offset = 0;
			pipelineDesc.vertexInput.pElements[1].format = V3D_FORMAT_R32G32_SFLOAT;
			pipelineDesc.vertexInput.pElements[1].location = 1;
			pipelineDesc.vertexInput.pElements[1].offset = 16;
			pipelineDesc.vertexInput.pLayouts[0].binding = 0;
			pipelineDesc.vertexInput.pLayouts[0].firstElement = 0;
			pipelineDesc.vertexInput.pLayouts[0].elementCount = 2;
			pipelineDesc.vertexInput.pLayouts[0].stride = sizeof(SimpleVertex);
			pipelineDesc.primitiveTopology = V3D_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			pipelineDesc.rasterization.polygonMode = V3D_POLYGON_MODE_FILL;
			pipelineDesc.rasterization.cullMode = V3D_CULL_MODE_NONE;
			pipelineDesc.depthStencil.depthTestEnable = false;
			pipelineDesc.depthStencil.depthWriteEnable = false;
			pipelineDesc.depthStencil.depthCompareOp = V3D_COMPARE_OP_ALWAYS;
			pipelineDesc.colorBlend.pAttachments[0] = InitializeColorBlendAttachment(BLEND_MODE_COPY);
			pipelineDesc.subpass = 0;

			subpass.pipelineHandle = std::make_shared<PipelineHandle>();
		}

		/********************************/
		/* パイプライン - SelectMapping */
		/********************************/

		{
			GraphicsFactory::Stage& stage = m_Stages[GraphicsFactory::ST_SCENE_IMAGE_EFFECT];
			GraphicsFactory::StageSubpass& subpass = stage.subpasses[SST_SCENE_IMAGE_EFFECT_SELECT_MAPPING];

			subpass.pipelineType = GraphicsFactory::PT_SELECT_MAPPING;

			GraphicsPipelineDesc& pipelineDesc = subpass.pipelineDesc;
			pipelineDesc.Allocate(2, 1, 1);
			pipelineDesc.vertexShader.pModule = m_ShaderModules[GraphicsFactory::SMT_SIMPLE_VERT];
			pipelineDesc.vertexShader.pEntryPointName = "main";
			pipelineDesc.fragmentShader.pModule = m_ShaderModules[GraphicsFactory::SMT_SELECTMAPPING_FRAG];
			pipelineDesc.fragmentShader.pEntryPointName = "main";
			pipelineDesc.vertexInput.pElements[0].format = V3D_FORMAT_R32G32B32A32_SFLOAT;
			pipelineDesc.vertexInput.pElements[0].location = 0;
			pipelineDesc.vertexInput.pElements[0].offset = 0;
			pipelineDesc.vertexInput.pElements[1].format = V3D_FORMAT_R32G32_SFLOAT;
			pipelineDesc.vertexInput.pElements[1].location = 1;
			pipelineDesc.vertexInput.pElements[1].offset = 16;
			pipelineDesc.vertexInput.pLayouts[0].binding = 0;
			pipelineDesc.vertexInput.pLayouts[0].firstElement = 0;
			pipelineDesc.vertexInput.pLayouts[0].elementCount = 2;
			pipelineDesc.vertexInput.pLayouts[0].stride = sizeof(SimpleVertex);
			pipelineDesc.primitiveTopology = V3D_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			pipelineDesc.rasterization.polygonMode = V3D_POLYGON_MODE_FILL;
			pipelineDesc.rasterization.cullMode = V3D_CULL_MODE_NONE;
			pipelineDesc.depthStencil.depthTestEnable = false;
			pipelineDesc.depthStencil.depthWriteEnable = false;
			pipelineDesc.depthStencil.depthCompareOp = V3D_COMPARE_OP_ALWAYS;
			pipelineDesc.colorBlend.pAttachments[0] = InitializeColorBlendAttachment(BLEND_MODE_NORMAL);
			pipelineDesc.subpass = 2;

			subpass.pipelineHandle = std::make_shared<PipelineHandle>();
		}

		/************************/
		/* パイプライン - Debug */
		/************************/

		{
			GraphicsFactory::Stage& stage = m_Stages[GraphicsFactory::ST_SCENE_IMAGE_EFFECT];
			GraphicsFactory::StageSubpass& subpass = stage.subpasses[SST_SCENE_IMAGE_EFFECT_DEBUG];

			subpass.pipelineType = GraphicsFactory::PT_DEBUG;

			GraphicsPipelineDesc& pipelineDesc = subpass.pipelineDesc;
			pipelineDesc.Allocate(2, 1, 1);
			pipelineDesc.vertexShader.pModule = m_ShaderModules[GraphicsFactory::SMT_DEBUG_VERT];
			pipelineDesc.vertexShader.pEntryPointName = "main";
			pipelineDesc.fragmentShader.pModule = m_ShaderModules[GraphicsFactory::SMT_DEBUG_FRAG];
			pipelineDesc.fragmentShader.pEntryPointName = "main";
			pipelineDesc.vertexInput.pElements[0].format = V3D_FORMAT_R32G32B32A32_SFLOAT;
			pipelineDesc.vertexInput.pElements[0].location = 0;
			pipelineDesc.vertexInput.pElements[0].offset = 0;
			pipelineDesc.vertexInput.pElements[1].format = V3D_FORMAT_R32G32B32A32_SFLOAT;
			pipelineDesc.vertexInput.pElements[1].location = 1;
			pipelineDesc.vertexInput.pElements[1].offset = 16;
			pipelineDesc.vertexInput.pLayouts[0].binding = 0;
			pipelineDesc.vertexInput.pLayouts[0].firstElement = 0;
			pipelineDesc.vertexInput.pLayouts[0].elementCount = 2;
			pipelineDesc.vertexInput.pLayouts[0].stride = sizeof(DebugVertex);
			pipelineDesc.primitiveTopology = V3D_PRIMITIVE_TOPOLOGY_LINE_LIST;
			pipelineDesc.rasterization.polygonMode = V3D_POLYGON_MODE_LINE;
			pipelineDesc.rasterization.cullMode = V3D_CULL_MODE_NONE;
			pipelineDesc.depthStencil.depthTestEnable = true;
			pipelineDesc.depthStencil.depthWriteEnable = false;
			pipelineDesc.depthStencil.depthCompareOp = V3D_COMPARE_OP_LESS;
			pipelineDesc.colorBlend.pAttachments[0] = InitializeColorBlendAttachment(BLEND_MODE_COPY);
			pipelineDesc.subpass = 3;

			subpass.pipelineHandle = std::make_shared<PipelineHandle>();
		}

		/***********************/
		/* パイプライン - FXAA */
		/***********************/

		// On
		{
			GraphicsFactory::Stage& stage = m_Stages[GraphicsFactory::ST_SCENE_IMAGE_EFFECT];
			GraphicsFactory::StageSubpass& subpass = stage.subpasses[SST_SCENE_IMAGE_EFFECT_FXAA];

			subpass.pipelineType = GraphicsFactory::PT_FXAA;

			GraphicsPipelineDesc& pipelineDesc = subpass.pipelineDesc;
			pipelineDesc.Allocate(2, 1, 1);
			pipelineDesc.vertexShader.pModule = m_ShaderModules[GraphicsFactory::SMT_SIMPLE_VERT];
			pipelineDesc.vertexShader.pEntryPointName = "main";
			pipelineDesc.fragmentShader.pModule = m_ShaderModules[GraphicsFactory::SMT_FXAA_FRAG];
			pipelineDesc.fragmentShader.pEntryPointName = "main";
			pipelineDesc.vertexInput.pElements[0].format = V3D_FORMAT_R32G32B32A32_SFLOAT;
			pipelineDesc.vertexInput.pElements[0].location = 0;
			pipelineDesc.vertexInput.pElements[0].offset = 0;
			pipelineDesc.vertexInput.pElements[1].format = V3D_FORMAT_R32G32_SFLOAT;
			pipelineDesc.vertexInput.pElements[1].location = 1;
			pipelineDesc.vertexInput.pElements[1].offset = 16;
			pipelineDesc.vertexInput.pLayouts[0].binding = 0;
			pipelineDesc.vertexInput.pLayouts[0].firstElement = 0;
			pipelineDesc.vertexInput.pLayouts[0].elementCount = 2;
			pipelineDesc.vertexInput.pLayouts[0].stride = sizeof(SimpleVertex);
			pipelineDesc.primitiveTopology = V3D_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			pipelineDesc.rasterization.polygonMode = V3D_POLYGON_MODE_FILL;
			pipelineDesc.rasterization.cullMode = V3D_CULL_MODE_NONE;
			pipelineDesc.depthStencil.depthTestEnable = false;
			pipelineDesc.depthStencil.depthWriteEnable = false;
			pipelineDesc.depthStencil.depthCompareOp = V3D_COMPARE_OP_ALWAYS;
			pipelineDesc.colorBlend.pAttachments[0] = InitializeColorBlendAttachment(BLEND_MODE_COPY);
			pipelineDesc.subpass = 4;

			subpass.pipelineHandle = std::make_shared<PipelineHandle>();
		}

		// On
		{
			GraphicsFactory::Stage& stage = m_Stages[GraphicsFactory::ST_SCENE_IMAGE_EFFECT];
			GraphicsFactory::StageSubpass& subpass = stage.subpasses[SST_SCENE_IMAGE_EFFECT_FXAA_OFF];

			subpass.pipelineType = GraphicsFactory::PT_SIMPLE
;

			GraphicsPipelineDesc& pipelineDesc = subpass.pipelineDesc;
			pipelineDesc.Allocate(2, 1, 1);
			pipelineDesc.vertexShader.pModule = m_ShaderModules[GraphicsFactory::SMT_SIMPLE_VERT];
			pipelineDesc.vertexShader.pEntryPointName = "main";
			pipelineDesc.fragmentShader.pModule = m_ShaderModules[GraphicsFactory::SMT_SIMPLE_FRAG];
			pipelineDesc.fragmentShader.pEntryPointName = "main";
			pipelineDesc.vertexInput.pElements[0].format = V3D_FORMAT_R32G32B32A32_SFLOAT;
			pipelineDesc.vertexInput.pElements[0].location = 0;
			pipelineDesc.vertexInput.pElements[0].offset = 0;
			pipelineDesc.vertexInput.pElements[1].format = V3D_FORMAT_R32G32_SFLOAT;
			pipelineDesc.vertexInput.pElements[1].location = 1;
			pipelineDesc.vertexInput.pElements[1].offset = 16;
			pipelineDesc.vertexInput.pLayouts[0].binding = 0;
			pipelineDesc.vertexInput.pLayouts[0].firstElement = 0;
			pipelineDesc.vertexInput.pLayouts[0].elementCount = 2;
			pipelineDesc.vertexInput.pLayouts[0].stride = sizeof(SimpleVertex);
			pipelineDesc.primitiveTopology = V3D_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			pipelineDesc.rasterization.polygonMode = V3D_POLYGON_MODE_FILL;
			pipelineDesc.rasterization.cullMode = V3D_CULL_MODE_NONE;
			pipelineDesc.depthStencil.depthTestEnable = false;
			pipelineDesc.depthStencil.depthWriteEnable = false;
			pipelineDesc.depthStencil.depthCompareOp = V3D_COMPARE_OP_ALWAYS;
			pipelineDesc.colorBlend.pAttachments[0] = InitializeColorBlendAttachment(BLEND_MODE_COPY);
			pipelineDesc.subpass = 4;

			subpass.pipelineHandle = std::make_shared<PipelineHandle>();
		}

		// ----------------------------------------------------------------------------------------------------
		// フィニッシュ
		// ----------------------------------------------------------------------------------------------------

		{
			GraphicsFactory::Stage& stage = m_Stages[GraphicsFactory::ST_SCENE_FINISH];
			VE_DEBUG_CODE(stage.debugName = L"FinishStage");

			/****************/
			/* レンダーパス */
			/****************/

			RenderPassDesc& renderPassDesc = stage.renderPassDesc;
			renderPassDesc.Allocate(1, 1, 2);

			renderPassDesc.pAttachments[0].format = V3D_FORMAT_UNDEFINED;
			renderPassDesc.pAttachments[0].samples = V3D_SAMPLE_COUNT_1;
			renderPassDesc.pAttachments[0].loadOp = V3D_ATTACHMENT_LOAD_OP_UNDEFINED;
			renderPassDesc.pAttachments[0].storeOp = V3D_ATTACHMENT_STORE_OP_STORE;
			renderPassDesc.pAttachments[0].stencilLoadOp = V3D_ATTACHMENT_LOAD_OP_UNDEFINED;
			renderPassDesc.pAttachments[0].stencilStoreOp = V3D_ATTACHMENT_STORE_OP_UNDEFINED;
			renderPassDesc.pAttachments[0].initialLayout = V3D_IMAGE_LAYOUT_PRESENT_SRC;
			renderPassDesc.pAttachments[0].finalLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			renderPassDesc.pAttachments[0].clearValue = V3DClearValue{};

			RenderPassDesc::Subpass* pSubpass = renderPassDesc.AllocateSubpass(0, 0, 1, false, 0);
			pSubpass->colorAttachments[0].attachment = 0;
			pSubpass->colorAttachments[0].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;

			renderPassDesc.pSubpassDependencies[0].srcSubpass = V3D_SUBPASS_EXTERNAL;
			renderPassDesc.pSubpassDependencies[0].dstSubpass = 0;
			renderPassDesc.pSubpassDependencies[0].srcStageMask = V3D_PIPELINE_STAGE_BOTTOM_OF_PIPE;
			renderPassDesc.pSubpassDependencies[0].dstStageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
			renderPassDesc.pSubpassDependencies[0].srcAccessMask = V3D_ACCESS_MEMORY_READ;
			renderPassDesc.pSubpassDependencies[0].dstAccessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
			renderPassDesc.pSubpassDependencies[0].dependencyFlags = 0;

			renderPassDesc.pSubpassDependencies[1].srcSubpass = 0;
			renderPassDesc.pSubpassDependencies[1].dstSubpass = V3D_SUBPASS_EXTERNAL;
			renderPassDesc.pSubpassDependencies[1].srcStageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
			renderPassDesc.pSubpassDependencies[1].dstStageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
			renderPassDesc.pSubpassDependencies[1].srcAccessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
			renderPassDesc.pSubpassDependencies[1].dstAccessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
			renderPassDesc.pSubpassDependencies[1].dependencyFlags = 0;

			/**********************/
			/* フレームバッファー */
			/**********************/

			stage.frameBuffers.resize(1);
			stage.frameBuffers[0].attachmentIndices.resize(1);
			stage.frameBuffers[0].attachmentIndices[0] = ~0U;
			stage.frameBuffers[0].handle = std::make_shared<FrameBufferHandle>();
			stage.frameBuffers[0].handle->m_FrameBuffers.resize(frameCount, nullptr);

			/****************/
			/* パイプライン */
			/****************/

			stage.subpasses.resize(SST_SCENE_FINISH_MAX);
		}

		/***********************/
		/* パイプライン - Copy */
		/***********************/

		{
			GraphicsFactory::Stage& stage = m_Stages[GraphicsFactory::ST_SCENE_FINISH];
			GraphicsFactory::StageSubpass& subpass = stage.subpasses[SST_SCENE_FINISH_COPY];

			subpass.pipelineType = GraphicsFactory::PT_SIMPLE;

			GraphicsPipelineDesc& pipelineDesc = subpass.pipelineDesc;
			pipelineDesc.Allocate(2, 1, 1);
			pipelineDesc.vertexShader.pModule = m_ShaderModules[GraphicsFactory::SMT_SIMPLE_VERT];
			pipelineDesc.vertexShader.pEntryPointName = "main";
			pipelineDesc.fragmentShader.pModule = m_ShaderModules[GraphicsFactory::SMT_SIMPLE_FRAG];
			pipelineDesc.fragmentShader.pEntryPointName = "main";
			pipelineDesc.vertexInput.pElements[0].format = V3D_FORMAT_R32G32B32A32_SFLOAT;
			pipelineDesc.vertexInput.pElements[0].location = 0;
			pipelineDesc.vertexInput.pElements[0].offset = 0;
			pipelineDesc.vertexInput.pElements[1].format = V3D_FORMAT_R32G32_SFLOAT;
			pipelineDesc.vertexInput.pElements[1].location = 1;
			pipelineDesc.vertexInput.pElements[1].offset = 16;
			pipelineDesc.vertexInput.pLayouts[0].binding = 0;
			pipelineDesc.vertexInput.pLayouts[0].firstElement = 0;
			pipelineDesc.vertexInput.pLayouts[0].elementCount = 2;
			pipelineDesc.vertexInput.pLayouts[0].stride = sizeof(SimpleVertex);
			pipelineDesc.primitiveTopology = V3D_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			pipelineDesc.rasterization.polygonMode = V3D_POLYGON_MODE_FILL;
			pipelineDesc.rasterization.cullMode = V3D_CULL_MODE_NONE;
			pipelineDesc.depthStencil.depthTestEnable = false;
			pipelineDesc.depthStencil.depthWriteEnable = false;
			pipelineDesc.depthStencil.depthCompareOp = V3D_COMPARE_OP_ALWAYS;
			pipelineDesc.colorBlend.pAttachments[0] = InitializeColorBlendAttachment(BLEND_MODE_COPY);
			pipelineDesc.subpass = 0;

			subpass.pipelineHandle = std::make_shared<PipelineHandle>();
		}

		// ----------------------------------------------------------------------------------------------------
		// GUI
		// ----------------------------------------------------------------------------------------------------

		{
			GraphicsFactory::Stage& stage = m_Stages[GraphicsFactory::ST_GUI];

			/****************/
			/* レンダーパス */
			/****************/

			RenderPassDesc& renderPassDesc = stage.renderPassDesc;
			renderPassDesc.Allocate(1, 1, 1);

			renderPassDesc.pAttachments[0].format = V3D_FORMAT_UNDEFINED;
			renderPassDesc.pAttachments[0].samples = V3D_SAMPLE_COUNT_1;
			renderPassDesc.pAttachments[0].loadOp = V3D_ATTACHMENT_LOAD_OP_LOAD;
			renderPassDesc.pAttachments[0].storeOp = V3D_ATTACHMENT_STORE_OP_STORE;
			renderPassDesc.pAttachments[0].stencilLoadOp = V3D_ATTACHMENT_LOAD_OP_UNDEFINED;
			renderPassDesc.pAttachments[0].stencilStoreOp = V3D_ATTACHMENT_STORE_OP_UNDEFINED;
			renderPassDesc.pAttachments[0].initialLayout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;
			renderPassDesc.pAttachments[0].finalLayout = V3D_IMAGE_LAYOUT_PRESENT_SRC;
			renderPassDesc.pAttachments[0].clearValue = V3DClearValue{};

			RenderPassDesc::Subpass* pSubpass = renderPassDesc.AllocateSubpass(0, 0, 1, false, 0);
			pSubpass->colorAttachments[0].attachment = 0;
			pSubpass->colorAttachments[0].layout = V3D_IMAGE_LAYOUT_COLOR_ATTACHMENT;

			renderPassDesc.pSubpassDependencies[0].srcSubpass = 0;
			renderPassDesc.pSubpassDependencies[0].dstSubpass = V3D_SUBPASS_EXTERNAL;
			renderPassDesc.pSubpassDependencies[0].srcStageMask = V3D_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
			renderPassDesc.pSubpassDependencies[0].dstStageMask = V3D_PIPELINE_STAGE_BOTTOM_OF_PIPE;
			renderPassDesc.pSubpassDependencies[0].srcAccessMask = V3D_ACCESS_COLOR_ATTACHMENT_READ | V3D_ACCESS_COLOR_ATTACHMENT_WRITE;
			renderPassDesc.pSubpassDependencies[0].dstAccessMask = V3D_ACCESS_MEMORY_READ;
			renderPassDesc.pSubpassDependencies[0].dependencyFlags = 0;

			/**********************/
			/* フレームバッファー */
			/**********************/

			stage.frameBuffers.resize(1);
			stage.frameBuffers[0].attachmentIndices.resize(1);
			stage.frameBuffers[0].attachmentIndices[0] = ~0U;
			stage.frameBuffers[0].handle = std::make_shared<FrameBufferHandle>();
			stage.frameBuffers[0].handle->m_FrameBuffers.resize(frameCount, nullptr);

			/****************/
			/* パイプライン */
			/****************/

			stage.subpasses.resize(SST_GUI_MAX);
		}

		/***********************/
		/* パイプライン - Copy */
		/***********************/

		{
			GraphicsFactory::Stage& stage = m_Stages[GraphicsFactory::ST_GUI];
			GraphicsFactory::StageSubpass& subpass = stage.subpasses[SST_GUI_COPY];

			subpass.pipelineType = GraphicsFactory::PT_GUI;

			GraphicsPipelineDesc& pipelineDesc = subpass.pipelineDesc;
			pipelineDesc.Allocate(3, 1, 1);
			pipelineDesc.vertexShader.pModule = m_ShaderModules[GraphicsFactory::SMT_GUI_VERT];
			pipelineDesc.vertexShader.pEntryPointName = "main";
			pipelineDesc.fragmentShader.pModule = m_ShaderModules[GraphicsFactory::SMT_GUI_FRAG];
			pipelineDesc.fragmentShader.pEntryPointName = "main";
			pipelineDesc.vertexInput.pElements[0].format = V3D_FORMAT_R32G32_SFLOAT;
			pipelineDesc.vertexInput.pElements[0].location = 0;
			pipelineDesc.vertexInput.pElements[0].offset = 0;
			pipelineDesc.vertexInput.pElements[1].format = V3D_FORMAT_R32G32_SFLOAT;
			pipelineDesc.vertexInput.pElements[1].location = 1;
			pipelineDesc.vertexInput.pElements[1].offset = 8;
			pipelineDesc.vertexInput.pElements[2].format = V3D_FORMAT_R8G8B8A8_UNORM;
			pipelineDesc.vertexInput.pElements[2].location = 2;
			pipelineDesc.vertexInput.pElements[2].offset = 16;
			pipelineDesc.vertexInput.pLayouts[0].binding = 0;
			pipelineDesc.vertexInput.pLayouts[0].firstElement = 0;
			pipelineDesc.vertexInput.pLayouts[0].elementCount = 3;
			pipelineDesc.vertexInput.pLayouts[0].stride = sizeof(ImDrawVert);
			pipelineDesc.primitiveTopology = V3D_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			pipelineDesc.rasterization.polygonMode = V3D_POLYGON_MODE_FILL;
			pipelineDesc.rasterization.cullMode = V3D_CULL_MODE_NONE;
			pipelineDesc.depthStencil.depthTestEnable = false;
			pipelineDesc.depthStencil.depthWriteEnable = false;
			pipelineDesc.depthStencil.depthCompareOp = V3D_COMPARE_OP_ALWAYS;
			pipelineDesc.colorBlend.pAttachments[0] = InitializeColorBlendAttachment(BLEND_MODE_NORMAL);
			pipelineDesc.subpass = 0;

			subpass.pipelineHandle = std::make_shared<PipelineHandle>();
		}

		// ----------------------------------------------------------------------------------------------------

		return true;
	}

	void GraphicsFactory::LostStages(DeletingQueue* pDeletingQueue)
	{
		for (GraphicsFactory::Stage& stage : m_Stages)
		{
			if (stage.subpasses.empty() == false)
			{
				auto it_begin = stage.subpasses.begin();
				auto it_end = stage.subpasses.end();

				for (auto it = it_begin; it != it_end; ++it)
				{
					DeleteDeviceChild(pDeletingQueue, &it->pipelineHandle->m_pPipeline);
				}
			}

			auto it_fb_begin = stage.frameBuffers.begin();
			auto it_fb_end = stage.frameBuffers.end();
			for (auto it_fb = it_fb_begin; it_fb != it_fb_end; ++it_fb)
			{
				DeleteDeviceChild(pDeletingQueue, it_fb->handle.get()->m_FrameBuffers);
			}

			DeleteDeviceChild(pDeletingQueue, &stage.pNativeRenderPass);
		}
	}

	bool GraphicsFactory::RestoreStages()
	{
		IV3DDevice* pNativeDevice = m_pDeviceContext->GetNativeDevicePtr();
		const V3DSwapChainDesc& nativeSwapChainDesc = m_pDeviceContext->GetNativeSwapchainDesc();
		uint32_t frameCount = nativeSwapChainDesc.imageCount;
		collection::Vector<IV3DImageView*> attachments;

		for (GraphicsFactory::Stage& stage : m_Stages)
		{
			// ----------------------------------------------------------------------------------------------------
			// レンダーパスを作成
			// ----------------------------------------------------------------------------------------------------

			RenderPassDesc& renderPassDesc = stage.renderPassDesc;
			auto& attachmentIndices = stage.frameBuffers[0].attachmentIndices;
			VE_ASSERT(attachmentIndices.size() == renderPassDesc.attachmentCount);

			IV3DRenderPass* pRenderPass;

			for (uint32_t i = 0; i < renderPassDesc.attachmentCount; i++)
			{
				if (attachmentIndices[i] == ~0U)
				{
					renderPassDesc.pAttachments[i].format = nativeSwapChainDesc.imageFormat;
				}
			}

			if (pNativeDevice->CreateRenderPass(
				renderPassDesc.attachmentCount, renderPassDesc.pAttachments,
				renderPassDesc.subpassCount, renderPassDesc.pSubpasses,
				renderPassDesc.subpassDependencyCount, renderPassDesc.pSubpassDependencies,
				&pRenderPass, VE_INTERFACE_DEBUG_NAME(stage.debugName.c_str())) != V3D_OK)
			{
				return false;
			}

			stage.pNativeRenderPass = pRenderPass;

			// ----------------------------------------------------------------------------------------------------
			// フレームバッファーを作成
			// ----------------------------------------------------------------------------------------------------

			attachments.resize(renderPassDesc.attachmentCount);

			auto it_fb_begin = stage.frameBuffers.begin();
			auto it_fb_end = stage.frameBuffers.end();
			for (auto it_fb = it_fb_begin; it_fb != it_fb_end; ++it_fb)
			{
				GraphicsFactory::FrameBuffer& frameBuffer = *it_fb;

				for (uint32_t i = 0; i < frameCount; i++)
				{
					for (uint32_t j = 0; j < renderPassDesc.attachmentCount; j++)
					{
						if (frameBuffer.attachmentIndices[j] != ~0U)
						{
							attachments[j] = m_Attachments[frameBuffer.attachmentIndices[j]].imageViews[i];
						}
						else
						{
							attachments[j] = m_pDeviceContext->GetFramePtr(i)->pImageView;
						}
					}

					VE_ASSERT(frameBuffer.handle->m_FrameBuffers[i] == nullptr);

					if (pNativeDevice->CreateFrameBuffer(
						pRenderPass,
						renderPassDesc.attachmentCount, attachments.data(),
						&frameBuffer.handle->m_FrameBuffers[i], VE_INTERFACE_DEBUG_NAME(stage.debugName.c_str())) != V3D_OK)
					{
						return false;
					}
				}
			}

			// ----------------------------------------------------------------------------------------------------
			// パイプラインを作成
			// ----------------------------------------------------------------------------------------------------

			for (GraphicsFactory::StageSubpass& subpass : stage.subpasses)
			{
				subpass.pipelineDesc.pRenderPass = stage.pNativeRenderPass;

				VE_ASSERT(subpass.pipelineHandle->m_pPipeline == nullptr);

				if (pNativeDevice->CreateGraphicsPipeline(
					m_Pipelines[subpass.pipelineType].pNativeLayout,
					subpass.pipelineDesc,
					&subpass.pipelineHandle->m_pPipeline, VE_INTERFACE_DEBUG_NAME(subpass.debugName.c_str())) != V3D_OK)
				{
					return false;
				}
			}

			// ----------------------------------------------------------------------------------------------------
		}

		return true;
	}

	bool GraphicsFactory::CreateMaterialSet(GraphicsFactory::MaterialSet** ppMaterialSet, uint32_t shaderFlags, bool createPipeline, size_t boneCount, uint32_t vertexStride, V3D_POLYGON_MODE polygonMode, V3D_CULL_MODE cullMode, BLEND_MODE blendMode)
	{
		if ((shaderFlags & MATERIAL_SHADER_SKELETAL) && (boneCount == 0))
		{
			shaderFlags ^= MATERIAL_SHADER_SKELETAL;
		}

		GraphicsFactory::MaterialSetKey materialSetKey = GraphicsFactory::MaterialSetKey(shaderFlags, boneCount, polygonMode, cullMode, blendMode);

		auto it_material_set = m_MaterialSetMap.find(materialSetKey);
		if (it_material_set == m_MaterialSetMap.end())
		{
			m_MaterialSetMap[materialSetKey] = GraphicsFactory::MaterialSet{};

			it_material_set = m_MaterialSetMap.find(materialSetKey);
		}

		if (CreateMaterialSet_Color(it_material_set->second.pipelines[GraphicsFactory::MPT_COLOR], shaderFlags, createPipeline, boneCount, vertexStride, polygonMode, cullMode, blendMode) == false)
		{
			return false;
		}

		if (CreateMaterialSet_Shadow(it_material_set->second.pipelines[GraphicsFactory::MPT_SHADOW], shaderFlags, createPipeline, boneCount, vertexStride) == false)
		{
			return false;
		}

		if (CreateMaterialSet_Select(it_material_set->second.pipelines[GraphicsFactory::MPT_SELECT], shaderFlags, createPipeline, boneCount, vertexStride) == false)
		{
			return false;
		}

		*ppMaterialSet = &it_material_set->second;

		return true;
	}

	bool GraphicsFactory::CreateMaterialSet_Color(
		GraphicsFactory::MaterialPipeline& pipeline,
		uint32_t shaderFlags, bool createPipeline,
		size_t boneCount,
		uint32_t vertexStride,
		V3D_POLYGON_MODE polygonMode, V3D_CULL_MODE cullMode,
		BLEND_MODE blendMode)
	{
		// ----------------------------------------------------------------------------------------------------
		// パイプラインの取得
		// ----------------------------------------------------------------------------------------------------

		uint32_t pipelineFlags = shaderFlags & MATERIAL_SHADER_PIPELINE_MASK;

		auto it_material_pipeline = m_MaterialPipelineMap.find(pipelineFlags);
		if (it_material_pipeline == m_MaterialPipelineMap.end())
		{
			m_MaterialPipelineMap[pipelineFlags] = GraphicsFactory::Pipeline{};
			it_material_pipeline = m_MaterialPipelineMap.find(pipelineFlags);
		}

		bool createPipelineLayout = (it_material_pipeline->second.pNativeLayout == nullptr);

		// ----------------------------------------------------------------------------------------------------
		// デスクリプタセットレイアウトを作成
		// ----------------------------------------------------------------------------------------------------

		IV3DDescriptorSetLayout* pNativeDescriptorSetLayout = nullptr;

		if (createPipelineLayout == true)
		{
			collection::Vector<V3DDescriptorDesc> descriptors;

			V3DDescriptorDesc desc;
			desc.stageFlags = V3D_SHADER_STAGE_FRAGMENT;
			desc.type = V3D_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			desc.binding = 0;

			descriptors.push_back(desc);

			if (shaderFlags & MATERIAL_SHADER_DIFFUSE_TEXTURE)
			{
				V3DDescriptorDesc desc;
				desc.stageFlags = V3D_SHADER_STAGE_FRAGMENT;
				desc.type = V3D_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				desc.binding = 1;

				descriptors.push_back(desc);
			}

			if (shaderFlags & MATERIAL_SHADER_SPECULAR_TEXTURE)
			{
				V3DDescriptorDesc desc;
				desc.stageFlags = V3D_SHADER_STAGE_FRAGMENT;
				desc.type = V3D_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				desc.binding = 2;

				descriptors.push_back(desc);
			}

			if (shaderFlags & MATERIAL_SHADER_BUMP_TEXTURE)
			{
				V3DDescriptorDesc desc;
				desc.stageFlags = V3D_SHADER_STAGE_FRAGMENT;
				desc.type = V3D_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				desc.binding = 3;

				descriptors.push_back(desc);
			}

			VE_DEBUG_CODE(StringA debugName = "Scene_Material_" + std::bitset<8>(shaderFlags).to_string());
			VE_DEBUG_CODE(ToWideString(debugName.c_str(), it_material_pipeline->second.debugName));

			if (m_pDeviceContext->GetNativeDevicePtr()->CreateDescriptorSetLayout(
				static_cast<uint32_t>(descriptors.size()), descriptors.data(),
				256, 256,
				&pNativeDescriptorSetLayout, VE_INTERFACE_DEBUG_NAME(it_material_pipeline->second.debugName.c_str())) != V3D_OK)
			{
				return false;
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// パイプラインレイアウトを作成
		// ----------------------------------------------------------------------------------------------------

		if (createPipelineLayout == true)
		{
			VE_ASSERT(pNativeDescriptorSetLayout != nullptr);

			collection::Vector<V3DConstantDesc> constants;

			if (shaderFlags & MATERIAL_SHADER_TRANSPARENCY)
			{
				constants.resize(2);
				constants[0].shaderStageFlags = V3D_SHADER_STAGE_VERTEX;
				constants[0].offset = 0;
				constants[0].size = sizeof(glm::mat4);
				constants[1].shaderStageFlags = V3D_SHADER_STAGE_FRAGMENT;
				constants[1].offset = 64;
				constants[1].size = sizeof(DirectionalLightingConstantF);
			}
			else
			{
				constants.resize(1);
				constants[0].shaderStageFlags = V3D_SHADER_STAGE_VERTEX;
				constants[0].offset = 0;
				constants[0].size = sizeof(glm::mat4);
			}

			IV3DDescriptorSetLayout* descriptorSetLayouts[2] =
			{
				m_DescriptorSets[GraphicsFactory::DST_MESH].pNativeLayout,
				pNativeDescriptorSetLayout,
			};

			if (m_pDeviceContext->GetNativeDevicePtr()->CreatePipelineLayout(
				static_cast<uint32_t>(constants.size()), constants.data(),
				_countof(descriptorSetLayouts), descriptorSetLayouts,
				&it_material_pipeline->second.pNativeLayout, VE_INTERFACE_DEBUG_NAME(it_material_pipeline->second.debugName.c_str())) != V3D_OK)
			{
				pNativeDescriptorSetLayout->Release();
				return false;
			}
		}

		VE_RELEASE(pNativeDescriptorSetLayout);

		// ----------------------------------------------------------------------------------------------------
		// シェーダーを作成
		// ----------------------------------------------------------------------------------------------------

		static constexpr char* SHADER_ENTRY_POINT_NAME = "main";

		IV3DShaderModule* pVertShaderModule;
		IV3DShaderModule* pFragShaderModule;

		if ((createPipeline == true) && (pipeline.handle == nullptr))
		{
			vsc::CompilerOptions options;

			if (shaderFlags & MATERIAL_SHADER_SKELETAL)
			{
				StringA boneCountValue = std::to_string(boneCount);

				options.AddMacroDefinition("SKELETAL_ENABLE");
				options.AddMacroDefinition("BONE_COUNT", boneCountValue.c_str());
			}

			if (shaderFlags & MATERIAL_SHADER_TEXTURE_MASK)
			{
				options.AddMacroDefinition("TEXTURE_ENABLE");
			}

			if (shaderFlags & MATERIAL_SHADER_DIFFUSE_TEXTURE)
			{
				options.AddMacroDefinition("DIFFUSE_TEXTURE_ENABLE");
			}

			if (shaderFlags & MATERIAL_SHADER_SPECULAR_TEXTURE)
			{
				options.AddMacroDefinition("SPECULAR_TEXTURE_ENABLE");
			}

			if (shaderFlags & MATERIAL_SHADER_BUMP_TEXTURE)
			{
				options.AddMacroDefinition("BUMP_TEXTURE_ENABLE");
			}

			if (shaderFlags & MATERIAL_SHADER_TRANSPARENCY)
			{
				options.AddMacroDefinition("TRANSPARENCY_ENABLE");
			}

			options.SetOptimize(true);

			vsc::Compiler vertCompiler;

			vsc::Result vertResult;
			vertCompiler.GlslToSpv("model.vert", VE_GF_Scene_Mesh_Vert, vsc::SHADER_TYPE_VERTEX, SHADER_ENTRY_POINT_NAME, &options, &vertResult);
			if (vertResult.GetErrorCount() != 0)
			{
				for (size_t i = 0; i < vertResult.GetErrorCount(); i++)
				{
					OutputDebugStringA(vertResult.GetErrorMessage(i));
					OutputDebugStringA("\n");
				}

				return false;
			}

			vsc::Compiler fragCompiler;

			vsc::Result fragResult;
			fragCompiler.GlslToSpv("model.frag", VE_GF_Scene_Mesh_Frag, vsc::SHADER_TYPE_FRAGMENT, SHADER_ENTRY_POINT_NAME, &options, &fragResult);
			if (fragResult.GetErrorCount() != 0)
			{
				for (size_t i = 0; i < fragResult.GetErrorCount(); i++)
				{
					OutputDebugStringA(fragResult.GetErrorMessage(i));
					OutputDebugStringA("\n");
				}

				return false;
			}

			if (m_pDeviceContext->GetNativeDevicePtr()->CreateShaderModule(vertResult.GetBlobSize(), vertResult.GetBlob(), &pVertShaderModule) != V3D_OK)
			{
				return false;
			}

			if (m_pDeviceContext->GetNativeDevicePtr()->CreateShaderModule(fragResult.GetBlobSize(), fragResult.GetBlob(), &pFragShaderModule) != V3D_OK)
			{
				pVertShaderModule->Release();
				return false;
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// パイプラインを作成
		// ----------------------------------------------------------------------------------------------------

		if ((createPipeline == true) && (pipeline.handle == nullptr))
		{
			VE_ASSERT(it_material_pipeline->second.pNativeLayout != nullptr);

			uint32_t elementIndex = 0;

			GraphicsPipelineDesc& pipelineDesc = pipeline.desc;

			uint32_t vertexElementCount = (shaderFlags & MATERIAL_SHADER_SKELETAL) ? 7 : 5;

			if (shaderFlags & MATERIAL_SHADER_TRANSPARENCY)
			{
				pipelineDesc.Allocate(vertexElementCount, 1, 3);
			}
			else
			{
				pipelineDesc.Allocate(vertexElementCount, 1, 4);
			}

			pipelineDesc.vertexInput.elementCount = 0;
			pipelineDesc.vertexInput.layoutCount = 1;

			pipelineDesc.vertexShader.pModule = pVertShaderModule;
			pipelineDesc.vertexShader.pEntryPointName = SHADER_ENTRY_POINT_NAME;

			pipelineDesc.fragmentShader.pModule = pFragShaderModule;
			pipelineDesc.fragmentShader.pEntryPointName = SHADER_ENTRY_POINT_NAME;

			// pos
			pipelineDesc.vertexInput.pElements[elementIndex].format = V3D_FORMAT_R32G32B32_SFLOAT;
			pipelineDesc.vertexInput.pElements[elementIndex].location = 0;
			pipelineDesc.vertexInput.pElements[elementIndex].offset = 0;
			pipelineDesc.vertexInput.elementCount = ++elementIndex;

			if (shaderFlags & MATERIAL_SHADER_TEXTURE_MASK)
			{
				// uv
				pipelineDesc.vertexInput.pElements[elementIndex].format = V3D_FORMAT_R32G32_SFLOAT;
				pipelineDesc.vertexInput.pElements[elementIndex].location = 1;
				pipelineDesc.vertexInput.pElements[elementIndex].offset = 12;
				pipelineDesc.vertexInput.elementCount = ++elementIndex;
			}

			// normal
			pipelineDesc.vertexInput.pElements[elementIndex].format = V3D_FORMAT_R32G32B32_SFLOAT;
			pipelineDesc.vertexInput.pElements[elementIndex].location = 2;
			pipelineDesc.vertexInput.pElements[elementIndex].offset = 20;
			pipelineDesc.vertexInput.elementCount = ++elementIndex;

			if (shaderFlags & MATERIAL_SHADER_BUMP_TEXTURE)
			{
				// tangent
				pipelineDesc.vertexInput.pElements[elementIndex].format = V3D_FORMAT_R32G32B32_SFLOAT;
				pipelineDesc.vertexInput.pElements[elementIndex].location = 3;
				pipelineDesc.vertexInput.pElements[elementIndex].offset = 32;
				pipelineDesc.vertexInput.elementCount = ++elementIndex;

				// binormal
				pipelineDesc.vertexInput.pElements[elementIndex].format = V3D_FORMAT_R32G32B32_SFLOAT;
				pipelineDesc.vertexInput.pElements[elementIndex].location = 4;
				pipelineDesc.vertexInput.pElements[elementIndex].offset = 44;
				pipelineDesc.vertexInput.elementCount = ++elementIndex;
			}

			if (shaderFlags & MATERIAL_SHADER_SKELETAL)
			{
				// indices
				pipelineDesc.vertexInput.pElements[elementIndex].format = V3D_FORMAT_R32G32B32A32_SINT;
				pipelineDesc.vertexInput.pElements[elementIndex].location = 5;
				pipelineDesc.vertexInput.pElements[elementIndex].offset = 56;
				pipelineDesc.vertexInput.elementCount = ++elementIndex;

				// weights
				pipelineDesc.vertexInput.pElements[elementIndex].format = V3D_FORMAT_R32G32B32A32_SFLOAT;
				pipelineDesc.vertexInput.pElements[elementIndex].location = 6;
				pipelineDesc.vertexInput.pElements[elementIndex].offset = 72;
				pipelineDesc.vertexInput.elementCount = ++elementIndex;
			}

			pipelineDesc.vertexInput.pLayouts->binding = 0;
			pipelineDesc.vertexInput.pLayouts->firstElement = 0;
			pipelineDesc.vertexInput.pLayouts->elementCount = elementIndex;
			pipelineDesc.vertexInput.pLayouts->stride = vertexStride;

			pipelineDesc.primitiveTopology = V3D_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

			pipelineDesc.rasterization.polygonMode = polygonMode;
			pipelineDesc.rasterization.cullMode = cullMode;

			if (shaderFlags & MATERIAL_SHADER_TRANSPARENCY)
			{
				pipelineDesc.depthStencil.depthTestEnable = true;
				pipelineDesc.depthStencil.depthWriteEnable = false;
				pipelineDesc.depthStencil.depthCompareOp = V3D_COMPARE_OP_LESS;

				pipelineDesc.colorBlend.pAttachments[0] = InitializeColorBlendAttachment(blendMode);
				pipelineDesc.colorBlend.pAttachments[1] = InitializeColorBlendAttachment(BLEND_MODE_ADD);
				pipelineDesc.colorBlend.pAttachments[2] = InitializeColorBlendAttachment(BLEND_MODE_COPY);
			}
			else
			{
				pipelineDesc.depthStencil.depthTestEnable = true;
				pipelineDesc.depthStencil.depthWriteEnable = true;
				pipelineDesc.depthStencil.depthCompareOp = V3D_COMPARE_OP_LESS;

				pipelineDesc.colorBlend.pAttachments[0] = InitializeColorBlendAttachment(blendMode);
				pipelineDesc.colorBlend.pAttachments[1] = InitializeColorBlendAttachment(BLEND_MODE_COPY);
				pipelineDesc.colorBlend.pAttachments[2] = InitializeColorBlendAttachment(BLEND_MODE_COPY);
				pipelineDesc.colorBlend.pAttachments[3] = InitializeColorBlendAttachment(BLEND_MODE_COPY);
			}

			if (shaderFlags & MATERIAL_SHADER_TRANSPARENCY)
			{
				pipelineDesc.pRenderPass = m_Stages[GraphicsFactory::ST_SCENE_FORWARD].pNativeRenderPass;
				pipelineDesc.subpass = 0;
			}
			else
			{
				pipelineDesc.pRenderPass = m_Stages[GraphicsFactory::ST_SCENE_GEOMETRY].pNativeRenderPass;
				pipelineDesc.subpass = 1;
			}

			pipeline.handle = std::make_shared<PipelineHandle>();

			if (m_pDeviceContext->GetNativeDevicePtr()->CreateGraphicsPipeline(
				it_material_pipeline->second.pNativeLayout, pipelineDesc,
				&pipeline.handle->m_pPipeline, VE_INTERFACE_DEBUG_NAME(L"Scene_Material")) != V3D_OK)
			{
				pVertShaderModule->Release();
				pFragShaderModule->Release();

				return false;
			}
		}

		// ----------------------------------------------------------------------------------------------------

		return true;
	}

	bool GraphicsFactory::CreateMaterialSet_Shadow(
		GraphicsFactory::MaterialPipeline& pipeline,
		uint32_t shaderFlags, bool createPipeline,
		size_t boneCount,
		uint32_t vertexStride)
	{
		// ----------------------------------------------------------------------------------------------------
		// パイプラインの取得
		// ----------------------------------------------------------------------------------------------------

		uint32_t pipelineFlags = shaderFlags & MATERIAL_SHADER_SHADOW_PIPELINE_MASK;
		if ((shaderFlags & MATERIAL_SHADER_SHADOW) == 0)
		{
			pipelineFlags |= MATERIAL_SHADER_SHADOW;
		}

		auto it_material_pipeline = m_MaterialPipelineMap.find(pipelineFlags);
		if (it_material_pipeline == m_MaterialPipelineMap.end())
		{
			m_MaterialPipelineMap[pipelineFlags] = GraphicsFactory::Pipeline{};
			it_material_pipeline = m_MaterialPipelineMap.find(pipelineFlags);
		}

		bool createPipelineLayout = (it_material_pipeline->second.pNativeLayout == nullptr);

		// ----------------------------------------------------------------------------------------------------
		// デスクリプタセットレイアウトを作成
		// ----------------------------------------------------------------------------------------------------

		IV3DDescriptorSetLayout* pNativeDescriptorSetLayout = nullptr;

		if (createPipelineLayout == true)
		{
			collection::Vector<V3DDescriptorDesc> descriptors;

			V3DDescriptorDesc desc;
			desc.stageFlags = V3D_SHADER_STAGE_FRAGMENT;
			desc.type = V3D_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			desc.binding = 0;
			descriptors.push_back(desc);

			if (shaderFlags & MATERIAL_SHADER_DIFFUSE_TEXTURE)
			{
				V3DDescriptorDesc desc;
				desc.stageFlags = V3D_SHADER_STAGE_FRAGMENT;
				desc.type = V3D_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				desc.binding = 1;
				descriptors.push_back(desc);
			}

			VE_DEBUG_CODE(StringA debugName = "Scene_MaterialShadow_" + std::bitset<8>(shaderFlags).to_string());
			VE_DEBUG_CODE(ToWideString(debugName.c_str(), it_material_pipeline->second.debugName));

			if (m_pDeviceContext->GetNativeDevicePtr()->CreateDescriptorSetLayout(
				static_cast<uint32_t>(descriptors.size()), descriptors.data(),
				256, 256,
				&pNativeDescriptorSetLayout, VE_INTERFACE_DEBUG_NAME(it_material_pipeline->second.debugName.c_str())) != V3D_OK)
			{
				return false;
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// パイプラインレイアウトを作成
		// ----------------------------------------------------------------------------------------------------

		if (createPipelineLayout == true)
		{
			VE_ASSERT(pNativeDescriptorSetLayout != nullptr);

			V3DConstantDesc constants[1];
			constants[0].shaderStageFlags = V3D_SHADER_STAGE_VERTEX;
			constants[0].offset = 0;
			constants[0].size = sizeof(glm::mat4);

			IV3DDescriptorSetLayout* descriptorSetLayouts[2] =
			{
				m_DescriptorSets[GraphicsFactory::DST_MESH_SHADOW].pNativeLayout,
				pNativeDescriptorSetLayout,
			};

			if (m_pDeviceContext->GetNativeDevicePtr()->CreatePipelineLayout(
				_countof(constants), constants,
				_countof(descriptorSetLayouts), descriptorSetLayouts,
				&it_material_pipeline->second.pNativeLayout, VE_INTERFACE_DEBUG_NAME(it_material_pipeline->second.debugName.c_str())) != V3D_OK)
			{
				pNativeDescriptorSetLayout->Release();
				return false;
			}
		}

		VE_RELEASE(pNativeDescriptorSetLayout);

		// ----------------------------------------------------------------------------------------------------
		// シェーダーを作成
		// ----------------------------------------------------------------------------------------------------

		static constexpr char* SHADER_ENTRY_POINT_NAME = "main";

		IV3DShaderModule* pVertShaderModule;
		IV3DShaderModule* pFragShaderModule;

		if ((createPipeline == true) && (pipeline.handle == nullptr))
		{
			vsc::CompilerOptions options;

			if (shaderFlags & MATERIAL_SHADER_SKELETAL)
			{
				StringA boneCountValue = std::to_string(boneCount);

				options.AddMacroDefinition("SKELETAL_ENABLE");
				options.AddMacroDefinition("BONE_COUNT", boneCountValue.c_str());
			}

			if (shaderFlags & MATERIAL_SHADER_DIFFUSE_TEXTURE)
			{
				options.AddMacroDefinition("TEXTURE_ENABLE");
				options.AddMacroDefinition("DIFFUSE_TEXTURE_ENABLE");
			}

			options.SetOptimize(true);

			vsc::Compiler vertCompiler;
			vsc::Result vertResult;

			vertCompiler.GlslToSpv("meshShadow.vert", VE_GF_Scene_MeshShadow_Vert, vsc::SHADER_TYPE_VERTEX, SHADER_ENTRY_POINT_NAME, &options, &vertResult);
			if (vertResult.GetErrorCount() != 0)
			{
				for (size_t i = 0; i < vertResult.GetErrorCount(); i++)
				{
					OutputDebugStringA(vertResult.GetErrorMessage(i));
					OutputDebugStringA(" vert\n");
				}

				return false;
			}

			vsc::Compiler fragCompiler;
			vsc::Result fragResult;

			fragCompiler.GlslToSpv("meshShadow.frag", VE_GF_Scene_MeshShadow_Frag, vsc::SHADER_TYPE_FRAGMENT, SHADER_ENTRY_POINT_NAME, &options, &fragResult);
			if (fragResult.GetErrorCount() != 0)
			{
				for (size_t i = 0; i < fragResult.GetErrorCount(); i++)
				{
					OutputDebugStringA(fragResult.GetErrorMessage(i));
					OutputDebugStringA(" frag\n");
				}

				return false;
			}

			if (m_pDeviceContext->GetNativeDevicePtr()->CreateShaderModule(vertResult.GetBlobSize(), vertResult.GetBlob(), &pVertShaderModule) != V3D_OK)
			{
				return false;
			}

			if (m_pDeviceContext->GetNativeDevicePtr()->CreateShaderModule(fragResult.GetBlobSize(), fragResult.GetBlob(), &pFragShaderModule) != V3D_OK)
			{
				pVertShaderModule->Release();
				return false;
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// パイプラインを作成
		// ----------------------------------------------------------------------------------------------------

		if ((createPipeline == true) && (pipeline.handle == nullptr))
		{
			uint32_t elementIndex = 0;

			GraphicsPipelineDesc& pipelineDesc = pipeline.desc;

			uint32_t vertexElementCount = 1;
			if (shaderFlags & MATERIAL_SHADER_SKELETAL) { vertexElementCount += 2; }
			if (shaderFlags & MATERIAL_SHADER_TEXTURE_MASK) { vertexElementCount += 1; }

			pipelineDesc.Allocate(vertexElementCount, 1, 1);

			pipelineDesc.vertexInput.elementCount = 0;
			pipelineDesc.vertexInput.layoutCount = 1;

			pipelineDesc.vertexShader.pModule = pVertShaderModule;
			pipelineDesc.vertexShader.pEntryPointName = SHADER_ENTRY_POINT_NAME;

			pipelineDesc.fragmentShader.pModule = pFragShaderModule;
			pipelineDesc.fragmentShader.pEntryPointName = SHADER_ENTRY_POINT_NAME;

			// pos
			pipelineDesc.vertexInput.pElements[elementIndex].format = V3D_FORMAT_R32G32B32_SFLOAT;
			pipelineDesc.vertexInput.pElements[elementIndex].location = 0;
			pipelineDesc.vertexInput.pElements[elementIndex].offset = 0;
			pipelineDesc.vertexInput.elementCount = ++elementIndex;

			if (shaderFlags & MATERIAL_SHADER_DIFFUSE_TEXTURE)
			{
				// uv
				pipelineDesc.vertexInput.pElements[elementIndex].format = V3D_FORMAT_R32G32_SFLOAT;
				pipelineDesc.vertexInput.pElements[elementIndex].location = 1;
				pipelineDesc.vertexInput.pElements[elementIndex].offset = 12;
				pipelineDesc.vertexInput.elementCount = ++elementIndex;
			}

			if (shaderFlags & MATERIAL_SHADER_SKELETAL)
			{
				// indices
				pipelineDesc.vertexInput.pElements[elementIndex].format = V3D_FORMAT_R32G32B32A32_SINT;
				pipelineDesc.vertexInput.pElements[elementIndex].location = 5;
				pipelineDesc.vertexInput.pElements[elementIndex].offset = 56;
				pipelineDesc.vertexInput.elementCount = ++elementIndex;

				// weights
				pipelineDesc.vertexInput.pElements[elementIndex].format = V3D_FORMAT_R32G32B32A32_SFLOAT;
				pipelineDesc.vertexInput.pElements[elementIndex].location = 6;
				pipelineDesc.vertexInput.pElements[elementIndex].offset = 72;
				pipelineDesc.vertexInput.elementCount = ++elementIndex;
			}

			pipelineDesc.vertexInput.pLayouts->binding = 0;
			pipelineDesc.vertexInput.pLayouts->firstElement = 0;
			pipelineDesc.vertexInput.pLayouts->elementCount = elementIndex;
			pipelineDesc.vertexInput.pLayouts->stride = vertexStride;

			pipelineDesc.primitiveTopology = V3D_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

			pipelineDesc.rasterization.polygonMode = V3D_POLYGON_MODE_FILL;
			pipelineDesc.rasterization.cullMode = V3D_CULL_MODE_FRONT;

			pipelineDesc.depthStencil.depthTestEnable = true;
			pipelineDesc.depthStencil.depthWriteEnable = true;
			pipelineDesc.depthStencil.depthCompareOp = V3D_COMPARE_OP_LESS;

			pipelineDesc.colorBlend.pAttachments[0] = InitializeColorBlendAttachment(BLEND_MODE_COPY);

			pipelineDesc.pRenderPass = m_Stages[GraphicsFactory::ST_SCENE_SHADOW].pNativeRenderPass;
			pipelineDesc.subpass = 0;

			pipeline.handle = std::make_shared<PipelineHandle>();

			if (m_pDeviceContext->GetNativeDevicePtr()->CreateGraphicsPipeline(
				it_material_pipeline->second.pNativeLayout, pipelineDesc,
				&pipeline.handle->m_pPipeline, VE_INTERFACE_DEBUG_NAME(L"Scene_Material_Shadow")) != V3D_OK)
			{
				pVertShaderModule->Release();
				pFragShaderModule->Release();

				return false;
			}
		}

		// ----------------------------------------------------------------------------------------------------

		return true;
	}

	bool GraphicsFactory::CreateMaterialSet_Select(
		GraphicsFactory::MaterialPipeline& pipeline,
		uint32_t shaderFlags, bool createPipeline,
		size_t boneCount,
		uint32_t vertexStride)
	{
		// ----------------------------------------------------------------------------------------------------
		// シェーダーを作成
		// ----------------------------------------------------------------------------------------------------

		static constexpr char* SHADER_ENTRY_POINT_NAME = "main";

		bool skeletalEnable = ((shaderFlags & MATERIAL_SHADER_SKELETAL) && (boneCount > 0));

		IV3DShaderModule* pVertShaderModule;
		IV3DShaderModule* pFragShaderModule;

		if ((createPipeline == true) && (pipeline.handle == nullptr))
		{
			vsc::CompilerOptions options;

			if (shaderFlags & MATERIAL_SHADER_SKELETAL)
			{
				StringA boneCountValue = std::to_string(boneCount);

				options.AddMacroDefinition("SKELETAL_ENABLE");
				options.AddMacroDefinition("BONE_COUNT", boneCountValue.c_str());
			}

			options.SetOptimize(true);

			vsc::Compiler vertCompiler;
			vsc::Result vertResult;

			vertCompiler.GlslToSpv("meshSelect.vert", VE_GF_Scene_MeshSelect_Vert, vsc::SHADER_TYPE_VERTEX, SHADER_ENTRY_POINT_NAME, &options, &vertResult);
			if (vertResult.GetErrorCount() != 0)
			{
				for (size_t i = 0; i < vertResult.GetErrorCount(); i++)
				{
					OutputDebugStringA(vertResult.GetErrorMessage(i));
					OutputDebugStringA(" vert\n");
				}

				return false;
			}

			vsc::Compiler fragCompiler;
			vsc::Result fragResult;

			fragCompiler.GlslToSpv("meshSelect.frag", VE_GF_Scene_MeshSelect_Frag, vsc::SHADER_TYPE_FRAGMENT, SHADER_ENTRY_POINT_NAME, &options, &fragResult);
			if (fragResult.GetErrorCount() != 0)
			{
				for (size_t i = 0; i < fragResult.GetErrorCount(); i++)
				{
					OutputDebugStringA(fragResult.GetErrorMessage(i));
					OutputDebugStringA(" frag\n");
				}

				return false;
			}

			if (m_pDeviceContext->GetNativeDevicePtr()->CreateShaderModule(vertResult.GetBlobSize(), vertResult.GetBlob(), &pVertShaderModule) != V3D_OK)
			{
				return false;
			}

			if (m_pDeviceContext->GetNativeDevicePtr()->CreateShaderModule(fragResult.GetBlobSize(), fragResult.GetBlob(), &pFragShaderModule) != V3D_OK)
			{
				pVertShaderModule->Release();
				return false;
			}
		}

		// ----------------------------------------------------------------------------------------------------
		// パイプラインを作成
		// ----------------------------------------------------------------------------------------------------

		if ((createPipeline == true) && (pipeline.handle == nullptr))
		{
			uint32_t elementIndex = 0;

			GraphicsPipelineDesc& pipelineDesc = pipeline.desc;

			uint32_t vertexElementCount = (shaderFlags & MATERIAL_SHADER_SKELETAL) ? 3 : 1;

			pipelineDesc.Allocate(vertexElementCount, 1, 1);

			pipelineDesc.vertexInput.elementCount = 0;
			pipelineDesc.vertexInput.layoutCount = 1;

			pipelineDesc.vertexShader.pModule = pVertShaderModule;
			pipelineDesc.vertexShader.pEntryPointName = SHADER_ENTRY_POINT_NAME;

			pipelineDesc.fragmentShader.pModule = pFragShaderModule;
			pipelineDesc.fragmentShader.pEntryPointName = SHADER_ENTRY_POINT_NAME;

			// pos
			pipelineDesc.vertexInput.pElements[elementIndex].format = V3D_FORMAT_R32G32B32_SFLOAT;
			pipelineDesc.vertexInput.pElements[elementIndex].location = 0;
			pipelineDesc.vertexInput.pElements[elementIndex].offset = 0;
			pipelineDesc.vertexInput.elementCount = ++elementIndex;

			if (shaderFlags & MATERIAL_SHADER_SKELETAL)
			{
				// indices
				pipelineDesc.vertexInput.pElements[elementIndex].format = V3D_FORMAT_R32G32B32A32_SINT;
				pipelineDesc.vertexInput.pElements[elementIndex].location = 5;
				pipelineDesc.vertexInput.pElements[elementIndex].offset = 56;
				pipelineDesc.vertexInput.elementCount = ++elementIndex;

				// weights
				pipelineDesc.vertexInput.pElements[elementIndex].format = V3D_FORMAT_R32G32B32A32_SFLOAT;
				pipelineDesc.vertexInput.pElements[elementIndex].location = 6;
				pipelineDesc.vertexInput.pElements[elementIndex].offset = 72;
				pipelineDesc.vertexInput.elementCount = ++elementIndex;
			}

			pipelineDesc.vertexInput.pLayouts->binding = 0;
			pipelineDesc.vertexInput.pLayouts->firstElement = 0;
			pipelineDesc.vertexInput.pLayouts->elementCount = elementIndex;
			pipelineDesc.vertexInput.pLayouts->stride = vertexStride;

			pipelineDesc.primitiveTopology = V3D_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

			pipelineDesc.rasterization.polygonMode = V3D_POLYGON_MODE_FILL;
			pipelineDesc.rasterization.cullMode = V3D_CULL_MODE_NONE;

			pipelineDesc.depthStencil.depthTestEnable = false;
			pipelineDesc.depthStencil.depthWriteEnable = false;
			pipelineDesc.depthStencil.depthCompareOp = V3D_COMPARE_OP_ALWAYS;

			pipelineDesc.colorBlend.pAttachments[0] = InitializeColorBlendAttachment(BLEND_MODE_COPY);

			pipelineDesc.pRenderPass = m_Stages[GraphicsFactory::ST_SCENE_IMAGE_EFFECT].pNativeRenderPass;
			pipelineDesc.subpass = 1;

			pipeline.handle = std::make_shared<PipelineHandle>();

			if (m_pDeviceContext->GetNativeDevicePtr()->CreateGraphicsPipeline(
				m_Pipelines[GraphicsFactory::PT_MESH_SELECT].pNativeLayout, pipelineDesc,
				&pipeline.handle->m_pPipeline, VE_INTERFACE_DEBUG_NAME(L"Scene_Material_Select")) != V3D_OK)
			{
				pVertShaderModule->Release();
				pFragShaderModule->Release();

				return false;
			}

		}

		// ----------------------------------------------------------------------------------------------------

		return true;
	}

	void GraphicsFactory::LostMaterialSets(DeletingQueue* pDeletingQueue)
	{
		LockGuard<Mutex> lock(m_Mutex);

		if (m_MaterialSetMap.empty() == false)
		{
			auto it_begin = m_MaterialSetMap.begin();
			auto it_end = m_MaterialSetMap.end();

			for (auto it = it_begin; it != it_end; ++it)
			{
				for (auto i = 0; i < GraphicsFactory::MPT_MAX; i++)
				{
					GraphicsFactory::MaterialPipeline& pipeline = it->second.pipelines[i];

					pipeline.desc.pRenderPass = nullptr;
					DeleteDeviceChild(pDeletingQueue, &pipeline.handle->m_pPipeline);
				}
			}
		}
	}

	bool GraphicsFactory::RestoreMaterialSets()
	{
		LockGuard<Mutex> lock(m_Mutex);

		if (m_MaterialSetMap.empty() == false)
		{
			auto it_begin = m_MaterialSetMap.begin();
			auto it_end = m_MaterialSetMap.end();

			IV3DRenderPass* pNativeDifferedColorRenderPass = m_Stages[GraphicsFactory::ST_SCENE_GEOMETRY].pNativeRenderPass;
			IV3DRenderPass* pNativeForwardColorRenderPass = m_Stages[GraphicsFactory::ST_SCENE_FORWARD].pNativeRenderPass;
			IV3DRenderPass* pNativeShadowRenderPass = m_Stages[GraphicsFactory::ST_SCENE_SHADOW].pNativeRenderPass;
			IV3DRenderPass* pNativeSelectRenderPass = m_Stages[GraphicsFactory::ST_SCENE_IMAGE_EFFECT].pNativeRenderPass;

			for (auto it = it_begin; it != it_end; ++it)
			{
				GraphicsFactory::MaterialPipeline& colorPipeline = it->second.pipelines[GraphicsFactory::MPT_COLOR];
				if (colorPipeline.handle != nullptr)
				{
					VE_ASSERT(colorPipeline.desc.pRenderPass == nullptr);
					VE_ASSERT(colorPipeline.handle->m_pPipeline == nullptr);

					uint32_t pipelineFlags = it->first.m_ShaderFlags & MATERIAL_SHADER_PIPELINE_MASK;

					auto it_pipeline = m_MaterialPipelineMap.find(pipelineFlags);
					VE_ASSERT(it_pipeline != m_MaterialPipelineMap.end());
					VE_ASSERT(it_pipeline->second.pNativeLayout != nullptr);

					if (pipelineFlags & MATERIAL_SHADER_TRANSPARENCY)
					{
						colorPipeline.desc.pRenderPass = pNativeForwardColorRenderPass;
					}
					else
					{
						colorPipeline.desc.pRenderPass = pNativeDifferedColorRenderPass;
					}

					if (m_pDeviceContext->GetNativeDevicePtr()->CreateGraphicsPipeline(
						it_pipeline->second.pNativeLayout, colorPipeline.desc,
						&colorPipeline.handle->m_pPipeline, VE_INTERFACE_DEBUG_NAME(L"Scene_Material_Color")) != V3D_OK)
					{
						return false;
					}
				}

				GraphicsFactory::MaterialPipeline& shadowPipeline = it->second.pipelines[GraphicsFactory::MPT_SHADOW];
				if (shadowPipeline.handle != nullptr)
				{
					VE_ASSERT(shadowPipeline.desc.pRenderPass == nullptr);
					VE_ASSERT(shadowPipeline.handle->m_pPipeline == nullptr);

					uint32_t pipelineFlags = it->first.m_ShaderFlags & MATERIAL_SHADER_SHADOW_PIPELINE_MASK;
					if ((pipelineFlags & MATERIAL_SHADER_SHADOW) == 0)
					{
						pipelineFlags |= MATERIAL_SHADER_SHADOW;
					}

					auto it_pipeline = m_MaterialPipelineMap.find(pipelineFlags);
					VE_ASSERT(it_pipeline != m_MaterialPipelineMap.end());
					VE_ASSERT(it_pipeline->second.pNativeLayout != nullptr);

					shadowPipeline.desc.pRenderPass = pNativeShadowRenderPass;

					if (m_pDeviceContext->GetNativeDevicePtr()->CreateGraphicsPipeline(
						it_pipeline->second.pNativeLayout, shadowPipeline.desc,
						&shadowPipeline.handle->m_pPipeline, VE_INTERFACE_DEBUG_NAME(L"Scene_Material_Shadow")) != V3D_OK)
					{
						return false;
					}
				}

				GraphicsFactory::MaterialPipeline& selectPipeline = it->second.pipelines[GraphicsFactory::MPT_SELECT];
				if (selectPipeline.handle != nullptr)
				{
					VE_ASSERT(selectPipeline.desc.pRenderPass == nullptr);
					VE_ASSERT(selectPipeline.handle->m_pPipeline == nullptr);

					selectPipeline.desc.pRenderPass = pNativeSelectRenderPass;

					if (m_pDeviceContext->GetNativeDevicePtr()->CreateGraphicsPipeline(
						m_Pipelines[GraphicsFactory::PT_MESH_SELECT].pNativeLayout, selectPipeline.desc,
						&selectPipeline.handle->m_pPipeline, VE_INTERFACE_DEBUG_NAME(L"Scene_Material_Select")) != V3D_OK)
					{
						return false;
					}
				}
			}
		}

		return true;
	}

	void GraphicsFactory::Lost()
	{
		DeletingQueue* pDeletingQueue = m_pDeviceContext->GetDeletingQueuePtr();

		LostMaterialSets(pDeletingQueue);
		LostStages(pDeletingQueue);
		LostAttachments(pDeletingQueue, false);
	}

	bool GraphicsFactory::Restore()
	{
		if (RestoreAttachments() == false)
		{
			return false;
		}

		if (RestoreStages() == false)
		{
			return false;
		}

		if (RestoreMaterialSets() == false)
		{
			return false;
		}

		return true;
	}

}