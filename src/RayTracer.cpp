// The main ray tracer.

#include <Fl/fl_ask.h>

#include "RayTracer.h"
#include "scene/light.h"
#include "scene/material.h"
#include "scene/ray.h"
#include "fileio/read.h"
#include "fileio/parse.h"
#include "ui/TraceUI.h"
extern TraceUI* traceUI;
// Trace a top-level ray through normalized window coordinates (x,y)
// through the projection plane, and out into the scene.  All we do is
// enter the main ray-tracing method, getting things started by plugging
// in an initial ray weight of (0.0,0.0,0.0) and an initial recursion depth of 0.
vec3f RayTracer::trace( Scene *scene, double x, double y )
{
    ray r( vec3f(0,0,0), vec3f(0,0,0) );
    scene->getCamera()->rayThrough( x,y,r );
	vec3f new_thresh(traceUI->getThreshold(), traceUI->getThreshold(), traceUI->getThreshold());
	return traceRay( scene, r, new_thresh, 0 ).clamp();
}

// Do recursive ray tracing!  You'll want to insert a lot of code here
// (or places called from here) to handle reflection, refraction, etc etc.
vec3f RayTracer::traceRay( Scene *scene, const ray& r, 
	const vec3f& thresh, int depth, stack<float> mediumStack  ,list<vec3f> k_product)
{
	isect i;

	if( scene->intersect( r, i ) ) {
		// YOUR CODE HERE

		// An intersection occured!  We've got work to do.  For now,
		// this code gets the material for the surface that was intersected,
		// and asks that material to provide a color for the ray.  

		// This is a great place to insert code for recursive ray tracing.
		// Instead of just returning the result of shade(), add some
		// more steps: add in the contributions from reflected and refracted
		// rays.
		vec3f incidentDir = r.getDirection().normalize();

		vec3f Q= r.at(i.t);
		ray r1(Q, incidentDir);
		const Material& m = i.getMaterial();
		vec3f I = m.shade(scene, r1, i);
		vec3f N = i.N;

		if (k_product.front()[0] < thresh[0] && k_product.back()[0] < thresh[0] &&
			k_product.front()[1] < thresh[1] && k_product.back()[1] < thresh[1] &&
			k_product.front()[2] < thresh[2] && k_product.back()[2] < thresh[2]) {

			return I;
		}

		if (mediumStack.empty()) {
			// Use the dot product to determine if the ray starts inside the material or outside
			if (incidentDir * N < 0) {
				// Ray is entering the material from outside (or the viewpoint is in air)
				mediumStack.push(1.0f);  // Assume the initial medium is air
			}
			else {
				// Ray starts inside the material
				mediumStack.push(m.index);  // Start inside the current medium
			}
		}

		// Determine if the ray is entering or exiting the material
		float n1 = mediumStack.top();  // The refractive index of the current medium
		float n2 = m.index;            // Refractive index of the intersected material

		if (incidentDir * N < 0) {
			// Ray is entering the material
			mediumStack.push(n2);  // Push the new medium onto the stack

			// Update cumulative products for kt and kr
			k_product.back() = k_product.back().multiply(m.kt);  // Update cumulative kt product
			k_product.front() = k_product.front().multiply(m.kr);  // Update cumulative kr product
		}
		else {
			// Ray is exiting the material
			mediumStack.pop();  // Pop the current medium off the stack

			// Determine if the ray is transitioning directly to another medium or into air
			ray offsetRay(Q + RAY_EPSILON * N, incidentDir);  // Offset ray to avoid self-intersection
			isect nextIsect;

			if (scene->intersect(offsetRay, nextIsect)) {
				// The ray intersects another object immediately—transitioning from one material to another
				n2 = nextIsect.getMaterial().index;  // Set n2 to the refractive index of the next medium
				mediumStack.push(n2);  // Push the new medium onto the stack
			}
			else {
				// The ray is exiting to air
				n2 = 1.0f;
			}
		

			N = -N;  // Invert the normal for correct exit calculations
		}

		if (depth < traceUI->getDepth()) {
			vec3f reflectDir = incidentDir - 2 * (incidentDir * i.N) * i.N;
			//vec3f reflectDir = (2 * (incidentDir * i.N) * i.N) - incidentDir;  // Reflection of light around the normal
			reflectDir = reflectDir.normalize();  // Normalize the reflected direction

			// Offset the origin to avoid self-intersection
			vec3f reflectOrigin = Q + i.N * RAY_EPSILON;

			// Create the reflected ray
			ray reflectedRay(reflectOrigin, reflectDir);

			// Trace the reflected ray recursively
			vec3f reflectedColor = traceRay(scene, reflectedRay, thresh, depth + 1,mediumStack,k_product);

			I += reflectedColor.multiply(m.kr); // Reflection component


			float eta = n1 / n2;
			float cosThetaI = (-incidentDir * N);  // Dot product between I and N
			float sin2ThetaI = max(0.0f, 1.0f - cosThetaI * cosThetaI);
			float sin2ThetaT = eta * eta * sin2ThetaI;


			vec3f refractedColor(0, 0, 0);

			if (sin2ThetaT <= 1.0f) {
				float cosThetaT = sqrt(1.0f - sin2ThetaT);
				vec3f refractDir = eta * incidentDir + (eta * cosThetaI - cosThetaT) * N;
				refractDir = refractDir.normalize();  // Normalize the refracted direction

				// Offset the origin to avoid self-intersection
				vec3f refractOrigin = Q + refractDir * RAY_EPSILON;
				ray refractedRay(refractOrigin, refractDir);
				refractedColor = traceRay(scene, refractedRay, thresh, depth + 1, mediumStack, k_product);
				I +=  refractedColor.multiply(m.kt); // Refraction component
			}

		}
		


		return I.clamp();
	
	} else {
		// No intersection.  This ray travels to infinity, so we color
		// it according to the background color, which in this (simple) case
		// is just black.

		return vec3f( 0.0, 0.0, 0.0 );
	}
}

RayTracer::RayTracer()
{
	buffer = NULL;
	buffer_width = buffer_height = 256;
	scene = NULL;

	m_bSceneLoaded = false;
}


RayTracer::~RayTracer()
{
	delete [] buffer;
	delete scene;
}

void RayTracer::getBuffer( unsigned char *&buf, int &w, int &h )
{
	buf = buffer;
	w = buffer_width;
	h = buffer_height;
}

double RayTracer::aspectRatio()
{
	return scene ? scene->getCamera()->getAspectRatio() : 1;
}

bool RayTracer::sceneLoaded()
{
	return m_bSceneLoaded;
}

bool RayTracer::loadScene( char* fn )
{
	try
	{
		scene = readScene( fn );
	}
	catch( ParseError pe )
	{
		fl_alert( "ParseError: %s\n", pe );
		return false;
	}

	if( !scene )
		return false;
	
	buffer_width = 256;
	buffer_height = (int)(buffer_width / scene->getCamera()->getAspectRatio() + 0.5);

	bufferSize = buffer_width * buffer_height * 3;
	buffer = new unsigned char[ bufferSize ];
	
	// separate objects into bounded and unbounded
	scene->initScene();
	
	// Add any specialized scene loading code here
	
	m_bSceneLoaded = true;

	return true;
}

void RayTracer::traceSetup( int w, int h )
{
	if( buffer_width != w || buffer_height != h )
	{
		buffer_width = w;
		buffer_height = h;

		bufferSize = buffer_width * buffer_height * 3;
		delete [] buffer;
		buffer = new unsigned char[ bufferSize ];
	}
	memset( buffer, 0, w*h*3 );
}

void RayTracer::traceLines( int start, int stop )
{
	vec3f col;
	if( !scene )
		return;

	if( stop > buffer_height )
		stop = buffer_height;

	for( int j = start; j < stop; ++j )
		for( int i = 0; i < buffer_width; ++i )
			tracePixel(i,j);
}

void RayTracer::tracePixel( int i, int j )
{
	vec3f col;

	if( !scene )
		return;

	double x = double(i)/double(buffer_width);
	double y = double(j)/double(buffer_height);

	col = trace( scene,x,y );

	unsigned char *pixel = buffer + ( i + j * buffer_width ) * 3;

	pixel[0] = (int)( 255.0 * col[0]);
	pixel[1] = (int)( 255.0 * col[1]);
	pixel[2] = (int)( 255.0 * col[2]);
}