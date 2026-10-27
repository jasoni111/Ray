#include <cmath>
#include <assert.h>

#include "Box.h"

bool Box::intersectLocal( const ray& r, isect& i ) const
{
	// YOUR CODE HERE:
    // Add box intersection code here.
	// it currently ignores all boxes and just returns false.

	double tMin, tMax;

	BoundingBox box;
	box.max = vec3f(0.5, 0.5, 0.5);
	box.min = vec3f(-0.5, -0.5, -0.5);
	if (!box.intersect(r, tMin, tMax))
		return false;
	if (tMax < RAY_EPSILON)
		return false;

	const double t = (tMin >= RAY_EPSILON) ? tMin : tMax;
	if (t < RAY_EPSILON)
		return false;

	i.obj = this;
	i.t = t;

	i.N = vec3f(0, 0, 0);
	vec3f pos = r.at(t);
	for (int j = 0; j < 3; ++j) {
		if (abs(abs(pos[j]) - 0.5) < RAY_EPSILON) {
			i.N[j] = pos[j] > 0 ? 1 : -1;

			i.i_box_x = pos[(j + 1) % 3] + 0.5;
			i.i_box_y = pos[(j + 2) % 3] + 0.5;
		}
	}

	if (r.getDirection() * i.N > 0)
		i.N = -i.N;

	return true;
}
