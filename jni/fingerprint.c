#include <stdio.h>
#include <errno.h>
#include <limits.h>

#include <sha1_api.h>

#include "kiss_fft.h"
#include "dejavu.h"

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

inline Fingerprint **alloc_fpn_tab(int peaks_num)
{
	// each peak will have DEFAULT_FAN_VALUE hashes (constellation)
	return (Fingerprint **) calloc(peaks_num * DEFAULT_FAN_VALUE + 1, sizeof(Fingerprint *));
}

static struct song *get_sound_peaks(const char *pcmfile_path)
{
	FILE *pcmfile;
	Peak **peak_tab;

	pcmfile = fopen(pcmfile_path, "r");
	TEST_PTR(pcmfile);

	struct song *song = (struct song *) malloc(sizeof(struct song));
	TEST_PTR(song);

	song->name = strdup(pcmfile_path);
	TEST_PTR(song->name);

	size_t f_size = get_file_size(pcmfile);
	int peaks_num = FREQ_RANGE_NUM * (f_size / DEFAULT_WINDOW_SIZE / sizeof(int16_t));

	// Allocate array of pointers
	peak_tab = (Peak **) calloc(peaks_num + 1, sizeof(Peak *)); // + 1 for the NULL element (last element)
	TEST_PTR(peak_tab);

	Fingerprint **fpn_tab = alloc_fpn_tab(peaks_num - 1);
	TEST_PTR(fpn_tab);

	// Initialize pointer arrays;
	int j = 0;
	for (int i = 0; i < peaks_num; i++) {
		peak_tab[i] = (Peak *) malloc(sizeof(Peak));
		TEST_PTR(peak_tab[i]);
		for (int  k = 0; k < DEFAULT_FAN_VALUE; k++, j++) {
			fpn_tab[j] = (Fingerprint *) malloc(sizeof(Fingerprint));
			TEST_PTR(fpn_tab[j]);
			memset(fpn_tab[j], 0, sizeof(Fingerprint));
		}
		memset(peak_tab[i], 0, sizeof(Peak));
	}

	int16_t pcm_frame[PCM_FRAME_SIZE];

	int offset = 0;
	int i = 0;
	while (i < peaks_num) {
		int frame_size = fread(pcm_frame, sizeof(int16_t), PCM_FRAME_SIZE, pcmfile);
		if (frame_size == 0) // Reached to the end of file
			break;

		kiss_fft_cpx *spectrum_frame = make_fft(pcm_frame, frame_size);

		// For each frequency range find peak point
		for (int j = 0; j < FREQ_RANGE_NUM && i < peaks_num ; j++) {
			peak_tab[i]->pt = get_peak(spectrum_frame, &freq_ranges[j]);
			peak_tab[i]->offset = offset;

			// Filter-out low amplitude peaks
			if (peak_tab[i]->pt.amp < DEFAULT_AMP_MIN)
				peaks_num--;
			else
				i++;
		}

		free(spectrum_frame);
		offset++;
	}

	peak_tab[i] = NULL; // fill last pointer with NULL

	fclose(pcmfile);

	song->peak_tab = peak_tab;
	song->fpn_tab = fpn_tab;

	// Return valuable number off peak points
	// discarding last element
	song->peak_tab_sz = peaks_num;
}

void fingerprint_song(struct song *song)
{
	unsigned char bin_hash[HASH_SIZE];
	char hex_hash[HEX_HASH_LEN];
	//char data[HEX_HASH_LEN];

	const char *tmp_fmt = "%d %d %d";
	const int maxlen = 4 * 3; // max frequency - 4 chars, max time difference - 4 chars
	char hashgen_str[maxlen];

	int fpn_idx = 0;
	for (Peak **it = song->peak_tab; *it != NULL; it++) {
			Peak *anchor_point = *it;
			for (int j = 1; j <= DEFAULT_FAN_VALUE; j++, fpn_idx++) {
				if (*(it + j) == NULL)
					break;
				Peak *pair_point = *(it + j);
				int freq1 = anchor_point->pt.freq;
				int freq2 = pair_point->pt.freq;

				int t1 = anchor_point->offset;
				int t2 = pair_point->offset;
				int dt = t2 - t1;

				snprintf(hashgen_str, maxlen, "%d %d %d", freq1, freq2, dt);
				//sprintf(data,"(%d, %.3f) - (%d, %.3f) | %d", freq1, anchor_point->pt.amp, freq2, pair_point->pt.amp, dt);

				sha1_calc(hashgen_str, strlen(hashgen_str), bin_hash);
				sha1_toHexString(bin_hash, hex_hash);

				strncpy(song->fpn_tab[fpn_idx]->hash, hex_hash, HEX_HASH_LEN);
				//song->fpn_tab[fpn_idx]->debug_data = strdup(data);
				song->fpn_tab[fpn_idx]->t1 = t1;
			}
	}
	song->fpn_tab[fpn_idx] = NULL; // add last element to the tab
}

struct song *learn_song(const char *pcmfile_path)
{
	struct song* song = get_sound_peaks(pcmfile_path);
	fingerprint_song(song);
	sort_fpn_table(song);

	return song;
}

void free_song_mem(struct song *song)
{
	free(song->name);

	Peak **it;
	for (it = song->peak_tab; *it != NULL; it++)
		free(*it);
	free(*it);

	Fingerprint **it2;
	for (it2 = song->fpn_tab; *it2 != NULL; it2++) {
	//	free((*it2)->debug_data);
		free(*it2);
	}
	free(*it2);
}

int songcmp(struct song *s1, struct song *s2)
{
	int found_match = 0;

	int match_diff = 0; // match difference accumulator
	// 0 - is total match.

	for (Fingerprint **it = s1->fpn_tab; *it != NULL; it++) {
		Fingerprint *res;
		if (res = find_hash((*it)->hash, s2)) {
			found_match = 1;
			int dt = abs((*it)->t1 - res->t1);
			//printf("\t%.2d <-> %.2d | %.2d\n", res->t1, (*it)->t1, dt);
			match_diff += dt;
		}
	}

	match_diff += abs(s1->peak_tab_sz - s2->peak_tab_sz);
	return (found_match) ? match_diff : INT_MAX;
}

