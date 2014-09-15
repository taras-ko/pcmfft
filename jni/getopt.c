#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

struct command user_commands[10];

void parse_options(int argc, char *argv[])
{
	int c;
	int digit_optind = 0;

	struct action respond = {
		.action = NO_ACTION,
		.optarg = ""
	};

	while (1) {
		int this_option_optind = optind ? optind : 1;
		int option_index = 0;
		static struct option long_options[] = {
			{ "fingerprint", required_argument, 0,  'f' },
			{ "append",      no_argument,       0,  0 },
			{ "delete",      required_argument, 0,  0 },
			{ "verbose",     no_argument,       0,  0 },
			{ "create",      required_argument, 0, 'c'},
			{ "file",        required_argument, 0,  0 },
			{ 0,             0,                 0,  0 }
		};

		c = getopt_long(argc, argv, "f:bc:d:012", long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
			case 0:
				printf("option %s", long_options[option_index].name);
				if (optarg)
					printf(" with arg %s", optarg);
				printf("\n");
				break;

			case '0':
			case '1':
			case '2':
				if (digit_optind != 0 && digit_optind != this_option_optind)
					printf("digits occur in two different argv-elements.\n");
				digit_optind = this_option_optind;
				printf("option %c\n", c);
				break;

			case 'f':
				printf("%s\n", optarg);
				action
				break;

			case 'b':
				printf("option b\n");
				break;

			case 'c':
				printf("option c with value '%s'\n", optarg);
				break;

			case 'd':
				printf("option d with value '%s'\n", optarg);
				break;

			case '?':
				break;

			default:
				printf("?? getopt returned character code 0%o ??\n", c);
			}
		}

		if (optind < argc) {
			printf("non-option ARGV-elements: ");
			while (optind < argc)
				printf("%s ", argv[optind++]);
		printf("\n");
		}

}
