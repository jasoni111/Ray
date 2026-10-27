#ifndef TRIMESH_H__
#define TRIMESH_H__

#include <vector>

#include "../scene/ray.h"
#include "../scene/material.h"
#include "../scene/scene.h"
class TrimeshFace;

class Trimesh : public MaterialSceneObject
{
    friend class TrimeshFace;
    typedef vector<vec3f> Normals;
    typedef vector<vec3f> Vertices;
    typedef vector<TrimeshFace*> Faces;
    typedef vector<Material*> Materials;
    Vertices vertices;
    Faces faces;
    Normals normals;
    Materials materials;
public:
    Trimesh( Scene *scene, Material *mat, TransformNode *transform )
        : MaterialSceneObject(scene, mat)
    {
        this->transform = transform;
    }

    ~Trimesh();
    
    // must add vertices, normals, and materials IN ORDER
    void addVertex( const vec3f & );
    void addMaterial( Material *m );
    void addNormal( const vec3f & );

    bool addFace( int a, int b, int c );

    char *doubleCheck();
    
    void generateNormals();
    virtual bool isIntersectable() const { return false; }
};

class TrimeshFace : public MaterialSceneObject
{
    Trimesh *parent;
    int ids[3];
    vec3f w0, e1, e2, faceN;
    vec3f wn0, wn1, wn2;
    bool degenerate;
    bool hasVNorm;
    bool hasVMat;
public:
    TrimeshFace( Scene *scene, Material *mat, Trimesh *parent, int a, int b, int c)
        : MaterialSceneObject( scene, mat ), degenerate(false), hasVNorm(false), hasVMat(false)
    {
        this->parent = parent;
        ids[0] = a;
        ids[1] = b;
        ids[2] = c;
    }

    int operator[]( int i ) const
    {
        return ids[i];
    }

    virtual bool intersectLocal( const ray& r, isect& i ) const;

    virtual bool hasBoundingBoxCapability() const { return true; }
    virtual bool isTransparent() const;
    virtual void prepareForTrace();
    void cacheShadingAttributes();

    virtual void ComputeBoundingBox()
    {
        Geometry::ComputeBoundingBox();

        const vec3f& a = parent->vertices[ids[0]];
        const vec3f& b = parent->vertices[ids[1]];
        const vec3f& c = parent->vertices[ids[2]];
        w0 = transform->localToGlobalCoords(a);
        const vec3f w1 = transform->localToGlobalCoords(b);
        const vec3f w2 = transform->localToGlobalCoords(c);
        e1 = w1 - w0;
        e2 = w2 - w0;
        const vec3f cv = e1.cross(e2);
        degenerate = cv.iszero();
        if (!degenerate)
            faceN = cv.normalize();
        worldSpace = true;
    }
      
    virtual BoundingBox ComputeLocalBoundingBox()
    {
        BoundingBox localbounds;
        localbounds.max = maximum( parent->vertices[ids[0]], parent->vertices[ids[1]]);
		localbounds.min = minimum( parent->vertices[ids[0]], parent->vertices[ids[1]]);
        
        localbounds.max = maximum( parent->vertices[ids[2]], localbounds.max);
		localbounds.min = minimum( parent->vertices[ids[2]], localbounds.min);
        return localbounds;
    }
    
};


#endif // TRIMESH_H__
