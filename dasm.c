#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include "instructions.h"

struct cli_opts
{
	char *infile;
	char *outfile;
	int start;
	int len;
	bool datamode;
	bool comment;
};

void usage(char *progname)
{
	printf("usage: %s [-h] [-i infile] [-o outfile] [-s offset] [-n len] [-d] [-c]\n",
	       progname);
	printf("omit -i or -o to use stdin or stdout\n");
	printf("-d emits each word in the format .data 0xabcd ; instr\n");
	printf("-c emits each word in the format istr ; 0xabcd \n");
	printf("-h : this text\n");
}

int parse_cli(int argc, char *argv[], struct cli_opts *opts)
{
	int ch;

	opts->len = -1;

	while ((ch = getopt(argc, argv, "hi:o:s:n:dc")) != -1) {
		switch (ch) {
		case 'h': return -1;
		case 'i': opts->infile = optarg; break;
		case 'o': opts->outfile = optarg; break;
		case 's': opts->start = strtol(optarg, NULL, 0); break;
		case 'n': opts->len = strtol(optarg, NULL, 0); break;
		case 'd': opts->datamode = true; break;
		case 'c': opts->comment = true; break;
		default: break;
		}
	}

	return 0;
}

int disassemble(uint16_t instr, char *outstr, size_t len, char **newout)
{
	int opcode, subcode;
	char *syntax, *nmemonic;
	int fields[3];
	int argnum;
	int status;
	
	decode_instruction(&opcode, &subcode, &fields[0], instr);
	resolve_nmemonic(&nmemonic, opcode, subcode);
	nmemonic_syntax(&syntax, nmemonic);

	status = snprintf(outstr, len, "%s ", nmemonic);
	len -= status;
	outstr += status;

	argnum = 0;
	while(*syntax) {
		switch(*syntax) {
		case 'c':
			status = snprintf(outstr, len, "c%u", fields[argnum++]);
			len -= status;
			outstr += status;
			break;
		case 'q':
			status = snprintf(outstr, len, "q%u", fields[argnum++]);
			len -= status;
			outstr += status;
			break;
		case 'r':
			status = snprintf(outstr, len, "r%u", fields[argnum++]);
			len -= status;
			outstr += status;
			break;
		case 's':
			status = strtol(syntax+1, &syntax, 10);
			syntax--;
			status = snprintf(outstr, len, "#%d", fields[argnum++]);
			len -= status;
			outstr += status;
			break;
		case 'u':
			status = strtol(syntax+1, &syntax, 10);
			syntax--;
			status = snprintf(outstr, len, "#%u", fields[argnum++]);
			len -= status;
			outstr += status;
			break;
		case 'x':
			status = snprintf(outstr, len, "x%u", fields[argnum++]);
			len -= status;
			outstr += status;
			break;
		default:
			fprintf(stderr, "bad syntax specifier '%c' for instruction %s\n",
			        *syntax, nmemonic);
			return -1;
		}
		if(syntax[1]) {
			status = snprintf(outstr, len, ", ");
			len -= status;
			outstr += status;
		}
		syntax++;
	}

	len -= status;
	*newout = outstr;
	return 0;
}

int main(int argc, char *argv[])
{
	int status;
	struct cli_opts opts;
	uint16_t word;
	int infd, outfd;
	char outstr[256], *cursor, *limit;
	int numwords;

	limit = outstr + sizeof(outstr) - 1;
	
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
	numwords = 0;
	while(0 < (status = read(infd, &word, sizeof(word)))) {
		if(opts.start != 0) {
			--opts.start;
			continue;
		}
		if(opts.len == 0) {
			continue;
		}
		--opts.len;

		cursor = outstr;

		if(opts.datamode) {
			status = snprintf(cursor, limit-cursor, ".data 0x%4.4x", word);
			cursor += status;
		} else {
			status = disassemble(word, cursor, limit-cursor, &cursor);
			if(status) goto dis_error;
		}

		if(opts.comment) {
			if(opts.datamode) {
				status = snprintf(cursor, limit-cursor, " ; ");
				cursor += status;
				status = disassemble(word, cursor, limit-cursor, &cursor);
				if(status) goto dis_error;
			} else {
				status = snprintf(cursor, limit-cursor, " ; 0x%4.4x", word);
				cursor += status;
			}
		}

		snprintf(cursor, limit-cursor, "\n");

		write(outfd, outstr, strlen(outstr));
		numwords++;
	}

	return 0;

dis_error:
	fprintf(stderr, "error disassembling word %d\n", numwords);
	return -1;
}
