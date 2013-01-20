#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "opcodes.h"

struct cli_opts
{
	char *infile;
	char *outfile;
	int start;
	int len;
};

void usage(char *progname)
{
	printf("usage: %s [-h] [-i infile] [-o outfile] [-s offset] [-n len]\n",
	       progname);
	printf("omit -i or -o to use stdin or stdout\n");
	printf("-h : this text\n");
}

int parse_cli(int argc, char *argv[], struct cli_opts *opts)
{
	int ch;

	opts->len = -1;

	while ((ch = getopt(argc, argv, "hi:o:s:n:")) != -1) {
		switch (ch) {
		case 'h':
			return -1;
		case 'i':
			opts->infile = optarg;
			break;
		case 'o':
			opts->outfile = optarg;
			break;
		case 's':
			opts->start = strtol(optarg, NULL, 0);
			break;
		case 'n':
			opts->len = strtol(optarg, NULL, 0);
			break;
		default:
			break;
		}
	}

	return 0;
}

void disassemble(uint16_t op, char *outstr, size_t len)
{
	unsigned int opcode;
	int arg1;
	int arg2;
	int arg3;
	unsigned int subcode;
	char *nmemonic;
	optype_t type;

	decode_instr(op, &type, &opcode, &arg1, &arg2, &arg3, &subcode);
	resolve_opcode(opcode, subcode, &nmemonic, NULL);

	switch(type) {
		case OP3:
			snprintf(outstr, len,
			         "%s r%d, r%d, r%d; 0x%4.4x\n",
			         nmemonic, arg1, arg2, arg3, op);
			break;
		case OP2U:
		case OP2S:
			snprintf(outstr, len, 
                     "%s r%d, r%d, #%d; 0x%4.4x\n",
			         nmemonic, arg1, arg2, arg3, op);
			break;
		case OP1U:
		case OP1S:
			snprintf(outstr, len, "%s r%d, #%d; 0x%4.4x\n",
			         nmemonic, arg1, arg2, op);
			break;
		case OP0U:
		case OP0S:
			snprintf(outstr, len, "%s #%d; 0x%4.4x\n",
			         nmemonic, arg1, op);
			break;
	}
}

int main(int argc, char *argv[])
{
	int status;
	struct cli_opts opts;
	uint16_t instruction;
	int infd, outfd;
	char outstr[256];
	
	memset(&opts, 0, sizeof(opts));

	/* parse cmdline opts */
	status = parse_cli(argc, argv, &opts);
	if(status) {
		usage(argv[0]);
		return status;
	}

	/* open input and output files */
	if(opts.infile == NULL) {
		infd = STDIN_FILENO;
	} else {
		infd = open(opts.infile, O_RDONLY);
		if(infd < 0) {
			fprintf(stderr, "error opening input file %s (errno=%d, %s)\n",
			        opts.infile, errno, strerror(errno));
			return -1;
		}
	}

	if(opts.outfile == NULL) {
		outfd = STDOUT_FILENO;
	} else {
		outfd = open(opts.outfile, O_WRONLY|O_CREAT, 0666);
		if(outfd < 0) {
			fprintf(stderr, "error opening output file %s (errno=%d, %s)\n",
			        opts.outfile, errno, strerror(errno));
			return -1;
		}
	}

	/* decode and output */
	while(0 < (status = read(infd, &instruction, sizeof(instruction)))) {
		if(opts.start != 0) {
			--opts.start;
			continue;
		}
		if(opts.len == 0) {
			continue;
		}
		--opts.len;
		disassemble(instruction, outstr, sizeof(outstr));
		write(outfd, outstr, strlen(outstr));
	}

	return 0;
}
