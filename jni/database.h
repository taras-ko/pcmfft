#ifndef DATABASE_H
#define DATABASE_H

#include <stdio.h>

FILE *db_open();
void db_write_song(struct song *song, FILE *db);
struct song **db_load_song_list();

#endif //DATABASE_H
