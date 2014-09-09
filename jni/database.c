#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <sha1_api.h>
#include "database.h"

struct tnode *database[];

struct tnode *talloc(void)
{
	return (struct tnode *) malloc(sizeof(struct tnode));
}

struct tnode *addtree(struct tnode *p, int id, char *hash)
{
	int cond;

	if (p == NULL) {
		p = talloc();
		p->id = id;
		p->hash = strdup(hash);
		p->left = p->right = NULL;
	} else if ((cond = strcmp(hash, p->hash)) == 0)
		;
	else if (cond < 0)
		p->left = addtree(p->left, id, hash);
	else
		p->right = addtree(p->right, id, hash);

	return p;
}

void treeprint(struct tnode *p)
{
	if (p != NULL) {
		treeprint(p->left);
		printf("%d. %s\n", p->id, p->hash);
		treeprint(p->right);
	}
}

struct tnode *treefind(struct tnode *p, char *hash)
{
	int cond;

	if (!p)
		return p;

	if ((cond = strcmp(hash, p->hash)) == 0)
		return p;
	else if (cond < 0)
		return treefind(p->left, hash);
	else
		return treefind(p->right, hash);
}

void freetree(struct tnode *p)
{
	struct tnode *p_right;

	if (p != NULL) {
		freetree(p->left);
		free(p->hash);
		p_right = p->right;
		free(p);
		freetree(p_right);
	}
}

int test()
{
	struct tnode *root;

	root = NULL;

	unsigned char hash[HASH_SIZE];
	char hex_hash[HEX_HASH_LEN];

	srand(time(NULL));

	for (int i = 0; i < 10; i++) {
		int r = rand();
		char s[11];
		snprintf(s, 11, "%d", r);

		sha1_calc(s, strlen(s), hash);
		sha1_toHexString(hash, hex_hash);

		printf("%d. %s\n", i, hex_hash);
		root = addtree(root, i, hex_hash);
	}

	printf("\n");

	treeprint(root);

	printf("Enter hash to find:\n");
	scanf("%s", hex_hash);

	printf("Search result: %d\n", treefind(root, hex_hash));

	freetree(root);
	return 0;
}
