#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "dejavu.h"
#include "fingerprint.h"

void db_write_song(struct song *song, FILE *db)
{
	// 1. put song name
	fputs(song->name, db);
	fputc('\n', db);

	// 2. write number of peaks
	fwrite(&song->peak_tab_sz, sizeof(int), 1, db);

	// 3. write hashes
	int fpn_num = song->peak_tab_sz - 1;
	for (int i = 0; i < fpn_num; i++)
		fwrite(song->fpn_tab[i], sizeof(Fingerprint), 1, db);
}

static int db_errno;
enum {
	EREAD_NAME,
	EREAD_TAB_SZ,
	EREAD_FP
};

struct song *db_load_song(FILE *fdb)
{
	char song_name[FILENAME_MAX + 1];
	if (fgets(song_name, FILENAME_MAX, fdb) == NULL) {
		db_errno = EREAD_NAME;
		return NULL;
	}
	song_name[strlen(song_name) - 1] = '\0'; // remove '\n' character

	int peak_tab_sz = 0;
	fread(&peak_tab_sz, sizeof(int), 1, fdb);
	if (peak_tab_sz == 0) {
		db_errno = EREAD_TAB_SZ;
		return NULL;
	}

	int fpn_num = peak_tab_sz - 1;
	Fingerprint **fpn_tab = alloc_fpn_tab(fpn_num);
	TEST_PTR(fpn_tab);

	// Initialize pointer arrays;
	for (int i = 0; i < fpn_num; i++) {
		fpn_tab[i] = (Fingerprint *) malloc(sizeof(Fingerprint));
		TEST_PTR(fpn_tab[i]);
		memset(fpn_tab[i], 0, sizeof(Fingerprint));
	}

	for (int i = 0; i < fpn_num; i++) {
		if (!fread(fpn_tab[i], sizeof(Fingerprint), 1, fdb)) {
			db_errno = EREAD_FP;
			return NULL;
		}
	}

	struct song *song = (struct song *) malloc(sizeof(struct song));
	TEST_PTR(song);

	song->name = strdup(song_name);
	TEST_PTR(song->name);

	song->peak_tab_sz = peak_tab_sz;

	song->fpn_tab = fpn_tab;

	return song;
}

struct song **db_load_song_list()
{
	FILE *fdb;
	fdb = fopen("dejavu.db", "r");
	TEST_PTR(fdb);

	struct song **song_list = NULL;

	struct song *song;
	int song_idx = 0;
	while (song = db_load_song(fdb)) {
		struct song **old_ptr = song_list;
		size_t new_size = sizeof(struct song *) * (song_idx + 1 + 1);
		// +1 for last NULL elem
		song_list = (struct song **) realloc(song_list, new_size);

		if (song_list == NULL) {
			perror("realloc() failed");
			break; // Failed realloc()
		}

		song_list[song_idx++] = song;
	}
	song_list[song_idx] = NULL;

	fclose(fdb);

	return song_list;
}

