#include <cstdlib>
#include "sha1.h"

#include "sha1_api.h"

EXTERN void sha1_calc(char *src, size_t src_sz, unsigned char *hash)
{
	sha1::calc(src, src_sz, hash);
}

EXTERN void sha1_toHexString(const unsigned char *hash, char *hexString)
{
	sha1::toHexString(hash, hexString);
}
