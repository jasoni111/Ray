#include "texture.h"

Texture::Texture() {
	pMap = nullptr;
	map_height = 0;
	map_width = 0;
}

vec3f Texture::getColor(const double &u, const double &v) {
	if (!pMap || map_width <= 0 || map_height <= 0)
		return vec3f(1, 1, 1);
	int x = int(u * map_width);
	int y = int(v * map_height);
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (x >= map_width) x = map_width - 1;
	if (y >= map_height) y = map_height - 1;
	int loc = 3 * (y * map_width + x);
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
}