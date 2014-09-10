#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

#include <sha1_api.h>
#include "dejavu.h"
#include "kiss_fft.h"
#include "database.h"

#define PCM_FRAME_SIZE 4096

//Size of the FFT window, affects frequency granularity
#define DEFAULT_WINDOW_SIZE PCM_FRAME_SIZE

// Ratio by which each sequential window overlaps the last and the
// next window. Higher overlap will allow a higher granularity of offset
// matching, but potentially more fingerprints.
#define DEFAULT_OVERLAP_RATIO 0.5

#define RANGE_NUM 3
int freq_ranges[RANGE_NUM + 1] = { 200, 300, 500, 800 };
int n_freqs = sizeof(freq_ranges) / sizeof (*freq_ranges);

inline float fft_abs(kiss_fft_cpx *fft_val)
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

SoundPixel **get_sound_peaks(const char *pcmfile_path)
{
	FILE *pcmfile;

	pcmfile = fopen(pcmfile_path, "r");
	if (!pcmfile) {
		fprintf(stderr, "Problem with %s: %s\n", pcmfile_path, strerror(errno));
		exit(1);
	}

	size_t f_size = get_file_size(pcmfile);
	int peaks_num = n_freqs * (f_size / DEFAULT_WINDOW_SIZE + 1);
	size_t peaks_size = peaks_num * sizeof(SoundPixel);

	// Allocate array of pointers
	SoundPixel **peaks_ptrs = (SoundPixel **) calloc(peaks_num + 1, sizeof(SoundPixel *)); // + 1 for the NULL element (last element)

	if (!peaks_ptrs) {
		perror("Unable allocate memory for SoundPixel peaks[] array!");
		exit(1);
	}
	// Allocate array of structures
	SoundPixel *peaks = (SoundPixel *) calloc(peaks_num, sizeof(SoundPixel));

	if (!peaks) {
		perror("Unable allocate memory for SoundPixel peaks[] array!");
		exit(1);
	}

	// Initialize pointer array;
	int i;
	for (i = 0; i < peaks_num; i++) {
		peaks_ptrs[i] = &peaks[i];
	}
	peaks_ptrs[i] = NULL; // Sign of the end of pointer array

	memset(peaks, 0, peaks_size);

	int16_t pcm_frame[PCM_FRAME_SIZE];

	SoundPixel **it;
	for (it = peaks_ptrs; *it != NULL; it++) {
		int nfft = fread(pcm_frame, 1, PCM_FRAME_SIZE, pcmfile);
		if (nfft == 0)
			break;

		get_sound_peak(*it, pcm_frame, nfft);

		if ((*it)->mag == 0.0)// frames with zeroes
			continue;

		(*it)->offset = it - peaks_ptrs;
	}
	*it = NULL;

	fclose(pcmfile);

	return peaks_ptrs;
}

void free_peaks_mem(SoundPixel **peaks_ptrs)
{
	free(*peaks_ptrs);
	free(peaks_ptrs);
}

int main(int argc, char *argv[])
{
	char *file1_path = argv[1];
	char *file2_path = argv[2];

	SoundPixel **peaks_ptrs_1 = get_sound_peaks(file1_path);
	SoundPixel **peaks_ptrs_2 = get_sound_peaks(file2_path);

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

	song1.root = buildtree(peaks_ptrs_1);
	song2.root = buildtree(peaks_ptrs_2);

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

	printf("Match rate: %d\n", tree_find_matches(song1.root, song2.root));

	freetree(song1.root);
	freetree(song2.root);
	free_peaks_mem(peaks_ptrs_1);
	free_peaks_mem(peaks_ptrs_2);

	return 0;

}
