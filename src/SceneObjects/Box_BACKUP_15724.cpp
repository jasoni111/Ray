#include <cmath>
#include <assert.h>

#include "Box.h"


bool Box::intersectLocal( const ray& r, isect& i ) const
{
	// YOUR CODE HERE:
    // Add box intersection code here.
	// it currently ignores all boxes and just returns false.
<<<<<<< HEAD
	
	return false;
=======

	// https://www.rose-hulman.edu/class/csse/csse451/AABB/
	// Also read the implementation in scene.cpp

	vec3f e = r.getPosition();
	vec3f d = r.getDirection();

	vec3f minVertex(-0.5, -0.5, -0.5);
	vec3f maxVertex(0.5, 0.5, 0.5);

	double t[6];

	i.obj = this;

	// find intersection on the 6 bounding planes
	double minT = 0;
	for (int axis = 0; axis < 3; ++axis) {
		double t0 = (minVertex[axis] - e[axis]) / d[axis];
		double t1 = (maxVertex[axis] - e[axis]) / d[axis];

		if (t0 > t1) {
			t[axis * 2] = t0;
			t[axis * 2 + 1] = t1;

			if (t1 > 0 && t1 < minT) {
				minT = t1;

				vec3f N(0, 0, 0);
				N[(axis + 1) % 3] = 1;
				i.N = N;

				i.t = t1;
			}
		}
		else {
			t[axis * 2] = t1;
			t[axis * 2 + 1] = t0;

			if (t0 > 0 && t0 < minT) {
				minT = t0;

				vec3f N(0, 0, 0);
				N[(axis + 1) % 3] = -1;
				i.N = N;

				i.t = t0;
			}
		}
	}

	// check if closet intersection is less than the farthest intersect
	// in the other two dimensions
	for (int axis = 0; axis < 3; ++axis) {
		if (t[axis * 2 + 1] > t[((axis + 1) * 2) % 6] ||
			t[axis * 2 + 1] > t[((axis + 2) * 2) % 6]) {
			return false;
		}
	}

	return true;
>>>>>>> a04a7da62da8c3773d454dea03960057a28aa55a
}
