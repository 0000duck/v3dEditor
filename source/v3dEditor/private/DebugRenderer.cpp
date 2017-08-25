#include "DebugRenderer.h"
#include "DynamicBuffer.h"

namespace ve {

	DebugRenderer* DebugRenderer::Create(DeviceContextPtr deviceContext)
	{
		DebugRenderer* pDebugRenderer = VE_NEW_T(DebugRenderer);
		if (pDebugRenderer == nullptr)
		{
			return nullptr;
		}

		if (pDebugRenderer->Initialize(deviceContext) == false)
		{
			VE_DELETE_T(pDebugRenderer, DebugRenderer);
			return nullptr;
		}

		return pDebugRenderer;
	}

	void DebugRenderer::Destroy()
	{
		VE_DELETE_THIS_T(this, DebugRenderer);
	}

	const glm::vec4& DebugRenderer::GetColor(DEBUG_COLOR_TYPE type) const
	{
		return m_Colors[type];
	}

	void DebugRenderer::SetColor(DEBUG_COLOR_TYPE type, const glm::vec4& color)
	{
		m_Colors[type] = color;
	}

	void DebugRenderer::AddLine(DEBUG_COLOR_TYPE colorType, const glm::vec3& p0, const glm::vec3& p1)
	{
		const glm::vec4& color = m_Colors[colorType];

		DebugVertex* pVertex = m_Vertices.Add(2);
		pVertex[0].pos = glm::vec4(p0, 1.0f);
		pVertex[0].color = color;
		pVertex[1].pos = glm::vec4(p1, 1.0f);
		pVertex[1].color = color;
	}

	void DebugRenderer::AddAABB(DEBUG_COLOR_TYPE colorType, const AABB& aabb)
	{
		static constexpr uint32_t refTable[24] =
		{
			0, 1,
			1, 2,
			2, 3,
			3, 0,

			4, 5,
			5, 6,
			6, 7,
			7, 4,

			0, 4,
			1, 5,
			2, 6,
			3, 7,
		};

		static auto constexpr refCount = _countof(refTable);

		const glm::vec4& color = m_Colors[colorType];
		const glm::vec3* points = aabb.points;

		DebugVertex* pVertex = m_Vertices.Add(24);

		for (auto i = 0; i < refCount; i += 2)
		{
			pVertex[0].pos = glm::vec4(points[refTable[i + 0]], 1.0f);
			pVertex[0].color = color;

			pVertex[1].pos = glm::vec4(points[refTable[i + 1]], 1.0f);
			pVertex[1].color = color;

			pVertex += 2;
		}
	}

	void DebugRenderer::AddSpehre(DEBUG_COLOR_TYPE colorType, const Sphere& sphere)
	{
		const glm::vec4& color = m_Colors[colorType];

		const glm::vec3* pPoint = &(m_SpherePoints[0]);

		DebugVertex* pVertex = m_Vertices.Add(DebugRenderer::SpherePointCount);
		DebugVertex* pVertexEnd = pVertex + DebugRenderer::SpherePointCount;

		glm::vec4 center = glm::vec4(sphere.center, 0.0f);

		while (pVertex != pVertexEnd)
		{
			pVertex->pos = center + glm::vec4(*pPoint * sphere.radius, 1.0f);
			pVertex->color = color;
			pVertex++;
			pPoint++;
		}
	}

	void DebugRenderer::Update()
	{
		m_VertexCount = static_cast<uint32_t>(m_Vertices.GetCount());

		// ----------------------------------------------------------------------------------------------------
		// バーテックスバッファーのサイズ調整
		// ----------------------------------------------------------------------------------------------------

		size_t vertexMemorySize = sizeof(DebugVertex) * m_VertexCount;
		if (vertexMemorySize == 0)
		{
			return;
		}

		if (m_pVertexBuffer->GetNativeRangeSize() < vertexMemorySize)
		{
			vertexMemorySize = (vertexMemorySize + DebugRenderer::VerticesResizeStep - 1) / DebugRenderer::VerticesResizeStep * DebugRenderer::VerticesResizeStep;

			DynamicBuffer* pNextVertexBuffer = DynamicBuffer::Create(m_DeviceContext, V3D_BUFFER_USAGE_VERTEX, vertexMemorySize, V3D_PIPELINE_STAGE_VERTEX_INPUT, V3D_ACCESS_VERTEX_READ, VE_INTERFACE_DEBUG_NAME(L"VE_DebugRenderer"));
			if (pNextVertexBuffer != nullptr)
			{
				m_pVertexBuffer->Destroy();
				m_pVertexBuffer = pNextVertexBuffer;
			}
			else
			{
				// メモリ不足の場合はなかったことにする
				m_VertexCount = 0;
			}
		}

		if (m_VertexCount == 0)
		{
			return;
		}

		// ----------------------------------------------------------------------------------------------------
		// 頂点を書き込む
		// ----------------------------------------------------------------------------------------------------

		void* pMemory = m_pVertexBuffer->Map();
		VE_ASSERT(pMemory != nullptr);
		memcpy_s(pMemory, m_pVertexBuffer->GetNativeRangeSize(), m_Vertices.GetData(), vertexMemorySize);
		m_pVertexBuffer->Unmap();

		// ----------------------------------------------------------------------------------------------------
		// クリア
		// ----------------------------------------------------------------------------------------------------

		m_Vertices.Clear();
	}

	void DebugRenderer::Draw(IV3DCommandBuffer* pCommandBuffer, uint32_t frameIndex)
	{
		if (m_VertexCount > 0)
		{
			pCommandBuffer->BindVertexBuffer(0, m_pVertexBuffer->GetNativeBufferPtr(), m_pVertexBuffer->GetNativeRangeSize() * frameIndex);
			pCommandBuffer->Draw(m_VertexCount, 1, 0, 0);
			m_VertexCount = 0;
		}
	}

	DebugRenderer::DebugRenderer() :
		m_Vertices(DebugRenderer::VerticesResizeStep, DebugRenderer::VerticesResizeStep),
		m_pVertexBuffer(nullptr),
		m_VertexCount(0)
	{
		m_Colors[DEBUG_COLOR_TYPE_MESH_BOUNDS] = glm::vec4(0.0f, 0.702f, 0.475f, 1.0f);
		m_Colors[DEBUG_COLOR_TYPE_LIGHT_SHAPE] = glm::vec4(0.784f, 0.784f, 0.502f, 1.0f);
	}

	DebugRenderer::~DebugRenderer()
	{
		if (m_pVertexBuffer != nullptr)
		{
			m_pVertexBuffer->Destroy();
		}
	}

	bool DebugRenderer::Initialize(DeviceContextPtr deviceContext)
	{
		m_DeviceContext = deviceContext;

		m_pVertexBuffer = DynamicBuffer::Create(m_DeviceContext, V3D_BUFFER_USAGE_VERTEX, sizeof(DebugVertex) * DebugRenderer::VerticesResizeStep, V3D_PIPELINE_STAGE_VERTEX_INPUT, V3D_ACCESS_VERTEX_READ, VE_INTERFACE_DEBUG_NAME(L"VE_DebugRenderer"));
		if (m_pVertexBuffer == nullptr)
		{
			return false;
		}

		// ----------------------------------------------------------------------------------------------------
		// 球
		// ----------------------------------------------------------------------------------------------------

		{
			glm::vec4 ppx(0.0f, 0.0f, 1.0f, 1.0f);
			glm::vec4 ppy(0.0f, 0.0f, 1.0f, 1.0f);
			glm::vec4 ppz(1.0f, 0.0f, 0.0f, 1.0f);

			for (int i = 0; i < (DebugRenderer::SphereStepCount + 1); i++)
			{
				float angle = DebugRenderer::SphereStep * static_cast<float>(i);

				glm::vec4 px(0.0f, 0.0f, 1.0f, 1.0f);
				glm::vec4 py(0.0f, 0.0f, 1.0f, 1.0f);
				glm::vec4 pz(1.0f, 0.0f, 0.0f, 1.0f);

				glm::mat4 mx = glm::toMat4(glm::angleAxis(angle, glm::vec3(1.0f, 0.0f, 0.0f)));
				glm::mat4 my = glm::toMat4(glm::angleAxis(angle, glm::vec3(0.0f, 1.0f, 0.0f)));
				glm::mat4 mz = glm::toMat4(glm::angleAxis(angle, glm::vec3(0.0f, 0.0f, 1.0f)));

				px = mx * px;
				py = my * py;
				pz = mz * pz;

				m_SpherePoints[i * 6 + 0] = ppx;
				m_SpherePoints[i * 6 + 1] = px;

				m_SpherePoints[i * 6 + 2] = ppy;
				m_SpherePoints[i * 6 + 3] = py;

				m_SpherePoints[i * 6 + 4] = ppz;
				m_SpherePoints[i * 6 + 5] = pz;

				ppx = px;
				ppy = py;
				ppz = pz;
			}
		}

		// ----------------------------------------------------------------------------------------------------

		return true;
	}

}