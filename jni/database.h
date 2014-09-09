#ifndef DATABASE_H
#define DATABASE_H

#define HEX_HASH_LEN 41
#define HASH_SIZE 20

struct tnode {
	int id;
	char *hash;
	struct tnode *left;
	struct tnode *right;
};

struct tnode *talloc(void);
struct tnode *addtree(struct tnode *p, int id, char *hash);
struct tnode *treefind(struct tnode *p, char *hash);
void treeprint(struct tnode *p);
void freetree(struct tnode *p);

#endif //DATABASE_H
