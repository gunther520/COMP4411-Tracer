#include <cmath>
#include <assert.h>

#include "Box.h"

#include <limits>
#include <cmath>

bool Box::intersectLocal(const ray& r, isect& i) const
{
    double Tnear = -std::numeric_limits<double>::infinity();
    double Tfar = std::numeric_limits<double>::infinity();
    vec3f p = r.getPosition();   // Ray origin
    vec3f d = r.getDirection();  // Ray direction

    // Get bounding box min and max points
    vec3f min_ = this->getBoundingBox().min;
    vec3f max_ = this->getBoundingBox().max;

    vec3f normalNear;
    vec3f normalFar;


    for (int axis = 0; axis < 3; axis++) {
        if (fabs(d[axis]) < RAY_EPSILON) {
            // Ray is parallel to slab. No hit if origin not within slab
            if (p[axis] < min_[axis] || p[axis] > max_[axis]) {
                return false;
            }
        }
        else {
            double invD = 1.0 / d[axis];
            double T1 = (min_[axis] - p[axis]) * invD;
            double T2 = (max_[axis] - p[axis]) * invD;

            vec3f n1(0.0, 0.0, 0.0);
            vec3f n2(0.0, 0.0, 0.0);
            n1[axis] = -1.0;
            n2[axis] = 1.0;

            if (T1 > T2) {
                std::swap(T1, T2);
                std::swap(n1, n2);
            }

            if (T1 > Tnear) {
                Tnear = T1;
                normalNear = n1;
            }
            if (T2 < Tfar) {
                Tfar = T2;
                normalFar = n2;
            }

            // Early exit cases
            if (Tnear > Tfar) {
                return false;
            }
            if (Tfar < 0) {
                return false;
            }
        }
    }

    // Determine intersection point and normal
    if (Tnear >= 0) {
        i.t = Tnear;
        i.N = normalNear;
    }
    else if (Tfar >= 0) {
        // Ray starts inside the box
        i.t = Tfar;
        i.N = normalFar;
    }
    else {
        return false;
    }

    i.obj = this;
    return true;
}

