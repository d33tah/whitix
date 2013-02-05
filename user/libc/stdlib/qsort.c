#include <stdlib.h>

/* Adapted from Ray Gardner's shell sort routine. */

void qsort(void* base,size_t num,size_t width,int (*fncompare)(const void*,const void*))
{
	size_t wGap, i, j, k;
	char temp;

	if (width > 0 && num > 1)
	{
		wGap=0;
		do {
			wGap=3*wGap+1;
		} while (wGap < (num-1)/3);

		wGap*=width;
		num*=width;

		do
		{
			i=wGap;
			do
			{
				j=i;
				do
				{
					char* a, *b;
					j-=wGap;
					a=j+((char*)base);
					b=a+wGap;
					if ((*fncompare)(a, b) <= 0)
						break;

					k=width;
					do
					{
						temp=*a;
						*a++=*b;
						*b++=temp;
					} while (--k);
				} while (j>=wGap);
				i+=width;
			}while(i<num);
			wGap=(wGap-width)/3;
		} while (wGap);
	}
}
