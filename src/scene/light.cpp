#define NOMINMAX
#include <cmath>

#include "light.h"

#include <ppl.h>
#include <vector>

#include "../ui/TraceUI.h"
#include "ray.h"


extern TraceUI* traceUI;


double DirectionalLight::distanceAttenuation(const vec3f& P) const
{
	// distance to light is infinite, so f(di) goes to 0.  Return 1.
	return 1.0;
}


vec3f DirectionalLight::shadowAttenuation(const vec3f& P) const
{
	// YOUR CODE HERE:
	// You should implement shadow-handling code here.

	vec3f d = getDirection(P);
	isect i;
	ray r(P, d);
	vec3f ret = color;

	if (this->getScene()->intersect(r, i))
	{
		ret = prod(color, i.getMaterial().kt);
	}
	const auto isSoftShadow = traceUI->m_softShadow->value();
	if (isSoftShadow)
	{
		auto sampleVecs = helperFun::sampleRay(d, 0.05, 20);
		for (const auto& sampleVec : sampleVecs)
		{
			isect i;
			if (scene->intersect(ray{P, sampleVec}, i))
			{
				ret += prod(color, i.getMaterial().kt);
			}
			else
			{
				ret += color;
			}
		}
		// Concurrency::parallel_for_each(sampleVecs.begin(), sampleVecs.end(),
		//                                [&](const vec3f& sampleVec)
		//                                {
		// 	                               isect i;
		// 	                               if (scene->intersect(ray{P, sampleVec}, i))
		// 	                               {
		// 		                               ret += prod(color, i.getMaterial().kt);
		// 	                               }
		// 	                               else
		// 	                               {
		// 		                               ret += color;
		// 	                               }
		//                                }
		// );
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
	const auto d = getDirection(P);
	isect i;
	ray r(P, d);
	auto ret = color;

	if (this->getScene()->intersect(r, i))
	{
		ret = prod(color, i.getMaterial().kt);
	}
	const auto isSoftShadow = false;
	if (isSoftShadow)
	{
		auto sampleVecs = helperFun::sampleRay(d, 0.05, 20);
		for (const auto& sampleVec : sampleVecs)
		{
			isect i;
			if (scene->intersect(ray{ P, sampleVec }, i))
			{
				ret += prod(color, i.getMaterial().kt);
			}
			else
			{
				ret += color;
			}
		}
		ret /= 21;
	}

	return ret;
}
