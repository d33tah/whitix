#include <math.h>
#include <stdio.h> //TEMP

double floor(double x);
double ceil(double x);

double modf(double x, double* iptr)
{
	double y;

	if (x < 0)
		y=ceil(x);
	else
		y=floor(x);

	*iptr=y;
	return x-y;
}

double fabs(double x)
{
	if (x < 0.0)
		x=-x;

	return x;
}

double pow(double x, double y)
{
	printf("pow: TODO\n");
	return 0.0;
}

#define EXPMSK	0x800F
#define MEXP	0x7FF
#define NBITS	53

static unsigned short bitMask[] = {
	0xffff,
	0xfffe,
	0xfffc,
	0xfff8,
	0xfff0,
	0xffe0,
	0xffc0,
	0xff80,
	0xff00,
	0xfe00,
	0xfc00,
	0xf800,
	0xf000,
	0xe000,
	0xc000,
	0x8000,
	0x0000,
};

double floor(double x)
{
	/* Adapted from the Cephes math library. */
	union
	{
		double y;
		unsigned short sh[4];
	} u;

	unsigned short* p;
	int e;

	u.y = x;

	/* Find the exponent. */
	p=(unsigned short*)&u.sh[3];
	e=((*p >> 4) & 0x7FF) - 0x3FF;
	p-=3;

	/* For cases when exponent is below 0. */
	if ( e < 0)
	{
		if (u.y < 0.0)
			return -1.0;
		else
			return 0.0;
	}

	e=(NBITS-1)-e;

	while (e >= 16)
	{
		*p++=0;
		e-=16;
	}

	if (e > 0)
		*p &= bitMask[e];

	if ((x < 0) && (u.y != x))
		u.y-= 1.0;

	return u.y;
}

double ceil(double x)
{
	double y;

	y=floor(x);
	if (y < x)
		y+=1.0;

	return y;
}

double fmod(double x, double y)
{
	printf("fmod: TODO\n");
	return 0.0;
}

double log(double x)
{
	printf("log: TODO\n");
	return 0.0;
}

double exp(double x)
{
	printf("exp: TODO\n");
	return 0.0;
}

double frexp(double x, int* exp)
{
	union
	{
		double y;
		unsigned short sh[4];
	}u;

	int i, k;
	short* q;

	q=(short*)&u.sh[3];

	i=(*q >> 4) & MEXP;
	if (i)
		goto ieeeDon;

	if (u.y == 0.0)
	{
		*exp=0;
		return 0.0;
	}

	do
	{
		u.y*=2.0;
		i-=1;
		k=(*q >> 4) & MEXP;
	} while (k == 0);

	i+=k;

	ieeeDon:
	i-=0x3FE;
	*exp=i;
	*q &= 0x800F;
	*q |= 0x3FE0;
	return u.y;
}

static long	sqtab[64] =
{
	0x6cdb2, 0x726d4, 0x77ea3, 0x7d52f, 0x82a85, 0x87eb1, 0x8d1c0, 0x923bd,
	0x974b2, 0x9c4a8, 0xa13a9, 0xa61be, 0xaaeee, 0xafb41, 0xb46bf, 0xb916e,
	0xbdb55, 0xc247a, 0xc6ce3, 0xcb495, 0xcfb95, 0xd41ea, 0xd8796, 0xdcca0,
	0xe110c, 0xe54dd, 0xe9818, 0xedac0, 0xf1cd9, 0xf5e67, 0xf9f6e, 0xfdfef,
	0x01fe0, 0x05ee6, 0x09cfd, 0x0da30, 0x11687, 0x1520c, 0x18cc8, 0x1c6c1,
	0x20000, 0x2388a, 0x27068, 0x2a79e, 0x2de32, 0x3142b, 0x3498c, 0x37e5b,
	0x3b29d, 0x3e655, 0x41989, 0x44c3b, 0x47e70, 0x4b02b, 0x4e16f, 0x51241,
	0x542a2, 0x57296, 0x5a220, 0x5d142, 0x60000, 0x62e5a, 0x65c55, 0x689f2,
};

/* TODO: WRITE OWN! */

double sqrt(double arg)
{
#if 0
	if (x < 0.0)
	{
		errno=EDOM;
		return 0;
	}
#endif

	int e, ms;
	double a, t;
	union
	{
		double	d;
		struct
		{
			long	ms;
			long	ls;
		};
	} u;

	u.d = arg;
	ms = u.ms;

	/*
	 * sign extend the mantissa with
	 * exponent. result should be > 0 for
	 * normal case.
	 */
	e = ms >> 20;
	if(e <= 0) {
		if(e == 0)
			return 0;
		return 0;
	}

	/*
	 * pick up arg/4 by adjusting exponent
	 */
	u.ms = ms - (2 << 20);
	a = u.d;

	/*
	 * use 5 bits of mantissa and 1 bit
	 * of exponent to form table index.
	 * insert exponent/2 - 1.
	 */
	e = (((e - 1023) >> 1) + 1022) << 20;
	u.ms = *(long*)((char*)sqtab + ((ms >> 13) & 0xfc)) | e;
	u.ls = 0;

	/*
	 * three laps of newton
	 */
	e = 1 << 20;
	t = u.d;
	u.d = t + a/t;
	u.ms -= e;		/* u.d /= 2; */
	t = u.d;
	u.d = t + a/t;
	u.ms -= e;		/* u.d /= 2; */
	t = u.d;

	return t + a/t;
}

double log10(double x)
{
	printf("log10\n");
	return 0.0;
}

long int lrintf(float x)
{
	printf("lrintf\n");
	return 0;
}

long long int llrint(double x)
{
	printf("llrint\n");
	return 0;
}

double rint(double x)
{
	printf("rint\n");
	return 0;
}

int isnan(double x)
{
	printf("isnan\n");
	return 0;
}

double trunc(double x)
{
	printf("trunc\n");
}
