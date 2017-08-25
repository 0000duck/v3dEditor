#pragma once

namespace ve {

	class DeviceContext;
	class DeletingQueue;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// バーテックス
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct SimpleVertex
	{
		glm::vec4 pos;
		glm::vec2 uv;
	};

	struct DebugVertex
	{
		glm::vec4 pos;
		glm::vec4 color;
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 定数 ( PushConstant )
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct PasteConstant
	{
		glm::vec4 color;
	};

	struct BrightPassConstant
	{
		float threshold;
		float offset;
	};

	struct DownSamplingConstant
	{
		glm::vec4 texelSize;

		static DownSamplingConstant Initialize(uint32_t width, uint32_t height)
		{
			DownSamplingConstant constant;

			constant.texelSize.x = 1.0f / static_cast<float>(width);
			constant.texelSize.y = 1.0f / static_cast<float>(height);

			return constant;
		}
	};

	struct GaussianBlurConstant
	{
		glm::vec4 step;
		glm::vec4 inc;
		int32_t sampleCount;

		static GaussianBlurConstant Initialize(uint32_t width, uint32_t height, const glm::vec2& dir)
		{
			GaussianBlurConstant constant;

			constant.step.x = 1.0f / static_cast<float>(width) * dir.x;
			constant.step.y = 1.0f / static_cast<float>(height) * dir.y;

			return constant;
		}

		void Set(float radius, int32_t sampleCount)
		{
			static const float TWO_PI_SQR = sqrtf(6.2831853071795f);

			float sigma = radius / 8.0f;
			float sigma2 = sigma * sigma;

			inc.x = 1.0f / (TWO_PI_SQR * sigma);
			inc.y = expf(-0.5f / sigma2);
			inc.z = inc.y * inc.y;

			this->sampleCount = sampleCount;
		}
	};

	struct MeshUniform
	{
		glm::mat4 worldMat;
		uint32_t key;
	};

	struct DirectionalLightingConstant
	{
		glm::vec4 eyePos;
		glm::vec4 lightDir; // xyz=lightDir w=(1.0 / shadowFadeDistance)
		glm::vec4 lightColor;
		glm::vec4 cameraParam; // x=nearClip y=farClip z=fadeStart w=(1.0 / fadeMargin)
		glm::vec4 shadowParam; // xy=texelSize z=bias w=dencity
		glm::mat4 invViewProjMat;
		glm::mat4 lightMat;
	};

	struct SsaoConstant
	{
		glm::vec4 param0; // x=radius y=threshold z=depth w=intesity
		glm::vec4 param1; // xy=noiseTextureScale z=nearClip w=farClip
	};

	struct DirectionalLightingConstantF
	{
		glm::vec4 eyePos;
		glm::vec4 lightDir;
		glm::vec4 lightColor;
	};

	struct SelectMappingConstant
	{
		glm::vec4 texelSize;
		glm::vec4 color;
	};

	struct FxaaConstant
	{
		glm::vec4 texOffset0;
		glm::vec4 texOffset1;
		glm::vec4 invTexSize;
	};

	struct GuiConstant
	{
		glm::vec2 scale;
		glm::vec2 translate;
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// ハンドル
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class FrameBufferHandle
	{
	public:
		IV3DFrameBuffer* GetPtr(uint32_t frameIndex) { return m_FrameBuffers[frameIndex]; }

	private:
		collection::Vector<IV3DFrameBuffer*> m_FrameBuffers;

		friend class GraphicsFactory;
	};

	class PipelineHandle
	{
	public:
		PipelineHandle() : m_pPipeline(nullptr) {}
		IV3DPipeline* GetPtr() { return m_pPipeline; }

	private:
		IV3DPipeline* m_pPipeline;

		friend class GraphicsFactory;
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// グラフィックスファクトリ
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class GraphicsFactory final
	{
	public:
		// ----------------------------------------------------------------------------------------------------
		// 列挙型
		// ----------------------------------------------------------------------------------------------------

		enum VIEWPORT_TYPE
		{
			VT_DEFAULT = 0, // スクリーンサイズ
			VT_HALF = 1, // スクリーンサイズ 1/2
			VT_QUARTER = 2, // スクリーンサイズ 1/4
			VT_SHADOW = 3, // シャドウ

			VT_MAX = 4,
		};

		enum STAGE_TYPE
		{
			// シーン
			ST_SCENE_PASTE = 0,
			ST_SCENE_BRIGHT_PASS = 1,
			ST_SCENE_BLUR = 2,
			ST_SCENE_GEOMETRY = 3,
			ST_SCENE_SSAO = 4,
			ST_SCENE_SHADOW = 5,
			ST_SCENE_LIGHTING = 6,
			ST_SCENE_FORWARD = 7,
			ST_SCENE_IMAGE_EFFECT = 8,
			ST_SCENE_FINISH = 9,

			// グラフィカルユーザーインターフェース
			ST_GUI = 10,

			ST_MAX = 11,
		};

		enum STAGE_SUBPASS_TYPE
		{
			// ST_SCENE_PASTE
			SST_SCENE_PASTE_0_ADD = 0,
			SST_SCENE_PASTE_0_SCREEN = 1,
			SST_SCENE_PASTE_MAX = 2,

			// ST_SCENE_BRIGHT_PASS
			SST_SCENE_BRIGHT_PASS_SAMPLING = 0,
			SST_SCENE_BRIGHT_PASS_MAX = 1,

			// ST_SCENE_BLUR
			SST_SCENE_BLUR_DOWN_SAMPLING = 0,
			SST_SCENE_BLUR_HORIZONAL = 1,
			SST_SCENE_BLUR_VERTICAL = 2,
			SST_SCENE_BLUR_MAX = 3,

			// ST_SCENE_GEOMETRY
			SST_SCENE_GEOMETRY_GRID = 0,
			SST_SCENE_GEOMETRY_MODEL = 1,
			SST_SCENE_GEOMETRY_MAX = 1,

			// ST_SCENE_SSAO
			SST_SCENE_SSAO_SAMPLING = 0,
			SST_SCENE_SSAO_MAX = 1,

			// ST_SCENE_SHADOW
			SST_SCENE_SHADOW_MODEL = 0,
			SST_SCENE_SHADOW_MAX = 0,

			// ST_SCENE_LIGHTING
			SST_SCENE_LIGHTING_DIRECTIONAL = 0,
			SST_SCENE_LIGHTING_FINISH = 1,
			SST_SCENE_LIGHTING_MAX = 2,

			// ST_SCENE_FORWARD
			SST_SCENE_FORWARD_TRANSPARENCY = 0,
			SST_SCENE_FORWARD_MAX = 0,

			// ST_SCENE_IMAGE_EFFECT
			SST_SCENE_IMAGE_EFFECT_TONE_MAPPING = 0,
			SST_SCENE_IMAGE_EFFECT_TONE_MAPPING_OFF = 1,
			SST_SCENE_IMAGE_EFFECT_SELECT_MAPPING = 2,
			SST_SCENE_IMAGE_EFFECT_DEBUG = 3,
			SST_SCENE_IMAGE_EFFECT_FXAA = 4,
			SST_SCENE_IMAGE_EFFECT_FXAA_OFF = 5,
			SST_SCENE_IMAGE_EFFECT_MAX = 6,

			// ST_SCENE_FINISH
			SST_SCENE_FINISH_COPY = 0,
			SST_SCENE_FINISH_MAX = 1,

			// ST_GUI
			SST_GUI_COPY = 0,
			SST_GUI_MAX = 1,
		};

		enum DESCRIPTOR_SET_TYPE
		{
			DST_SIMPLE = 0,
			DST_MESH = 1,
			DST_MESH_SHADOW = 2,
			DST_SSAO = 3,
			DST_DIRECTIONAL_LIGHTING_O = 4,
			DST_FINISH_LIGHTING = 5,

			DST_MAX = 6,
		};

		enum PIPELINE_TYPE
		{
			PT_SIMPLE = 0,
			PT_PASTE = 1,
			PT_BRIGHT_PASS = 2,
			PT_DOWN_SAMPLING = 3,
			PT_GAUSSIAN_BLUR = 4,
			PT_GRID = 5,
			PT_SSAO = 6,
			PT_DIRECTIONAL_LIGHTING = 7,
			PT_FINISH_LIGHTING = 8,
			PT_MESH_SELECT = 9,
			PT_SELECT_MAPPING = 10,
			PT_DEBUG = 11,
			PT_FXAA = 12,
			PT_GUI = 13,

			PT_MAX = 14,
		};

		enum ATTACHMENT_TYPE
		{
			// ブライトパス
			AT_BR_COLOR = 0,

			// ブラー
			AT_BL_HALF_0 = 1,
			AT_BL_HALF_1 = 2,
			AT_BL_QUARTER_0 = 3,
			AT_BL_QUARTER_1 = 4,

			// ジオメトリ
			AT_GE_COLOR = 5,
			AT_GE_BUFFER_0 = 6,
			AT_GE_BUFFER_1 = 7,
			AT_GE_SELECT = 8,
			AT_GE_DEPTH = 9,

			// スクリーンスペースアンビエントオクルージョン
			AT_SS_COLOR = 10,

			// シャドウ
			AT_SA_BUFFER = 11,
			AT_SA_DEPTH = 12,

			// ライティング
			AT_LI_COLOR = 13,
			AT_GL_COLOR = 14,
			AT_SH_COLOR = 15, // AT_GE_COLOR に AT_LI_COLOR を合成したもの

			// イメージエフェクト
			AT_IE_LDR_COLOR_0 = 16,
			AT_IE_LDR_COLOR_1 = 17,
			AT_IE_SELECT_COLOR = 18,

			AT_MAX = 19,
		};

		enum MATERIAL_PIPELINE_TYPE
		{
			MPT_COLOR = 0,
			MPT_SHADOW = 1,
			MPT_SELECT = 2,

			MPT_MAX = 3,
		};

		// ----------------------------------------------------------------------------------------------------
		// メソッド
		// ----------------------------------------------------------------------------------------------------

		glm::vec4 GetBackgroundColor();
		void SetBackgroundColor(const glm::vec4& color);

		const V3DViewport& GetNativeViewport(GraphicsFactory::VIEWPORT_TYPE type) const;

		uint32_t GetShadowResolution() const;
		void SetShadowResolution(uint32_t resolution);
		const glm::vec2& GetShadowTexelSize() const;

		IV3DImageView* GetNativeAttachmentPtr(GraphicsFactory::ATTACHMENT_TYPE type, uint32_t frameIndex);
		const V3DImageDesc& GetNativeAttachmentDesc(GraphicsFactory::ATTACHMENT_TYPE type) const;

		IV3DDescriptorSet* CreateNativeDescriptorSet(GraphicsFactory::DESCRIPTOR_SET_TYPE type);
		IV3DDescriptorSet* CreateNativeDescriptorSet(GraphicsFactory::STAGE_TYPE type, GraphicsFactory::STAGE_SUBPASS_TYPE subpassType, uint32_t descriptorSetLayoutIndex = 0);
		IV3DDescriptorSet* CreateNativeDescriptorSet(MATERIAL_PIPELINE_TYPE type, uint32_t shaderFlags);

		FrameBufferHandlePtr GetFrameBufferHandle(GraphicsFactory::STAGE_TYPE type, uint32_t index = 0);

		PipelineHandlePtr GetPipelineHandle(GraphicsFactory::STAGE_TYPE type, GraphicsFactory::STAGE_SUBPASS_TYPE subpassType);
		PipelineHandlePtr GetPipelineHandle(MATERIAL_PIPELINE_TYPE type, uint32_t shaderFlags, size_t boneCount, uint32_t vertexStride, V3D_POLYGON_MODE polygonMode, V3D_CULL_MODE cullMode, BLEND_MODE blendMode);

	private:
		// ----------------------------------------------------------------------------------------------------
		// 列挙型
		// ----------------------------------------------------------------------------------------------------

		/************************/
		/* シェーダーモジュール */
		/************************/

		enum SHADER_MODULE_TYPE
		{
			SMT_SIMPLE_VERT = 0,
			SMT_SIMPLE_FRAG = 1,
			SMT_PASTE_FRAG = 2,
			SMT_BRIGHT_PASS_FRAG = 3,
			SMT_DOWN_SAMPLING_2x2 = 4,
			SMT_GAUSSIAN_BLUR = 5,
			SMT_GRID_VERT = 6,
			SMT_GRID_FRAG = 7,
			SMT_SSAO_FRAG = 8,
			SMT_DIRECTIONAL_LIGHTING = 9,
			SMT_FINISH_LIGHTING = 10,
			SMT_SELECTMAPPING_FRAG = 11,
			SMT_TONEMAPPING_FRAG = 12,
			SMT_FXAA_FRAG = 13,
			SMT_DEBUG_VERT = 14,
			SMT_DEBUG_FRAG = 15,

			SMT_GUI_VERT = 16,
			SMT_GUI_FRAG = 17,

			SMT_MAX = 18,
		};

		enum ATTACHMENT_SIZE_TYPE
		{
			AST_DEFAULT = 0,
			AST_HALF = 1,
			AST_QUARTER = 2,
			AST_FIXED = 3,
		};

		// ----------------------------------------------------------------------------------------------------
		// 構造体
		// ----------------------------------------------------------------------------------------------------

		/**********************/
		/* デスクリプタセット */
		/**********************/

		struct DescriptorSet
		{
			IV3DDescriptorSetLayout* pNativeLayout;
#ifdef _DEBUG
			StringW debugName;
#endif //_DEBUG
		};

		/****************/
		/* パイプライン */
		/****************/

		struct Pipeline
		{
			IV3DPipelineLayout* pNativeLayout;
#ifdef _DEBUG
			StringW debugName;
#endif //_DEBUG
		};

		/**************/
		/* マテリアル */
		/**************/

		struct MaterialSetKey
		{
		public:
			uint32_t m_ShaderFlags;
			size_t m_BoneCount;
			V3D_POLYGON_MODE m_PolygonMode;
			V3D_CULL_MODE m_CullMode;
			BLEND_MODE m_BlendMode;

			MaterialSetKey() {}

			MaterialSetKey(uint32_t shaderFlags, size_t boneCount, V3D_POLYGON_MODE polygonMode, V3D_CULL_MODE cullMode, BLEND_MODE blendMode) :
				m_ShaderFlags(shaderFlags),
				m_BoneCount(boneCount),
				m_PolygonMode(polygonMode),
				m_CullMode(cullMode),
				m_BlendMode(blendMode)
			{
			}

			bool operator == (const MaterialSetKey& rhs) const
			{
				if ((m_ShaderFlags != rhs.m_ShaderFlags) ||
					(m_BoneCount != rhs.m_BoneCount) ||
					(m_PolygonMode != rhs.m_PolygonMode) ||
					(m_CullMode != rhs.m_CullMode) ||
					(m_BlendMode != rhs.m_BlendMode))
				{
					return false;
				}

				return true;
			}

			bool operator != (const MaterialSetKey& rhs) const
			{
				return !(operator == (rhs));
			}

			uint32_t operator()(const MaterialSetKey& key) const
			{
				uint32_t value0 = (1 << m_PolygonMode);
				uint32_t value1 = (1 << m_CullMode) << 8;
				uint32_t value2 = (1 << m_BlendMode) << 16;

				return value0 | value1 | value2;
			}
		};

		struct MaterialPipeline
		{
			GraphicsPipelineDesc desc;
			PipelineHandlePtr handle;
#ifdef _DEBUG
			StringW debugName;
#endif //_DEBUG
		};

		struct MaterialSet
		{
			MaterialPipeline pipelines[GraphicsFactory::MPT_MAX];
		};

		/******************/
		/* アタッチメント */
		/******************/

		struct Attachment
		{
			ATTACHMENT_SIZE_TYPE sizeType;

			V3DImageDesc imageDesc;
			IV3DImage* pImage;
			ResourceAllocation imageAllocation;

			V3DImageViewDesc imageViewDesc;
			collection::Vector<IV3DImageView*> imageViews;

			V3DFlags stageMask;
			V3DFlags accessMask;
			V3D_IMAGE_LAYOUT layout;
#ifdef _DEBUG
			StringW debugName;
#endif //_DEBUG
		};

		/************/
		/* ステージ */
		/************/

		struct StageSubpass
		{
			GraphicsFactory::PIPELINE_TYPE pipelineType;
			GraphicsPipelineDesc pipelineDesc;
			PipelineHandlePtr pipelineHandle;
#ifdef _DEBUG
			StringW debugName;
#endif //_DEBUG
		};

		struct FrameBuffer
		{
			collection::Vector<uint32_t> attachmentIndices;
			FrameBufferHandlePtr handle;
		};

		struct Stage
		{
			RenderPassDesc renderPassDesc;
			IV3DRenderPass* pNativeRenderPass;
			collection::Vector<GraphicsFactory::FrameBuffer> frameBuffers;
			collection::Vector<StageSubpass> subpasses;
#ifdef _DEBUG
			StringW debugName;
#endif //_DEBUG
		};

		// ----------------------------------------------------------------------------------------------------
		// メンバ
		// ----------------------------------------------------------------------------------------------------

		Mutex m_Mutex;

		DeviceContext* m_pDeviceContext;

		collection::Array1<V3DViewport, GraphicsFactory::VT_MAX> m_Viewports;

		collection::Array1<IV3DShaderModule*, GraphicsFactory::SMT_MAX> m_ShaderModules;
		collection::Array1<GraphicsFactory::DescriptorSet, GraphicsFactory::DST_MAX> m_DescriptorSets;
		collection::Array1<GraphicsFactory::Pipeline, GraphicsFactory::PT_MAX> m_Pipelines;

		collection::Map<uint32_t, GraphicsFactory::Pipeline> m_MaterialPipelineMap;
		collection::HashMap<GraphicsFactory::MaterialSetKey, GraphicsFactory::MaterialSet> m_MaterialSetMap;

		uint32_t m_ShadowResolution;
		glm::vec2 m_ShadowTexelSize;

		collection::Array1<GraphicsFactory::Attachment, GraphicsFactory::AT_MAX> m_Attachments;

		collection::Array1<GraphicsFactory::Stage, GraphicsFactory::ST_MAX> m_Stages;

		// ----------------------------------------------------------------------------------------------------
		// メソッド
		// ----------------------------------------------------------------------------------------------------

		static GraphicsFactory* Create(DeviceContext* pDeviceContext);
		void Destroy();

		GraphicsFactory(DeviceContext* pDeviceContext);
		~GraphicsFactory();

		bool Initialize();

		bool InitializeBase();

		bool InitializeAttachments();
		void LostAttachments(DeletingQueue* pDeletingQueue, bool force);
		bool RestoreAttachments();

		bool InitializeStages();
		void LostStages(DeletingQueue* pDeletingQueue);
		bool RestoreStages();

		bool CreateMaterialSet(
			GraphicsFactory::MaterialSet** ppMaterialSet,
			uint32_t shaderFlags, bool createPipeline,
			size_t boneCount = 0,
			uint32_t vertexStride = 0,
			V3D_POLYGON_MODE polygonMode = V3D_POLYGON_MODE_FILL, V3D_CULL_MODE cullMode = V3D_CULL_MODE_BACK,
			BLEND_MODE blendMode = BLEND_MODE_COPY);

		bool CreateMaterialSet_Color(
			GraphicsFactory::MaterialPipeline& pipeline,
			uint32_t shaderFlags, bool createPipeline,
			size_t boneCount,
			uint32_t vertexStride,
			V3D_POLYGON_MODE polygonMode, V3D_CULL_MODE cullMode,
			BLEND_MODE blendMode);

		bool CreateMaterialSet_Shadow(
			GraphicsFactory::MaterialPipeline& pipeline,
			uint32_t shaderFlags, bool createPipeline,
			size_t boneCount,
			uint32_t vertexStride);

		bool CreateMaterialSet_Select(
			GraphicsFactory::MaterialPipeline& pipeline,
			uint32_t shaderFlags, bool createPipeline,
			size_t boneCount,
			uint32_t vertexStride);

		void LostMaterialSets(DeletingQueue* pDeletingQueue);
		bool RestoreMaterialSets();

		void Lost();
		bool Restore();

		friend class DeviceContext;

		VE_DECLARE_ALLOCATOR
	};

}