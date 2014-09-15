#include <string.h>

#include  "dejavu.h"

static void swap(Fingerprint *v[], int i, int j)
{
	Fingerprint *temp;

	temp = v[i];
	v[i] = v[j];
	v[j] = temp;
}

// qsort: sort v[left]...v[right] in increase order
static void qsort(Fingerprint *v[], int left, int right, int reverse)
{
	int i, last;

	if (left >= right)  /* nothing to do if in array */
		return;			/* if less than 2 elements */

	swap(v, left, (left + right)/2);

	last = left;

	for (i = left + 1; i <= right; i++)
		if (!reverse && strcmp((const char *) v[i]->hash, (const char *) v[left]->hash) < 0 ||
			reverse && strcmp((const char *) v[i]->hash, (const char *) v[left]->hash) > 0)
			swap(v, ++last, i);

	swap(v, left, last);

	qsort(v, left, last - 1, reverse);
	qsort(v, last + 1, right, reverse);
}

#define HASH_NOT_FOUND -1
static int binsearch(Fingerprint *tab[], const char *hash, int left, int right)
{
	// continually narrow search until just one element remains
	while (left < right) {
		int mid = (left + right) / 2;

		int res = strcmp(hash, tab[mid]->hash);
		if (res > 0)
			left = mid + 1;
		else
			right = mid;
	}

	if ((right == left) && (strcmp(hash, tab[left]->hash) == 0))
		return left;
	else
		return HASH_NOT_FOUND;
}

void sort_fpn_table(struct song *song)
{
	int reverse = 0;

	int hashes = song->peak_tab_sz - 1;
	int last_hash_pos = hashes - 1;
	qsort(song->fpn_tab, 0, last_hash_pos, reverse);
}

Fingerprint *find_hash(const char *hash, struct song *song)
{
	int res;

	int hashes = song->peak_tab_sz - 1;
	int last_hash_pos = hashes - 1;
	res = binsearch(song->fpn_tab, hash, 0, last_hash_pos);

	return (res == HASH_NOT_FOUND) ? NULL : song->fpn_tab[res];
}

