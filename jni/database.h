#ifndef DATABASE_H
#define DATABASE_H

#include "dejavu.h"

#define HEX_HASH_LEN 41
#define HASH_SIZE 20

struct song_tree {
	int sid;
	const char *song_name;
	struct tnode *root;
};

struct tnode {
	int t1;
	char *hash;
	struct tnode *left;
	struct tnode *right;
};

struct tnode *talloc(void);
struct tnode *buildtree(Peak **peaks_ptrs);
struct tnode *addtree(struct tnode *p, char *hash, int t1);
struct tnode *treefind(struct tnode *p, char *hash);
int tree_find_matches(struct tnode *root1, struct tnode *root2);
void treeprint(struct tnode *p);
void freetree(struct tnode *p);

#endif //DATABASE_H
