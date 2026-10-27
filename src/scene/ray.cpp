#include "ray.h"
#include "material.h"
#include "scene.h"

#include <cmath>

const Material&
isect::getMaterial() const
{
	return obj->getMaterial();
}

const Material&
isect::getMaterial(Material& scratch) const
{
	if (!vm0)
		return obj->getMaterial();

	const double w = 1.0 - bu - bv;
	scratch = w * (*vm0);
	scratch += bu * (*vm1);
	scratch += bv * (*vm2);
	scratch.id = vm0->id;
	return scratch;
}

namespace helperFun
{
	double getRand(double r)
	{
		thread_local unsigned int s = 0x9e3779b9u;
		s = s * 1664525u + 1013904223u;
		return (s / 4294967296.0) * 2.0 * r - r;
	}

	void sampleRay(const vec3f& rayVec, double r, int num_sample, vec3f* out)
	{
		vec3f up(0.0, 1.0, 0.0);
		if (std::abs(rayVec.dot(up)) > 0.9)
			up = vec3f(1.0, 0.0, 0.0);

		vec3f u = rayVec.cross(up);
		if (u.length_squared() < 1e-16)
			u = rayVec.cross(vec3f(0.0, 0.0, 1.0));
		u = u.normalize();
		const vec3f v = u.cross(rayVec).normalize();

		for (int i = 0; i < num_sample; ++i)
			out[i] = (rayVec + u * getRand(r) + v * getRand(r)).normalize();
	}
}
