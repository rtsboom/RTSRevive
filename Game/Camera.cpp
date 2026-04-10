#include "pch.h"
#include "Camera.h"
#include <algorithm>

void Camera::MoveCamera(float dx, float dz)
{
	// dx, dz are in local space

	// local forward direction is	(0, 0, 1),
	// local right direction is		(1, 0, 0)

	// world forward direction is	(sin(yaw), 0,  cos(yaw))
	// world right direction is		(cos(yaw), 0, -sin(yaw))

	float worldDx = sinf(m_yaw) * dz + cosf(m_yaw) * dx; // forward.x * dz + right.x * dx
	float worldDz = cosf(m_yaw) * dz - sinf(m_yaw) * dx; // forward.z * dz + right.z * dx

	m_focus.x += worldDx;
	m_focus.z += worldDz;
}

void Camera::RotateCamera(float dyaw, float dpitch)
{
	m_yaw += dyaw;
	m_pitch += dpitch;
	m_pitch = std::clamp(m_pitch, 0.f, rrv::PI / 2 - 0.1f);
}

void Camera::ZoomCamera(float delta)
{
	m_distance -= delta;
	m_distance = std::clamp(m_distance, 0.1f, 1000.0f);
}

rrv::Vec3 Camera::GetEyePosition() const noexcept
{
	rrv::Vec3 eyePos = {};
	eyePos.x = m_focus.x - m_distance * cosf(m_pitch) * sinf(m_yaw);
	eyePos.y = m_focus.y + m_distance * sinf(m_pitch);
	eyePos.z = m_focus.z - m_distance * cosf(m_pitch) * cosf(m_yaw);

	return eyePos;
}
