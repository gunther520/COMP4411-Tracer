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
	return traceRay( scene, r, vec3f(1.0,1.0,1.0), 0 ).clamp();
}

// Do recursive ray tracing!  You'll want to insert a lot of code here
// (or places called from here) to handle reflection, refraction, etc etc.
vec3f RayTracer::traceRay( Scene *scene, const ray& r, 
	const vec3f& thresh, int depth )
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
		vec3f incidentDir = r.getDirection();
		incidentDir=incidentDir.normalize();  // Ensure incident direction is normalized

		vec3f Q= r.at(i.t);
		ray r1(Q, incidentDir);
		const Material& m = i.getMaterial();
		vec3f I = m.shade(scene, r1, i);

		if (depth < traceUI->getDepth()) {


			vec3f reflectDir = incidentDir - 2 * (incidentDir * i.N) * i.N;
			reflectDir= reflectDir.normalize();  // Normalize the reflected direction

			// Offset the origin to avoid self-intersection
			vec3f reflectOrigin = Q + reflectDir * RAY_EPSILON;

			// Create the reflected ray
			ray reflectedRay(reflectOrigin, reflectDir);

			// Trace the reflected ray recursively
			vec3f reflectedColor = traceRay(scene, reflectedRay, thresh, depth + 1);

			I+= reflectedColor.multiply(m.kr);

			float n1, n2;
			vec3f N = i.N;  // Surface normal


			// Determine if we are inside or outside the material
			if (incidentDir * N < 0) {
				// Ray is entering the material
				n1 = 1.0f;  // Assuming air
				n2 = m.index;  // Material's index of refraction
			}
			else {
				// Ray is exiting the material
				n1 = m.index;  // Material's index of refraction
				n2 = 1.0f;  // Assuming air
				N = -N;  // Invert the normal !!! need special care

			}

			float eta = n1 / n2;
			float cosThetaI = -(incidentDir * N);  // Dot product between I and N
			float sin2ThetaI = max(0.0f, 1.0f - cosThetaI * cosThetaI);
			float sin2ThetaT = eta * eta * sin2ThetaI;
			if (sin2ThetaT <= 1.0f) {
				// No total internal reflection
				float cosThetaT = sqrt(1.0f - sin2ThetaT);
				vec3f refractDir = eta * incidentDir + (eta * cosThetaI - cosThetaT) * N;
				refractDir= refractDir.normalize();  // Normalize the refracted direction

				// Offset the origin to avoid self-intersection
				vec3f refractOrigin = Q + refractDir * RAY_EPSILON;

				// Create the refracted ray
				ray refractedRay(refractOrigin, refractDir);

				// Trace the refracted ray recursively
				vec3f refractedColor = traceRay(scene, refractedRay, thresh, depth + 1);

				I += refractedColor.multiply(m.kt);
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