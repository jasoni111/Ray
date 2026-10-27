#define NOMINMAX
#include <cmath>

#include "light.h"
#include "ray.h"

namespace
{
	const double kDirectionalShadowT = 1.0e308;

	vec3f shadowAlongRay(Scene* scene, const vec3f& P, const vec3f& dir, double maxT, const vec3f& lightColor)
	{
		vec3f origin = P;
		double remaining = maxT;
		vec3f atten = lightColor;
		const bool anyHit = !scene->hasTransparentObjects();

		for (int hop = 0; hop < 8; ++hop)
		{
			if (remaining <= RAY_EPSILON)
				return atten;

			const ray r(origin + dir * RAY_EPSILON, dir);
			isect i;
			if (!scene->intersect(r, i, remaining, anyHit) || i.t > remaining)
				return atten;

			if (anyHit)
				return vec3f(0.0, 0.0, 0.0);

			Material scratch;
			const Material& m = i.getMaterial(scratch);
			if (m.kt.iszero())
				return vec3f(0.0, 0.0, 0.0);

			atten = prod(atten, m.kt);
			origin = r.at(i.t);
			remaining -= i.t;
		}
		return atten;
	}
}


double DirectionalLight::distanceAttenuation(const vec3f& P) const
{
	// distance to light is infinite, so f(di) goes to 0.  Return 1.
	return 1.0;
}


vec3f DirectionalLight::shadowAttenuation(const vec3f& P) const
{
	const vec3f d = getDirection(P);
	vec3f ret = shadowAlongRay(getScene(), P, d, kDirectionalShadowT, color);

	if (g_render.softShadow)
	{
		vec3f sampleVecs[20];
		helperFun::sampleRay(d, 0.05, 20, sampleVecs);
		for (int s = 0; s < 20; ++s)
			ret += shadowAlongRay(getScene(), P, sampleVecs[s], kDirectionalShadowT, color);
		ret /= 21;
	}

	return ret;
}

vec3f DirectionalLight::getColor(const vec3f& P) const
{
	// Color doesn't depend on P 
	return color;
}

vec3f DirectionalLight::getDirection(const vec3f& P) const
{
	return -orientation;
}

double PointLight::distanceAttenuation(const vec3f& P) const
{
	// YOUR CODE HERE

	// You'll need to modify this method to attenuate the intensity 
	// of the light based on the distance between the source and the 
	// point P.  For now, I assume no attenuation and just return 1.0
	double d = vec3f(P - position).length();
	// pScene->
	// double a = pScene->distAtteA, b = pScene->distAtteB, c = pScene->distAtteC;
	// double ret = 1.0 / (a + b * d + c * d * d);
	// return std::min(1.0, ret);
	double ret = 1.0 / (g_render.attenConst + g_render.attenLinear * d + g_render.attenQuad * d * d);


	return std::min(1.0, ret);
}

vec3f PointLight::getColor(const vec3f& P) const
{
	// Color doesn't depend on P 
	return color;
}

vec3f PointLight::getDirection(const vec3f& P) const
{
	return (position - P).normalize();
}


vec3f PointLight::shadowAttenuation(const vec3f& P) const
{
	const vec3f toLight = position - P;
	const double dist = toLight.length();
	if (dist < RAY_EPSILON)
		return color;

	const vec3f d = toLight / dist;
	vec3f ret = shadowAlongRay(getScene(), P, d, dist, color);

	if (g_render.softShadow)
	{
		vec3f sampleVecs[20];
		helperFun::sampleRay(d, 0.05, 20, sampleVecs);
		for (int s = 0; s < 20; ++s)
		{
			const double denom = sampleVecs[s].dot(d);
			if (denom < RAY_EPSILON)
			{
				ret += color;
				continue;
			}
			ret += shadowAlongRay(getScene(), P, sampleVecs[s], dist / denom, color);
		}
		ret /= 21;
	}

	return ret;
}
