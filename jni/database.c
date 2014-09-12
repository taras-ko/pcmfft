#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <sha1_api.h>
#include "database.h"
#include "dejavu.h"

struct tnode **database;

struct tnode *talloc(void)
{
	return (struct tnode *) malloc(sizeof(struct tnode));
}

struct tnode *addtree(struct tnode *p, char *hash, int t1)
{
	int cond;

	if (p == NULL) {
		p = talloc();
		p->hash = strdup(hash);
		p->t1 = t1;
		p->left = p->right = NULL;
	} else if ((cond = strcmp(hash, p->hash)) == 0)
		;
	else if (cond < 0)
		p->left = addtree(p->left, hash, t1);
	else
		p->right = addtree(p->right, hash, t1);

	return p;
}

void treeprint(struct tnode *p)
{
	if (p != NULL) {
		treeprint(p->left);
		printf("%d. %s\n", p->t1, p->hash);
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

struct tnode *buildtree(Peak **peaks_ptrs)
{
	unsigned char hash[HASH_SIZE];
	char hex_hash[HEX_HASH_LEN];

	// Hash will be composed from string "freq1 freq2 dt"
	const char *tmp_fmt = "%d %d %d";
	char tmp[10 * 3 + 2]; // INT_MAX 2,147,483,647 three of 10, and two '-'

	struct peakpair {
		char *hash;
		int t1;
	};

	struct tnode *root = NULL;

	for (Peak **it = peaks_ptrs; *it != NULL; it++) {
		for (int j = 1; j < DEFAULT_FAN_VALUE; j++) {
			if (*(it + j) == NULL)
				continue;
			int freq1 = (*it)->pt.freq;
			int freq2 = (*(it + j))->pt.freq;

			int t1 = (*it)->offset;
			int t2 = (*(it + j))->offset;
			int dt = t2 - t1;

			int n = sprintf(tmp, tmp_fmt, freq1, freq2, dt);

			sha1_calc(tmp, strlen(tmp), hash);
			sha1_toHexString(hash, hex_hash);

			root = addtree(root, hex_hash, t1);
		}
	}

	return root;
}

int tree_find_matches(struct tnode *root1, struct tnode *root2)
{
	static int matches = 0;

	if (root1 != NULL) {
		tree_find_matches(root1->left, root2);
		struct tnode *res_node;
		if (res_node = treefind(root2, root1->hash)) {
			printf("Found match! Time difference: %d\n", res_node->t1 - root1->t1);
			matches++;
		}
		tree_find_matches(root1->right, root2);
	}

	return matches;
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
		root = addtree(root, hex_hash, i);
	}

	printf("\n");

	treeprint(root);

	printf("Enter hash to find:\n");
	scanf("%s", hex_hash);

	printf("Search result: %d\n", treefind(root, hex_hash));

	freetree(root);
	return 0;
}
