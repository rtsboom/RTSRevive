#pragma once
#include "Common/MathUtils.h"

class Camera
{
public:
	void MoveCamera(float dx, float dz);
	void RotateCamera(float dyaw, float dpitch);
	void ZoomCamera(float delta);

	rrv::Vec3 GetFocusPosition() const noexcept { return m_focus; }
	rrv::Vec3 GetEyePosition() const noexcept;
private:
	rrv::Vec3 m_focus = {};
	float m_distance = 300.f;
	float m_yaw = 0.f;
	float m_pitch = 45.f;
};

