//
// ray.h
//
// The low-level classes used by ray tracing: the ray and isect classes.
//

#ifndef __RAY_H__
#define __RAY_H__

#include <vector>

#include "../vecmath/vecmath.h"
#include "material.h"

class SceneObject;

// A ray has a position where the ray starts, and a direction (which should
// always be normalized!)

class ray {
public:
	ray( const vec3f& pp, const vec3f& dd )
		: p( pp ), d( dd ) {}
	ray( const ray& other ) 
		: p( other.p ), d( other.d ) {}
	~ray() {}

	ray& operator =( const ray& other ) 
	{ p = other.p; d = other.d; return *this; }

	vec3f at( double t ) const
	{ return p + (t*d); }

	vec3f getPosition() const { return p; }
	vec3f getDirection() const { return d; }

protected:
	vec3f p;
	vec3f d;
};

// The description of an intersection point.

class isect
{
public:
    isect()
        : obj( NULL ), t( 0.0 ), N(), material(0) {}

    ~isect()
    {
        delete material;
    }
    
    void setObject( SceneObject *o ) { obj = o; }
    void setT( double tt ) { t = tt; }
    void setN( const vec3f& n ) { N = n; }
    void setMaterial( Material *m ) { delete material; material = m; }
        
    isect& operator =( const isect& other )
    {
        if( this != &other )
        {
            obj = other.obj;
            t = other.t;
            N = other.N;
            i_box_x = other.i_box_x;
            i_box_y = other.i_box_y;
//            material = other.material ? new Material( *(other.material) ) : 0;
			if( other.material )
            {
                if( material )
                    *material = *other.material;
                else
                    material = new Material(*other.material );
            }
            else
            {
                material = 0;
            }
        }
        return *this;
    }

public:
    const SceneObject 	*obj;
    double t;
    vec3f N;
    Material *material;         // if this intersection has its own material
                                // (as opposed to one in its associated object)
                                // as in the case where the material was interpolated

    // intersection on box plane, range[0, 1]
    double i_box_x, i_box_y;

    const Material &getMaterial() const;
    // Other info here.
};

const double RAY_EPSILON = 0.00001;
const double NORMAL_EPSILON = 0.00001;



namespace helperFun
{
    double inline getRand(double r);
    std::vector<vec3f> sampleRay(vec3f rayVec, double r, int num_sample);
}


#endif // __RAY_H__
