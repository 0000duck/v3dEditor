#include "Camera.h"

namespace ve {

	/*******************/
	/* public - Camera */
	/*******************/

	Camera::Camera() :
		m_Rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)),
		m_At(glm::vec3(0.0f)),
		m_Distance(0.0f),
		m_Eye(glm::vec3(0.0f)),
		m_LocalViewMatrix(glm::mat4(1.0f)),
		m_WorldViewMatrix(glm::mat4(1.0f)),
		m_Aspect(0.0f),
		m_FovY(glm::radians(60.0f)),
		m_NearClip(0.01f),
		m_FarClip(1000.0f),
		m_ProjectionMatrix(glm::mat4(1.0f)),
		m_ViewProjectionMatrix(glm::mat4(1.0f)),
		m_InverseViewMatrix(glm::mat4(1.0f)),
		m_InverseProjectionMatrix(glm::mat4(1.0f)),
		m_InverseViewProjectionMatrix(glm::mat4(1.0f))
	{
	}

	Camera::~Camera()
	{
	}

	const glm::mat4& Camera::GetViewProjectionMatrix() const
	{
		return m_ViewProjectionMatrix;
	}

	const glm::mat4& Camera::GetInverseViewMatrix() const
	{
		return m_InverseViewMatrix;
	}

	const glm::mat4& Camera::GetInverseProjectionMatrix() const
	{
		return m_InverseProjectionMatrix;
	}

	const glm::mat4& Camera::GetInverseViewProjectionMatrix() const
	{
		return m_InverseViewProjectionMatrix;
	}

	const glm::quat& Camera::GetRotation() const
	{
		return m_Rotation;
	}

	void Camera::SetRotation(const glm::quat& rotation)
	{
		m_Rotation = rotation;
	}

	const glm::vec3& Camera::GetAt() const
	{
		return m_At;
	}

	void Camera::SetAt(const glm::vec3& at)
	{
		m_At = at;
	}

	float Camera::GetDistance() const
	{
		return m_Distance;
	}

	void Camera::SetDistance(float distance)
	{
		m_Distance = distance;
	}

	const glm::vec3& Camera::GetEye() const
	{
		return m_Eye;
	}

	const glm::mat4& Camera::GetViewMatrix() const
	{
		return m_WorldViewMatrix;
	}

	void Camera::SetView(const glm::quat& rotation, const glm::vec3& at, float distance)
	{
		m_Rotation = rotation;
		m_At = at;
		m_Distance = distance;
	}

	const glm::mat4& Camera::GetProjectionMatrix() const
	{
		return m_ProjectionMatrix;
	}

	float Camera::GetFovY() const
	{
		return m_FovY;
	}

	void Camera::SetFovY(float fovY)
	{
		m_FovY = fovY;
	}

	float Camera::GetNearClip() const
	{
		return m_NearClip;
	}

	void Camera::SetNearClip(float nearClip)
	{
		m_NearClip = nearClip;
	}

	float Camera::GetFarClip() const
	{
		return m_FarClip;
	}

	void Camera::SetFarClip(float farClip)
	{
		m_FarClip = farClip;
	}

	/************************************/
	/* public override - NodeAttribute */
	/************************************/

	NodeAttribute::TYPE Camera::GetType() const
	{
		return NodeAttribute::TYPE_CAMERA;
	}

	void Camera::Update(const glm::mat4& worldMatrix)
	{
		// ビュー行列
		glm::mat4 rotationMatrix = glm::toMat4(m_Rotation);

		glm::vec3 localEye = rotationMatrix * glm::vec4(0.0f, 0.0f, m_Distance, 1.0f);

		glm::vec3 up = glm::vec3(rotationMatrix[1].x, rotationMatrix[1].y, rotationMatrix[1].z);
		up = glm::normalize(up);

		glm::vec3 n = glm::normalize(localEye);
		glm::vec3 u = glm::cross(up, n);
		glm::vec3 v = glm::cross(n, u);

		m_Eye = m_At + localEye;

		m_LocalViewMatrix[0] = glm::vec4(u.x, v.x, n.x, 0.0f);
		m_LocalViewMatrix[1] = glm::vec4(u.y, v.y, n.y, 0.0f);
		m_LocalViewMatrix[2] = glm::vec4(u.z, v.z, n.z, 0.0f);
		m_LocalViewMatrix[3] = glm::vec4(-glm::dot(m_Eye, u), -glm::dot(m_Eye, v), -glm::dot(m_Eye, n), 1.0f);

		// プロジェクションの行列

		// |  w|0.0| 0.0|0.0|
		// |0.0|  h| 0.0|0.0|
		// |0.0|0.0|  z1| z2|
		// |0.0|0.0|-1.0|0.0|

		float t = ::tanf(m_FovY * 0.5f);
		float w = VE_FLOAT_RECIPROCAL(m_Aspect * t);
		float h = VE_FLOAT_RECIPROCAL(t);
		float z1 = -VE_FLOAT_DIV((m_FarClip + m_NearClip), (m_FarClip - m_NearClip));
		float z2 = -VE_FLOAT_DIV((2.0f * m_FarClip * m_NearClip), (m_FarClip - m_NearClip));

		m_ProjectionMatrix[0] = glm::vec4(w, 0.0f, 0.0f, 0.0f);
		m_ProjectionMatrix[1] = glm::vec4(0.0f, h, 0.0f, 0.0f);
		m_ProjectionMatrix[2] = glm::vec4(0.0f, 0.0f, z1, -1.0f);
		m_ProjectionMatrix[3] = glm::vec4(0.0f, 0.0f, z2, 0.0f);

		m_WorldViewMatrix = m_LocalViewMatrix * worldMatrix;

		// ビュー * プロジェクションの行列
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_WorldViewMatrix;

		// ビュー の逆行列
		m_InverseViewMatrix = glm::inverse(m_WorldViewMatrix);

		// プロジェクションの逆行列
		m_InverseProjectionMatrix = glm::inverse(m_ProjectionMatrix);

		// ビュー * プロジェクションの逆行列
		m_InverseViewProjectionMatrix = glm::inverse(m_ViewProjectionMatrix);
	}

	/********************/
	/* private - Camera */
	/********************/

	CameraPtr Camera::Create()
	{
		return std::make_shared<Camera>();
	}

	void Camera::SetAspect(uint32_t width, uint32_t height)
	{
		m_Aspect = VE_FLOAT_DIV(static_cast<float>(width), static_cast<float>(height));
	}

}
