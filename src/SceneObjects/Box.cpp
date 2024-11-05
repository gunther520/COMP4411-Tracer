#include <cmath>
#include <assert.h>

#include "Box.h"

bool Box::intersectLocal( const ray& r, isect& i ) const
{
	// YOUR CODE HERE:
    // Add box intersection code here.
	// it currently ignores all boxes and just returns false.


	double Tnear = -INFINITY;
	double Tfar = INFINITY;
	double T1, T2;
	vec3f p = r.getPosition();
	vec3f d = r.getDirection();





}
