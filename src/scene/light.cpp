#define NOMINMAX
#include <cmath>

#include "light.h"

#include <vector>

#include "../ui/TraceUI.h"
#include "ray.h"


extern TraceUI* traceUI;

namespace
{
	const double kDirectionalShadowT = 1.0e308;

	vec3f shadowAlongRay(Scene* scene, const vec3f& P, const vec3f& dir, double maxT, const vec3f& lightColor)
	{
		vec3f origin = P;
		double remaining = maxT;
		vec3f atten = lightColor;

		for (int hop = 0; hop < 8; ++hop)
		{
			if (remaining <= RAY_EPSILON)
				return atten;

			const ray r(origin + dir * RAY_EPSILON, dir);
			isect i;
			if (!scene->intersect(r, i) || i.t > remaining)
				return atten;

			const Material& m = i.getMaterial();
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

	if (traceUI->m_softShadow->value())
	{
		auto sampleVecs = helperFun::sampleRay(d, 0.05, 20);
		for (const auto& sampleVec : sampleVecs)
		{
			ret += shadowAlongRay(getScene(), P, sampleVec, kDirectionalShadowT, color);
		}
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
	double ret = 1.0 / (traceUI->getAttenConst() + traceUI->getAttenLinear() * d + traceUI->getAttenQuad() * d * d);


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

	if (traceUI->m_softShadow->value())
	{
		auto sampleVecs = helperFun::sampleRay(d, 0.05, 20);
		for (const auto& sampleVec : sampleVecs)
		{
			const double denom = sampleVec.dot(d);
			if (denom < RAY_EPSILON)
			{
				ret += color;
				continue;
			}
			ret += shadowAlongRay(getScene(), P, sampleVec, dist / denom, color);
		}
		ret /= 21;
	}

	return ret;
}
