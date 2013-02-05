#ifndef MATH_H
#define MATH_H

#include <inttypes.h>

double atan2(double y, double x);
double cos(double x);
double acos(double x);
double sin(double x);
double sqrt(double x);
double ceil(double x);
double floor(double x);
double tan(double x);
int rand(void);
double fabs(double x);
double fmod(double x, double y);

double ldexp(double x,int exp);
uint64_t __udivdi3(uint64_t a,uint64_t b);
uint64_t __umoddi3(uint64_t a,uint64_t b);
int64_t __divdi3(int64_t a,int64_t b);
int64_t __moddi3(int64_t a,int64_t b);

int signbit(double x);
double pow(double x, double y);

#ifndef M_E
#define M_E 2.718281828
#endif

#ifndef M_SQRT1_2
#define M_SQRT1_2 0.70710678118654752440
#endif

#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880
#endif

#ifndef M_PI
#define M_PI 3.1415
#endif

#define FLT_RADIX		2
#define DBL_DIG			10
#define DBL_MAX_10_EXP	37

#endif
