#ifndef DEJAVU_H
#define DEJAVU_H

#define PCM_FRAME_SIZE 4096

//Size of the FFT window, affects frequency granularity
#define DEFAULT_WINDOW_SIZE PCM_FRAME_SIZE

// Ratio by which each sequential window overlaps the last and the
// next window. Higher overlap will allow a higher granularity of offset
// matching, but potentially more fingerprints.
#define DEFAULT_OVERLAP_RATIO 0.5

// Minimum amplitude in spectrogram in order to be considered a peak.
// This can be raised to reduce number of fingerprints, but can negatively
// affect accuracy.
#define DEFAULT_AMP_MIN 10

#define FREQ_RANGE_NUM 3
struct peak_point {
	int freq; // frequency
	double amp; // amplitude
};
// Degree to which a fingerprint can be paired with its neighbors --
// higher will cause more fingerprints, but potentially better accuracy.
#define DEFAULT_FAN_VALUE 1

#define HEX_HASH_LEN 41
#define HASH_SIZE 20

typedef struct {
	struct peak_point pt;
	int offset; // offset from the beginning of a file
} Peak;

typedef struct {
	char *hash;
	int t1;
} Fingerprint;

struct song {
	Peak **peak_tab;
	int peak_tab_sz;
	Fingerprint **fpn_tab;
};

#define TEST_PTR(x) \
	if (!(x)) { \
		perror("Problem with " #x " pointer"); \
		exit(1); \
	}

void sort_fpn_table(struct song *song);
Fingerprint *find_fingerprint(struct song *song, char *hash);

#endif //DEJAVU_H
