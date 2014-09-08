#include <stdio.h>
#include <string.h>

#include "sha1_api.h"

int main(int argc, char *argv[])
{
	unsigned char hash[20];
	char hex_hash[41];

	sha1_calc(argv[1], strlen(argv[1]), hash);
	sha1_toHexString(hash, hex_hash);

	printf("%s", hex_hash);
}
