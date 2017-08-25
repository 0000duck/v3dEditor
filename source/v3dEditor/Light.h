#pragma once

#include "NodeAttribute.h"
#include "Node.h"
#include "Sphere.h"

namespace ve {

	class Light final : public NodeAttribute
	{
	public:
		static LightPtr Create();

		Light();
		~Light();

		const glm::vec4& GetColor() const;
		void SetColor(const glm::vec4& color);

		const glm::vec3& GetDirection() const;

		/******************/
		/* NodeAttribute */
		/******************/

		NodeAttribute::TYPE GetType() const override;
		void Update(const glm::mat4& worldMatrix) override;

		VE_DECLARE_ALLOCATOR

	protected:
		/******************/
		/* NodeAttribute */
		/******************/

		bool IsSelectSupported() const override;
		void SetSelectKey(uint32_t key) override;
		void DrawSelect(uint32_t frameIndex, SelectDrawSet& drawSet) override;

		bool Draw(
			const Frustum& frustum,
			uint32_t frameIndex,
			collection::DynamicContainer<OpacityDrawSet>& opacityDrawSets,
			collection::DynamicContainer<TransparencyDrawSet>& transparencyDrawSets) override
		{
			return true;
		}

		void DebugDraw(uint32_t flags, DebugRenderer* pDebugRenderer) override;

	private:
		glm::vec4 m_Color;
		glm::vec3 m_LocalDirection;
		glm::vec3 m_WorldDirection;

		Sphere m_DebugSphere;
		float m_DebugLineLength;
	};

}