//
// scene.h
//
// The Scene class and the geometric types that it can contain.
//
#define  NOMINMAX

#ifndef __SCENE_H__
#define __SCENE_H__

#include <list>
#include <algorithm>
#include <memory>
#include <vector>
#include <cmath>

using namespace std;

#include "ray.h"
#include "material.h"
#include "camera.h"
#include "../vecmath/vecmath.h"

class Light;
class Scene;

class SceneElement
{
public:
	virtual ~SceneElement()
	{
	}

	Scene* getScene() const { return scene; }

protected:
	SceneElement(Scene* s)
		: scene(s)
	{
	}

	Scene* scene;
};

class BoundingBox
{
public:
	vec3f min;
	vec3f max;

	void operator=(const BoundingBox& target);

	// Does this bounding box intersect the target?
	bool intersects(const BoundingBox& target) const;

	// does the box contain this point?
	bool intersects(const vec3f& point) const;

	// if the ray hits the box, put the "t" value of the intersection
	// closest to the origin in tMin and the "t" value of the far intersection
	// in tMax and return true, else return false.
	bool intersect(const ray& r, double& tMin, double& tMax) const;
};

class TransformNode
{
protected:

	// information about this node's transformation
	mat4f xform;
	mat4f inverse;
	mat3f normi;

	// information about parent & children
	TransformNode* parent;
	list<TransformNode*> children;

public:
	typedef list<TransformNode*>::iterator child_iter;
	typedef list<TransformNode*>::const_iterator child_citer;

	~TransformNode()
	{
		for (child_iter c = children.begin(); c != children.end(); ++c)
			delete (*c);
	}

	TransformNode* createChild(const mat4f& xform)
	{
		TransformNode* child = new TransformNode(this, xform);
		children.push_back(child);
		return child;
	}

	bool isIdentity() const { return identity; }

	// Coordinate-Space transformation
	vec3f globalToLocalCoords(const vec3f& v) const
	{
		return inverse * v;
	}

	vec3f localToGlobalCoords(const vec3f& v) const
	{
		return xform * v;
	}

	vec4f localToGlobalCoords(const vec4f& v) const
	{
		return xform * v;
	}

	vec3f globalToLocalDirection(const vec3f& d) const
	{
		return vec3f(
			inverse[0][0] * d[0] + inverse[0][1] * d[1] + inverse[0][2] * d[2],
			inverse[1][0] * d[0] + inverse[1][1] * d[1] + inverse[1][2] * d[2],
			inverse[2][0] * d[0] + inverse[2][1] * d[1] + inverse[2][2] * d[2]);
	}

	vec3f localToGlobalCoordsNormal(const vec3f& v) const
	{
		return (normi * v).normalize();
	}

protected:
	bool identity;

	static bool matrixIsIdentity(const mat4f& m)
	{
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				const double expect = (i == j) ? 1.0 : 0.0;
				if (std::fabs(m[i][j] - expect) > 1e-12)
					return false;
			}
		}
		return true;
	}

	// protected so that users can't directly construct one of these...
	// force them to use the createChild() method.  Note that they CAN
	// directly create a TransformRoot object.
	TransformNode(TransformNode* parent, const mat4f& xform)
		: children(), identity(false)
	{
		this->parent = parent;
		if (parent == NULL)
			this->xform = xform;
		else
			this->xform = parent->xform * xform;

		inverse = this->xform.inverse();
		normi = this->xform.upper33().inverse().transpose();
		identity = matrixIsIdentity(this->xform);
	}
};

class TransformRoot : public TransformNode
{
public:
	TransformRoot()
		: TransformNode(NULL, mat4f())
	{
	}
};

// A Geometry object is anything that has extent in three dimensions.
// It may not be an actual visible scene object.  For example, hierarchical
// spatial subdivision could be expressed in terms of Geometry instances.
class Geometry
	: public SceneElement
{
public:
	// intersections performed in the global coordinate space.
	virtual bool intersect(const ray& r, isect& i, double maxT = 1.0e308) const;

	// intersections performed in the object's local coordinate space
	// do not call directly - this should only be called by intersect()
	virtual bool intersectLocal(const ray& r, isect& i) const;


	virtual bool hasBoundingBoxCapability() const;
	virtual bool isTransparent() const { return false; }
	const BoundingBox& getBoundingBox() const { return bounds; }

	virtual void ComputeBoundingBox()
	{
		// take the object's local bounding box, transform all 8 points on it,
		// and use those to find a new bounding box.

		BoundingBox localBounds = ComputeLocalBoundingBox();

		vec3f min = localBounds.min;
		vec3f max = localBounds.max;

		vec4f v, newMax, newMin;

		v = transform->localToGlobalCoords(vec4f(min[0], min[1], min[2], 1));
		newMax = v;
		newMin = v;
		v = transform->localToGlobalCoords(vec4f(max[0], min[1], min[2], 1));
		newMax = maximum(newMax, v);
		newMin = minimum(newMin, v);
		v = transform->localToGlobalCoords(vec4f(min[0], max[1], min[2], 1));
		newMax = maximum(newMax, v);
		newMin = minimum(newMin, v);
		v = transform->localToGlobalCoords(vec4f(max[0], max[1], min[2], 1));
		newMax = maximum(newMax, v);
		newMin = minimum(newMin, v);
		v = transform->localToGlobalCoords(vec4f(min[0], min[1], max[2], 1));
		newMax = maximum(newMax, v);
		newMin = minimum(newMin, v);
		v = transform->localToGlobalCoords(vec4f(max[0], min[1], max[2], 1));
		newMax = maximum(newMax, v);
		newMin = minimum(newMin, v);
		v = transform->localToGlobalCoords(vec4f(min[0], max[1], max[2], 1));
		newMax = maximum(newMax, v);
		newMin = minimum(newMin, v);
		v = transform->localToGlobalCoords(vec4f(max[0], max[1], max[2], 1));
		newMax = maximum(newMax, v);
		newMin = minimum(newMin, v);

		bounds.max = vec3f(newMax);
		bounds.min = vec3f(newMin);
		identityTransform = transform && transform->isIdentity();
	}

	// default method for ComputeLocalBoundingBox returns a bogus bounding box;
	// this should be overridden if hasBoundingBoxCapability() is true.
	virtual BoundingBox ComputeLocalBoundingBox() { return BoundingBox(); }

	void setTransform(TransformNode* transform) { this->transform = transform; };

	Geometry(Scene* scene)
		: SceneElement(scene), identityTransform(false), worldSpace(false), transform(nullptr)
	{
	}

protected:
	BoundingBox bounds;
	bool identityTransform;
	bool worldSpace;
	TransformNode* transform;
};

// A SceneObject is a real actual thing that we want to model in the 
// world.  It has extent (its Geometry heritage) and surface properties
// (its material binding).  The decision of how to store that material
// is left up to the subclass.
class SceneObject
	: public Geometry
{
public:
	virtual const Material& getMaterial() const = 0;
	virtual void setMaterial(Material* m) = 0;
	bool isBox = false;
protected:
	SceneObject(Scene* scene)
		: Geometry(scene)
	{
	}
};

// A simple extension of SceneObject that adds an instance of Material
// for simple material bindings.
class MaterialSceneObject
	: public SceneObject
{
public:
	virtual ~MaterialSceneObject() { if (material) delete material; }

	virtual const Material& getMaterial() const { return *material; }
	virtual void setMaterial(Material* m) { material = m; }
	virtual bool isTransparent() const { return material && !material->kt.iszero(); }

protected:
	MaterialSceneObject(Scene* scene, Material* mat)
		: SceneObject(scene), material(mat)
	{
	}

	//	MaterialSceneObject( Scene *scene ) 
	//	: SceneObject( scene ), material( new Material ) {}

	Material* material;
};

class Scene
{
public:
	typedef vector<Light*>::iterator liter;
	typedef vector<Light*>::const_iterator cliter;

	typedef list<Geometry*>::iterator giter;
	typedef list<Geometry*>::const_iterator cgiter;

	TransformRoot transformRoot;

public:
	Scene()
		: transformRoot(), objects(), lights(), hasTransparent(false)
	{
	}

	virtual ~Scene();

	void add(Geometry* obj)
	{
		obj->ComputeBoundingBox();
		objects.push_back(obj);
	}

	void add(Light* light)
	{
		lights.push_back(light);
	}

	bool intersect(const ray& r, isect& i, double maxT = 1.0e308, bool anyHit = false) const;
	void initScene();
	bool hasTransparentObjects() const { return hasTransparent; }

	const vector<Light*>& getLights() const { return lights; }
	vector<Light*>::const_iterator beginLights() const { return lights.begin(); }
	vector<Light*>::const_iterator endLights() const { return lights.end(); }

	Camera* getCamera() { return &camera; }

private:
	list<Geometry*> objects;
	vector<Geometry*> nonboundedobjects;
	vector<Geometry*> boundedobjects;
	vector<Light*> lights;
	Camera camera;
	bool hasTransparent;

	// Each object in the scene, provided that it has hasBoundingBoxCapability(),
	// must fall within this bounding box.  Objects that don't have hasBoundingBoxCapability()
	// are exempt from this requirement.
	BoundingBox sceneBounds;
};

namespace octTree
{
	class Octree
	{
	public:
		static void insert(Octree* cur_ptr, Geometry* const& obj);
		void split();

		bool isLeaf;
		int depth;
		std::unique_ptr<Octree> child[8];
		std::vector<Geometry*> Geometries;
		BoundingBox bound;

		Octree() = delete;

		Octree(vec3f _min, vec3f _max, int _depth = 0) : isLeaf(true), depth(_depth)
		{
			bound.min = _min;
			bound.max = _max;
		}

		Octree(double _minx, double _miny, double _minz,
			double _maxx, double _maxy, double _maxz, int _depth = 0)
			: isLeaf(true), depth(_depth)
		{
			bound.min = vec3f{ _minx, _miny, _minz };
			bound.max = vec3f{ _maxx, _maxy, _maxz };
		}
	};
}


#endif // __SCENE_H__
