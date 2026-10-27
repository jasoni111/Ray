#include <cmath>
#include "trimesh.h"

Trimesh::~Trimesh()
{
	for (Materials::iterator i = materials.begin(); i != materials.end(); ++i)
	{
		delete* i;
	}
}

// must add vertices, normals, and materials IN ORDER
void Trimesh::addVertex(const vec3f& v)
{
	vertices.push_back(v);
}

void Trimesh::addMaterial(Material* m)
{
	materials.push_back(m);
}

void Trimesh::addNormal(const vec3f& n)
{
	normals.push_back(n);
}

// Returns false if the vertices a,b,c don't all exist
bool Trimesh::addFace(int a, int b, int c)
{
	int vcnt = vertices.size();

	if (a >= vcnt || b >= vcnt || c >= vcnt)
		return false;

	TrimeshFace* newFace = new TrimeshFace(scene, new Material(*this->material), this, a, b, c);
	newFace->setTransform(this->transform);
	faces.push_back(newFace);
	scene->add(newFace);
	return true;
}

char*
Trimesh::doubleCheck()
// Check to make sure that if we have per-vertex materials or normals
// they are the right number.
{
	if (materials.size() && materials.size() != vertices.size())
		return "Bad Trimesh: Wrong number of materials.";
	if (normals.size() && normals.size() != vertices.size())
		return "Bad Trimesh: Wrong number of normals.";

	return 0;
}

// Intersect ray r with the triangle abc.  If it hits returns true,
// and put the parameter in t and the barycentric coordinates of the
// intersection in bary.
// Uses the algorithm and notation from _Graphic Gems 5_, p. 232.
//
// Calculates and returns the normal of the triangle too.
bool TrimeshFace::isTransparent() const
{
	if (material && !material->kt.iszero())
		return true;
	if (parent && !parent->materials.empty())
	{
		for (int k = 0; k < 3; ++k)
		{
			if (parent->materials[ids[k]] && !parent->materials[ids[k]]->kt.iszero())
				return true;
		}
	}
	return false;
}

bool TrimeshFace::intersectLocal(const ray& r, isect& i) const
{
	if (degenerate)
		return false;

	const vec3f& orig = r.getPosition();
	const vec3f& dir = r.getDirection();

	if (-dir.dot(faceN) < NORMAL_EPSILON)
		return false;

	const vec3f pvec = dir.cross(e2);
	const double det = e1.dot(pvec);
	if (det > -RAY_EPSILON && det < RAY_EPSILON)
		return false;

	const double invDet = 1.0 / det;
	const vec3f tvec = orig - w0;
	const double u = tvec.dot(pvec) * invDet;
	if (u < 0.0 || u > 1.0)
		return false;

	const vec3f qvec = tvec.cross(e1);
	const double v = dir.dot(qvec) * invDet;
	if (v < 0.0 || u + v > 1.0)
		return false;

	const double t = e2.dot(qvec) * invDet;
	if (t < RAY_EPSILON)
		return false;

	i.obj = this;
	i.t = t;
	if (hasVNorm)
	{
		const double w = 1.0 - u - v;
		i.N = (w * wn0 + u * wn1 + v * wn2).normalize();
	}
	else
	{
		i.N = faceN;
	}

	if (hasVMat)
	{
		i.bu = u;
		i.bv = v;
		i.vm0 = parent->materials[ids[0]];
		i.vm1 = parent->materials[ids[1]];
		i.vm2 = parent->materials[ids[2]];
	}

	return true;
}

void
Trimesh::generateNormals()
// Once you've loaded all the verts and faces, we can generate per
// vertex normals by averaging the normals of the neighboring faces.
{
	int cnt = vertices.size();
	normals.resize(cnt);
	vector<int> numFaces(cnt, 0);

	for (TrimeshFace* facePtr : faces)
	{
		const vec3f& a = vertices[(*facePtr)[0]];
		const vec3f& b = vertices[(*facePtr)[1]];
		const vec3f& c = vertices[(*facePtr)[2]];

		vec3f faceNormal = ((b - a).cross(c - a)).normalize();

		for (int i = 0; i < 3; ++i)
		{
			normals[(*facePtr)[i]] += faceNormal;
			++numFaces[(*facePtr)[i]];
		}
	}

	for (int i = 0; i < cnt; ++i)
	{
		if (numFaces[i])
			normals[i] /= numFaces[i];
	}

}

