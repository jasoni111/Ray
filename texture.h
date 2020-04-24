#pragma once
#include "src/fileio/bitmap.h"
#include "src/vecmath/vecmath.h"

class Texture
{
public:
	Texture();
	vec3f getColor(const double &u, const double &v);
	void loadMap(const char* p);
	
	int map_width, map_height;
	unsigned char* pMap = NULL;
};

