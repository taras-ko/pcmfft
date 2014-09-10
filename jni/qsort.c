#include <string.h>

static void swap(void *v[], int i, int j)
{
	void *temp;

	temp = v[i];
	v[i] = v[j];
	v[j] = temp;
}

// qsort: sort v[left]...v[right] in increase order
void qsort(void *v[], int left, int right, int reverse)
{
	int i, last;

	if (left >= right)  /* nothing to do if in array */
		return;			/* if less than 2 elements */

	swap(v, left, (left + right)/2);

	last = left;

	for (i = left + 1; i <= right; i++)
		if (!reverse && strcmp((const char *) v[i], (const char *) v[left]) < 0 ||
			reverse && strcmp((const char *) v[i], (const char *) v[left]) > 0)
			swap(v, ++last, i);

	swap(v, left, last);

	qsort(v, left, last - 1, reverse);
	qsort(v, last + 1, right, reverse);
}
