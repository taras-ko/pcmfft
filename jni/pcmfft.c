#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

#include "kiss_fft.h"
#include "sha1/sha1_api.h"

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

FILE *pcmfile;
int16_t pcm_frame[PCM_FRAME_SIZE];

typedef struct {
  int time;
  int freq; // frequency
  float mag; // magnitude
} MusicPoint;

MusicPoint *peaks;
static int peaks_cnt;

inline float fft_abs(kiss_fft_cpx *fft_val)
{
  return sqrt(pow(fft_val->r, 2) + pow(fft_val->i, 2));
}

int handle_next_frame()
{
	static int frame_num;
	int nfft;
	int is_inverse_fft = 0;

	nfft = fread(pcm_frame, 1, PCM_FRAME_SIZE, pcmfile);

	if (nfft == 0) // end of file
		return 0;

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

	int time = 0;

	for (k = 0; k < n_freqs - 1; ++k) {
		int low_lim = freq_ranges[k];
		int upper_lim = freq_ranges[k + 1];
		int freq;

		MusicPoint *peak = &peaks[peaks_cnt];
		peak->time = frame_num;

		for (freq = low_lim; freq <= upper_lim; freq++) {
			float cur_mag = fft_abs(&out[freq]);
			if (peak->mag < cur_mag) {
				peak->mag = cur_mag;
				peak->freq = freq;
			}
		}
		if (peak->mag != 0.0)
			peaks_cnt++;
	}

	printf("\n");

	free(in);
	free(out);
	free(cfg);

	frame_num++;

	return 1;
}

size_t get_file_size(FILE *file)
{
	fseek(file, 0L, SEEK_END);
	size_t f_sz = ftell(file);
	fseek(file, 0L, SEEK_SET);

	return f_sz;
}

int main(int argc, char *argv[])
{
	char *infile_path;
	char *default_infile ="/sdcard/mic.pcm";

	infile_path = (argc == 1) ? default_infile : argv[1];

	pcmfile = fopen(infile_path, "r");
	if (!pcmfile) {
		fprintf(stderr, "Problem with %s: %s\n", infile_path, strerror(errno));
		exit(1);
	}

	size_t f_size = get_file_size(pcmfile);
	size_t peaks_size = n_freqs * (f_size / DEFAULT_WINDOW_SIZE + 1) * sizeof(MusicPoint);

	peaks = malloc(peaks_size);
	if (!peaks) {
		perror("Unable allocate memory for MusicPoints peaks[]!");
		exit(1);
	}
	memset(peaks, 0, peaks_size);

	while (handle_next_frame())
		;

	unsigned char hash[20];
	char hex_hash[41];

	char *tmp_fmt = "%d-%d-%d";
	char tmp[10 * 3 + 2]; // INT_MAX 2,147,483,647 three of 10, and two '-'

	for (int i = 0; i < peaks_cnt; i++) {
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
			printf("\t(%.10s, %d)\n", hex_hash, dt);
		}
		printf("\n");
	}

	fclose(pcmfile);
	free(peaks);
}
