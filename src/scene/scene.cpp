#define NOMINMAX
#include <cmath>
#include <algorithm>
#include "scene.h"
#include "light.h"
#include "../ui/TraceUI.h"
#include <array>
#include <cassert>
extern TraceUI* traceUI;

namespace octTree
{
	//reference : http://chiranjivi.tripod.com/octrav.html
	const int cap = 2;
	std::unique_ptr<octree> root = nullptr;

	void octree::split()
	{
		const auto& max = bound.max;
		const auto& min = bound.min;

		isLeaf = false;
		auto mid = (max + min) / 2;
		child[0] = make_unique<octree>(min[0], min[1], min[2]
		                               , mid[0], mid[1], mid[2]);
		child[1] = make_unique<octree>(min[0], min[1], mid[2]
		                               , mid[0], mid[1], max[2]);
		child[2] = make_unique<octree>(min[0], mid[1], min[2]
		                               , mid[0], max[1], mid[2]);
		child[3] = make_unique<octree>(min[0], mid[1], mid[2]
		                               , mid[0], max[1], max[2]);

		child[4] = make_unique<octree>(mid[0], min[1], min[2]
		                               , max[0], mid[1], mid[2]);
		child[5] = make_unique<octree>(mid[0], min[1], mid[2]
		                               , max[0], mid[1], max[2]);
		child[6] = make_unique<octree>(mid[0], mid[1], min[2]
		                               , max[0], max[1], mid[2]);
		child[7] = make_unique<octree>(mid[0], mid[1], mid[2]
		                               , max[0], max[1], max[2]);
		auto tempGeometries = std::move(Geometries);
		Geometries.clear();
		for (auto G : tempGeometries)
		{
			insert(this, G);
		}
	}


	void octree::insert(octree* cur_ptr, Geometry* const& obj)
	{
		if (cur_ptr->isLeaf)
		{
			//just adding, no problem
			if (cur_ptr->Geometries.size() < cap)
			{
				cur_ptr->Geometries.push_back(obj);
				return;
			}
			//split
			{
				cur_ptr->split();
			}
		}

		const auto& max = cur_ptr->bound.max;
		const auto& min = cur_ptr->bound.min;

		auto objBound = obj->getBoundingBox();
		auto min_rate = (objBound.min - min);
		auto max_rate = (objBound.max - min);
		auto range = (max - min);
		min_rate[0] /= range[0];
		min_rate[1] /= range[1];
		min_rate[2] /= range[2];

		max_rate[0] /= range[0];
		max_rate[1] /= range[1];
		max_rate[2] /= range[2];
		if (min_rate[0] <= 0.5 && max_rate[0] >= 0.5
			|| min_rate[1] <= 0.5 && max_rate[1] >= 0.5
			|| min_rate[2] <= 0.5 && max_rate[2] >= 0.5)
		{
			cur_ptr->Geometries.push_back(obj);
			return;
		}
		int to_add = 0;
		if (max_rate[0] >= 0.5)
		{
			to_add |= 4;
		}
		if (max_rate[2] >= 0.5)
		{
			to_add |= 2;
		}
		if (max_rate[3] >= 0.5)
		{
			to_add |= 1;
		}
		insert(cur_ptr->child[to_add].get(), obj);
		return;
	}

	void constrictOctTree(list<Geometry*> objs, vec3f min, vec3f max)
	{
		static auto num_init = 0;
		if (!root)
		{
			assert(num_init++ < 1);
			root = make_unique<octree>(min, max);
		}
		for (const auto& obj : objs)
		{
			octree::insert(root.get(), obj);
		}
	}


	list<octree*> ray_step(octree* cur_ptr, const ray& r)
	{
		list<octree*> to_return;
		if (cur_ptr == nullptr)
		{
			assert(0);
			assert(1);
		}
		double tmin, tmax;
		if (cur_ptr->bound.intersect(r, tmin, tmax))
		{
			to_return.push_back(cur_ptr);
			if (!(cur_ptr->isLeaf))
			{
				for (auto i = 0; i < 8; ++i)
				{
					auto b = ray_step(cur_ptr->child[i].get(), r);;
					to_return.insert(to_return.end(), b.begin(), b.end());
				}
			}
		}
		return to_return;
	}

	// void proc_subtree(const float& tx0, const float& ty0, const float& tz0,
	//                   const float& tx1, const float& ty1, const float& tz1,
	//                   octree* n);
	//
	// void ray_step(octree* root, ray r, const vec3f& min, const vec3f& max)
	// {
	// 	a = 0;
	// 	auto dir = r.getDirection();
	// 	auto& dx = dir[0];
	// 	auto& dy = dir[1];
	// 	auto& dz = dir[2];
	// 	auto pos = r.getPosition();
	// 	auto size = max - min;
	//
	// 	if (dx < 0)
	// 	{
	// 		pos[0] = size[0] - pos[0];
	// 		dx = -dx;
	// 		a |= 4;
	// 	}
	// 	if (dy < 0)
	// 	{
	// 		pos[1] = size[1] - pos[1];
	// 		dy = -dy;
	// 		a |= 2;
	// 	}
	// 	if (dz < 0)
	// 	{
	// 		pos[2] = size[2] - pos[2];
	// 		dz = -dz;
	// 		a |= 1;
	// 	}
	//
	// 	const auto tx0 = (min[0] - pos[0]) / dx;
	// 	const auto tx1 = (max[0] - pos[0]) / dx;
	// 	const auto ty0 = (min[1] - pos[1]) / dy;
	// 	const auto ty1 = (max[1] - pos[1]) / dy;
	// 	const auto tz0 = (min[2] - pos[2]) / dz;
	// 	const auto tz1 = (max[2] - pos[2]) / dz;
	//
	// 	const float tmin = std::max(std::max(tx0, ty0), tz0);
	// 	const float tmax = std::min(std::min(tx1, ty1), tz1);
	//
	// 	if ((tmin < tmax) && (tmax > 0.0f))
	// 		proc_subtree(tx0, ty0, tz0, tx1, ty1, tz1, root);
	// }
	//
	// inline int find_firstNode(const float& tx0, const float& ty0, const float& tz0,
	//                           const float& txM, const float& tyM, const float& tzM)
	// {
	// 	int to_return = 0;
	// 	if (txM < tz0)
	// 	{
	// 		to_return |= 4;
	// 	}
	// 	if (tyM < tz0)
	// 	{
	// 		to_return |= 2;
	// 	}
	//
	// 	if (tyM < tx0)
	// 	{
	// 		to_return |= 2;
	// 	}
	// 	if (tzM < tx0)
	// 	{
	// 		to_return |= 1;
	// 	}
	//
	// 	if (txM < ty0)
	// 	{
	// 		to_return |= 4;
	// 	}
	// 	if (tzM < ty0)
	// 	{
	// 		to_return |= 1;
	// 	}
	//
	// 	return to_return;
	// }
	//
	// inline int next_Node(float a, float b, float c, int f, int s, int t)
	// {
	// 	if (a >= b && a >= c)
	// 	{
	// 		return f;
	// 	}
	// 	if (b >= c)
	// 	{
	// 		return s;
	// 	}
	// 	return t;
	// }
	//
	// void proc_subtree(const float& tx0, const float& ty0, const float& tz0,
	//                   const float& tx1, const float& ty1, const float& tz1,
	//                   octree* n)
	// {
	// 	int currNode;
	//
	// 	if ((tx1 <= 0.0f) || (ty1 <= 0.0f) || (tz1 <= 0.0f))
	// 		return;
	//
	// 	if (n->isLeaf)
	// 	{
	// 		addToList(n);
	// 		return;
	// 	}
	//
	// 	float txM = 0.5 * (tx0 + tx1);
	// 	float tyM = 0.5 * (ty0 + ty1);
	// 	float tzM = 0.5 * (tz0 + tz1);
	//
	// 	// Determining the first node requires knowing which of the t0's is the largest...
	// 	// as well as comparing the tM's of the other axes against that largest t0.
	// 	// Hence, the function should only require the 3 t0-values and the 3 tM-values.
	// 	currNode = find_firstNode(tx0, ty0, tz0, txM, tyM, tzM);
	//
	// 	do
	// 	{
	// 		// next_Node() takes the t1 values for a child (which may or may not have tM's of the parent)
	// 		// and determines the next node.  Rather than passing in the currNode value, we pass in possible values
	// 		// for the next node.  A value of 8 refers to an exit from the parent.
	// 		// While having more parameters does use more stack bandwidth, it allows for a smaller function
	// 		// with fewer branches and less redundant code.  The possibilities for the next node are passed in
	// 		// the same respective order as the t-values.  Hence if the first parameter is found as the greatest, the
	// 		// fourth parameter will be the return value.  If the 2nd parameter is the greatest, the 5th will be returned, etc.
	// 		switch (currNode)
	// 		{
	// 		case 0: proc_subtree(tx0, ty0, tz0, txM, tyM, tzM, n->child[a].get());
	// 			currNode = next_Node(txM, tyM, tzM, 4, 2, 1);
	// 			break;
	// 		case 1: proc_subtree(tx0, ty0, tzM, txM, tyM, tz1, n->child[1 ^ a].get());
	// 			currNode = next_Node(txM, tyM, tz1, 5, 3, 8);
	// 			break;
	// 		case 2: proc_subtree(tx0, tyM, tz0, txM, ty1, tzM, n->child[2 ^ a].get());
	// 			currNode = next_Node(txM, ty1, tzM, 6, 8, 3);
	// 			break;
	// 		case 3: proc_subtree(tx0, tyM, tzM, txM, ty1, tz1, n->child[3 ^ a].get());
	// 			currNode = next_Node(txM, ty1, tz1, 7, 8, 8);
	// 			break;
	// 		case 4: proc_subtree(txM, ty0, tz0, tx1, tyM, tzM, n->child[4 ^ a].get());
	// 			currNode = next_Node(tx1, tyM, tzM, 8, 6, 5);
	// 			break;
	// 		case 5: proc_subtree(txM, ty0, tzM, tx1, tyM, tz1, n->child[5 ^ a].get());
	// 			currNode = next_Node(tx1, tyM, tz1, 8, 7, 8);
	// 			break;
	// 		case 6: proc_subtree(txM, tyM, tz0, tx1, ty1, tzM, n->child[6 ^ a].get());
	// 			currNode = next_Node(tx1, ty1, tzM, 8, 8, 7);
	// 			break;
	// 		case 7: proc_subtree(txM, txM, tzM, tx1, ty1, tz1, n->child[7].get());
	// 			currNode = 8;
	// 			break;
	// 		}
	// 	}
	// 	while (currNode < 8);
	// }
}


void BoundingBox::operator=(const BoundingBox& target)
{
	min = target.min;
	max = target.max;
}

// Does this bounding box intersect the target?
bool BoundingBox::intersects(const BoundingBox& target) const
{
	return ((target.min[0] - RAY_EPSILON <= max[0]) && (target.max[0] + RAY_EPSILON >= min[0]) &&
		(target.min[1] - RAY_EPSILON <= max[1]) && (target.max[1] + RAY_EPSILON >= min[1]) &&
		(target.min[2] - RAY_EPSILON <= max[2]) && (target.max[2] + RAY_EPSILON >= min[2]));
}

// does the box contain this point?
bool BoundingBox::intersects(const vec3f& point) const
{
	return ((point[0] + RAY_EPSILON >= min[0]) && (point[1] + RAY_EPSILON >= min[1]) && (point[2] + RAY_EPSILON >= min[2
		]) &&
		(point[0] - RAY_EPSILON <= max[0]) && (point[1] - RAY_EPSILON <= max[1]) && (point[2] - RAY_EPSILON <= max[2]));
}

// if the ray hits the box, put the "t" value of the intersection
// closest to the origin in tMin and the "t" value of the far intersection
// in tMax and return true, else return false.
// Using Kay/Kajiya algorithm.
bool BoundingBox::intersect(const ray& r, double& tMin, double& tMax) const
{
	vec3f R0 = r.getPosition();
	vec3f Rd = r.getDirection();

	tMin = -1.0e308; // 1.0e308 is close to infinity... close enough for us!
	tMax = 1.0e308;
	double ttemp;

	for (int currentaxis = 0; currentaxis < 3; currentaxis++)
	{
		double vd = Rd[currentaxis];

		// if the ray is parallel to the face's plane (=0.0)
		if (vd == 0.0)
			continue;

		double v1 = min[currentaxis] - R0[currentaxis];
		double v2 = max[currentaxis] - R0[currentaxis];

		// two slab intersections
		double t1 = v1 / vd;
		double t2 = v2 / vd;

		if (t1 > t2)
		{
			// swap t1 & t2
			ttemp = t1;
			t1 = t2;
			t2 = ttemp;
		}

		if (t1 > tMin)
			tMin = t1;
		if (t2 < tMax)
			tMax = t2;

		if (tMin > tMax) // box is missed
			return false;
		if (tMax < 0.0) // box is behind ray
			return false;
	}
	return true; // it made it past all 3 axes.
}


bool Geometry::intersect(const ray& r, isect& i) const
{
	// Transform the ray into the object's local coordinate space
	vec3f pos = transform->globalToLocalCoords(r.getPosition());
	vec3f dir = transform->globalToLocalCoords(r.getPosition() + r.getDirection()) - pos;
	double length = dir.length();
	dir /= length;

	ray localRay(pos, dir);

	if (intersectLocal(localRay, i))
	{
		// Transform the intersection point & normal returned back into global space.
		i.N = transform->localToGlobalCoordsNormal(i.N);
		i.t /= length;

		return true;
	}
	else
	{
		return false;
	}
}

bool Geometry::intersectLocal(const ray& r, isect& i) const
{
	return false;
}

bool Geometry::hasBoundingBoxCapability() const
{
	// by default, primitives do not have to specify a bounding box.
	// If this method returns true for a primitive, then either the ComputeBoundingBox() or
	// the ComputeLocalBoundingBox() method must be implemented.

	// If no bounding box capability is supported for an object, that object will
	// be checked against every single ray drawn.  This should be avoided whenever possible,
	// but this possibility exists so that new primitives will not have to have bounding
	// boxes implemented for them.

	return false;
}

Scene::~Scene()
{
	giter g;
	liter l;

	for (g = objects.begin(); g != objects.end(); ++g)
	{
		delete (*g);
	}

	for (g = boundedobjects.begin(); g != boundedobjects.end(); ++g)
	{
		delete (*g);
	}

	for (g = nonboundedobjects.begin(); g != boundedobjects.end(); ++g)
	{
		delete (*g);
	}

	for (l = lights.begin(); l != lights.end(); ++l)
	{
		delete (*l);
	}
}

// Get any intersection with an object.  Return information about the 
// intersection through the reference parameter.
bool Scene::intersect(const ray& r, isect& i) const
{
	typedef list<Geometry*>::const_iterator iter;
	iter j;

	isect cur;
	bool have_one = false;

	auto traverse_tree = octTree::ray_step(octTree::root.get(), r);

	for (list<octTree::octree*>::const_iterator node = traverse_tree.begin(); node !=
	     traverse_tree.end(); ++node)
	{
		for (j = (*node)->Geometries.begin(); j != (*node)->Geometries.end(); ++j)
		{
			if ((*j)->intersect(r, cur))
			{
				if (!have_one || (cur.t < i.t))
				{
					i = cur;
					have_one = true;
				}
			}
		}
	}

	// try the non-bounded objects
	for (j = nonboundedobjects.begin(); j != nonboundedobjects.end(); ++j)
	{
		if ((*j)->intersect(r, cur))
		{
			if (!have_one || (cur.t < i.t))
			{
				i = cur;
				have_one = true;
			}
		}
	}

	return have_one;
}

void Scene::initScene()
{
	bool first_boundedobject = true;
	BoundingBox b;

	typedef list<Geometry*>::const_iterator iter;
	// split the objects into two categories: bounded and non-bounded
	for (iter j = objects.begin(); j != objects.end(); ++j)
	{
		if ((*j)->hasBoundingBoxCapability())
		{
			boundedobjects.push_back(*j);

			// widen the scene's bounding box, if necessary
			if (first_boundedobject)
			{
				sceneBounds = (*j)->getBoundingBox();
				first_boundedobject = false;
			}
			else
			{
				b = (*j)->getBoundingBox();
				sceneBounds.max = maximum(sceneBounds.max, b.max);
				sceneBounds.min = minimum(sceneBounds.min, b.min);
			}
		}
		else
			nonboundedobjects.push_back(*j);
	}

	octTree::constrictOctTree(boundedobjects, sceneBounds.min, sceneBounds.max);
}
