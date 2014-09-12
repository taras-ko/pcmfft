#ifndef DEJAVU_H
#define DEJAVU_H

#define FREQ_RANGE_NUM 3

struct peak_point {
	int freq; // frequency
	double amp; // amplitude
};
// Degree to which a fingerprint can be paired with its neighbors --
// higher will cause more fingerprints, but potentially better accuracy.
#define DEFAULT_FAN_VALUE 3
typedef struct {
	struct peak_point pt;
	int offset; // offset from the beginning of a file
	char *hashes[DEFAULT_FAN_VALUE]; // peaks constellation
} Peak;

#endif //DEJAVU_H
