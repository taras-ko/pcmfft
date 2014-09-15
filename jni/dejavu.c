#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "dejavu.h"
#include "getopt.h"

#include "database.h"
#include "fingerprint.h"

int main(int argc, char *argv[])
{
//	parse_options(argc, argv);

	struct song **song_list = NULL;

	song_list = db_load_song_list();

	FILE *db = fopen("dejavu.db", "a");
	TEST_PTR(db);

	while (--argc >= 1) {
		struct song *song, *song2;
		song = learn_song(*++argv);

		struct song *most_matched_song = NULL;

		struct match_pair {
			struct song *song;
			int match_diff; // the largest value means largest difference
		} result = { NULL, INT_MAX };

		for (struct song **it = song_list; *it != NULL; it++) {
			int match_diff = songcmp(song, (*it));
			if (match_diff < result.match_diff) {
				result.song = (*it);
				result.match_diff = match_diff;
			}
		}

		if (result.song) // found match!
			printf("%s\n", result.song->name);
		else // add song to database
			db_write_song(song, db);
	}

	fclose(db);

	exit(EXIT_SUCCESS);
}
