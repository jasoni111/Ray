#include "ray.h"
#include "material.h"
#include "scene.h"

#include <cmath>
#include <vector>

const Material &
isect::getMaterial() const
{
    return material ? *material : obj->getMaterial();
}

namespace helperFun
{
	double getRand(double r)
	{
		thread_local unsigned int s = 0x9e3779b9u;
		s = s * 1664525u + 1013904223u;
		return (s / 4294967296.0) * 2.0 * r - r;
	}

	std::vector<vec3f> sampleRay(vec3f rayVec, double r, int num_sample)
	{
		std::vector<vec3f> toReturn;
		toReturn.reserve(num_sample);

		vec3f up(0.0, 1.0, 0.0);
		if (std::abs(rayVec.dot(up)) > 0.9)
			up = vec3f(1.0, 0.0, 0.0);

		vec3f u = rayVec.cross(up);
		if (u.length_squared() < 1e-16)
			u = rayVec.cross(vec3f(0.0, 0.0, 1.0));
		u = u.normalize();
		const vec3f v = u.cross(rayVec).normalize();

		for (int i = 0; i < num_sample; ++i)
		{
			toReturn.push_back((rayVec + u * getRand(r) + v * getRand(r)).normalize());
		}
		return toReturn;
	}
}
