#ifndef GETOPT_H
#define GETOPT_H

enum Commands {
	FINGERPRINT,
	NO_ACTION
};

struct command {
	enum Command what;
	char *optarg;
};

void parse_options(int argc, char *argv[]);

#endif //GETOPT_H
