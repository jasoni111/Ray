// The main ray tracer.
#define _USE_MATH_DEFINES
#define  NOMINMAX
#include <Fl/fl_ask.h>

#include "RayTracer.h"

#include "scene/light.h"
#include "scene/material.h"
#include "scene/ray.h"
#include "fileio/read.h"
#include "fileio/parse.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <string.h>
#include <ppl.h>

#include "ui/TraceUI.h"
extern TraceUI* traceUI;

namespace
{
	std::array<double, 2> getInOutRefractionIndex(std::vector<Medium>& materials_in, const Material& m)
	{
		double inRefractionIndex;
		double OutRefractionIndex;

		if (materials_in.empty())
		{
			inRefractionIndex = 1;
			OutRefractionIndex = m.index;
			materials_in.push_back(Medium{ m.index, m.id });
		}
		else
		{
			inRefractionIndex = materials_in.back().index;
			if (materials_in.back().id == m.id)
			{
				materials_in.pop_back();
				if (materials_in.empty())
				{
					OutRefractionIndex = 1;
				}
				else
				{
					OutRefractionIndex = materials_in.back().index;
				}
			}
			else
			{
				materials_in.push_back(Medium{ m.index, m.id });
				OutRefractionIndex = m.index;
			}
		}

		return std::array<double, 2>{inRefractionIndex, OutRefractionIndex};
	}

	template <class T>
	T clamp(T x, T min, T max)
	{
		return std::max(std::min(x, max), min);
	}

	vec3f calRefractionVec(vec3f i, vec3f Normal, double inIndex, double outIndex)
	{
		//reference : https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-shading/reflection-refraction-fresnel
		if (std::abs(std::abs(Normal * i) - 1) < RAY_EPSILON)
			return i;
		if (inIndex == outIndex)
		{
			return i;
		}
		const auto eta = inIndex / outIndex;
		auto c1 = Normal.dot(i.normalize());
		c1 = clamp(-1.0, 1.0, c1);
		if (c1 < 0)
		{
			c1 = -c1;
		}

		auto c2 = 1 - eta * eta * (1 - c1 * c1);
		if (c2 < 0 || c2 > 1)
		{
			return { 0, 0, 0 };
		}
		c2 = std::sqrt(c2);
		return eta * (i + c1 * Normal) - Normal * c2;
	}
}

// Trace a top-level ray through normalized window coordinates (x,y)
// through the projection plane, and out into the scene.  All we do is
// enter the main ray-tracing method, getting things started by plugging
// in an initial ray weight of (0.0,0.0,0.0) and an initial recursion depth of 0.
vec3f RayTracer::trace(Scene* scene, double x, double y)
{
	ray r(vec3f(0, 0, 0), vec3f(0, 0, 0));
	scene->getCamera()->rayThrough(x, y, r);

	std::vector<Medium> materials_in;

	return traceRay(scene, r, vec3f(1.0, 1.0, 1.0), 0,
		materials_in).clamp();
}


// Do recursive ray tracing!  You'll want to insert a lot of code here
// (or places called from here) to handle reflection, refraction, etc etc.
vec3f RayTracer::traceRay(Scene* scene, const ray& r,
	const vec3f& thresh, int depth,
	const std::vector<Medium>& materials_in)
{
	isect i;

	if (depth > max_depth)
	{
		return vec3f(0, 0, 0);
	}

	if (scene->intersect(r, i))
	{
		// YOUR CODE HERE

		// An intersection occured!  We've got work to do.  For now,
		// this code gets the material for the surface that was intersected,
		// and asks that material to provide a color for the ray.  

		// This is a great place to insert code for recursive ray tracing.
		// Instead of just returning the result of shade(), add some
		// more steps: add in the contributions from reflected and refracted
		// rays.

		// auto phong = m.shade(scene,r,i,in)
		vec3f Normal = i.N;
		const auto& V = r.getDirection();
		const auto Pos = r.at(i.t);

		Material matScratch;
		const Material& m = i.getMaterial(matScratch);

		const bool use_diffuseMap = i.obj->isBox && g_render.boxDiffuse;
		const bool use_emissionMap = i.obj->isBox && g_render.boxEmission;
		const bool use_opacityMap = i.obj->isBox && g_render.boxOpacity;
		const bool use_specularMap = i.obj->isBox && g_render.boxSpecular;

		const auto phong = m.shade(scene, r, i,
			use_diffuseMap, use_diffuseMap ? diffuseMap.getColor(i.i_box_x, i.i_box_y) : m.kd,
			use_emissionMap, use_emissionMap ? emissionMap.getColor(i.i_box_x, i.i_box_y) : m.ke,
			use_opacityMap, use_opacityMap ? opacityMap.getColor(i.i_box_x, i.i_box_y) : vec3f(1, 1, 1) - m.kt,
			use_specularMap, use_specularMap ? specularMap.getColor(i.i_box_x, i.i_box_y) : m.ks
		);

		if (g_render.threshold > 0.0 && phong.length_squared() < g_render.threshold * g_render.threshold)
		{
			return phong;
		}

		if (!materials_in.empty())
		{
			if (materials_in.back().id == m.id)
			{
				Normal = -Normal;
			}
		}

		vec3f reflectColor(0.0, 0.0, 0.0);
		if (depth < max_depth && !m.kr.iszero())
		{
			const auto rVec = (V - 2 * Normal.dot(V) * Normal).normalize();
			ray reflectedRay{ Pos + rVec * RAY_EPSILON, rVec };
			reflectColor = traceRay(scene, reflectedRay, thresh, depth + 1, materials_in);
			if (g_render.glossyReflection)
			{
				vec3f sampleVecs[20];
				helperFun::sampleRay(rVec, 0.01, 20, sampleVecs);
				for (int s = 0; s < 20; ++s)
				{
					ray glossyRay{ Pos + sampleVecs[s] * RAY_EPSILON, sampleVecs[s] };
					reflectColor += traceRay(scene, glossyRay, thresh,
						std::max(max_depth - 1, depth + 1), materials_in);
				}
				reflectColor /= 21;
			}
			reflectColor = prod(reflectColor, m.kr);
		}

		vec3f refractColor(0.0, 0.0, 0.0);
		if (depth < max_depth && !m.kt.iszero())
		{
			std::vector<Medium> stack = materials_in;
			const auto InOut = getInOutRefractionIndex(stack, m);
			const auto refractionVec = calRefractionVec(V, Normal, InOut[0], InOut[1]);
			if (!refractionVec.iszero())
			{
				ray refractionRay{ Pos + refractionVec * RAY_EPSILON, refractionVec };
				refractColor = traceRay(scene, refractionRay, thresh, depth + 1, stack);
			}
			refractColor = prod(refractColor, m.kt);
		}

		return phong + reflectColor + refractColor;
	}
	else
	{
		// No intersection.  This ray travels to infinity, so we color
		// it according to the background color, which in this (simple) case
		// is just black.

		return vec3f(0.0, 0.0, 0.0);
	}
}

RayTracer::RayTracer()
{
	buffer = nullptr;

	//set up at traceSetup
	buffer_width = buffer_height = bufferSize = -1;
	scene = nullptr;

	m_bSceneLoaded = false;
}


RayTracer::~RayTracer()
{
	delete[] buffer;
	delete scene;
}

void RayTracer::getBuffer(unsigned char*& buf, int& w, int& h)
{
	buf = buffer;
	w = buffer_width;
	h = buffer_height;
}

double RayTracer::aspectRatio()
{
	return scene ? scene->getCamera()->getAspectRatio() : 1;
}

bool RayTracer::sceneLoaded()
{
	return m_bSceneLoaded;
}

bool RayTracer::loadScene(char* fn)
{
	delete scene;
	scene = nullptr;
	m_bSceneLoaded = false;

	try
	{
		scene = readScene(fn);
	}
	catch (ParseError pe)
	{
		fl_alert("ParseError: %s\n", pe);
		return false;
	}

	if (!scene)
		return false;

	buffer_width = 256;
	buffer_height = (int)(buffer_width / scene->getCamera()->getAspectRatio() + 0.5);

	bufferSize = buffer_width * buffer_height * 3;
	delete[] buffer;
	buffer = new unsigned char[bufferSize];

	// separate objects into bounded and unbounded
	scene->initScene();

	// Add any specialized scene loading code here

	m_bSceneLoaded = true;

	return true;
}

void RayTracer::traceSetup(int w, int h)
{
	if (buffer_width != w || buffer_height != h)
	{
		buffer_width = w;
		buffer_height = h;

		bufferSize = buffer_width * buffer_height * 3;
		delete[] buffer;
		buffer = new unsigned char[bufferSize];
	}
	memset(buffer, 0, w * h * 3);

	if (traceUI)
	{
		g_render.threshold = traceUI->getThreshold();
		g_render.attenConst = traceUI->getAttenConst();
		g_render.attenLinear = traceUI->getAttenLinear();
		g_render.attenQuad = traceUI->getAttenQuad();
		g_render.superSampleRate = static_cast<int>(traceUI->getNumOfSupPixel());
		if (g_render.superSampleRate < 1)
			g_render.superSampleRate = 1;
		g_render.softShadow = traceUI->m_softShadow && traceUI->m_softShadow->value();
		g_render.glossyReflection = traceUI->m_glossyReflection && traceUI->m_glossyReflection->value();
		g_render.adaptiveSampling = traceUI->m_adaptiveSampling && traceUI->m_adaptiveSampling->value();
		g_render.jitter = traceUI->m_jitter && traceUI->m_jitter->value();
		g_render.accel = static_cast<AccelMode>(traceUI->getAccelMode());
		g_render.boxDiffuse = traceUI->m_boxDiffuseTexture && traceUI->m_boxDiffuseTexture->value();
		g_render.boxEmission = traceUI->m_boxEmissionTexture && traceUI->m_boxEmissionTexture->value();
		g_render.boxOpacity = traceUI->m_boxOpacity && traceUI->m_boxOpacity->value();
		g_render.boxSpecular = traceUI->m_boxSpecularTexture && traceUI->m_boxSpecularTexture->value();
	}
	g_render.parallel = std::getenv("RAY_SERIAL") == nullptr;
	if (const char* a = std::getenv("RAY_ACCEL"))
	{
		if (_stricmp(a, "none") == 0 || _stricmp(a, "off") == 0 || _stricmp(a, "linear") == 0)
			g_render.accel = AccelNone;
		else if (_stricmp(a, "octree") == 0 || _stricmp(a, "oct") == 0 || _stricmp(a, "octtree") == 0)
			g_render.accel = AccelOctree;
		else if (_stricmp(a, "bvh") == 0)
			g_render.accel = AccelBVH;
	}
	if (std::getenv("RAY_NO_ACCEL"))
		g_render.accel = AccelNone;
}

void RayTracer::traceLines(int start, int stop)
{
	if (!scene)
		return;

	if (stop > buffer_height)
		stop = buffer_height;

	if (g_render.parallel && (stop - start) > 1)
	{
		Concurrency::parallel_for(start, stop, [this](int j)
		{
			for (int i = 0; i < buffer_width; ++i)
				tracePixel(i, j);
		});
	}
	else
	{
		for (int j = start; j < stop; ++j)
			for (int i = 0; i < buffer_width; ++i)
				tracePixel(i, j);
	}
}


vec3f RayTracer::adaptiveTracePixel(const double& i, const double& j, const double& dw, const double& dh, int depth)
{
	if (depth >= 5)
	{
		return trace(scene, i, j);
	}

	double x0 = i - dw / 2.f, x1 = i + dw / 2.f;
	double y0 = j - dh / 2.f, y1 = j + dh / 2.f;
	const vec3f c00 = trace(scene, x0, y0);
	const vec3f c01 = trace(scene, x0, y1);
	const vec3f c10 = trace(scene, x1, y0);
	const vec3f c11 = trace(scene, x1, y1);
	const vec3f c = trace(scene, i, j);
	vec3f toReturn = c;

	auto num_trace = 1;

	if ((c00 - c).length() > 0.05)
	{
		toReturn += adaptiveTracePixel(i - dw / 4.f, j - dh / 4.f, dw / 2.f, dh / 2.f, depth + 1);
		++num_trace;
	}
	if ((c01 - c).length() > 0.05)
	{
		toReturn += adaptiveTracePixel(i - dw / 4.f, j + dh / 4.f, dw / 2.f, dh / 2.f, depth + 1);
		++num_trace;
	}
	if ((c11 - c).length() > 0.05)
	{
		toReturn += adaptiveTracePixel(i + dw / 4.f, j + dh / 4.f, dw / 2.f, dh / 2.f, depth + 1);
		++num_trace;
	}
	if ((c10 - c).length() > 0.05)
	{
		toReturn += adaptiveTracePixel(i + dw / 4.f, j - dh / 4.f, dw / 2.f, dh / 2.f, depth + 1);
		++num_trace;
	}
	return toReturn / num_trace;
}


void RayTracer::tracePixel(int i, int j)
{
	vec3f col;

	if (!scene)
		return;

	double x = double(i) / double(buffer_width);
	double y = double(j) / double(buffer_height);

	if (g_render.adaptiveSampling)
	{
		col = this->adaptiveTracePixel(x, y, 1.0 / buffer_width, 1.0 / buffer_height, 0);
	}
	else
	{
		col = { 0, 0, 0 };

		const int superSampleRate = g_render.superSampleRate;
		const int jitter = g_render.jitter ? 1 : 0;
		const auto dw = 1.0 / buffer_width / superSampleRate;
		const auto dh = 1.0 / buffer_height / superSampleRate;
		for (int si = 0; si < superSampleRate; ++si)
		{
			for (int sj = 0; sj < superSampleRate; ++sj)
			{
				col += trace(scene, x + si * dw + helperFun::getRand(dw) * jitter,
					y + sj * dh + helperFun::getRand(dh) * jitter);
			}
		}
		col /= superSampleRate * superSampleRate;
	}

	unsigned char* pixel = buffer + (i + j * buffer_width) * 3;

	pixel[0] = (int)(255.0 * col[0]);
	pixel[1] = (int)(255.0 * col[1]);
	pixel[2] = (int)(255.0 * col[2]);
}

// ray RayTracer::reflectDirection(const ray& rDirect, const isect& i)
// {
// 	// https://www.fabrizioduroni.it/2017/08/25/how-to-calculate-reflection-vector.html
//
// 	const auto& Normal = i.N;
// 	const auto& V = rDirect.getDirection();;
//
// 	vec3f outPos = rDirect.at(i.t);
// 	vec3f outDir =
// 		(V - 2 * Normal.dot(V) * Normal).normalize();
//
//
// 	return ray(outPos, outDir);
// }
