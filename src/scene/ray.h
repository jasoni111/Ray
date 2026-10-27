//
// ray.h
//
// The low-level classes used by ray tracing: the ray and isect classes.
//

#ifndef __RAY_H__
#define __RAY_H__

#include "../vecmath/vecmath.h"
#include "material.h"

class SceneObject;

// A ray has a position where the ray starts, and a direction (which should
// always be normalized!)

class ray {
public:
	ray(const vec3f& pp, const vec3f& dd)
		: p(pp), d(dd)
	{
		inv_d[0] = (dd[0] != 0.0) ? (1.0 / dd[0]) : 1.0e308;
		inv_d[1] = (dd[1] != 0.0) ? (1.0 / dd[1]) : 1.0e308;
		inv_d[2] = (dd[2] != 0.0) ? (1.0 / dd[2]) : 1.0e308;
	}

	vec3f at(double t) const
	{
		return p + (t * d);
	}

	const vec3f& getPosition() const { return p; }
	const vec3f& getDirection() const { return d; }
	const vec3f& getInvDirection() const { return inv_d; }

protected:
	vec3f p;
	vec3f d;
	vec3f inv_d;
};

// The description of an intersection point.

class isect
{
public:
	isect()
		: obj(nullptr), t(0.0), N(), i_box_x(0.0), i_box_y(0.0),
		  bu(0.0), bv(0.0), vm0(nullptr), vm1(nullptr), vm2(nullptr) {}

	void setObject(SceneObject* o) { obj = o; }
	void setT(double tt) { t = tt; }
	void setN(const vec3f& n) { N = n; }

public:
	const SceneObject* obj;
	double t;
	vec3f N;

	// intersection on box plane, range[0, 1]
	double i_box_x, i_box_y;

	// optional barycentric vertex-material interpolation (no heap)
	double bu, bv;
	const Material* vm0;
	const Material* vm1;
	const Material* vm2;

	const Material& getMaterial() const;
	const Material& getMaterial(Material& scratch) const;
};

const double RAY_EPSILON = 0.00001;
const double NORMAL_EPSILON = 0.00001;

struct Medium
{
	double index;
	double id;
};

enum AccelMode
{
	AccelNone = 0,
	AccelOctree = 1,
	AccelBVH = 2
};

struct RenderSettings
{
	double threshold = 0.0;
	double attenConst = 0.0;
	double attenLinear = 0.0;
	double attenQuad = 1.0;
	int superSampleRate = 1;
	bool softShadow = false;
	bool glossyReflection = false;
	bool adaptiveSampling = false;
	bool jitter = false;
	AccelMode accel = AccelBVH;
	bool boxDiffuse = false;
	bool boxEmission = false;
	bool boxOpacity = false;
	bool boxSpecular = false;
	bool parallel = true;
};

extern RenderSettings g_render;

namespace helperFun
{
	double getRand(double r);
	void sampleRay(const vec3f& rayVec, double r, int num_sample, vec3f* out);
}


#endif // __RAY_H__
