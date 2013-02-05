#include <stdlib.h>

void* bsearch(const void* key, const void* base, size_t high, size_t size,
		int (*compare)(const void*, const void*))
{
	size_t low, mid;
	int r;
	char* p;

	if (size > 0)
	{
		low = 0;

		while (low < high)
		{
			mid = low + ((high-low) >> 1);
			p = ((char*)base) + mid * size;
			r = (*compare)(key, p);

			if (r > 0)
				low = mid + 1;
			else if (r < 0)
				high = mid;
			else
				return p;
		}
	}

	return NULL;
} 
