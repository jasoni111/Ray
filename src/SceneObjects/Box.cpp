#include <cmath>

#include "Box.h"

bool Box::intersectLocal( const ray& r, isect& i ) const
{
	const vec3f& p = r.getPosition();
	const vec3f& d = r.getDirection();
	const vec3f& inv = r.getInvDirection();

	double tMin = -1.0e308;
	double tMax = 1.0e308;

	for (int axis = 0; axis < 3; ++axis)
	{
		const double vd = d[axis];
		if (vd == 0.0)
		{
			if (p[axis] < -0.5 || p[axis] > 0.5)
				return false;
			continue;
		}

		double t1 = (-0.5 - p[axis]) * inv[axis];
		double t2 = ( 0.5 - p[axis]) * inv[axis];
		if (t1 > t2)
		{
			const double tmp = t1;
			t1 = t2;
			t2 = tmp;
		}
		if (t1 > tMin)
			tMin = t1;
		if (t2 < tMax)
			tMax = t2;
		if (tMin > tMax)
			return false;
		if (tMax < 0.0)
			return false;
	}

	double t;
	if (tMin >= RAY_EPSILON)
		t = tMin;
	else if (tMin < 0.0 && tMax > RAY_EPSILON)
		t = tMax;
	else
		return false;

	const vec3f pos = r.at(t);
	int face = -1;
	double best = 1.0e308;
	for (int j = 0; j < 3; ++j)
	{
		const double dPos = std::abs(pos[j] - 0.5);
		const double dNeg = std::abs(pos[j] + 0.5);
		if (dPos < best)
		{
			best = dPos;
			face = j;
		}
		if (dNeg < best)
		{
			best = dNeg;
			face = j;
		}
	}
	if (face < 0)
		return false;

	vec3f N(0, 0, 0);
	N[face] = (pos[face] >= 0.0) ? 1.0 : -1.0;
	if (d * N > 0)
		N = -N;

	i.obj = this;
	i.t = t;
	i.N = N;
	i.i_box_x = pos[(face + 1) % 3] + 0.5;
	i.i_box_y = pos[(face + 2) % 3] + 0.5;
	return true;
}
