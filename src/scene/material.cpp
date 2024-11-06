#include "ray.h"
#include "material.h"
#include "light.h"

// Apply the phong model to this point on the surface of the object, returning
// the color of that point.
vec3f Material::shade( Scene *scene, const ray& r, const isect& i ) const
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

	vec3f transparency = vec3f(1,1,1)-kt;
	vec3f I = ke + (scene->getAmbientSum().clamp().multiply(ka));

	list<Light*>::const_iterator litr;
	for ( litr= scene->beginLights(); litr != scene->endLights(); ++litr) {

		vec3f lightDir = (*litr)->getDirection(r.getPosition());  // Direction from point to light source


		vec3f atten = (*litr)->distanceAttenuation(r.getPosition())*
						(*litr)->shadowAttenuation(r.getPosition());

		float diffuseFactor = max(0.0, i.N * lightDir);
		vec3f diffuse = (kd * diffuseFactor).multiply(transparency);

		vec3f reflectDir = (2 * (i.N * lightDir) * i.N) - lightDir;  // Reflection of light around the normal
		reflectDir =reflectDir.normalize();

		float specularFactor = max(0.0, (- r.getDirection()) * reflectDir);
		//float specularFactor = max(0.0, (lightDir+ (-(r.getDirection()/2))) * i.N);
		vec3f specular = (ks * pow(specularFactor, (shininess * 128.0f))).multiply(transparency);  // Adjust shininess to control highlight sharpness
		
		I += atten.multiply(diffuse + specular);

	}


	return I.clamp();
}
