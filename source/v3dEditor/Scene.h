#pragma once

#include "GraphicsFactory.h"
#include "DynamicContainer.h"
#include "Frustum.h"

namespace ve {

	class DeletingQueue;
	class DynamicBuffer;
	class NodeSelector;
	class DebugRenderer;

	class Scene final
	{
	public:
		static auto constexpr GlarePageCount = 2;

		struct Grid
		{
			bool enable;
			float cellSize;
			uint32_t halfCellCount;
			glm::vec4 color;
		};

		enum SHADOW_RESOLUTION
		{
			SHADOW_RESOLUTION_1024 = 0,
			SHADOW_RESOLUTION_2048 = 1,
			SHADOW_RESOLUTION_4096 = 2,
		};

		struct Ssao
		{
			bool enable;
			float radius;
			float threshold;
			float depth;
			float intensity;

			float blurRadius;
			int32_t blurSamples;
		};

		struct Shadow
		{
			bool enable;
			Scene::SHADOW_RESOLUTION resolution;
			float range;
			float fadeMargin;
			float dencity;
			float centerOffset;
			float boundsBias;
			float depthBias;
		};

		struct Glare
		{
			bool enable;

			float bpThreshold;
			float bpOffset;

			float blurRadius;
			int32_t blurSamples;

			float intensity[Scene::GlarePageCount];
		};

		struct ImageEffect
		{
			bool toneMappingEnable;
			bool fxaaEnable;
		};

		static ScenePtr Create(DeviceContextPtr deviceContext);

		Scene();
		~Scene();

		glm::vec4 GetBackgroundColor();
		void SetBackgroundColor(const glm::vec4& color);

		const Scene::Grid& GetGrid() const;
		void SetGrid(const Scene::Grid& grid);

		const Scene::Ssao& GetSsao() const;
		void SetSsao(const Scene::Ssao& ssao);

		const Scene::Shadow& GetShadow() const;
		void SetShadow(const Scene::Shadow& shadow);

		const Scene::Glare& GetGlare() const;
		void SetGlare(const Scene::Glare& glare);

		const Scene::ImageEffect& GetImageEffect() const;
		void SetImageEffect(const Scene::ImageEffect& imageEffect);

		const glm::vec4 GetSelectColor() const;
		void SetSelectColor(const glm::vec4& color);
		NodePtr GetSelect();
		NodePtr Select(const glm::ivec2& pos);
		void Select(NodePtr node);
		void Deselect();

		uint32_t GetDebugDrawFlags() const;
		void SetDebugDrawFlags(uint32_t flags);
		const glm::vec4& GetDebugColor(DEBUG_COLOR_TYPE type) const;
		void SetDebugColor(DEBUG_COLOR_TYPE type, const glm::vec4& color);

		CameraPtr GetCamera();
		LightPtr GetLight();

		NodePtr GetRootNode();
		NodePtr AddNode(NodePtr parent, const wchar_t* pName, uint32_t groupFlags, ModelPtr model);
		void RemoveNodeByGroup(NodePtr parent, uint32_t groupFlags);

		void Update();
		void Render();

		void Lost();
		bool Restore();

		void Dispose();

		VE_DECLARE_ALLOCATOR

	private:
		static constexpr uint64_t OpacityDrawSet_DefaultCount = 1024;
		static constexpr uint64_t OpacityDrawSet_ResizeStep = 256;

		static constexpr uint64_t TransparencyDrawSet_DefaultCount = OpacityDrawSet_DefaultCount * 4;
		static constexpr uint64_t TransparencyDrawSet_ResizeStep = OpacityDrawSet_ResizeStep * 4;

		static constexpr uint64_t ShadowDrawSet_DefaultCount = 1024;
		static constexpr uint64_t ShadowDrawSet_ResizeStep = 256;

		// ----------------------------------------------------------------------------------------------------

		struct PasteStage
		{
			FrameBufferHandlePtr frameBufferHandle;

			struct Sampling
			{
				collection::Array1<PipelineHandlePtr, 2> pipelineHandle;
			}sampling;
		};

		struct BrightPassStage
		{
			FrameBufferHandlePtr frameBufferHandle;

			struct Sampling
			{
				PipelineHandlePtr pipelineHandle;
			}sampling;
		};

		struct BlurStage
		{
			collection::Array1<FrameBufferHandlePtr, 2> frameBufferHandle;

			struct DownSampling
			{
				PipelineHandlePtr pipelineHandle;
			}downSampling;

			struct Horizonal
			{
				PipelineHandlePtr pipelineHandle;
				collection::Array1<collection::Vector<IV3DDescriptorSet*>, 2> descriptorSets;
			}horizonal;

			struct Vertical
			{
				PipelineHandlePtr pipelineHandle;
				collection::Array1<collection::Vector<IV3DDescriptorSet*>, 2> descriptorSets;
			}vertical;
		};

		struct GeometryStage
		{
			FrameBufferHandlePtr frameBufferHandle;

			struct Grid
			{
				PipelineHandlePtr pipelineHandle;
				DynamicBuffer* pVertexBuffer;
				uint32_t vertexCount;
			}grid;
		};

		struct SsaoStage
		{
			FrameBufferHandlePtr frameBufferHandle;

			struct Sampling
			{
				SsaoConstant constant;
				PipelineHandlePtr pipelineHandle;
				collection::Vector<IV3DDescriptorSet*> descriptorSets;
				TexturePtr noiseTexture;
			}sampling;

			struct Blur
			{
				DownSamplingConstant constantDS;
				GaussianBlurConstant constantH;
				GaussianBlurConstant constantV;
				collection::Vector<IV3DDescriptorSet*> descriptorSets;
			}blur;
		};

		struct ShadowStage
		{
			FrameBufferHandlePtr frameBufferHandle;
		};

		struct LightingStage
		{
			FrameBufferHandlePtr frameBufferHandle;

			struct Directional
			{
				DirectionalLightingConstant constant;
				PipelineHandlePtr pipelineHandle;
				collection::Vector<IV3DDescriptorSet*> descriptorSets;
			}directional;

			struct Finish
			{
				PipelineHandlePtr pipelineHandle;
				collection::Vector<IV3DDescriptorSet*> descriptorSets;
			}finish;
		};

		struct GlareStage
		{
			struct BrightPass
			{
				BrightPassConstant constant;
				collection::Vector<IV3DDescriptorSet*> descriptorSets;
			}brightPass;

			struct Blur
			{
				DownSamplingConstant constantDS;
				GaussianBlurConstant constantH;
				GaussianBlurConstant constantV;
				collection::Vector<IV3DDescriptorSet*> descriptorSets;
			}blur[2];

			struct Paste
			{
				PasteConstant constant;
				collection::Vector<IV3DDescriptorSet*> descriptorSets;
			}paste[2];
		};

		struct ForwardStage
		{
			FrameBufferHandlePtr frameBufferHandle;

			struct Transparency
			{
				DirectionalLightingConstantF constant;
			}transparency;
		};

		struct ImageEffectStage
		{
			FrameBufferHandlePtr frameBufferHandle;

			struct ToneMapping
			{
				PipelineHandlePtr pipelineHandle[2];
				collection::Vector<IV3DDescriptorSet*> descriptorSets;
			}toneMapping;

			struct SelectMapping
			{
				SelectMappingConstant constant;
				PipelineHandlePtr pipelineHandle;
				collection::Vector<IV3DDescriptorSet*> descriptorSets;
			}selectMapping;

			struct Debug
			{
				PipelineHandlePtr pipelineHandle;
			}debug;

			struct Fxaa
			{
				FxaaConstant constant;
				PipelineHandlePtr pipelineHandle[2];
				collection::Vector<IV3DDescriptorSet*> descriptorSets;
			}fxaa;
		};

		struct FinishStage
		{
			FrameBufferHandlePtr frameBufferHandle;

			PipelineHandlePtr pipelineHandle;
			collection::Vector<IV3DDescriptorSet*> descriptorSets;
		};

		// ----------------------------------------------------------------------------------------------------

		DeviceContextPtr m_DeviceContext;
		DeletingQueue* m_pDeletingQueue;

		Buffer m_SimpleVertexBuffer;

		Scene::PasteStage m_PasteStage;
		Scene::BrightPassStage m_BrightPassStage;
		Scene::BlurStage m_BlurStage;

		Scene::GeometryStage m_GeometoryStage;
		Scene::SsaoStage m_SsaoStage;
		Scene::ShadowStage m_ShadowStage;
		Scene::LightingStage m_LightingStage;
		Scene::ForwardStage m_ForwardStage;
		Scene::GlareStage m_GlareStage;
		Scene::ImageEffectStage m_ImageEffectStage;
		Scene::FinishStage m_FinishStage;

		V3DViewport m_DefaultViewport;
		V3DViewport m_HalfViewport;
		V3DViewport m_QuarterViewport;

		Scene::Grid m_Grid;
		Scene::Ssao m_Ssao;
		Scene::Shadow m_Shadow;
		Scene::Glare m_Glare;
		Scene::ImageEffect m_ImageEffect;
		uint32_t m_DebugDrawFlags;

		NodePtr m_RootNode;

		CameraPtr m_Camera;
		LightPtr m_Light;

		Frustum m_Frustum;
		Sphere m_ShadowBounds;
		glm::mat4 m_LightMatrix;

		collection::DynamicContainer<OpacityDrawSet> m_OpacityDrawSets;
		collection::DynamicContainer<TransparencyDrawSet> m_TransparencyDrawSets;
		collection::DynamicContainer<ShadowDrawSet> m_ShadowDrawSets;
		SelectDrawSet m_SelectDrawSet;

		NodeSelector* m_pNodeSelector;
		Buffer m_SelectBuffer;

		DebugRenderer* m_pDebugRenderer;

		// ----------------------------------------------------------------------------------------------------

		bool Initialize(DeviceContextPtr deviceContext);

		bool InitializeBase();

		bool InitializeStages();
		void LostStages();
		bool RestoreStages();

		void UpdateGrid();

		void InternalClear();

		void RenderGeometry(IV3DCommandBuffer* pCommandBuffer, uint32_t frameIndex);
		void RenderIllumination(IV3DCommandBuffer* pCommandBuffer, uint32_t frameIndex);
		void RenderForward(IV3DCommandBuffer* pCommandBuffer, uint32_t frameIndex);
		void RenderGlare(IV3DCommandBuffer* pCommandBuffer, uint32_t frameIndex);
		void RenderImageEffect(IV3DCommandBuffer* pCommandBuffer, uint32_t frameIndex);
		void RenderFinish(IV3DCommandBuffer* pCommandBuffer, uint32_t frameIndex);

		static void SortOpacityDrawSet(OpacityDrawSet** list, int64_t first, int64_t last);
		static void SortTransparencyDrawSet(TransparencyDrawSet** list, int64_t first, int64_t last);
	};

}