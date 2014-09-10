#ifndef DEJAVU_H
#define DEJAVU_H

typedef struct {
	char *hash;
	int t1;
} HashBlob;

typedef struct {
	int freq; // frequency
	float mag; // magnitude
	int offset; // offset from the beginning of a file
	HashBlob fp; // sound fingerprint
} SoundPixel;

// Degree to which a fingerprint can be paired with its neighbors --
// higher will cause more fingerprints, but potentially better accuracy. 
#define DEFAULT_FAN_VALUE 15

#endif //DEJAVU_H
