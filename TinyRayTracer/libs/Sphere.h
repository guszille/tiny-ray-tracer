#pragma once

#include "Geometry.h"

struct Material
{
	float m_RefractiveIndex;
	Vec4f m_Albedo;
	Vec3f m_DiffuseColor;
	float m_SpecularExponent;

	Material()
		: m_RefractiveIndex(), m_Albedo(1.0f, 0.0f, 0.0f, 0.0f), m_DiffuseColor(), m_SpecularExponent() {}

	Material(const float& refractiveIndex, const Vec4f& albedo, const Vec3f& diffuseColor, const float& specularExponent)
		: m_RefractiveIndex(refractiveIndex), m_Albedo(albedo), m_DiffuseColor(diffuseColor), m_SpecularExponent(specularExponent) {}
};

struct Sphere
{
	Vec3f m_Center;
	float m_Radius;
	Material m_Material;
	
	Sphere(const Vec3f& center, const float& radius, const Material& material)
		: m_Center(center), m_Radius(radius), m_Material(material) {}

	bool RayIntersect(const Vec3f& origin, const Vec3f& direction, float& t) const
	{
		Vec3f xa = origin - m_Center;
		float b = xa * direction;
		float delta = (b * b) - (xa * xa) + (m_Radius * m_Radius);

		if (delta < 0) return false;

		float s1 = - b - sqrtf(delta);
		float s2 = - b + sqrtf(delta);

		if (s1 > 0) { t = s1; return true; }
		else if (s2 > 0) { t = s2; return true; }

		return false;
	}
};