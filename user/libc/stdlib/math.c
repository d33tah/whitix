#include <math.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

typedef union
{
	double value;
	struct
	{
		unsigned int msw,lsw;
	}parts;
}ieee_double_type;

#define GET_HIGH_WORD(i,d) \
do { \
  ieee_double_type gh_u; \
  gh_u.value = (d);	\
  (i) = gh_u.parts.msw;	\
} while (0)

int finite(double x)
{
	int hx;
	GET_HIGH_WORD(hx,x);
	return (int)((unsigned int)((hx & 0x7fffffff)-0x7ff00000) >> 31);
}

double scalbn(double x,int exp)
{
	printf("todo: scalbn\n");
	exit(0);
}

double ldexp(double value,int exp)
{
	if (!finite(value) || value == 0.0) return value;
	value=scalbn(value,exp);
	if(!finite(value) || value==0.0) errno = ERANGE;
	return value;
}

union union64
{
	uint64_t quad;
	int64_t sQuad;
	long sLong[2];
	unsigned long uLong[2];
};

typedef unsigned short digit;

#define HHALF(ul) (ul >> 16)
#define LHALF(ul) (ul & 0xFFFF)

#define COMBINE(a, b) (((unsigned long)(a) << 16 ) | (b))

uint64_t __qdivrem(uint64_t a, uint64_t b, uint64_t* rem)
{
	union union64 temp;
	digit* u, *v, *q;
	digit digit1, digit2;
	int n;
	unsigned long t;
	
	digit uspace[5], vspace[5], qspace[5];	

	/* Divide by zero? */
	if (b == 0)
	{
		static volatile const unsigned int zero = 0;
		
		temp.uLong[1] = temp.uLong[0] = 1 / zero;
		
		if (rem)
			*rem = b;
			
		return temp.quad;
	}
	
	if (a < b)
	{
		if (rem)
			*rem = a;
			
		return 0;
	}
	
	u = &uspace[0];
	v = &vspace[0];
	q = &qspace[0];
	
	temp.quad = a;
	u[0] = 0;
	u[1] = HHALF(temp.uLong[1]);
	u[2] = LHALF(temp.uLong[1]);
	u[3] = HHALF(temp.uLong[0]);
	u[4] = LHALF(temp.uLong[0]);
	
	temp.quad = b;
	v[1] = HHALF(temp.uLong[1]);
	v[2] = LHALF(temp.uLong[1]);
	v[3] = HHALF(temp.uLong[0]);
	v[4] = LHALF(temp.uLong[0]);
	
	for (n = 4; v[1] == 0; v++)
	{
		if (--n == 1)
		{
			unsigned long val;
			digit q1, q2, q3, q4;
			
			t = v[2];
			q1 = u[1]/t;
			
			val = COMBINE(u[1] % t, u[2]);
			q2 = val / t;
			val = COMBINE(val % t, u[3]);
			q3 = val / t;
			val = COMBINE(val % t, u[4]);
			q4 = val / t;
			
			if (rem)
				*rem = val % t;
				
			temp.uLong[1] = COMBINE(q1, q2);
			temp.uLong[0] = COMBINE(q3, q4);
			
			return temp.quad;
		}
	}
	
	printf("__qdivrem: TODO\n");
	exit(0);
}

uint64_t __udivdi3(uint64_t a, uint64_t b)
{
	return __qdivrem(a, b, NULL);
}

uint64_t __umoddi3(uint64_t a,uint64_t b)
{
	uint64_t rem;
	
	__qdivrem(a, b, &rem);
	
	return rem;
}

int64_t __divdi3(int64_t a, int64_t b)
{
	uint64_t ua, ub, uq;
	int neg;
	
	if (a < 0)
	{
		ua = - (int64_t)a;
		neg = 1;
	}else{
		ua = a;
		neg = 0;
	}
	
	if (b < 0)
	{
		ub = -(uint64_t)b;
		neg ^= 1;
	}else
		ub = b;
	
	uq = __qdivrem(ua, ub, NULL);
	
	return (neg ? -uq : uq);
}

int64_t __moddi3(int64_t a, int64_t b)
{
	uint64_t ua, ub, rem;
	int neg;
	
	if (a < 0)
	{
		ua = -(uint64_t)a;
		neg = 1;
	}else{
		ua = a;
		neg = 0;
	}
	
	if (b < 0)
	{
		ub = -(uint64_t)b;
	}else
		ub = b;
	
	__qdivrem(ua, ub, &rem);
	
	return (neg ? -rem : rem);
}

int signbit(double x)
{
	union
	{
		double d;
		int i[2];
	} u;

	u.d = x;

	return u.i[1] < 0;

	return 0;
}
