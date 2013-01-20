#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>

#include "instructions.h"

#define SGNEXTEND(VAL, TYPE, BITNUM) \
	( (TYPE) ( (VAL) << ((sizeof(VAL) * CHAR_BIT) - BITNUM) ) \
	                 >> ((sizeof(VAL) * CHAR_BIT) - BITNUM)     )

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
    char *args[3];
};
typedef struct linedec linedec_t;


/* GLOBALS */

labeldec_t *labels_start = NULL;
labeldec_t *labels_end = NULL;
linedec_t *splitlines_start = NULL;
linedec_t *splitlines_end = NULL;
#define MAX_WORDS 65536
uint16_t outbuf[MAX_WORDS];


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
	unsigned int bufoff = 0;
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
	char *cursor;
	int status;

	/* strip comment */
	if((cursor = strchr(line, ';'))) {
		*cursor = 0;
	}

	/* check for label */
	if((cursor = strchr(line, ':')) != NULL) {
		status = nextsym(line, &(split->label), issymchar);
		line = cursor + 1;
	}

	/* extract op */
	status = nextsym(line, &(split->op), issymchar);
	line += status;

	/* extract arg1 */
	status = nextsym(line, &(split->args[0]), issymchar);
	if(status <= 0) {
		return 0;
	}
	line += status;

	/* extract arg2 */
	status = nextsym(line, &(split->args[1]), issymchar);
	if(status <= 0) {
		return 0;
	}
	line += status;

	/* extract arg3 */
	status = nextsym(line, &(split->args[2]), issymchar);

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
			offset = strtol(cursor->args[0], NULL, 0);
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
		return MAX_WORDS + 1;
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
	int32_t offset, offset_max;
	char *nmemonic;
	uint16_t word;
	int32_t args[3], *args_cursor;
	int argidx;
	char *syntax, *syn_cursor;
	char **lineargs_cursor;
	int status;
	int numbits;
	
	offset = offset_max = 0;

	for(linecursor = splitlines; linecursor; linecursor = linecursor->next) {
		if(linecursor->op == NULL) {
			continue;
		}

		nmemonic = linecursor->op;

		if(strcmp(nmemonic, ".data") == 0) {
			if(linecursor->args[0] == NULL || (linecursor->args[0][0] != '@' && 
                                            linecursor->args[0][0] != '#')) {
				fprintf(stderr, "error line %d: %s requires [@|#|%%]\n",
				        linecursor->linenum, nmemonic);
				return -1;
			}

			if(linecursor->args[0][0] == '#') {
				word = strtol(linecursor->args[0]+1, NULL, 0);
			} else if(linecursor->args[0][0] == '@') {
				word = find_offset(linecursor->args[0] + 1, labels);
			} else if(linecursor->args[0][0] == '%') {
				word = find_reloff(offset, linecursor->args[0] + 1, labels);
			}

			outbuf[offset++] = word;
			goto offset_max_update;
		}

		if(strcmp(nmemonic, ".org") == 0) {
			if(linecursor->args[0] == NULL) {
				fprintf(stderr,
				        "error line %d: %s requires  addr\n",
				        linecursor->linenum, nmemonic);
				return -1;
			}
			status = strtol(linecursor->args[0], NULL, 0);
			offset = status;
			goto offset_max_update;
		} 

		nmemonic_syntax(&syntax, nmemonic);
		
		lineargs_cursor = &linecursor->args[0];
		syn_cursor = syntax;
		args_cursor = &args[0];

		argidx = 0;
		while(*syn_cursor) {
			assert(argidx < 3);

			switch(*syn_cursor) {
			case 'c':
#define SYN_TESTMISMATCH_FAIL(EXPECT) \
	do { \
		if(lineargs_cursor[argidx][0] != (EXPECT)) { \
			fprintf(stderr, \
			        "error line %d: syntax match failed %s (got arg %d = %c want %c)\n", \
			        linecursor->linenum, syntax, argidx, \
		            lineargs_cursor[argidx][0], (EXPECT)); \
			return -1; \
		} \
	} while(0);
				SYN_TESTMISMATCH_FAIL('c');
				*args_cursor = strtol(lineargs_cursor[argidx]+1, NULL, 10);
				if(*args_cursor > 0x7) {
					fprintf(stderr,
					        "error line %d: coproc num too large: c%d\n",
			                linecursor->linenum, *args_cursor);
							return -1;
				}
				syn_cursor++;
				break;
			case 'q':
				SYN_TESTMISMATCH_FAIL('q');
				*args_cursor = strtol(lineargs_cursor[argidx]+1, NULL, 10);
				if(*args_cursor > 0x1F) {
					fprintf(stderr,
					        "error line %d: ctl reg num too large: q%d\n",
			                linecursor->linenum, *args_cursor);
							return -1;
				}
				syn_cursor++;
				break;
			case 'r':
				SYN_TESTMISMATCH_FAIL('r');
				*args_cursor = strtol(lineargs_cursor[argidx]+1, NULL, 10);
				if(*args_cursor > 0x7) {
					fprintf(stderr,
					        "error line %d: gp reg num too large: r%d\n",
			                linecursor->linenum, *args_cursor);
							return -1;
				}
				syn_cursor++;
				break;
			case 's':
				numbits = strtol(syn_cursor + 1, &syn_cursor, 10);
				if(lineargs_cursor[argidx][0] == '%') {
					*args_cursor = find_reloff(offset,
					                           lineargs_cursor[argidx]+1,
					                           labels);
				} else if(lineargs_cursor[argidx][0] == '#') {
					*args_cursor = strtol(lineargs_cursor[argidx]+1, NULL, 0);
					/* FIXME: check for oversize values */
				} else {
					fprintf(stderr,
			                "error line %d: syntax match failed %s "
					        "(arg %d = %c)\n",
			                linecursor->linenum, syntax,
					        argidx, lineargs_cursor[argidx][0]);
					return -1;
				}
				if(*args_cursor > (0x1 << (numbits-1)) - 1 ||
				   *args_cursor < -(0x1 << (numbits-1))) {
					fprintf(stderr, "error line %d: immed overflow (0x%x)\n",
					        linecursor->linenum, *args_cursor);
					return -1;
				} 
				break;
			case 'u':
				numbits = strtol(syn_cursor + 1, &syn_cursor, 10);
				if(lineargs_cursor[argidx][0] == '@') {
					*args_cursor = find_offset(lineargs_cursor[argidx]+1,
					                           labels);
				} else if(lineargs_cursor[argidx][0] == '#') {
					*args_cursor = strtol(lineargs_cursor[argidx]+1, NULL, 0);
				} else {
					fprintf(stderr,
			                "error line %d: syntax match failed %s "
					        "(arg %d = %c)\n",
			                linecursor->linenum, syntax,
					        argidx, lineargs_cursor[argidx][0]);
					return -1;
				}
				if(*args_cursor >= 0x1 << numbits) {
					fprintf(stderr, "error line %d: immed overflow (0x%x)\n",
					        linecursor->linenum, *args_cursor);
					return -1;
				} 
				break;
			case 'x':
				SYN_TESTMISMATCH_FAIL('x');
				*args_cursor = strtol(lineargs_cursor[argidx] + 1, NULL, 10);
				if(*args_cursor > 0x1F) {
					fprintf(stderr,
					        "error line %d: cp reg num too large: x%d\n",
			                linecursor->linenum, *args_cursor);
							return -1;
				}
				syn_cursor++;
				break;
			default:
				fprintf(stderr, "error: bad syntax specifier %s\n", syntax);
				return -1;
				break;
			}
			//lineargs_cursor++;
			argidx++;
			args_cursor++;
		}
#undef SYN_MISMATCH
		if(assemble_instruction(&outbuf[offset++], nmemonic, args)) {
			fprintf(stderr, "error line %d: unknown nmemonic %s\n",
			        linecursor->linenum, nmemonic);
			return -1;
		}

offset_max_update:
		if(offset > offset_max) {
			offset_max = offset;
			if(offset_max >= MAX_WORDS) {
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
	linedec_t *newline;
	int maxlen;
	int infd;
	int outfd;

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
		       	newline->args[0] == NULL ? "(null)" : newline->args[0],
		       	newline->args[1] == NULL ? "(null)" : newline->args[1],
		       	newline->args[2] == NULL ? "(null)" : newline->args[2] );
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
	if(maxlen < 0) {
		return -1;
	}

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
