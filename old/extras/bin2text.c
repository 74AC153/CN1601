#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

/* out must be at least 17 characters long */
void ui16_to_bintext(uint16_t in, char *out)
{
	int i;

	for(i = 15; i >= 0; i--) {
		if(in & 0x1 << i) {
			*out++ = '1';
		} else {
			*out++ = '0';
		}
	}
	*out = '\0';
}

int main(int argc, char *argv[])
{
	char *infile, *outfile;
	int infd, outfd;
	uint16_t offset, word;
	int status;
	char binstr[17];

	infile = argv[1];
	outfile = argv[2];

	if(infile == NULL || outfile == NULL) {
		fprintf(stderr, "usage: %s infile outfile\n", argv[0]);
		fprintf(stderr, "(use '-' for stdin or stdout)\n", argv[0]);
		return -1;
	}

	if(strcmp(infile, "-") == 0) {
		infd = STDIN_FILENO;
	} else {
		infd = open(infile, O_RDONLY);
		if(infd < 0) {
			fprintf(stderr, "error opening input file %s (errno=%d, %s)\n",
			        infile, errno, strerror(errno));
			return -1;
		}
	}

	if(strcmp(outfile, "-") == 0) {
		outfd = STDOUT_FILENO;
	} else {
		outfd = open(outfile, O_WRONLY);
		if(outfd < 0) {
			fprintf(stderr, "error opening output file %s (errno=%d, %s)\n",
			        outfile, errno, strerror(errno));
			return -1;
		}
	}

	for(offset = 0; offset < 65535; offset++) {
		status = read(infd, &word, sizeof(word));
		if(status <= 0) {
			break;
		}
		ui16_to_bintext(offset, binstr);
		write(outfd, binstr, 16);
		write(outfd, " ", 1);
		ui16_to_bintext(word, binstr);
		write(outfd, binstr, 16);
		write(outfd, "\n", 1);
	}

}
