#pragma once

#include <SDKDDKVer.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif //WIN32_LEAN_AND_MEAN

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif //_WIN32_WINNT

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif //GLM_ENABLE_EXPERIMENTAL

#define VE_NAME_A "v3dEditor"
#define VE_NAME_W L"v3dEditor"

#define VE_VERSION_A "1.0.0"
#define VE_VERSION_W L"1.0.0"

#include <Windowsx.h>
#include <stdint.h>
#include <memory>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <algorithm>
#include <bitset>
#include <mutex>
#include <array>
#include <unordered_map>
#include <map>
#include <set>
#include <stack>
#include <Shlwapi.h>

// ImGui 対策
#ifdef GetWindowFont
#undef GetWindowFont
inline HFONT GetWindowFont(HWND hWnd)
{
	return FORWARD_WM_GETFONT(hWnd, SNDMSG);
}
#endif //GetWindowFont

#include <v3d.h>
#include <glm\glm.hpp>
#include <glm\gtx\quaternion.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtx\matrix_decompose.hpp>
#include <gli\gli.hpp>
#include <imgui.h>

#ifdef _DEBUG
#include <cassert>
#include <crtdbg.h>
#endif //_DEBUG

namespace ve {

	void Initialize();
	void Finalize();

	enum MEMORY_TYPE
	{
		MEMORY_TYPE_GENERAL = 0,

		MEMORY_TYPE_COUNT = 1,
	};

	void* AllocateMemory(size_t count, uint8_t category, const wchar_t* file, size_t line);
	void* AllocateAlignedMemory(size_t align, size_t count, uint8_t category, const wchar_t* file, size_t line);
	void* ReallocMemory(void* ptr, size_t count, const wchar_t* file, size_t line);
	void* ReallocAlignedMemory(void* ptr, size_t align, size_t count, const wchar_t* file, size_t line);
	void FreeMemory(void* ptr, const wchar_t* file, size_t line);

}

#ifdef _DEBUG
#define VE_ASSERT(cond) assert(cond)
#define VE_DEBUG_CODE(code) code
#define VE_INTERFACE_DEBUG_NAME(name) name
#else //_DEBUG
#define VE_ASSERT(cond)
#define VE_DEBUG_CODE(code)
#define VE_INTERFACE_DEBUG_NAME(name) nullptr
#endif //_DEBUG

#if 1

#define VE_DECLARE_ALLOCATOR

#define VE_MALLOC(size) _aligned_malloc(size, sizeof(void*))

#define VE_ALIGNED_MALLOC(align, size) _aligned_malloc(size, align)
#define VE_ALIGNED_REALLOC(ptr, align, size) _aligned_realloc(ptr, size, align)

#define VE_MALLOC_T(T, count) reinterpret_cast<T*>(_aligned_malloc(sizeof(T) * count, sizeof(void*)))
#define VE_REALLOC_T(ptr, T, count) reinterpret_cast<T*>(_aligned_realloc(ptr, sizeof(T) * count, sizeof(void*)))
#define VE_FREE(ptr) _aligned_free(ptr)

#define VE_NEW_T(T, ...) new T(__VA_ARGS__)
#define VE_NEW_ALIGNED_T(align, T, ...) new T(__VA_ARGS__)

#define VE_DELETE_T(ptr, T) if( ptr != NULL ) { delete ptr; ptr = nullptr; }
#define VE_DELETE_THIS_T(ptr, T) delete ptr

#define VE_NEW_ARRAY_T(T, count) new T[count]
#define VE_ALIGNED_NEW_ARRAY_T( align, T, count ) new T[count]
#define VE_DELETE_ARRAY_T(ptr, T) if(ptr != nullptr) { delete [] ptr; ptr = nullptr; }

#else

#define VE_DECLARE_ALLOCATOR \
	inline void* operator new( size_t size, uint8_t category, const wchar_t* file, size_t line ) { return ve::AllocateMemory(size, category, file, line); } \
	inline void operator delete( void* ptr, uint8_t category, const wchar_t* file, size_t line ) {} \
	inline void* operator new[]( size_t size, uint8_t category, const wchar_t* file, size_t line ) { return ve::AllocateMemory(size, category, file, line); } \
	inline void operator delete[]( void* ptr, uint8_t category, const wchar_t* file, size_t line ) {}
	inline void* operator new( size_t size, size_t align, uint8_t category, const wchar_t* file, size_t line ) { return ve::AllocateAlignedMemory(align, size, category, file, line); } \
	inline void operator delete( void* ptr, size_t align, uint8_t category, const wchar_t* file, size_t line ) {} \
	inline void* operator new[]( size_t size, size_t align, uint8_t category, const wchar_t* file, size_t line ) { return ve::AllocateAlignedMemory(align, size, category, file, line); } \
	inline void operator delete[]( void* ptr, size_t align, uint8_t category, const wchar_t* file, size_t line ) {}

#define VE_MALLOC(size) ve::AllocateMemory(size, ve::MEMORY_TYPE_GENERAL, __FILEW__, __LINE__)

#define VE_ALIGNED_MALLOC(align, size) ve::AllocateAlignedMemory(align, size, ve::MEMORY_TYPE_GENERAL, __FILEW__, __LINE__)
#define VE_ALIGNED_REALLOC(ptr, align, size) ve::ReallocAlignedMemory(ptr, align, size, __FILEW__, __LINE__)

#define VE_MALLOC_T(T, count) reinterpret_cast<T*>(ve::AllocateMemory(sizeof(T) * count, ve::MEMORY_TYPE_GENERAL, __FILEW__, __LINE__))
#define VE_REALLOC_T(ptr, T, count) reinterpret_cast<T*>(ve::ReallocMemory(ptr, sizeof(T) * count, __FILEW__, __LINE__))
#define VE_FREE(ptr) ve::FreeMemory(ptr, __FILEW__, __LINE__)

#define VE_NEW_T(T, ...) new(ve::MEMORY_TYPE_GENERAL, __FILEW__, __LINE__) T(__VA_ARGS__)
#define VE_NEW_ALIGNED_T(align, T, ...) new(align, ve::MEMORY_TYPE_GENERAL, __FILEW__, __LINE__) T(__VA_ARGS__)

#define VE_DELETE_T(ptr, T) if( ptr != NULL ) { ptr->~T(); ve::FreeMemory(ptr, __FILEW__, __LINE__); ptr = nullptr; }
#define VE_DELETE_THIS_T(ptr, T) ptr->~T(); ve::FreeMemory(ptr, __FILEW__, __LINE__);

#define VE_NEW_ARRAY_T(T, count) new( ve::MEMORY_TYPE_GENERAL, __FILEW__, __LINE__ ) T[count]
#define VE_ALIGNED_NEW_ARRAY_T( align, T, count ) new( align, ve::MEMORY_TYPE_GENERAL, __FILEW__, __LINE__ ) T[count]

#ifdef _DEBUG
#define VE_DELETE_ARRAY_T_MEM_CHECK VE_ASSERT(realPtr != nullptr)
#else //_DEBUG
#define VE_DELETE_ARRAY_T_MEM_CHECK
#endif //_DEBUG

#define VE_DELETE_ARRAY_T(ptr, T) \
	if (ptr != nullptr) \
	{ \
		uint32_t* temp = (uint32_t*)(ptr); \
		void* realPtr = nullptr; \
		uint32_t count = 0; \
		 \
		if (*(temp - 1) == 0x56474D56) \
		{ \
			realPtr = temp; \
			count = 0; \
		} \
		else if (*(temp - 2) == 0x56474D56) \
		{ \
			realPtr = (temp - 1); \
			count = *(temp - 1); \
		} \
		else if (*(temp - 3) == 0x56474D56) \
		{ \
			realPtr = (temp - 2); \
			count = *(temp - 2); \
		} \
		 \
		VE_DELETE_ARRAY_T_MEM_CHECK \
		 \
		if (count > 0) \
		{ \
			for (uint32_t i = 0; i < count; i++) \
			{ \
				(ptr)[i].~T(); \
			} \
		} \
		 \
		ve::FreeMemory(realPtr, __FILEW__, __LINE__); \
		 \
		ptr = nullptr; \
	}

#endif

#define VE_RELEASE(obj) if(obj != nullptr) { (obj)->Release(); (obj) = nullptr; }

#define VE_PI 3.14159265359f

#define VE_FLOAT_MAX 3.402823466e+38F
#define VE_FLOAT_EPSILON 1.192092896e-07F
#define VE_FLOAT_IS_ZERO(value) ((-VE_FLOAT_EPSILON <= value) && (value <= +VE_FLOAT_EPSILON))
#define VE_FLOAT_DIV(a, b) ((VE_FLOAT_IS_ZERO(b) == false)? ((a) / (b)) : 0.0f)
#define VE_FLOAT_RECIPROCAL(value) ((VE_FLOAT_IS_ZERO(value) == false)? (1.0f / (value)) : 0.0f)

#define VE_DOUBLE_EPSILON 2.2204460492503131e-016
#define VE_DOUBLE_IS_ZERO(value) ((-VE_DOUBLE_EPSILON <= value) && (value <= +VE_DOUBLE_EPSILON))
#define VE_DOUBLE_DIV(a, b) ((VE_DOUBLE_IS_ZERO(b) == false)? ((a) / (b)) : 0.0)
#define VE_DOUBLE_RECIPROCAL(value) ((VE_DOUBLE_IS_ZERO(value) == false)? (1.0 / (value)) : 0.0)

#define VE_SET_BIT(flag, bit) if((flag & (bit)) == 0) { flag |= (bit); }
#define VE_RESET_BIT(flag, bit) if((flag & (bit)) != 0) { flag ^= (bit); }
#define VE_TEST_BIT(flag, bit) (flag & (bit))

#define VE_CLAMP(value, minValue, maxValue) ((minValue) > value)? (value = (minValue)) : (((maxValue) < value)? (value = (maxValue)) : value)

namespace ve {

	// ----------------------------------------------------------------------------------------------------
	// 設定
	// ----------------------------------------------------------------------------------------------------

	// リソースメモリの最小サイズ ( ResourceMemoryManager )
	static constexpr uint64_t MIN_BUFFER_MEMORY_SIZE = 1024 * 1024 * 1;
	static constexpr uint64_t MIN_IMAGE_MEMORY_SIZE = 1024 * 1024 * 2;

	// ----------------------------------------------------------------------------------------------------
	// 定義
	// ----------------------------------------------------------------------------------------------------

	typedef std::mutex Mutex;

	template<class T>
	using LockGuard = std::lock_guard<T>;

	template<uint8_t memoryType, typename T>
	class STLAllocator
	{
	public:
		typedef T value_type;
		typedef T *pointer;
		typedef const T *const_pointer;
		typedef T &reference;
		typedef const T &const_reference;
		typedef size_t size_type;
		typedef ptrdiff_t difference_type;

		template <class U>
		struct rebind { typedef STLAllocator<memoryType, U> other; };

	public:
		STLAllocator(void) {}
		STLAllocator(const STLAllocator&) {}

		template <class U>
		STLAllocator(const STLAllocator<memoryType, U>&) {}

		~STLAllocator(void) {}

		pointer allocate(size_type num, void *hint = 0) { (void)hint; return (pointer)(ve::AllocateMemory(sizeof(T) * num, memoryType, __FILEW__, __LINE__)); }
		void deallocate(pointer p, size_type num) { (void)num; ve::FreeMemory(p, __FILEW__, __LINE__); }

		void construct(pointer p, const T& value) { new(p) T(value); }
		void destroy(pointer p) { p->~T(); }
		
		template<class _Uty>
		void destroy(_Uty *_Ptr)
		{
			_Ptr->~_Uty();
		}

		pointer address(reference value) const { return &value; }
		const_pointer address(const_reference value) const { return &value; }

		size_type max_size() const { return (std::numeric_limits<size_t>::max)() / sizeof(T); }

		template<class _Other>
		STLAllocator<memoryType, T>& operator=(const STLAllocator<memoryType, _Other>&)
		{
			return (*this);
		}
	};

	typedef std::string StringA;
	typedef std::wstring StringW;

	template<typename T>
	using SharedPtr = std::shared_ptr<T>;

	template<typename T>
	using WeakPtr = std::weak_ptr<T>;

	template<typename T>
	struct DeleteUniqueRefernece
	{
		void operator()(T* obj) const
		{
			obj->Release();
		}
	};

	template<typename T>
	using UniqueRefernecePtr = std::unique_ptr<T, DeleteUniqueRefernece<T>>;

	namespace collection {

		template<class T, size_t count>
		using Array1 = std::array<T, count>;

		template<class T, size_t count1, size_t count2>
		using Array2 = std::array<std::array<T, count2>, count1>;

		template<class T, size_t count1, size_t count2, size_t count3>
		using Array3 = std::array<std::array<std::array<T, count3>, count2>, count1>;

		template<class T>
		using Vector = std::vector<T, STLAllocator<MEMORY_TYPE_GENERAL, T>>;

		template<class T>
		using List = std::list<T, STLAllocator<MEMORY_TYPE_GENERAL, T>>;

		template<class T>
		using Set = std::set<T, STLAllocator<MEMORY_TYPE_GENERAL, T>>;

		template<class Key, class T, class Pr = std::less<Key>>
		using Map = std::map<Key, T, Pr, STLAllocator<MEMORY_TYPE_GENERAL, std::pair<const Key, T>>>;

		template<class Key, class T, class KeyEq = std::equal_to<Key>>
		using HashMap = std::unordered_map<Key, T, Key, KeyEq, STLAllocator<MEMORY_TYPE_GENERAL, std::pair<const Key, T>>>;
	}

	class Logger;
	class BackgroundJobHandle;
	class BackgroundQueue;
	class Device;
	class DeviceContext;
	class UpdatingHandle;
	class FrameBufferHandle;
	class PipelineHandle;
	class Node;
	class NodeAttribute;
	class Scene;
	class Camera;
	class Material;
	class Texture;
	class Light;
	class MainLight;
	class IModelSource;
	class IMaterialContainer;
	class IModel;
	class IMesh;
	class SkeletalModel;
	class SkeletalMesh;
	class Gui;
	class Project;

	typedef SharedPtr<Logger> LoggerPtr;
	typedef SharedPtr<BackgroundJobHandle> BackgroundJobHandlePtr;
	typedef SharedPtr<BackgroundQueue> BackgroundQueuePtr;
	typedef SharedPtr<Device> DevicePtr;
	typedef SharedPtr<DeviceContext> DeviceContextPtr;
	typedef SharedPtr<UpdatingHandle> UpdatingHandlePtr;
	typedef SharedPtr<FrameBufferHandle> FrameBufferHandlePtr;
	typedef SharedPtr<PipelineHandle> PipelineHandlePtr;
	typedef SharedPtr<Node> NodePtr;
	typedef SharedPtr<NodeAttribute> NodeAttributePtr;
	typedef SharedPtr<Scene> ScenePtr;
	typedef SharedPtr<Camera> CameraPtr;
	typedef SharedPtr<Material> MaterialPtr;
	typedef SharedPtr<Texture> TexturePtr;
	typedef SharedPtr<Light> LightPtr;
	typedef SharedPtr<IModelSource> ModelSourcePtr;
	typedef SharedPtr<IMaterialContainer> MaterialContainerPtr;
	typedef SharedPtr<IModel> ModelPtr;
	typedef SharedPtr<IMesh> MeshPtr;
	typedef SharedPtr<SkeletalModel> SkeletalModelPtr;
	typedef SharedPtr<SkeletalMesh> SkeletalMeshPtr;
	typedef SharedPtr<Gui> GuiPtr;
	typedef SharedPtr<Project> ProjectPtr;

	typedef struct ResourceAllocationT *ResourceAllocation;

	// ブレンドモード
	enum BLEND_MODE
	{
		BLEND_MODE_COPY = 0,
		BLEND_MODE_NORMAL = 1,
		BLEND_MODE_ADD = 2,
		BLEND_MODE_SUB = 3,
		BLEND_MODE_MUL = 4,
		BLEND_MODE_SCREEN = 5,

		BLEND_MODE_COUNT = 6,
	};

	// マテリアルシェーダーフラグ
	enum MATERIAL_SHADER_FLAG
	{
		MATERIAL_SHADER_SKELETAL = 0x00000001,
		MATERIAL_SHADER_DIFFUSE_TEXTURE = 0x00000002,
		MATERIAL_SHADER_SPECULAR_TEXTURE = 0x00000004,
		MATERIAL_SHADER_BUMP_TEXTURE = 0x00000008,
		MATERIAL_SHADER_TRANSPARENCY = 0x00000010,

		MATERIAL_SHADER_SHADOW = 0x00000020,

		MATERIAL_SHADER_TEXTURE_MASK = MATERIAL_SHADER_DIFFUSE_TEXTURE | MATERIAL_SHADER_SPECULAR_TEXTURE | MATERIAL_SHADER_BUMP_TEXTURE,
		MATERIAL_SHADER_PIPELINE_MASK = MATERIAL_SHADER_DIFFUSE_TEXTURE | MATERIAL_SHADER_SPECULAR_TEXTURE | MATERIAL_SHADER_BUMP_TEXTURE | MATERIAL_SHADER_TRANSPARENCY,
		MATERIAL_SHADER_SHADOW_PIPELINE_MASK = MATERIAL_SHADER_DIFFUSE_TEXTURE | MATERIAL_SHADER_SHADOW,
	};

	// デバッグ描画タイプ
	enum DEBUG_DRAW_FLAG
	{
		DEBUG_DRAW_MESH_BOUNDS = 0x00000001, // メッシュの境界ボックス
		DEBUG_DRAW_LIGHT_SHAPE = 0x00000002, // ライト
	};

	// デバッグカラータイプ
	enum DEBUG_COLOR_TYPE
	{
		DEBUG_COLOR_TYPE_MESH_BOUNDS = 0, // メッシュの境界ボックス
		DEBUG_COLOR_TYPE_LIGHT_SHAPE = 1, // ライト

		DEBUG_COLOR_TYPE_COUNT = 2,
	};

	// ----------------------------------------------------------------------------------------------------
	// クラス
	// ----------------------------------------------------------------------------------------------------

	struct RenderPassDesc
	{
	public:
		struct Subpass
		{
			collection::Vector<V3DAttachmentReference> inputAttachments;
			collection::Vector<V3DAttachmentReference> colorAttachments;
			V3DAttachmentReference depthStencilAttachment;
			collection::Vector<uint32_t> preserveAttachments;
		};

		uint32_t attachmentCount;
		V3DAttachmentDesc* pAttachments;

		uint32_t subpassCount;
		V3DSubpassDesc* pSubpasses;

		uint32_t subpassDependencyCount;
		V3DSubpassDependencyDesc* pSubpassDependencies;

		void Allocate(uint32_t attachmentCount, uint32_t subpassCount, uint32_t subpassDependencyCount)
		{
			m_Attachments.resize(attachmentCount);
			m_Subpasses.resize(subpassCount);
			m_InternbalSubpasses.resize(subpassCount);
			m_SubpassDependencies.resize(subpassDependencyCount);

			this->attachmentCount = static_cast<uint32_t>(m_Attachments.size());
			pAttachments = m_Attachments.data();

			this->subpassCount = static_cast<uint32_t>(m_Subpasses.size());
			pSubpasses = m_Subpasses.data();

			this->subpassDependencyCount = static_cast<uint32_t>(m_SubpassDependencies.size());
			pSubpassDependencies = m_SubpassDependencies.data();
		}

		RenderPassDesc::Subpass* AllocateSubpass(uint32_t subpass, uint32_t inputAttachmentCount, uint32_t colorAttachmentCount, bool depthStencilEnable, uint32_t preserveAttachmentCount)
		{
			m_InternbalSubpasses[subpass].inputAttachments.resize(inputAttachmentCount);
			m_InternbalSubpasses[subpass].colorAttachments.resize(colorAttachmentCount);
			m_InternbalSubpasses[subpass].preserveAttachments.resize(preserveAttachmentCount);

			m_Subpasses[subpass].inputAttachmentCount = static_cast<uint32_t>(m_InternbalSubpasses[subpass].inputAttachments.size());
			m_Subpasses[subpass].pInputAttachments = m_InternbalSubpasses[subpass].inputAttachments.data();
			m_Subpasses[subpass].colorAttachmentCount = static_cast<uint32_t>(m_InternbalSubpasses[subpass].colorAttachments.size());
			m_Subpasses[subpass].pColorAttachments = m_InternbalSubpasses[subpass].colorAttachments.data();
			m_Subpasses[subpass].pResolveAttachments = nullptr;
			m_Subpasses[subpass].pDepthStencilAttachment = (depthStencilEnable == true) ? &m_InternbalSubpasses[subpass].depthStencilAttachment : nullptr;
			m_Subpasses[subpass].preserveAttachmentCount = static_cast<uint32_t>(m_InternbalSubpasses[subpass].preserveAttachments.size());
			m_Subpasses[subpass].pPreserveAttachments = m_InternbalSubpasses[subpass].preserveAttachments.data();

			return &m_InternbalSubpasses[subpass];
		}

	private:
		collection::Vector<V3DAttachmentDesc> m_Attachments;
		collection::Vector<RenderPassDesc::Subpass> m_InternbalSubpasses;
		collection::Vector<V3DSubpassDesc> m_Subpasses;
		collection::Vector<V3DSubpassDependencyDesc> m_SubpassDependencies;
	};

	struct GraphicsPipelineDesc : public V3DGraphicsPipelineDesc
	{
	public:
		GraphicsPipelineDesc()
		{
			vertexShader = {};
			tessellationControlShader = {};
			tessellationEvaluationShader = {};
			geometryShader = {};
			fragmentShader = {};
			vertexInput = {};
			primitiveTopology = V3D_PRIMITIVE_TOPOLOGY_POINT_LIST;
			tessellation = {};
			rasterization = {};
			multisample = {};
			depthStencil = {};
			colorBlend = {};
			pRenderPass = 0;
			subpass = 0;
		}
			
		GraphicsPipelineDesc(uint32_t vertexElementCount, uint32_t vertexLayoutCount, uint32_t attachmentCount)
		{
			Allocate(vertexElementCount, vertexLayoutCount, attachmentCount);
		}

		void Allocate(uint32_t vertexElementCount, uint32_t vertexLayoutCount, uint32_t attachmentCount)
		{
			m_VertexElements.resize(vertexElementCount, V3DPipelineVertexElement{});
			this->vertexInput.elementCount = vertexElementCount;
			this->vertexInput.pElements = m_VertexElements.data();

			m_VertexLayouts.resize(vertexLayoutCount, V3DPipelineVertexLayout{});
			this->vertexInput.layoutCount = vertexLayoutCount;
			this->vertexInput.pLayouts = m_VertexLayouts.data();

			vertexShader = V3DPipelineShader{};
			tessellationControlShader = V3DPipelineShader{};
			tessellationEvaluationShader = V3DPipelineShader{};
			geometryShader = V3DPipelineShader{};
			fragmentShader = V3DPipelineShader{};

			tessellation.patchControlPoints = 0;

			rasterization.discardEnable = false;
			rasterization.depthClampEnable = false;
			rasterization.polygonMode = V3D_POLYGON_MODE_FILL;
			rasterization.cullMode = V3D_CULL_MODE_BACK;
			rasterization.depthBiasEnable = false;
			rasterization.depthBiasConstantFactor = 0.0f;
			rasterization.depthBiasClamp = 0.0f;
			rasterization.depthBiasSlopeFactor = 0.0f;

			multisample.rasterizationSamples = V3D_SAMPLE_COUNT_1;
			multisample.sampleShadingEnable = false;
			multisample.minSampleShading = 0.0f;
			multisample.alphaToCoverageEnable = false;
			multisample.alphaToOneEnable = false;

			depthStencil.depthTestEnable = true;
			depthStencil.depthWriteEnable = true;
			depthStencil.depthCompareOp = V3D_COMPARE_OP_LESS;
			depthStencil.stencilTestEnable = false;
			depthStencil.stencilFront.failOp = V3D_STENCIL_OP_KEEP;
			depthStencil.stencilFront.passOp = V3D_STENCIL_OP_KEEP;
			depthStencil.stencilFront.depthFailOp = V3D_STENCIL_OP_KEEP;
			depthStencil.stencilFront.compareOp = V3D_COMPARE_OP_ALWAYS;
			depthStencil.stencilFront.readMask = 0x000000FF;
			depthStencil.stencilFront.writeMask = 0x000000FF;
			depthStencil.stencilBack = depthStencil.stencilFront;
			depthStencil.depthBoundsTestEnable = false;
			depthStencil.minDepthBounds = 0.0f;
			depthStencil.maxDepthBounds = 1.0f;

			colorBlend.logicOpEnable = false;
			colorBlend.logicOp = V3D_LOGIC_OP_COPY;

			m_Attachments.resize(attachmentCount, V3DPipelineColorBlendAttachment{});
			this->colorBlend.attachmentCount = attachmentCount;
			this->colorBlend.pAttachments = m_Attachments.data();
		}

	private:
		collection::Vector<V3DPipelineVertexElement> m_VertexElements;
		collection::Vector<V3DPipelineVertexLayout> m_VertexLayouts;
		collection::Vector<V3DPipelineColorBlendAttachment> m_Attachments;
	};

	enum MODEL_SOURCE_FLAG : uint32_t
	{
		MODEL_SOURCE_FLIP_FACE = 0x00000001,
		MODEL_SOURCE_INVERT_TEXCOORD_U = 0x00000002,
		MODEL_SOURCE_INVERT_TEXCOORD_V = 0x00000004,
		MODEL_SOURCE_INVERT_NORMAL = 0x00000008,
		MODEL_SOURCE_TRANSFORM = 0x00000010, // ModelImportConfig::rotation and ModelImportConfig::scale
	};

	enum MODEL_SOURCE_PATH_TYPE
	{
		MODEL_SOURCE_PATH_TYPE_RELATIVE = 0,
		MODEL_SOURCE_PATH_TYPE_STRIP = 1, // file name only
	};

	struct ModelSourceConfig
	{
		uint32_t flags;
		glm::mat4 rotation;
		float scale;
		MODEL_SOURCE_PATH_TYPE pathType;
	};

	struct ModelRendererConfig
	{
		bool optimizeEnable;
		bool smoosingEnable;
		float smoosingCos;
	};

	struct Transform
	{
		glm::vec3 scale;
		glm::quat rotation;
		glm::vec3 translation;
	};

	struct Buffer
	{
		IV3DBuffer* pResource;
		ResourceAllocation resourceAllocation;
	};

	struct OpacityDrawSet
	{
		uint64_t sortKey;

		IV3DPipeline* pPipeline;
		IV3DDescriptorSet* descriptorSet[2];
		uint32_t dynamicOffsets[2];

		IV3DBuffer* pVertexBuffer;
		IV3DBuffer* pIndexBuffer;
		V3D_INDEX_TYPE indexType;

		uint32_t indexCount;
		uint32_t firstIndex;

		VE_DECLARE_ALLOCATOR
	};

	struct TransparencyDrawSet
	{
		float sortKey;

		IV3DPipeline* pPipeline;
		IV3DDescriptorSet* descriptorSet[2];
		uint32_t dynamicOffsets[2];

		IV3DBuffer* pVertexBuffer;
		IV3DBuffer* pIndexBuffer;
		V3D_INDEX_TYPE indexType;

		uint32_t indexCount;
		uint32_t firstIndex;

		VE_DECLARE_ALLOCATOR
	};

	struct ShadowDrawSet
	{
		float sortKey1;
		uint64_t sortKey2;

		IV3DPipeline* pPipeline;
		IV3DDescriptorSet* descriptorSet[2];
		uint32_t dynamicOffsets[2];

		IV3DBuffer* pVertexBuffer;
		IV3DBuffer* pIndexBuffer;
		V3D_INDEX_TYPE indexType;

		uint32_t indexCount;
		uint32_t firstIndex;

		VE_DECLARE_ALLOCATOR
	};

	struct SelectDrawSet
	{
		IV3DPipeline* pPipeline;
		IV3DDescriptorSet* pDescriptorSet;
		uint32_t dynamicOffset;

		IV3DBuffer* pVertexBuffer;
		IV3DBuffer* pIndexBuffer;
		V3D_INDEX_TYPE indexType;

		uint32_t indexCount;
		uint32_t firstIndex;
	};

	// ----------------------------------------------------------------------------------------------------
	// 関数
	// ----------------------------------------------------------------------------------------------------

#ifdef _DEBUG
	uint32_t U64ToU32(uint64_t value);
#endif //_DEBUG

	uint32_t ToMagicNumber(char a, char b, char c, char d);

	const char* ToString_ResourceType(V3D_RESOURCE_TYPE type);
	void ToString_MemoryProperty(V3DFlags flags, StringA& string);

	void ToMultibyteString(const wchar_t* pSrc, StringA& dst);
	void ToWideString(const char* pSrc, StringW& dst);

	void ParseStringW(const StringW& source, const wchar_t* pDelimiters, collection::Vector<StringW>& items);

	void RemoveFileExtensionW(const wchar_t* pSrc, StringW& dst);

	bool IsRelativePath(const wchar_t* pFilePath);
	void GetDirectoryPath(const wchar_t* pFilePath, StringW& directoryPath);

	bool RemoveFileSpecW(const wchar_t* pFilePath, StringW& dirPath);
	bool ToRelativePathW(const wchar_t* pPathFrom, uint32_t attrFrom, const wchar_t* pPathTo, uint32_t attrTo, wchar_t* pRelativePath, size_t relativePathSize);

	bool FileRead(HANDLE fileHandle, uint64_t size, void* pData);
	bool FileWrite(HANDLE fileHandle, uint64_t size, void* pData);

	V3DPipelineColorBlendAttachment InitializeColorBlendAttachment(BLEND_MODE mode, V3DFlags writeMask = V3D_COLOR_COMPONENT_ALL);
}

#ifdef _DEBUG
#define VE_U64_TO_U32(value) U64ToU32(value)
#else //_DEBUG
#define VE_U64_TO_U32(value) static_cast<uint32_t>(value)
#endif //_DEBUG