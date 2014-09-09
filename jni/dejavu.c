#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

#include "kiss_fft.h"
#include "database.h"
#include <sha1_api.h>

#define PCM_FRAME_SIZE 4096

//Size of the FFT window, affects frequency granularity
#define DEFAULT_WINDOW_SIZE PCM_FRAME_SIZE

// Ratio by which each sequential window overlaps the last and the
// next window. Higher overlap will allow a higher granularity of offset
// matching, but potentially more fingerprints.
#define DEFAULT_OVERLAP_RATIO 0.5

// Degree to which a fingerprint can be paired with its neighbors --
// higher will cause more fingerprints, but potentially better accuracy. 
#define DEFAULT_FAN_VALUE 15

#define RANGE_NUM 3
int freq_ranges[RANGE_NUM + 1] = { 200, 300, 500, 800 };
int n_freqs = sizeof(freq_ranges) / sizeof (*freq_ranges);

typedef struct {
  int time;
  int freq; // frequency
  float mag; // magnitude
} SoundPixel;

inline float fft_abs(kiss_fft_cpx *fft_val)
{
	return sqrt(pow(fft_val->r, 2) + pow(fft_val->i, 2));
}

void get_sound_peak(SoundPixel* peak, int16_t pcm_frame[], int nfft)
{
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

	for (k = 0; k < n_freqs - 1; ++k) {
		int low_lim = freq_ranges[k];
		int upper_lim = freq_ranges[k + 1];
		int freq;

		for (freq = low_lim; freq <= upper_lim; freq++) {
			float cur_mag = fft_abs(&out[freq]);
			if (peak->mag < cur_mag) {
				peak->mag = cur_mag;
				peak->freq = freq;
			}
		}
	}

	free(in);
	free(out);
	free(cfg);
}

size_t get_file_size(FILE *file)
{
	fseek(file, 0L, SEEK_END);
	size_t f_sz = ftell(file);
	fseek(file, 0L, SEEK_SET);

	return f_sz;
}

void *get_sound_peaks(const char *pcmfile_path)
{
	FILE *pcmfile;

	SoundPixel *peaks;
	static int peaks_cnt;

	pcmfile = fopen(pcmfile_path, "r");
	if (!pcmfile) {
		fprintf(stderr, "Problem with %s: %s\n", pcmfile_path, strerror(errno));
		exit(1);
	}

	size_t f_size = get_file_size(pcmfile);
	size_t peaks_size = n_freqs * (f_size / DEFAULT_WINDOW_SIZE + 1) * sizeof(SoundPixel);

	void *peaks_memory = malloc(peaks_size);
	if (!peaks) {
		perror("Unable allocate memory for SoundPixel peaks[] array!");
		exit(1);
	}
	peaks = (SoundPixel *) peaks_memory;

	memset(peaks, 0, peaks_size);

	int16_t pcm_frame[PCM_FRAME_SIZE];
	int nfft;

	SoundPixel *it = peaks;
	int frame_offset = 0;
	while (nfft = fread(pcm_frame, 1, PCM_FRAME_SIZE, pcmfile)) {
		get_sound_peak(it, pcm_frame, nfft);

		frame_offset++;

		if (it->mag == 0.0)// frames with zeroes
			continue;

		it->time = frame_offset;

		it++; // switch to next peak element of peaks[]
	}

	it = NULL; // sign of the end array

	fclose(pcmfile);

	return peaks_memory;
}

int main(int argc, char *argv[])
{
	char *file1_path = argv[1];
	char *file2_path = argv[2];

	void *peaks1_mem = get_sound_peaks(file1_path);
	SoundPixel *peaks1 = (SoundPixel *) peaks1_mem;
	free(peaks1_mem);

	unsigned char hash[HASH_SIZE];
	char hex_hash[HEX_HASH_LEN];

	// Hash will be composed from string "freq1-freq2-dt"
	char *tmp_fmt = "%d-%d-%d";
	char tmp[10 * 3 + 2]; // INT_MAX 2,147,483,647 three of 10, and two '-'

	struct peakpair {
		char *hash;
		int t1;
	};

	struct tnode *root = NULL;

	SoundPixel *it;
	for (SoundPixel *it = peaks1; *it != NULL; i++) {
		printf("Peak at %d:\n", peaks[i].time);
		for (int j = 1; j < DEFAULT_FAN_VALUE; j++) {
			if ((i + j) > peaks_cnt)
				continue;
			int freq1 = peaks[i].freq;
			int freq2 = peaks[i + j].freq;

			int t1 = peaks[i].time;
			int t2 = peaks[i + j].time;
			int dt = t2 - t1;

			int n = sprintf(tmp, tmp_fmt, freq1, freq2, dt);

			sha1_calc(tmp, strlen(tmp), hash);
			sha1_toHexString(hash, hex_hash);

			root = addtree(root, t1, hex_hash);
		}
		printf("\n");
	}
	free(peaks);

	treeprint(root);
	treesave();
	treeload();

	struct tnode *res_node;
	int t1;

	printf("Enter (hash, t1) pair to find match:\n");
	scanf("%s%d", hex_hash, &t1);

	if (res_node = treefind(root, hex_hash)) {
		printf("Found match! Time difference: %d\n", res_node->id - t1);
	}

	freetree(root);

	return 0;
}
