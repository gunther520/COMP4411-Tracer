#include <cmath>

#include "light.h"
#include "../ui/TraceUI.h"
extern TraceUI* traceUI;

double DirectionalLight::distanceAttenuation( const vec3f& P ) const
{
	// distance to light is infinite, so f(di) goes to 0.  Return 1.
	return 1.0;
}


vec3f DirectionalLight::shadowAttenuation( const vec3f& P ) const
{
    // YOUR CODE HERE:
    // You should implement shadow-handling code here.
	vec3f d = getDirection(P);
	ray r(P+RAY_EPSILON*d, d);
	isect i;
	if (scene->intersect(r, i))
		return vec3f(0, 0, 0);
	else
		return vec3f(1, 1, 1);

}

vec3f DirectionalLight::getColor( const vec3f& P ) const
{
	// Color doesn't depend on P 
	return color;
}

vec3f DirectionalLight::getDirection( const vec3f& P ) const
{
	return -orientation;
}

double PointLight::distanceAttenuation( const vec3f& P ) const
{
	// YOUR CODE HERE

	// You'll need to modify this method to attenuate the intensity 
	// of the light based on the distance between the source and the 
	// point P.  For now, I assume no attenuation and just return 1.0
	
	double d = (position - P).length();
	
	double atten = 1.0 / (traceUI->getAttenuationConstant() + 
					traceUI->getLinearAttenuation() * d + 
					traceUI->getQuadraticAttenuation() * d * d);

	atten = ((atten) < (1.0)) ? (atten) : (1.0);
	//print all attenuation values
	std::cout << "Constant: " << traceUI->getAttenuationConstant() << " Linear: " << traceUI->getLinearAttenuation() << " Quadratic: " << traceUI->getQuadraticAttenuation() << std::endl;
	if (atten > 1.0)
		atten = 1.0;
	return atten;
}

vec3f PointLight::getColor( const vec3f& P ) const
{
	// Color doesn't depend on P 
	return color;
}

vec3f PointLight::getDirection( const vec3f& P ) const
{
	return (position - P).normalize();
}


vec3f PointLight::shadowAttenuation(const vec3f& P) const
{
    // YOUR CODE HERE:
    // You should implement shadow-handling code here.

	vec3f d = getDirection(P);
	ray r(P+d*RAY_EPSILON, d);
	isect i;
	if (scene->intersect(r, i))
		{
		if ((r.at(i.t) - r.getPosition()).length() < (position - r.getPosition()).length())
			return vec3f(0, 0, 0);
		}
	return vec3f(1, 1, 1);

}

vec3f AmbientLight::shadowAttenuation(const vec3f& P) const
{
	return vec3f(0,0,0);
}

double AmbientLight::distanceAttenuation( const vec3f& P ) const
{
	return 0.0;
}

vec3f AmbientLight::getDirection( const vec3f& P ) const
{
	return vec3f(0,0,0);
}

vec3f AmbientLight::getColor( const vec3f& P ) const
{
	// Color doesn't depend on P 
	return color;
}
