#ifndef __RAYTRACER_H__
#define __RAYTRACER_H__

// The main ray tracer.

#include <vector>

#include "scene/scene.h"
#include "scene/ray.h"

#include "../texture.h"

class RayTracer
{
public:
	RayTracer();
	~RayTracer();

	vec3f trace(Scene* scene, double x, double y);
	vec3f traceRay(Scene* scene, const ray& r, const vec3f& thresh, int depth, vector< Material>);

	void getBuffer(unsigned char*& buf, int& w, int& h);
	double aspectRatio();
	void traceSetup(int w, int h);
	void traceLines(int start = 0, int stop = 10000000);

	vec3f adaptiveTracePixel(const double& i, const double& j, const double& dw, const double& dh, int depth);

	void tracePixel(int i, int j);

	bool loadScene(char* fn);

	bool sceneLoaded();

	//ray reflectDirection(const ray& rDirect, const isect& i);

	int max_depth = 10;

	Texture diffuseMap;
	Texture emissionMap;
	Texture specularMap;
	Texture opacityMap;

private:
	unsigned char* buffer;
	int buffer_width, buffer_height;
	int bufferSize;
	Scene* scene;

	bool m_bSceneLoaded;
};

#endif // __RAYTRACER_H__
