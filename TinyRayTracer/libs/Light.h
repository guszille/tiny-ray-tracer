#pragma once

#include "Geometry.h"

struct Light
{
	Vec3f m_Position;
	float m_Intensity;

	Light(const Vec3f& position, const float intensity)
		: m_Position(position), m_Intensity(intensity) {}
};