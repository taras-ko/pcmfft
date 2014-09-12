#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

#include <sha1_api.h>
#include "dejavu.h"
#include "kiss_fft.h"
#include "database.h"
#include "qsort.h"

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

struct freq_range {
	int low_lim;
	int upper_lim;
};

struct freq_range freq_ranges[FREQ_RANGE_NUM] = {
	{ 200, 300 },
	{ 300, 500 },
	{ 500, 800 }
};


inline double fft_abs(kiss_fft_cpx *fft_val)
{
	return sqrt(pow(fft_val->r, 2) + pow(fft_val->i, 2));
}

size_t get_file_size(FILE *file)
{
	fseek(file, 0L, SEEK_END);
	size_t f_sz = ftell(file);
	fseek(file, 0L, SEEK_SET);

	return f_sz;
}

// Don't forget to free returned pointer
kiss_fft_cpx *make_fft(int16_t pcm_frame[], int frame_size)
{
	int nfft = frame_size;
	int is_inverse_fft = 0;

	size_t buflen = sizeof(kiss_fft_cpx) * nfft;

	kiss_fft_cpx *in = (kiss_fft_cpx*) malloc(buflen);
	kiss_fft_cpx *out = (kiss_fft_cpx*) malloc(buflen);
	kiss_fft_cfg cfg = kiss_fft_alloc(nfft, is_inverse_fft, 0, 0);

	int k;
	for (k = 0; k < nfft; ++k) {
		in[k].r = pcm_frame[k];
		in[k].i = 0;
	}

	kiss_fft(cfg, in, out);

	free(in);
	free(cfg);

	return out;
}

// Find maximum magnintude in frequency range
struct peak_point get_peak(kiss_fft_cpx *spectrum_frame, struct freq_range *range)
{
	struct peak_point peak = { 0, 0.0 };

	for (int freq = range->low_lim; freq < range->upper_lim; freq++) {
		float amp = fft_abs(&spectrum_frame[freq]);
		if (peak.amp < amp) {
			peak.amp = amp;
			peak.freq = freq;
		}
	}

	return peak;
}

int get_sound_peaks(Peak ***p_peak_tab, const char *pcmfile_path)
{
	FILE *pcmfile;

	pcmfile = fopen(pcmfile_path, "r");
	if (!pcmfile) {
		fprintf(stderr, "Problem with %s: %s\n", pcmfile_path, strerror(errno));
		exit(1);
	}

	size_t f_size = get_file_size(pcmfile);
	int peaks_num = FREQ_RANGE_NUM * (f_size / DEFAULT_WINDOW_SIZE / sizeof(int16_t)) + 1;
	size_t peaks_size = peaks_num * sizeof(Peak);

	// Allocate array of pointers
	Peak **peak_tab = (Peak **) calloc(peaks_num + 1, sizeof(Peak *)); // + 1 for the NULL element (last element)

	if (!peak_tab) {
		perror("Unable allocate memory for Peak peaks[] array!");
		exit(1);
	}
	// Initialize pointer array;
	int i;
	for (i = 0; i < peaks_num; i++) {
		peak_tab[i] = (Peak *) malloc(sizeof(Peak));
		if (!peak_tab[i]) {
			perror("Unable allocate memory for Peak peaks[] array!");
			exit(1);
		}
		memset(peak_tab[i], 0, sizeof(Peak));
	}

	int16_t pcm_frame[PCM_FRAME_SIZE];

	int offset = 0;
	for (int i = 0, offset = 0; i < peaks_num; offset++) {
		int frame_size = fread(pcm_frame, sizeof(int16_t), PCM_FRAME_SIZE, pcmfile);
		if (frame_size == 0) // Reached to the end of file
			break;

		kiss_fft_cpx *spectrum_frame = make_fft(pcm_frame, frame_size);

		// For each frequency range find peak point
		for (int j = 0; j < FREQ_RANGE_NUM && i < peaks_num ; i++, j++) {
			peak_tab[i]->pt = get_peak(spectrum_frame, &freq_ranges[j]);
			peak_tab[i]->offset = offset;

			// Filter-out low amplitude peaks
			//if (peak_tab[i]->pt.amp < DEFAULT_AMP_MIN) {
			if (peak_tab[i]->pt.amp < DEFAULT_AMP_MIN) {
				i--;
				peaks_num--;
			}
		}

		free(spectrum_frame);
	}

	peak_tab[i] = NULL; // the last pointer is NULL

	fclose(pcmfile);

	*p_peak_tab = peak_tab;

	return peaks_num; // return valuable number off peak points
}

void fingerprint(Peak **peak_tab)
{
	unsigned char bin_hash[HASH_SIZE];
	char hex_hash[HEX_HASH_LEN];

	const char *tmp_fmt = "%d %d %d";
	const int maxlen = 4 * 3; // max frequency - 4 chars, max time difference - 4 chars
	// Hash will be composed from string "freq1 freq2 dt"
	char hashgen_str[maxlen];

	for (Peak **it = peak_tab; *it != NULL; it++) {
			Peak *anchor_point = *it;
			for (int j = 0; j < DEFAULT_FAN_VALUE; j++) {
				if (*(it + j + 1) == NULL)
					break;
				Peak *pair_point = *(it + j + 1);
				int freq1 = anchor_point->pt.freq;
				int freq2 = pair_point->pt.freq;

				int t1 = anchor_point->offset;
				int t2 = pair_point->offset;
				int dt = t2 - t1;

				snprintf(hashgen_str, maxlen, "%d %d %d", freq1, freq2, dt);

				sha1_calc(hashgen_str, strlen(hashgen_str), bin_hash);
				sha1_toHexString(bin_hash, hex_hash);

				if (pair_point->pt.amp >= DEFAULT_AMP_MIN)
					//anchor_point->hashes[j] = strdup(hashgen_str);
					anchor_point->hashes[j] = strdup(hex_hash);
			}
	}
}

void free_peaks_mem(Peak **peak_tab)
{
	Peak **it;
	for (it = peak_tab; *it != NULL; it++) {
		Peak *peak = *it;
		for (int i = 0; peak->hashes[i] && i < DEFAULT_FAN_VALUE; i++)
			free(peak->hashes[i]);
		free(peak);
	}
	free(*it); // free last NULL element

	free(peak_tab);
}

int main(int argc, char *argv[])
{
	char *file1_path = argv[1];
//	char *file2_path = argv[2];

	Peak **peak_tab_1, **peak_tab_2;

	int n1 = get_sound_peaks(&peak_tab_1, file1_path);
//	int n2 = get_sound_peaks(&peak_tab_2, file2_path);

	int reverse = 0;

	fingerprint(peak_tab_1);
//	fingerprint(peak_tab_2);

//	qsort((void **)peak_tab_1, 0, n1, reverse);
//	qsort((void **)peak_tab_2, 0, n2, reverse);

	for (int i = 0; i < n1; i++) {
		Peak *peak = peak_tab_1[i];
		printf("Anchor point %d.\n", peak->offset);
		for (int j = 0; peak->hashes[j] && j < DEFAULT_FAN_VALUE; j++)
		  printf("\t %s\n", peak->hashes[j]);
	}
#if 0 // tree organisation

	struct song_tree song1 = {
		.sid = 1,
		.song_name = "call1",
		.root = NULL
	};

	struct song_tree song2 = {
		.sid = 2,
		.song_name = "call2",
		.root = NULL
	};

	song1.root = buildtree(peak_tab_1);
	song2.root = buildtree(peak_tab_2);
#endif

#if 0
	treeprint(song1.root);
//	treesave();
//	treeload();

	struct tnode *res_node;
	char hex_hash[HEX_HASH_LEN];
	int t1;

	printf("Enter (hash, t1) pair to find match:\n");
	scanf("%s%d", hex_hash, &t1);
#endif

#if 0 // tree organisation
	printf("Match rate: %d\n", tree_find_matches(song1.root, song2.root));

	freetree(song1.root);
	freetree(song2.root);
#endif
	free_peaks_mem(peak_tab_1);
//	free_peaks_mem(peak_tab_2);

	return 0;

}
