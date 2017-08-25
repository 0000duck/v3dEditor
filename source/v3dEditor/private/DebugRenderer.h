#pragma once

#include "AABB.h"
#include "Sphere.h"
#include "BufferedContainer.h"
#include "GraphicsFactory.h"

namespace ve {

	class DynamicBuffer;

	class DebugRenderer final
	{
	public:
		static DebugRenderer* Create(DeviceContextPtr deviceContext);
		void Destroy();

		const glm::vec4& GetColor(DEBUG_COLOR_TYPE type) const;
		void SetColor(DEBUG_COLOR_TYPE type, const glm::vec4& color);

		void AddLine(DEBUG_COLOR_TYPE colorType, const glm::vec3& p0, const glm::vec3& p1);
		void AddAABB(DEBUG_COLOR_TYPE colorType, const AABB& aabb);
		void AddSpehre(DEBUG_COLOR_TYPE colorType, const Sphere& sphere);

		void Update();
		void Draw(IV3DCommandBuffer* pCommandBuffer, uint32_t frameIndex);

	private:
		static auto constexpr VerticesResizeStep = 256;

		static constexpr int32_t SphereStepCount = 10;
		static constexpr float SphereStep = VE_PI * 360.0f / static_cast<float>(SphereStepCount) / 180.0f;
		static constexpr int32_t SphereStepCounH = SphereStepCount >> 1;
		static constexpr float SphereStepH = VE_PI * 180.0f / static_cast<float>(SphereStepCounH) / 180.0f;
		static constexpr int32_t SpherePointCount = (SphereStepCount + 1) * 6;

		DeviceContextPtr m_DeviceContext;
		collection::Array1<glm::vec4, DEBUG_COLOR_TYPE_COUNT> m_Colors;
		collection::BufferedContainer<DebugVertex> m_Vertices;
		DynamicBuffer* m_pVertexBuffer;
		uint32_t m_VertexCount;

		glm::vec3 m_SpherePoints[DebugRenderer::SpherePointCount];

		DebugRenderer();
		~DebugRenderer();

		bool Initialize(DeviceContextPtr deviceContext);

		VE_DECLARE_ALLOCATOR
	};

}