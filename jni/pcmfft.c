#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "kiss_fft.h"

#define PCM_FRAME_SIZE 4096

#define RANGE_NUM 3
int freq_ranges[RANGE_NUM + 1] = { 2, 3, 5, 8 };
int n_freqs = sizeof(freq_ranges) / sizeof (*freq_ranges);

#define HANDLE_FRAMES_LIM 4096

FILE *pcmfile;
int16_t pcm_frame[PCM_FRAME_SIZE];

typedef struct {
  float mag; // magnitude
  int freq; // frequency
} MusicPoint;

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

  if (nfft == 0 || frame_num >= HANDLE_FRAMES_LIM) // end of file or limitation
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

  for (k = 0; k < n_freqs - 1; ++k) {
    int low_lim = freq_ranges[k];
    int upper_lim = freq_ranges[k + 1];
    int freq;
    MusicPoint mp = { 0.0, 0 };
    for (freq = low_lim; freq <= upper_lim; freq++) {
      float cur_mag = fft_abs(&out[freq]);
      if (mp.mag < cur_mag) {
        mp.mag = cur_mag;
        mp.freq = freq;
      }
    }
    printf("%f ", mp.mag);
  }
  printf("\n");

  free(in);
  free(out);
  free(cfg);

  frame_num++;

  return 1;
}

int main(int argc, char *argv[])
{
  char *infile_path;
  char *default_infile = "/sdcard/mic.pcm";


  infile_path = (argc == 1) ? default_infile : argv[1];

  pcmfile = fopen(infile_path, "r");
  if (!pcmfile) {
    fprintf(stderr, "Problem with %s: %s\n", infile_path, strerror(errno));
    exit(1);
  }

  while (handle_next_frame())
    ;

#if 0
  int i, j;
  for (i = 0, j = 0; i < n; i++, j++) {
    if (j < 40)
      printf("%d ", pcm_frame[i]);
    else {
      printf("%d\n", pcm_frame[i]);
      j = 0;
    }
  }
#endif



  fclose(pcmfile);
}
