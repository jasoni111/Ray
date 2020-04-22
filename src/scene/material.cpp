#include "ray.h"
#include "material.h"
#include "light.h"

// Apply the phong model to this point on the surface of the object, returning
// the color of that point.
vec3f Material::shade(Scene* scene, const ray& r, const isect& i) const
{
	// YOUR CODE HERE

	// For now, this method just returns the diffuse color of the object.
	// This gives a single matte color for every distinct surface in the
	// scene, and that's it.  Simple, but enough to get you started.
	// (It's also inconsistent with the phong model...)

	// Your mission is to fill in this method with the rest of the phong
	// shading model, including the contributions of all the light sources.
	// You will need to call both distanceAttenuation() and shadowAttenuation()
	// somewhere in your code in order to compute shadows and light falloff.


	auto Normal = i.N;
	auto V = r.getDirection();
	auto P = r.at(i.t);

	auto emissionColor = ke;
	auto diffuseColor = kd;
	auto specularColor = ks;
	auto opacity = vec3f{1.0, 1.0, 1.0} - kt;


	const vec3f ambientLight{0, 0, 0};
	// auto color = emissionColor + prod(ka, scene->ambientLight);
	auto color = emissionColor + prod(ambientLight, ka);

	for (auto l = scene->beginLights();
	     l != scene->endLights(); ++l)
	{
		Light* pLight = *l;
		auto L = pLight->getDirection(P);
		auto Lightcolor = pLight->getColor(P);
		const auto distAttenuation = pLight->distanceAttenuation(P);
		const auto shadowAttenuation = pLight->shadowAttenuation(P);

		const auto rVec = (L - 2 * L.dot(Normal) * Normal).normalize();
		const auto diffuse = std::max(0.0, Normal.dot(L));

		const auto spec = std::max(rVec.dot(V), 0.0);
		const auto specular = std::pow(spec, shininess * 128.0f);
		auto ret = prod(
			prod(distAttenuation *
			     (specular * specularColor +
				     prod(diffuseColor * diffuse, opacity))
			     , Lightcolor), shadowAttenuation);
		color += ret;
	}


	return color;
}
