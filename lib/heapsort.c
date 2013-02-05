/* An implementation of heap sort for the Whitix kernel.
 * Adapted from the Linux kernel code written by Matt Mackall <mpm@selenic.com>
 */

#include <module.h>
#include <typedefs.h>

typedef unsigned long size_t;

static void u32Swap(void* a, void* b, int size)
{
	unsigned long t = *(unsigned long*)a;
	*(unsigned long*)a = *(unsigned long*)b;
	*(unsigned long*)b = t;
}

static void genericSwap(void* a, void* b, int size)
{
	char t;

	do
	{
		t = *(char*)a;
		*(char*)a++ = *(char*)b;
		*(char*)b++ = t;
	} while (--size > 0);
}

void Sort(void* base, size_t num, size_t size, int (*compare)(const void*, const void*), void (*swap)(void*, void*, int size))
{
	int i = (num/2 - 1) * size, n = num * size, c, r;

	if (!swap)
		swap = (size == 4 ? u32Swap : genericSwap);

	/* Heapify */
	for (; i >= 0; i -= size)
	{
		for (r = i; r*2+size < n; r = c)
		{
			c = r*2 + size;
			if (c < n - size && compare(base + c, base +c + size) < 0)
				c += size;

			if (compare(base + r, base + c) >= 0)
				break;

			swap(base + r, base + c, size);
		}
	}

	/* Sort */
	for (i = n - size; i > 0; i -= size)
	{
		swap(base, base + i, size);

		for (r = 0; r*2 + size < i; r = c)
		{
			c = r*2 + size;

			if (c < i-size && compare(base + c, base + c + size) < 0)
				c +=size;

			if (compare(base + r, base + c) >= 0)
				break;

			swap(base + r, base + c, size);
		}
	}
}

SYMBOL_EXPORT(Sort);
