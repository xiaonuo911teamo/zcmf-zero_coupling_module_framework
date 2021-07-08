#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <math.h>

#ifdef M_PI
# undef M_PI
#endif
#define M_PI      (3.141592653589793)

#ifdef M_PI_2
# undef M_PI_2
#endif
#define M_PI_2    (M_PI / 2)

#define M_GOLDEN  (1.6180339)

#define M_2PI         (M_PI * 2)

#ifndef DEG_TO_RAD
#define DEG_TO_RAD      (M_PI / 180.0f)
#endif

#ifndef RAD_TO_DEG
#define RAD_TO_DEG      (180.0 / M_PI)
#endif

// GPS Specific double precision conversions
// The precision here does matter when using the wsg* functions for converting
// between LLH and ECEF coordinates.
static const double DEG_TO_RAD_DOUBLE = asin(1) / 90;
static const double RAD_TO_DEG_DOUBLE = 90 / asin(1);


#endif
