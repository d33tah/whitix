#include <stdlib.h>

div_t div(int numer,int denom)
{
	div_t retVal;
	retVal.quot=numer/denom;
	retVal.rem=numer % denom;
	return retVal;
}

ldiv_t ldiv(long int numer,long int denom)
{
	ldiv_t retVal;
	retVal.quot=numer/denom;
	retVal.rem=numer % denom;
	return retVal;
}
