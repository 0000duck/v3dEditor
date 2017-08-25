#pragma once

#include "NodeAttribute.h"
#include "Node.h"

namespace ve {

	class Camera final : public NodeAttribute
	{
	public:
		Camera();
		~Camera();

		const glm::mat4& GetViewProjectionMatrix() const;
		const glm::mat4& GetInverseViewMatrix() const;
		const glm::mat4& GetInverseProjectionMatrix() const;
		const glm::mat4& GetInverseViewProjectionMatrix() const;

		const glm::quat& GetRotation() const;
		void SetRotation(const glm::quat& rotation);

		const glm::vec3& GetAt() const;
		void SetAt(const glm::vec3& at);

		float GetDistance() const;
		void SetDistance(float distance);

		const glm::vec3& GetEye() const;
		const glm::mat4& GetViewMatrix() const;

		void SetView(const glm::quat& rotation, const glm::vec3& at, float distance);

		const glm::mat4& GetProjectionMatrix() const;
		float GetFovY() const;
		void SetFovY(float fovY);
		float GetNearClip() const;
		void SetNearClip(float nearClip);
		float GetFarClip() const;
		void SetFarClip(float farClip);

		/******************/
		/* NodeAttribute */
		/******************/

		NodeAttribute::TYPE GetType() const override;
		void Update(const glm::mat4& worldMatrix) override;

		VE_DECLARE_ALLOCATOR

	private:
		WeakPtr<Node> m_OwnerNode;

		glm::quat m_Rotation;
		glm::vec3 m_At;
		float m_Distance;
		glm::vec3 m_Eye;
		glm::mat4 m_LocalViewMatrix;
		glm::mat4 m_WorldViewMatrix;

		float m_Aspect;
		float m_FovY;
		float m_NearClip;
		float m_FarClip;

		glm::mat4 m_ProjectionMatrix;
		glm::mat4 m_ViewProjectionMatrix;
		glm::mat4 m_InverseViewMatrix;
		glm::mat4 m_InverseProjectionMatrix;
		glm::mat4 m_InverseViewProjectionMatrix;

		static CameraPtr Create();
		void SetAspect(uint32_t width, uint32_t height);

		friend class Scene;
	};

}
