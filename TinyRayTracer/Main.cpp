// TinyRayTracer developed by Gustavo Zille.

#define _USE_MATH_DEFINES

#include <cmath>
#include <limits>
#include <vector>
#include <fstream>

#include "libs/Geometry.h"
#include "libs/Sphere.h"
#include "libs/Light.h"

struct Hit
{
    Vec3f point;
    Vec3f normal;
    Material material;

    Hit() : point(), normal(), material() {}
};

Vec3f Reflect(const Vec3f& direction, const Vec3f& normal)
{
    return direction - (normal * 2.0f) * (direction * normal);
}

Vec3f Refract(const Vec3f& direction, const Vec3f& normal, const float& refractiveIndex) // Snell's law.
{
    Vec3f n = normal;
    float r = 1.0f / refractiveIndex;
    float c = - (n * direction);

    if (c < 0)
    {
        r = refractiveIndex;
        c = -c;
        n = -n;
    }

    // Here we can have a problem. If "k" is higher than 1,
    // the result inside the square root will be negative,
    // and we will have to deal with imaginary values.
    //
    float k = (r * r) * (1 - (c * c));
    float s = sqrtf(1.0f - k);

    return (direction * r) + (n * ((r * c) - s));
}

bool SceneIntersect(const Vec3f& origin, const Vec3f& direction, const std::vector<Sphere>& spheres, Hit& hitInfo)
{
    float spheresDistance = std::numeric_limits<float>::max();
    float checkerboardDistance = std::numeric_limits<float>::max();
    
    for (size_t i = 0; i < spheres.size(); i++)
    {
        float t;

        if (spheres[i].RayIntersect(origin, direction, t) && t < spheresDistance)
        {
            spheresDistance = t;

            hitInfo.point = origin + direction * t;
            hitInfo.normal = (hitInfo.point - spheres[i].m_Center).normalize();
            hitInfo.material = spheres[i].m_Material;
        }
    }

    
    if (fabs(direction.y) > 1e-3) // Drawning a plane (board).
    {
        float d = - (origin.y + 4.0f) / direction.y; // The checkerboard plane has equation "y = -4".
        Vec3f p = origin + direction * d;

        if (d > 0 && fabs(p.x) < 10 && p.z < -10 && p.z > -30 && d < spheresDistance)
        {
            Vec3f c = (int(0.5f * p.x + 1000) + int(0.5f * p.z)) & 1 ? Vec3f(1.0f, 1.0f, 1.0f) : Vec3f(1.0f, 0.7f, 0.3f);

            checkerboardDistance = d;

            hitInfo.point = p;
            hitInfo.normal = Vec3f(0, 1, 0);

            hitInfo.material.m_DiffuseColor = c * 0.3f;
        }
    }

    return std::min(spheresDistance, checkerboardDistance) < 1000; // Why "1000" here?
}

Vec3f CastRay(const Vec3f& origin, const Vec3f& direction,
              const std::vector<Sphere>& spheres, const std::vector<Light>& lights,
              size_t depth = 0)
{
    Hit hitInfo = Hit();
    float diffuseLightIntensity = 0.0f, specularLightIntensity = 0.0f;

    if (depth < 5 && SceneIntersect(origin, direction, spheres, hitInfo))
    {
        Vec3f reflectDirection = Reflect(direction, hitInfo.normal).normalize();
        Vec3f reflectOrigin = reflectDirection * hitInfo.normal < 0 ? hitInfo.point - hitInfo.normal * 1e-3 : hitInfo.point + hitInfo.normal * 1e-3; // Peventing intersection with the hitted point.
        Vec3f reflectColor = CastRay(reflectOrigin, reflectDirection, spheres, lights, depth + 1);

        Vec3f refractDirection = Refract(direction, hitInfo.normal, hitInfo.material.m_RefractiveIndex).normalize();
        Vec3f refractOrigin = refractDirection * hitInfo.normal < 0 ? hitInfo.point - hitInfo.normal * 1e-3 : hitInfo.point + hitInfo.normal * 1e-3; // Peventing intersection with the hitted point.
        Vec3f refractColor = CastRay(refractOrigin, refractDirection, spheres, lights, depth + 1);

        for (size_t i = 0; i < lights.size(); i++)
        {
            Hit shaddowInfo = Hit();

            Vec3f lightDirection = (lights[i].m_Position - hitInfo.point).normalize();
            float lightDistance = (lights[i].m_Position - hitInfo.point).norm();
            Vec3f shadowOrigin = lightDirection * hitInfo.normal < 0 ? hitInfo.point - hitInfo.normal * 1e-3 : hitInfo.point + hitInfo.normal * 1e-3; // Peventing intersection with the hitted point.

            if (SceneIntersect(shadowOrigin, lightDirection, spheres, shaddowInfo) && (shaddowInfo.point - shadowOrigin).norm() < lightDistance)
                continue;

            Vec3f reflectedLight = Reflect(lightDirection, hitInfo.normal);

            // We can use a simplified formula, like:
            //
            // DF = Light Direction * Normal
            //
            float diffuseFactor = (lightDirection * hitInfo.normal) / (lightDirection.norm() * hitInfo.normal.norm());

            diffuseLightIntensity += lights[i].m_Intensity * std::max(0.0f, diffuseFactor);
            specularLightIntensity += lights[i].m_Intensity * powf(std::max(0.0f, reflectedLight * direction), hitInfo.material.m_SpecularExponent);
        }

        Vec3f diffuseComp = hitInfo.material.m_DiffuseColor * hitInfo.material.m_Albedo[0] * diffuseLightIntensity;
        Vec3f specularComp = Vec3f(1.0f, 1.0f, 1.0f) * hitInfo.material.m_Albedo[1] * specularLightIntensity;

        Vec3f reflectComp = reflectColor * hitInfo.material.m_Albedo[2];
        Vec3f refractComp = refractColor * hitInfo.material.m_Albedo[3];

        return diffuseComp + specularComp + reflectComp + refractComp;
    }
    
    return Vec3f(0.2, 0.5, 0.8); // Background color.
}

void Render(const std::vector<Sphere>& spheres, const std::vector<Light>& lights)
{
    const int fov    = M_PI / 2.0;
    const int width  = 1024;
    const int height = 768;

    std::vector<Vec3f> framebuffer(width * height);

    #pragma omp parallel for
    for (size_t j = 0; j < height; j++) {
        for (size_t i = 0; i < width; i++) {
            float x =  (2 * (i + 0.5) / (float)width  - 1) * tan(fov / 2.0) * width / (float)height;
            float y = -(2 * (j + 0.5) / (float)height - 1) * tan(fov / 2.0);

            Vec3f viewDirection = Vec3f(x, y, -1).normalize();

            framebuffer[i + j * width] = CastRay(Vec3f(0, 0, 0), viewDirection, spheres, lights);
        }
    }

    std::ofstream ofs;
    ofs.open("outputs/image.ppm", std::ofstream::out | std::ofstream::binary);

    ofs << "P6\n" << width << " " << height << "\n255\n";

    for (size_t i = 0; i < width * height; i++) {
        // There is no need of the code below.
        // It would only be in case of color overflow.
        //
        // Vec3f &color = framebuffer[i];
        // float max = std::max(color[0], std::max(color[1], color[2]));
        //
        // if (max > 1) color = color * (1.0f / max);

        for (size_t j = 0; j < 3; j++) {
            ofs << (char)(255 * std::max(0.0f, std::min(1.0f, framebuffer[i][j])));
        }
    }

    ofs.close();
}

void main()
{
    Material     ivory(1.0, Vec4f(0.6,  0.3, 0.1, 0.0), Vec3f(0.4, 0.4, 0.3),   50.0);
    Material     glass(1.5, Vec4f(0.0,  0.5, 0.1, 0.8), Vec3f(0.6, 0.7, 0.8),  125.0);
    Material redRubber(1.0, Vec4f(0.9,  0.1, 0.0, 0.0), Vec3f(0.3, 0.1, 0.1),   10.0);
    Material    mirror(1.0, Vec4f(0.0, 10.0, 0.8, 0.0), Vec3f(1.0, 1.0, 1.0), 1425.0);

    std::vector<Sphere> spheres;
    spheres.push_back(Sphere(Vec3f(-3.0,  0.0, -16.0), 2,     ivory));
    spheres.push_back(Sphere(Vec3f(-1.0, -1.5, -12.0), 2,     glass));
    spheres.push_back(Sphere(Vec3f( 1.5, -0.5, -18.0), 3, redRubber));
    spheres.push_back(Sphere(Vec3f( 7.0,  5.0, -18.0), 4,    mirror));

    std::vector<Light> lights;
    lights.push_back(Light(Vec3f(-20.0, 20.0,  20.0), 1.5));
    lights.push_back(Light(Vec3f( 30.0, 50.0, -25.0), 1.8));
    lights.push_back(Light(Vec3f( 30.0, 20.0,  30.0), 1.7));

    Render(spheres, lights);
}