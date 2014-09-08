#ifndef SHA1_API_H
#define SHA1_API_H

#ifdef __cplusplus
#	define EXTERN extern "C"
#else
#	define EXTERN
#endif

EXTERN void sha1_calc(char *src, size_t src_sz, unsigned char *hash);
EXTERN void sha1_toHexString(const unsigned char *hash, char *hexString);

#endif //SHA1_API_H


