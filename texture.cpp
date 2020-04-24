#include "texture.h"

Texture::Texture() {
	pMap = nullptr;
	map_height = 0;
	map_width = 0;
}

vec3f Texture::getColor(const double &u, const double &v) {
	if (!pMap || u < 0 || v < 0)
		return vec3f(1, 1, 1);
	int loc = 3 * (int(v * map_height) * map_width + int(u * map_width));
	return vec3f(
		1.0 * pMap[loc + 0] / 255.0,
		1.0 * pMap[loc + 1] / 255.0,
		1.0 * pMap[loc + 2] / 255.0
	);
}

void Texture::loadMap(const char* p) {
	if (pMap) {
		delete pMap;
		pMap = nullptr;
	}
	pMap = readBMP(p, map_width, map_height);
	if (!pMap)
		while (1);
}