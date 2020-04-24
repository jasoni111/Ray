// The main ray tracer.
#define _USE_MATH_DEFINES
#define  NOMINMAX
#include <Fl/fl_ask.h>

#include "RayTracer.h"

#include <stack>

#include "scene/light.h"
#include "scene/material.h"
#include "scene/ray.h"
#include "fileio/read.h"
#include "fileio/parse.h"
#include <array>
#include <cmath>
#include <ppl.h>

#include "ui/TraceUI.h"
extern TraceUI* traceUI;

namespace helperFun
{
	double inline getRand(double r)
	{
		return (double)rand() / RAND_MAX * 2.0 * r - r;
	}

	std::vector<vec3f> sampleRay(vec3f rayVec, double r, int num_sample = 20)
	{
		std::vector<vec3f> toReturn;
		vec3f up{0, 1, 0};
		auto u = rayVec.cross(up).normalize();
		auto v = u.cross(rayVec).normalize();
		u = rayVec.cross(v).normalize();
		for (auto i = 0; i < num_sample; ++i)
		{
			toReturn.push_back((rayVec + u * getRand(r) + v * getRand(r)).normalize());
		}
		return toReturn;
	}
}


namespace
{
	std::array<double, 2> getInOutRefractionIndex(std::vector<Material>& materials_in, const Material& m)
	{
		std::array<double, 2> to_return;

		double inRefractionIndex;
		double OutRefractionIndex;

		if (materials_in.empty())
		{
			//getting in
			inRefractionIndex = 1;
			OutRefractionIndex = m.index;
			materials_in.push_back(m);
		}
		else
		{
			inRefractionIndex = materials_in.back().index;
			if (materials_in.back().id == m.id)
			{
				//getting out
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
				//getting in
				materials_in.push_back(m);
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
			return {0, 0, 0};
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

	std::vector<Material> materials_in;

	return traceRay(scene, r, vec3f(1.0, 1.0, 1.0), 0,
	                materials_in).clamp();
}


// Do recursive ray tracing!  You'll want to insert a lot of code here
// (or places called from here) to handle reflection, refraction, etc etc.
vec3f RayTracer::traceRay(Scene* scene, const ray& r,
                          const vec3f& thresh, int depth,
                          std::vector<Material> materials_in)
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
		auto Normal = i.N;
		const auto& V = r.getDirection();;
		const auto Pos = r.at(i.t);

		const Material& m = i.getMaterial();

		bool use_diffuseMap = i.obj->isBox && traceUI->m_boxDiffuseTexture->value();
		bool use_emissionMap = i.obj->isBox && traceUI->m_boxEmissionTexture->value();
		bool use_opacityMap = i.obj->isBox && traceUI->m_boxOpacity->value();
		bool use_specularMap = i.obj->isBox && traceUI->m_boxSpecularTexture->value();

		const auto phong = m.shade(scene, r, i, 
			use_diffuseMap, diffuseMap.getColor(i.i_box_x, i.i_box_y),
			use_emissionMap, emissionMap.getColor(i.i_box_x, i.i_box_y), 
			use_opacityMap, opacityMap.getColor(i.i_box_x, i.i_box_y),
			use_specularMap, specularMap.getColor(i.i_box_x, i.i_box_y)
			);

		if (traceUI->getThreshold() > phong.length())
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

		const auto rVec = (V - 2 * Normal.dot(V) * Normal).normalize();
		ray reflectedRay{Pos, rVec};
		auto reflectColor = traceRay(scene, reflectedRay, thresh
		                             , depth + 1, materials_in);
		auto glossyR = traceUI->m_glossyReflection->value();
		if (glossyR && depth < max_depth)
		{
			auto sampleVecs = helperFun::sampleRay(rVec, 0.01, 20);
			Concurrency::parallel_for_each(sampleVecs.begin(), sampleVecs.end(),
			                               [&](const vec3f& sampleVec)
			                               {
				                               // ray reflectedRay{Pos, sampleVec};
				                               reflectColor += traceRay(scene, ray{Pos, sampleVec}, thresh,
				                                                        std::max(max_depth - 1, depth + 1),
				                                                        materials_in);
			                               }
			);
			reflectColor /= 21;
		}
		reflectColor = prod(reflectColor, m.kr);

		vec3f refractRay{0.0, 0.0, 0.0};

		vec3f refractColor;
		if (m.kt.length())
		{
			const auto InOut = getInOutRefractionIndex(materials_in, m);
			const auto& In = InOut[0];
			const auto& Out = InOut[1];

			const auto refractionVec
				= calRefractionVec(V, Normal, In, Out);
			ray refractionRay{Pos, refractionVec};
			refractColor = traceRay(scene, refractionRay, thresh
			                        , depth + 1, materials_in);
		}
		refractColor = prod(refractColor, m.kt);

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
	buffer = NULL;
	buffer_width = buffer_height = 256;
	scene = NULL;

	m_bSceneLoaded = false;
}


RayTracer::~RayTracer()
{
	delete [] buffer;
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
	buffer = new unsigned char[ bufferSize ];

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
		delete [] buffer;
		buffer = new unsigned char[ bufferSize ];
	}
	memset(buffer, 0, w * h * 3);
}

void RayTracer::traceLines(int start, int stop)
{
	vec3f col;
	if (!scene)
		return;

	if (stop > buffer_height)
		stop = buffer_height;

	for (int j = start; j < stop; ++j)
		for (int i = 0; i < buffer_width; ++i)
			tracePixel(i, j);
}


vec3f RayTracer::adaptiveTracePixel(const double& i, const double& j, const double& dw, const double& dh, int depth)
{
	if (depth >= 5)
	{
		return trace(scene, i, j);
	}

	double x0 = i - dw / 2.f, x1 = i + dw / 2.f;
	double y0 = j - dh / 2.f, y1 = j + dh / 2.f;
	vec3f c00;
	vec3f c01;
	vec3f c10;
	vec3f c11;
	vec3f c;
	Concurrency::parallel_invoke(
		[&] { c00 = trace(scene, x0, y0); },
		[&] { c01 = trace(scene, x0, y1); },
		[&] { c10 = trace(scene, x1, y0); },
		[&] { c11 = trace(scene, x1, y1); },
		[&] { c = trace(scene, i, j); }
	);
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
	// toReturn = vec3f(1, 1, 1);
	return toReturn / num_trace;
}


void RayTracer::tracePixel(int i, int j)
{
	vec3f col;

	if (!scene)
		return;

	double x = double(i) / double(buffer_width);
	double y = double(j) / double(buffer_height);

	const auto isAdaptiveSuperRes = traceUI->m_adaptiveSampling->value();
	if (isAdaptiveSuperRes)
	{
		//adaptive
		col = this->adaptiveTracePixel(x, y, 1.0 / buffer_width, 1.0 / buffer_height, 0);
	}
	else
	{
		//super sample
		col = {0, 0, 0};

		const auto superSampleRate = traceUI->getNumOfSupPixel();

		//non adaptive
		const auto jitter = traceUI->m_jitter->value() == 1;
		const auto dw = 1.0 / buffer_width / superSampleRate;
		const auto dh = 1.0 / buffer_height / superSampleRate;
		for (auto i = 0; i < superSampleRate; ++ i)
		{
			for (auto j = 0; j < superSampleRate; ++j)
			{
				col += trace(scene, x + i * dw + helperFun::getRand(dw) * jitter,
				             y + j * dh + helperFun::getRand(dh) * jitter);
			}
		}
		col /= superSampleRate * superSampleRate;
		// col = trace(scene, x, y);
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
