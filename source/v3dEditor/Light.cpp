#include "Light.h"
#include "DebugRenderer.h"

namespace ve {

	LightPtr Light::Create()
	{
		return std::move(std::make_shared<Light>());
	}

	Light::Light() :
		m_Color(1.0f, 1.0f, 1.0f, 1.0f),
		m_LocalDirection(0.0f, 1.0f, 0.0f),
		m_WorldDirection(m_LocalDirection)
	{
	}

	Light::~Light()
	{
	}

	const glm::vec4& Light::GetColor() const
	{
		return m_Color;
	}

	void Light::SetColor(const glm::vec4& color)
	{
		m_Color = color;
	}

	const glm::vec3& Light::GetDirection() const
	{
		return m_WorldDirection;
	}

	/************************************/
	/* public override - NodeAttribute */
	/************************************/

	NodeAttribute::TYPE Light::GetType() const
	{
		return NodeAttribute::TYPE_LIGHT;
	}

	void Light::Update(const glm::mat4& worldMatrix)
	{
		m_WorldDirection = glm::normalize(glm::mat3(worldMatrix) * m_LocalDirection);

		m_DebugSphere.center = worldMatrix[3];
		m_DebugSphere.radius = glm::length(glm::mat3(worldMatrix) * glm::vec3(1.0f, 0.0f, 0.0f));
		m_DebugLineLength = 1000.0f * m_DebugSphere.radius;
	}

	/***************************************/
	/* protected override - NodeAttribute */
	/***************************************/

	bool Light::IsSelectSupported() const
	{
		return false;
	}

	void Light::SetSelectKey(uint32_t key)
	{
	}

	void Light::DrawSelect(uint32_t frameIndex, SelectDrawSet& drawSet)
	{
	}

	void Light::DebugDraw(uint32_t flags, DebugRenderer* pDebugRenderer)
	{
		if (flags & DEBUG_DRAW_LIGHT_SHAPE)
		{
			pDebugRenderer->AddSpehre(DEBUG_COLOR_TYPE_LIGHT_SHAPE, m_DebugSphere);
			pDebugRenderer->AddLine(DEBUG_COLOR_TYPE_LIGHT_SHAPE, m_DebugSphere.center, m_DebugSphere.center + m_WorldDirection * m_DebugLineLength);
		}
	}

}