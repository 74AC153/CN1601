#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "opcodes.h"

struct cli_opts
{
	char *infile;
	char *outfile;
	int labels;
	int verbose;
};

struct labeldec
{
	struct labeldec *next;
	int linenum;
	char *name;
	unsigned int offset;
};
typedef struct labeldec labeldec_t;

struct linedec
{
	struct linedec *next;
	int linenum;
	char *label;
    char *op;
    char *arg1;
    char *arg2;
    char *arg3;
};
typedef struct linedec linedec_t;


/* GLOBALS */

labeldec_t *labels_start = NULL;
labeldec_t *labels_end = NULL;
linedec_t *splitlines_start = NULL;
linedec_t *splitlines_end = NULL;
#define MAX_LINES 65536
uint16_t outbuf[MAX_LINES];


void usage(char *progname)
{
	printf("usage: %s [-h] [-i infile] [-o outfile] [-l] [-v]\n", progname);
	printf("(omit -i or -o to use stdin or stdout)\n");
	printf("-h : show this text and stop.\n");
	printf("-l : print labels to stdout and stop.\n");
	printf("-v : print parse results\n");
}

int parse_cli(int argc, char *argv[], struct cli_opts *opts)
{
	int ch;

	while ((ch = getopt(argc, argv, "i:o:lvh")) != -1) {
		switch (ch) {
		case 'i':
			opts->infile = optarg;
			break;
		case 'o':
			opts->outfile = optarg;
			break;
		case 'l':
			opts->labels = 1;
			break;
		case 'v':
			opts->verbose = 1;
			break;
		case 'h':
			return -1;
		default:
			break;
		}
	}

	return 0;
}

/* returns number of chars read including sentinel */
int readline(int fd, char *buf, unsigned int bufmax, char *sentinels)
{
	char c;
	int bufoff = 0;
	int status;

	while(1) {
		status = read(fd, &c, sizeof(c));

		if(status <= 0) {
			break;
		}

		if(bufoff == bufmax - 1) {
			continue;
		}

		buf[bufoff++] = c;

		if(strchr(sentinels, c) != NULL) {
			break;
		}
	}

	buf[bufoff] = 0;
	return bufoff;
}

int issymchar(int c)
{
	return isalnum(c)
           || c == '_'
           || c == '.'
           || c == '#'
           || c == '@'
           || c == '%'
	       || c == '-'
	       || c == '+';
}

int nextsym(char *line, char **sym, int (*valid)(int c) )
{
	char *symstart, *symend, *ret;

	for(symstart = line; ! valid(*symstart) && *symstart != 0; symstart++);

	if(*symstart == 0) {
		return -1;
	}

	for(symend = symstart; valid(*symend); symend++);

	ret = calloc(1, symend - symstart + 1);
	if(!ret) { 
		return -1;
	}
	*sym = ret;

	for( ; symstart < symend; symstart++, ret++) {
		*ret = *symstart;
	}
	*ret = 0;

	return symend - line;
}

int splitline(char *line, linedec_t *split)
{
	char *cursor, *symstart, *symend;
	int status;

	/* check for label */
	if((cursor = strchr(line, ':')) != NULL) {
		status = nextsym(line, &(split->label), issymchar);
		line = cursor + 1;
	}

	/* extract op */
	status = nextsym(line, &(split->op), issymchar);
	line += status;

	/* extract arg1 */
	status = nextsym(line, &(split->arg1), issymchar);
	if(status <= 0) {
		return 0;
	}
	line += status;

	/* extract arg2 */
	status = nextsym(line, &(split->arg2), issymchar);
	if(status <= 0) {
		return 0;
	}
	line += status;

	/* extract arg3 */
	status = nextsym(line, &(split->arg3), issymchar);

	return 0;
}

void generate_labels(
	linedec_t *splitlines_start,
	labeldec_t **labels_start,
	labeldec_t **labels_end)
{
	unsigned int offset = 0;
	linedec_t *cursor;
	labeldec_t templabel, *newlabel;

	for(cursor = splitlines_start; cursor; cursor = cursor->next) {
		if(cursor->op != NULL &&
		   strcmp(cursor->op, ".org") == 0) {
			offset = strtol(cursor->arg1, NULL, 0);
			continue;
		}

		if(cursor->label == NULL && cursor->op != NULL) {
			++offset;
			continue;
		}

		memset(&templabel, 0, sizeof(labeldec_t));
		templabel.name = strdup(cursor->label);
		templabel.offset = offset;
		newlabel = malloc(sizeof(labeldec_t));
		memcpy(newlabel, &templabel, sizeof(labeldec_t));

		if(*labels_start == NULL) {
			*labels_start = *labels_end = newlabel;
		} else {
			(*labels_end)->next = newlabel;
			*labels_end = newlabel;
		}

		if(cursor->op != NULL) {
			++offset;
		}
	}
}


int find_offset(char *find, labeldec_t *labels)
{
	labeldec_t *labelcursor;
	int ret = -1;
	char label[256];
	char *dot;

	strncpy(label, find, sizeof(label));
	label[sizeof(label) - 1] = 0;

	dot = strrchr(label, '.');
	if(dot != NULL) {
		*dot = 0;
	}

	/* look for label in label list */
	for(labelcursor = labels; labelcursor; labelcursor = labelcursor->next) {
		if(strcmp(label, labelcursor->name) == 0) {
			ret = labelcursor->offset;
			break;
		}
	}

	if(ret == -1) {
		fprintf(stderr, "error: label %s not found.\n", find);
	}

	if(dot != NULL) {
		if(dot[1] == 'h') {
			ret >>= 8;
		}
		if(dot[1] == 'l') {
			ret &= 0xFF;
		}
	}

	return ret;
}

int find_reloff(size_t cur_off, char *find, labeldec_t *labels)
{
	int found = find_offset(find, labels);
	if(found == -1) {
		return MAX_LINES + 1;
	}

	return found - (int) cur_off;
}

int generate_opcodes(
	linedec_t *splitlines,
	labeldec_t *labels,
	uint16_t *outbuf,
	size_t outbuflen)
{
	linedec_t *linecursor;
	int offset, offset_max;
	char *nmemonic, *dot;
	uint16_t instruction;
	int opcode, arg1, arg2, arg3, subtype;
	optype_t type;
	int status;

	offset = offset_max = 0;

	for(linecursor = splitlines; linecursor; linecursor = linecursor->next) {
		if(linecursor->op == NULL) {
			continue;
		}

		nmemonic = linecursor->op;
		arg1 = arg2 = arg3 = 0;

		if(strcmp(nmemonic, ".data") == 0) {
			if(linecursor->arg1 == NULL || (linecursor->arg1[0] != '@' && 
                                            linecursor->arg1[0] != '#')) {
				fprintf(stderr, "error line %d: %s requires [@|#]\n",
				        linecursor->linenum, nmemonic);
				return -1;
			}

			if(linecursor->arg1[0] == '#') {
				arg1 = strtol(linecursor->arg1 + 1, NULL, 0);
			} else if(linecursor->arg1[0] == '@') {
				arg1 = find_offset(linecursor->arg1 + 1, labels);
			}

			instruction = (uint16_t) arg1;
			outbuf[offset++] = instruction;
			continue;
		}

		if(strcmp(nmemonic, ".org") == 0) {
			if(linecursor->arg1 == NULL || linecursor->arg1[0] != '#') {
				fprintf(stderr, "error line %d: %s requires #\n",
				        linecursor->linenum, nmemonic);
				return -1;
			}
			status = strtol(linecursor->arg1 + 1, NULL, 0);
			offset = status;
			if(offset > offset_max) {
				offset_max = offset;
			}
			continue;
		} 

		status = resolve_nmemonic(nmemonic, &type, &opcode, &subtype);
		if(status != 0) {
			fprintf(stderr, "error line %d: bad nmemonic: %s\n",
			        linecursor->linenum, nmemonic);
				return -1;
		}

		switch(opcode) {
		case OP_BIT:
		case OP_MATH:
		case OP_SHIFT:
			if(linecursor->arg1 == NULL || linecursor->arg1[0] != 'r' ||
			   linecursor->arg2 == NULL || linecursor->arg2[0] != 'r' ||
			   linecursor->arg3 == NULL || linecursor->arg3[0] != 'r') {
				fprintf(stderr, "error line %d: %s requires r r r \n",
				        linecursor->linenum, nmemonic);
				return -1;
			}

			arg1 = strtol(linecursor->arg1 + 1, 0, 10);
			arg2 = strtol(linecursor->arg2 + 1, 0, 10);
			arg3 = strtol(linecursor->arg3 + 1, 0, 10);

			if(!would_safe_trunc(U3, arg1) ||
			   !would_safe_trunc(U3, arg2) ||
			   !would_safe_trunc(U3, arg3)) {
				fprintf(stderr, "error line %d: value out of range\n",
				        linecursor->linenum);
				return -1;
			}

			encode_instr(opcode, arg1, arg2, arg3, subtype, &instruction);
			outbuf[offset++] = instruction;
			break;

		case OP_LI:
		case OP_SHLLI:
		case OP_INCI:
		case OP_DECI:
		case OP_ORI:
		case OP_ANDI:
		case OP_XORI:
			if(linecursor->arg1 == NULL || linecursor->arg1[0] != 'r' ||
			   linecursor->arg2 == NULL || (linecursor->arg2[0] != '@' && 
                                            linecursor->arg2[0] != '#')) {
				fprintf(stderr, "error line %d: %s requires r [@|#]\n",
				        linecursor->linenum, nmemonic);
				return -1;
			}
			arg1 = strtol(linecursor->arg1 + 1, 0, 10);

			if(linecursor->arg2[0] == '#') {
				arg2 = strtol(linecursor->arg2 + 1, NULL, 0);
			} else if(linecursor->arg2[0] == '@') {
				arg2 = find_offset(linecursor->arg2 + 1, labels);
			}

			if(!would_safe_trunc(U3, arg1) ||
			   !would_safe_trunc(U8, arg2)) {
				fprintf(stderr, "error line %d: value out of range\n",
				        linecursor->linenum);
				return -1;
			}

			encode_instr(opcode, arg1, arg2, arg3, subtype, &instruction);
			outbuf[offset++] = instruction;
			break;

		case OP_SHLI:
		case OP_SHRI:
		case OP_SHRAI:
			if(linecursor->arg1 == NULL || linecursor->arg1[0] != 'r' ||
			   linecursor->arg2 == NULL || linecursor->arg2[0] != '#') {
				fprintf(stderr, "error line %d: %s requires r #\n",
				        linecursor->linenum, nmemonic);
				return -1;
			}

			arg1 = strtol(linecursor->arg1 + 1, 0, 10);
			arg2 = strtol(linecursor->arg2 + 1, NULL, 0);

			if(!would_safe_trunc(U3, arg1) ||
			   !would_safe_trunc(U5, arg2)) {
				fprintf(stderr, "error line %d: value out of range\n",
				        linecursor->linenum);
				return -1;
			}

			encode_instr(opcode, arg1, arg2, arg3, subtype, &instruction);
			outbuf[offset++] = instruction;
			break;

		case OP_CPOP:
			if(linecursor->arg1 == NULL || linecursor->arg1[0] != '#' ||
			   linecursor->arg2 == NULL || linecursor->arg2[0] != '#') {
				fprintf(stderr, "error line %d: %s requires # #\n",
				        linecursor->linenum, nmemonic);
				return -1;
			}

			arg1 = strtol(linecursor->arg1 + 1, 0, 10);
			arg2 = strtol(linecursor->arg2 + 1, NULL, 0);

			if(!would_safe_trunc(U3, arg1) ||
			   !would_safe_trunc(U8, arg2)) {
				fprintf(stderr, "error line %d: value out of range\n",
				        linecursor->linenum);
				return -1;
			}

			encode_instr(opcode, arg1, arg2, arg3, subtype, &instruction);
			outbuf[offset++] = instruction;
			break;


		case OP_BA:
		case OP_BAL:
			if(linecursor->arg1 == NULL || (linecursor->arg1[0] != '#' &&
			                                linecursor->arg1[0] != '%')) {
				fprintf(stderr, "error line %d: %s requires r [#|%%]\n",
				        linecursor->linenum, nmemonic);
				return -1;
			}
			if(linecursor->arg1[0] == '#') {
				arg1 = strtol(linecursor->arg1 + 1, NULL, 0);
			} else if(linecursor->arg1[0] == '%') {
				arg1 = find_reloff(offset, linecursor->arg1 + 1, labels);
			}

			if(!would_safe_trunc(S11, arg1)) {
				fprintf(stderr, "error line %d: value out of range\n",
				        linecursor->linenum);
				return -1;
			}

			encode_instr(opcode, arg1, arg2, arg3, subtype, &instruction);
			outbuf[offset++] = instruction;
			break;

		case OP_BZ:
		case OP_BNZ:
			if(linecursor->arg1 == NULL || linecursor->arg1[0] != 'r' ||
			   linecursor->arg2 == NULL || (linecursor->arg2[0] != '%' && 
			                                linecursor->arg2[0] != '#')) {
				fprintf(stderr, "error line %d: %s requires r [#|%%]\n",
				        linecursor->linenum, nmemonic);
				return -1;
			}

			arg1 = strtol(linecursor->arg1 + 1, 0, 10);
			if(linecursor->arg2[0] == '#') {
				arg2 = strtol(linecursor->arg2 + 1, NULL, 0);
			} else if(linecursor->arg2[0] == '%') {
				arg2 = find_reloff(offset, linecursor->arg2+1, labels_start);
			}

			if(!would_safe_trunc(U3, arg1) ||
			   !would_safe_trunc(S8, arg2)) {
				fprintf(stderr, "error line %d: value out of range\n",
				        linecursor->linenum);
				return -1;
			}

			encode_instr(opcode, arg1, arg2, arg3, subtype, &instruction);
			outbuf[offset++] = instruction;
			break;

		case OP_LHSWP:
		case OP_JR:
		case OP_JLR:
			if(linecursor->arg1 == NULL || linecursor->arg1[0] != 'r' ||
			   linecursor->arg2 == NULL || linecursor->arg2[0] != '#') {
				fprintf(stderr, "error line %d: %s requires r #\n",
				        linecursor->linenum, nmemonic);
				return -1;
			}

			arg1 = strtol(linecursor->arg1 + 1, 0, 10);
			arg2 = strtol(linecursor->arg2 + 1, 0, 0);

			if(!would_safe_trunc(U3, arg1) ||
			   !would_safe_trunc(S8, arg2)) {
				fprintf(stderr, "error line %d: value out of range\n",
				        linecursor->linenum);
				return -1;
			}

			encode_instr(opcode, arg1, arg2, arg3, subtype, &instruction);
			outbuf[offset++] = instruction;
			break;

		case OP_LDW:
		case OP_STW:
		case OP_SC:
			if(linecursor->arg1 == NULL || linecursor->arg1[0] != 'r' ||
			   linecursor->arg2 == NULL || linecursor->arg2[0] != 'r' ||
			   linecursor->arg3 == NULL || linecursor->arg3[0] != '#') {
				fprintf(stderr, "error line %d: %s requires r r #\n",
				        linecursor->linenum, nmemonic);
				return -1;
			}

			arg1 = strtol(linecursor->arg1 + 1, NULL, 10);
			arg2 = strtol(linecursor->arg2 + 1, NULL, 10);
			arg3 = strtol(linecursor->arg3 + 1, NULL, 0);

			if(!would_safe_trunc(U3, arg1) ||
			   !would_safe_trunc(U3, arg2) ||
			   !would_safe_trunc(S5, arg3)) {
				fprintf(stderr, "error line %d: value out of range\n",
				        linecursor->linenum);
				return -1;
			}

			encode_instr(opcode, arg1, arg2, arg3, subtype, &instruction);
			outbuf[offset++] = instruction;
			break;

		case OP_MFCP:
		case OP_MTCP:
			if(linecursor->arg1 == NULL || linecursor->arg1[0] != 'r' ||
			   linecursor->arg2 == NULL || linecursor->arg2[0] != '#' ||
			   linecursor->arg3 == NULL || linecursor->arg3[0] != 'r') {
				fprintf(stderr, "error line %d: %s requires r # r\n",
				        linecursor->linenum, nmemonic);
				return -1;
			}
			arg1 = strtol(linecursor->arg1 + 1, NULL, 10);
			arg2 = strtol(linecursor->arg2 + 1, NULL, 0);
			arg3 = strtol(linecursor->arg3 + 1, NULL, 10);

			if(!would_safe_trunc(U3, arg1) ||
			   !would_safe_trunc(U3, arg2) ||
			   !would_safe_trunc(U5, arg3)) {
				fprintf(stderr, "error line %d: value out of range\n",
				        linecursor->linenum);
				return -1;
			}

			encode_instr(opcode, arg1, arg2, arg3, subtype, &instruction);
			outbuf[offset++] = instruction;
			break;

		case OP_CPOP:
			if(linecursor->arg1 == NULL || linecursor->arg1[0] != 'r' ||
			   linecursor->arg2 == NULL || linecursor->arg2[0] != '#' ||
			   linecursor->arg3 != NULL) {
				fprintf(stderr, "error line %d: %s requires r #\n",
				        linecursor->linenum, nmemonic);
				return -1;
			}
			arg1 = strtol(linecursor->arg1 + 1, NULL, 10);
			arg2 = strtol(linecursor->arg2 + 1, NULL, 0);

			if(!would_safe_trunc(U3, arg1) ||
			   !would_safe_trunc(U3, arg2)) {
				fprintf(stderr, "error line %d: value out of range\n",
				        linecursor->linenum);
				return -1;
			}

			encode_instr(opcode, arg1, arg2, arg3, subtype, &instruction);
			outbuf[offset++] = instruction;
			break;

		case OP_INT:
			if(linecursor->arg1 == NULL || linecursor->arg1[0] != '#') {
				fprintf(stderr, "error line %d: %s requires #\n",
				        linecursor->linenum, nmemonic);
				return -1;
			}

			arg1 = strtol(linecursor->arg1 + 1, NULL, 0);

			if(!would_safe_trunc(U11, arg1)) {
				fprintf(stderr, "error line %d: value out of range\n",
				        linecursor->linenum);
				return -1;
			}

			encode_instr(opcode, arg1, arg2, arg3, subtype, &instruction);
			outbuf[offset++] = instruction;
			break;

		case OP_RFI:
			encode_instr(opcode, arg1, arg2, arg3, subtype, &instruction);
			outbuf[offset++] = instruction;
			break;

		case OP_MFCTL:
		case OP_MTCTL:
			if(linecursor->arg1 == NULL || linecursor->arg1[0] != 'r' ||
			   linecursor->arg2 == NULL || linecursor->arg2[0] != 'c') {
				fprintf(stderr, "error line %d: %s requires r c\n",
				        linecursor->linenum, nmemonic);
				return -1;
			}
			arg1 = strtol(linecursor->arg1 + 1, NULL, 10);
			arg2 = strtol(linecursor->arg2 + 1, NULL, 0);
			arg3 = strtol(linecursor->arg3 + 1, NULL, 10);

			if(!would_safe_trunc(U3, arg1) ||
			   !would_safe_trunc(U3, arg2) ||
			   !would_safe_trunc(U5, arg3)) {
				fprintf(stderr, "error line %d: value out of range\n",
				        linecursor->linenum);
				return -1;
			}

			encode_instr(opcode, arg1, arg2, arg3, subtype, &instruction);
			outbuf[offset++] = instruction;
			break;
		}

		if(offset > offset_max) {
			offset_max = offset;
			if(offset_max >= MAX_LINES) {
				fprintf(stderr, "error line %d: max image length exceeded\n",
				        linecursor->linenum);
				return -1;
			}
		}
	}

	return offset_max;
}

int tokenize(int infd, linedec_t **lines_head, linedec_t **lines_tail)
{
	char *semicolon;
	char linebuf[256];
	linedec_t linetemp, *newline;
	int status;
	int linenum = 0;

	while(0 < (status = readline(infd, linebuf, sizeof(linebuf), "\n"))) {
		++linenum;

		/* strip comments */
		semicolon = strchr(linebuf, ';');
		if(semicolon) {
			*semicolon = '\0';
		}

		/* tokenize */
		memset(&linetemp, 0, sizeof(linetemp));
		splitline(linebuf, &linetemp);
		if(linetemp.op == NULL && linetemp.label == NULL) {
			continue;
		}

		/* append to splitlines */
		newline = malloc(sizeof(linedec_t));
		memcpy(newline, &linetemp, sizeof(linetemp));
		newline->linenum = linenum;
		if(splitlines_end == NULL) {
			*lines_head = *lines_tail = newline;
		} else {
			(*lines_tail)->next = newline;
			(*lines_tail) = newline;
		}
	}
	return 0;
}

int main(int argc, char *argv[])
{
	int status;
	struct cli_opts opts;
	char c, linebuf[256];
	int linebufoff = 0;
	unsigned int input_line = 1;
	linedec_t linetemp, *newline;
	size_t maxlen;
	int infd;
	int outfd;
	int i;

	memset(&opts, 0, sizeof(opts));

	/* parse cmdline opts */
	status = parse_cli(argc, argv, &opts);
	if(status) {
		usage(argv[0]);
		return status;
	}

	/* open input file */
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

	/* split each line in input into tokens. */
	status = tokenize(infd, &splitlines_start, &splitlines_end);
	if(status) {
		return -1;
	}

	if(opts.verbose) {
		printf("tokenized:\n");
		for(newline = splitlines_start;
		    newline != NULL;
		    newline = newline->next) {
			printf("line: label: %s, op: %s, arg1: %s, arg2: %s, arg3: %s\n",
		       	newline->label == NULL ? "(null)" : newline->label,
		       	newline->op == NULL ? "(null)" : newline->op,
		       	newline->arg1 == NULL ? "(null)" : newline->arg1,
		       	newline->arg2 == NULL ? "(null)" : newline->arg2,
		       	newline->arg3 == NULL ? "(null)" : newline->arg3 );
		}
	}

	/* pass 1: scan for labels */
	generate_labels(splitlines_start, &labels_start, &labels_end);

	if(opts.labels) {
		labeldec_t *labeltemp;
		for(labeltemp = labels_start;
		    labeltemp;
		    labeltemp = labeltemp->next) {
			printf("%s %d\n",
			       labeltemp->name, labeltemp->offset);
		}
		return 0;
	}

	/* pass 2: generate opcodes */
	maxlen =
	  generate_opcodes(splitlines_start, labels_start, outbuf, sizeof(outbuf));

	/* write buffer to output file */
	if(opts.outfile == NULL) {
		outfd = STDOUT_FILENO;
	} else {
		outfd = open(opts.outfile, O_WRONLY|O_CREAT, 0666);
		if(outfd < 0) {
			fprintf(stderr, "error opening otput file %s (errno=%d, %s)\n",
			        opts.outfile, errno, strerror(errno));
			return -1;
		}
	}
	write(outfd, outbuf, maxlen * sizeof(uint16_t));

	return 0;
}
