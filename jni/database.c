#include <stdio.h>
#include <limits.h>

#include "dejavu.h"

struct song **song_db;

void db_save(struct song *song)
{
	FILE *fout;
	fout = fopen("dejavu.out", "a");
	CHECK_PTR(fout);

	// 1. put song name
	fputs(song->name, fout);
	fputc('\n',fout);

	// 2. write number of peaks
	fwrite(&song->peak_tab_sz, sizeof(int), 1, fout);

	// 3. write hashes
	for (int i = 0; i < song->peak_tab_sz - 1; i++)
		fwrite(song->fpn_tab[i], sizeof(Fingerprint), 1, fout);

	fclose(fout);
}

void db_load()
{
	FILE *fin;

	fin = fopen("dejavu.db", "r");

	char fname[FILENAME_MAX + 1];
	fgets(fname, FILENAME_MAX, fin);

	int peak_tab_sz = 0;
	fread(&peak_tab_sz, sizeof(int), 1, fin);

	Fingerprint **fpn_tab = (Fingerprint **) calloc(peak_tab_sz * DEFAULT_FAN_VALUE + 1, sizeof(Fingerprint *));
	TEST_PTR(fpn_tab);

	// Initialize pointer arrays;
	int fpn_num = peak_tab_sz - 1;

	for (int i = 0; i < fpn_num; i++) {
		fpn_tab[i] = (Fingerprint *) malloc(sizeof(Fingerprint));
		TEST_PTR(fpn_tab[i]);
		memset(fpn_tab[i], 0, sizeof(Fingerprint));
	}

	for (int i = 0; i < fpn_num; i++) {
		fread(fpn_tab[i], sizeof(Fingerprint), 1, fin);
		printf("%s %d\n", fpn_tab[i]->hash, fpn_tab[i]->t1);
	}

	fclose(fin);
}
