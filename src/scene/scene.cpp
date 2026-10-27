#define NOMINMAX
#include <cmath>
#include <algorithm>
#include "scene.h"
#include "light.h"

RenderSettings g_render;

namespace accel
{
	const int kLeaf = 4;
	const int kMaxDepth = 24;

	struct Node
	{
		BoundingBox bound;
		int left = -1;
		int right = -1;
		int first = 0;
		int count = 0;
	};

	std::vector<Node> nodes;
	std::vector<Geometry*> prims;

	void clear()
	{
		nodes.clear();
		prims.clear();
	}

	int longestAxis(const BoundingBox& b)
	{
		const vec3f e = b.max - b.min;
		if (e[0] >= e[1] && e[0] >= e[2])
			return 0;
		if (e[1] >= e[2])
			return 1;
		return 2;
	}

	int buildRange(int start, int end, int depth)
	{
		Node node;
		node.bound = prims[start]->getBoundingBox();
		for (int i = start + 1; i < end; ++i)
		{
			const BoundingBox& b = prims[i]->getBoundingBox();
			node.bound.min = minimum(node.bound.min, b.min);
			node.bound.max = maximum(node.bound.max, b.max);
		}

		const int count = end - start;
		if (count <= kLeaf || depth >= kMaxDepth)
		{
			node.first = start;
			node.count = count;
			nodes.push_back(node);
			return static_cast<int>(nodes.size()) - 1;
		}

		const int axis = longestAxis(node.bound);
		int mid = (start + end) / 2;
		std::nth_element(prims.begin() + start, prims.begin() + mid, prims.begin() + end,
			[axis](Geometry* ga, Geometry* gb)
			{
				const BoundingBox& ba = ga->getBoundingBox();
				const BoundingBox& bb = gb->getBoundingBox();
				return (ba.min[axis] + ba.max[axis]) < (bb.min[axis] + bb.max[axis]);
			});
		if (mid <= start)
			mid = start + 1;
		if (mid >= end)
			mid = end - 1;

		nodes.push_back(Node());
		const int idx = static_cast<int>(nodes.size()) - 1;
		const int L = buildRange(start, mid, depth + 1);
		const int R = buildRange(mid, end, depth + 1);
		nodes[idx].left = L;
		nodes[idx].right = R;
		nodes[idx].count = 0;
		nodes[idx].bound.min = minimum(nodes[L].bound.min, nodes[R].bound.min);
		nodes[idx].bound.max = maximum(nodes[L].bound.max, nodes[R].bound.max);
		return idx;
	}

	void build(const std::vector<Geometry*>& objs)
	{
		clear();
		if (objs.empty())
			return;
		prims = objs;
		buildRange(0, static_cast<int>(prims.size()), 0);
	}

	void intersectClosest(const ray& r, isect& closest, bool& have_one, double maxT)
	{
		if (nodes.empty())
			return;

		int stack[64];
		int sp = 0;
		stack[sp++] = 0;

		while (sp)
		{
			const Node& n = nodes[stack[--sp]];
			double tmin, tmax;
			if (!n.bound.intersect(r, tmin, tmax) || tmin > maxT)
				continue;
			if (have_one && tmin >= closest.t)
				continue;

			if (n.count > 0)
			{
				for (int i = 0; i < n.count; ++i)
				{
					isect cur;
					if (prims[n.first + i]->intersect(r, cur, maxT) && (!have_one || cur.t < closest.t))
					{
						closest = cur;
						have_one = true;
						if (closest.t < maxT)
							maxT = closest.t;
					}
				}
			}
			else
			{
				double t0, T0, t1, T1;
				const bool h0 = n.left >= 0 && nodes[n.left].bound.intersect(r, t0, T0)
					&& t0 <= maxT && !(have_one && t0 >= closest.t);
				const bool h1 = n.right >= 0 && nodes[n.right].bound.intersect(r, t1, T1)
					&& t1 <= maxT && !(have_one && t1 >= closest.t);
				if (h0 && h1)
				{
					if (t0 > t1)
					{
						stack[sp++] = n.left;
						stack[sp++] = n.right;
					}
					else
					{
						stack[sp++] = n.right;
						stack[sp++] = n.left;
					}
				}
				else if (h0)
					stack[sp++] = n.left;
				else if (h1)
					stack[sp++] = n.right;
			}
		}
	}

	bool intersectAny(const ray& r, isect& hit, double maxT)
	{
		if (nodes.empty())
			return false;

		int stack[64];
		int sp = 0;
		stack[sp++] = 0;

		while (sp)
		{
			const Node& n = nodes[stack[--sp]];
			double tmin, tmax;
			if (!n.bound.intersect(r, tmin, tmax) || tmin > maxT)
				continue;

			if (n.count > 0)
			{
				for (int i = 0; i < n.count; ++i)
				{
					isect cur;
					if (prims[n.first + i]->intersect(r, cur, maxT) && cur.t <= maxT)
					{
						hit = cur;
						return true;
					}
				}
			}
			else
			{
				if (n.left >= 0)
					stack[sp++] = n.left;
				if (n.right >= 0)
					stack[sp++] = n.right;
			}
		}
		return false;
	}
}

namespace octTree
{
	const int cap = 64;
	const int maxDepth = 12;
	std::unique_ptr<Octree> root = nullptr;

	void Octree::split()
	{
		const auto& max = bound.max;
		const auto& min = bound.min;

		isLeaf = false;
		const auto mid = (max + min) / 2;
		const int childDepth = depth + 1;
		child[0] = make_unique<Octree>(min[0], min[1], min[2],
			mid[0], mid[1], mid[2], childDepth);
		child[1] = make_unique<Octree>(min[0], min[1], mid[2],
			mid[0], mid[1], max[2], childDepth);
		child[2] = make_unique<Octree>(min[0], mid[1], min[2],
			mid[0], max[1], mid[2], childDepth);
		child[3] = make_unique<Octree>(min[0], mid[1], mid[2],
			mid[0], max[1], max[2], childDepth);
		child[4] = make_unique<Octree>(mid[0], min[1], min[2],
			max[0], mid[1], mid[2], childDepth);
		child[5] = make_unique<Octree>(mid[0], min[1], mid[2],
			max[0], mid[1], max[2], childDepth);
		child[6] = make_unique<Octree>(mid[0], mid[1], min[2],
			max[0], max[1], mid[2], childDepth);
		child[7] = make_unique<Octree>(mid[0], mid[1], mid[2],
			max[0], max[1], max[2], childDepth);
		std::vector<Geometry*> tempGeometries = std::move(Geometries);
		Geometries.clear();
		for (Geometry* G : tempGeometries)
			insert(this, G);
	}

	void Octree::insert(Octree* cur_ptr, Geometry* const& obj)
	{
		if (cur_ptr->isLeaf)
		{
			if (cur_ptr->Geometries.size() < cap || cur_ptr->depth >= maxDepth)
			{
				cur_ptr->Geometries.push_back(obj);
				return;
			}
			cur_ptr->split();
		}

		const auto& max = cur_ptr->bound.max;
		const auto& min = cur_ptr->bound.min;

		auto objBound = obj->getBoundingBox();
		auto min_rate = (objBound.min - min);
		auto max_rate = (objBound.max - min);
		auto range = (max - min);
		if (range[0] < RAY_EPSILON || range[1] < RAY_EPSILON || range[2] < RAY_EPSILON)
		{
			cur_ptr->Geometries.push_back(obj);
			return;
		}
		min_rate[0] /= range[0];
		min_rate[1] /= range[1];
		min_rate[2] /= range[2];
		max_rate[0] /= range[0];
		max_rate[1] /= range[1];
		max_rate[2] /= range[2];
		if ((min_rate[0] <= 0.5 && max_rate[0] >= 0.5)
			|| (min_rate[1] <= 0.5 && max_rate[1] >= 0.5)
			|| (min_rate[2] <= 0.5 && max_rate[2] >= 0.5))
		{
			cur_ptr->Geometries.push_back(obj);
			return;
		}
		int to_add = 0;
		if (min_rate[0] >= 0.5)
			to_add |= 4;
		if (min_rate[1] >= 0.5)
			to_add |= 2;
		if (min_rate[2] >= 0.5)
			to_add |= 1;
		if (!cur_ptr->child[to_add])
		{
			cur_ptr->Geometries.push_back(obj);
			return;
		}
		insert(cur_ptr->child[to_add].get(), obj);
	}

	void constrictOctTree(const std::vector<Geometry*>& objs, const vec3f& min, const vec3f& max)
	{
		root.reset();
		if (objs.empty())
			return;
		root = make_unique<Octree>(min, max);
		for (Geometry* obj : objs)
			Octree::insert(root.get(), obj);
	}

	void intersectTree(Octree* node, const ray& r, isect& closest, bool& have_one, double maxT, bool anyHit)
	{
		if (!node)
			return;

		double tmin, tmax;
		if (!node->bound.intersect(r, tmin, tmax) || tmin > maxT)
			return;
		if (have_one && tmin >= closest.t)
			return;

		for (Geometry* geometry : node->Geometries)
		{
			isect cur;
			if (geometry->intersect(r, cur, maxT) && (!have_one || cur.t < closest.t))
			{
				closest = cur;
				have_one = true;
				if (anyHit)
					return;
				if (closest.t < maxT)
					maxT = closest.t;
			}
		}

		if (!node->isLeaf)
		{
			for (int i = 0; i < 8; ++i)
			{
				intersectTree(node->child[i].get(), r, closest, have_one, maxT, anyHit);
				if (anyHit && have_one)
					return;
			}
		}
	}
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
	const vec3f& R0 = r.getPosition();
	const vec3f& Rd = r.getDirection();
	const vec3f& inv = r.getInvDirection();

	tMin = -1.0e308;
	tMax = 1.0e308;

	for (int currentaxis = 0; currentaxis < 3; currentaxis++)
	{
		const double vd = Rd[currentaxis];

		if (vd == 0.0)
		{
			if (R0[currentaxis] < min[currentaxis] - RAY_EPSILON ||
				R0[currentaxis] > max[currentaxis] + RAY_EPSILON)
				return false;
			continue;
		}

		double t1 = ((min[currentaxis] - RAY_EPSILON) - R0[currentaxis]) * inv[currentaxis];
		double t2 = ((max[currentaxis] + RAY_EPSILON) - R0[currentaxis]) * inv[currentaxis];
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

		if (tMin > tMax + RAY_EPSILON)
			return false;
		if (tMax < -RAY_EPSILON)
			return false;
	}
	return true;
}


bool Geometry::intersect(const ray& r, isect& i, double maxT) const
{
	if (hasBoundingBoxCapability())
	{
		double tmin, tmax;
		if (!bounds.intersect(r, tmin, tmax) || tmin > maxT)
			return false;
	}

	if (identityTransform || worldSpace)
	{
		if (!intersectLocal(r, i))
			return false;
		return i.t <= maxT;
	}

	const vec3f pos = transform->globalToLocalCoords(r.getPosition());
	vec3f dir = transform->globalToLocalCoords(r.getPosition() + r.getDirection()) - pos;
	const double length = dir.length();
	if (length < RAY_EPSILON)
		return false;
	dir /= length;

	ray localRay(pos, dir);
	if (!intersectLocal(localRay, i))
		return false;

	i.N = transform->localToGlobalCoordsNormal(i.N);
	i.t /= length;
	return i.t <= maxT;
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
	accel::clear();

	for (Geometry* g : objects) {
		delete g;
	}
	objects.clear();
	boundedobjects.clear();
	nonboundedobjects.clear();
	for (Light* l : lights) {
		delete l;
	}
	lights.clear();
}

bool Scene::intersect(const ray& r, isect& i, double maxT, bool anyHit) const
{
	bool have_one = false;

	if (g_render.accel == AccelBVH && !accel::nodes.empty())
	{
		if (anyHit)
		{
			have_one = accel::intersectAny(r, i, maxT);
			if (have_one && nonboundedobjects.empty())
				return true;
		}
		else
		{
			accel::intersectClosest(r, i, have_one, maxT);
			if (have_one)
				maxT = i.t;
		}
	}
	else if (g_render.accel == AccelOctree && octTree::root)
	{
		octTree::intersectTree(octTree::root.get(), r, i, have_one, maxT, anyHit);
		if (have_one)
		{
			maxT = i.t;
			if (anyHit && nonboundedobjects.empty())
				return true;
		}
	}
	else
	{
		for (Geometry* geometry : boundedobjects)
		{
			isect cur;
			if (geometry->intersect(r, cur, maxT) && (!have_one || cur.t < i.t))
			{
				i = cur;
				have_one = true;
				if (anyHit)
					return true;
				maxT = i.t;
			}
		}
	}

	for (Geometry* geometry : nonboundedobjects)
	{
		isect cur;
		if (geometry->intersect(r, cur, maxT) && (!have_one || cur.t < i.t))
		{
			i = cur;
			have_one = true;
			if (anyHit)
				return true;
			maxT = i.t;
		}
	}

	return have_one;
}

void Scene::initScene()
{
	boundedobjects.clear();
	nonboundedobjects.clear();
	hasTransparent = false;
	bool first_boundedobject = true;

	for (Geometry* object : objects)
	{
		object->prepareForTrace();

		if (object->isTransparent())
			hasTransparent = true;

		if (!object->isIntersectable())
			continue;

		if (object->hasBoundingBoxCapability())
		{
			boundedobjects.push_back(object);

			if (first_boundedobject)
			{
				sceneBounds = object->getBoundingBox();
				first_boundedobject = false;
			}
			else
			{
				const BoundingBox& b = object->getBoundingBox();
				sceneBounds.max = maximum(sceneBounds.max, b.max);
				sceneBounds.min = minimum(sceneBounds.min, b.min);
			}
		}
		else {
			nonboundedobjects.push_back(object);
		}
	}

	if (!boundedobjects.empty())
	{
		accel::build(boundedobjects);
		octTree::constrictOctTree(boundedobjects, sceneBounds.min, sceneBounds.max);
	}
	else
	{
		accel::clear();
		octTree::root.reset();
	}
}
