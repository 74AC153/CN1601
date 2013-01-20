#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "opcodes.h"

#define NUM_GPREGISTERS 8 /* see enum regid def */
#define NUM_COPROCESSORS 8
#define NUM_CPREGISTERS 32
#define PHYSMEM_WORDS 65536

struct cpstate
{
	uint16_t reg[NUM_CPREGISTERS];
};

struct fetch_inargs
{
	uint16_t pc;
};

struct fetch_outargs
{
	uint16_t instruction;
};

struct decode_inargs
{
	uint16_t instruction;
};

struct decode_outargs
{
	optype_t type;
	int op, arg1, arg2, arg3, subtype;
};

struct exec_inargs
{
	optype_t type;
	int op, arg1, arg2, arg3, subtype;
};

enum memop { NONE, LOAD, STORE };

struct exec_outargs
{
	uint16_t pc;
	enum memop op;
	int reg;
	uint16_t addr;
};

struct memio_inargs
{
	enum memop op;
	int reg;
	uint16_t addr;
};

struct cpustate
{
	uint16_t reg[NUM_GPREGISTERS];
	uint16_t PC;
	uint16_t OF;
	int LL; /* load linked valid bit */
	int USR; /* in user mode */

	struct fetch_inargs fetch_in;
	struct fetch_outargs fetch_out;

	struct decode_inargs decode_in;
	struct decode_outargs decode_out;

	struct exec_inargs exec_in;
	struct exec_outargs exec_out;

	struct memio_inargs memio_in;

	uint16_t pmem[PHYSMEM_WORDS];
};
typedef struct cpustate cpustate_t;

struct cli_opts
{
	char *infile;
	char *outfile;
	int trace;
	int mem;
	int rtrace;
	int regdump;
};

struct cli_opts opts;

void usage(char *progname)
{
	printf("usage: %s -i image [-o memdump] [-r] [-t] [-u] [-m]\n", progname);
	printf("-h : this text\n");
	printf("-r : print registers when execution completes\n");
	printf("-t : trace instruction word offsets during exec\n");
	printf("-u : print register writes during execution\n");
	printf("-m : print loads and stores during exec\n");
}

int parse_cli(int argc, char *argv[], struct cli_opts *opts)
{
	int ch;

	while((ch = getopt(argc, argv, "i:o:tumrh")) != -1) {
		switch(ch) {
		case 'i': opts->infile = optarg; break;
		case 'o': opts->outfile = optarg; break;
		case 't': opts->trace = 1; break;
		case 'u': opts->rtrace = 1; break;
		case 'm': opts->mem = 1; break;
		case 'h': return -1;
		case 'r': opts->regdump = 1; break;
		default: break;
		}
	}

	if(opts->infile == NULL) {
		return -1;
	}

	return 0;
}

int write_mem(cpustate_t *s, size_t loc, uint16_t srcval)
{
	if(opts.mem) {
		printf("MEM W loc %x val %x\n", loc, srcval);
	}
	s->pmem[loc] = srcval;

	return 0;
}

int read_mem(cpustate_t *s, size_t loc, uint16_t *dstval)
{
	*dstval = s->pmem[loc];
	if(opts.mem) {
		printf("MEM R loc %x val %x\n", loc, *dstval);
	}

	return 0;
}

int write_gpreg(cpustate_t *s, int reg, uint16_t val)
{
	if(opts.rtrace) {
		printf("WREG %d %x\n", reg, val);
	}

	s->reg[reg] = val;

	return 0;
}

int read_gpreg(cpustate_t *s, int reg, uint16_t *val)
{
	*val = s->reg[reg];

	return 0;
}

/*
struct exec_inargs
{
	int op, arg1, arg2, arg3, subtype;
};

enum memop { NONE, LOAD, STORE };

struct exec_outargs
{
	uint16_t pc;
	enum memop op;
	int write_reg;
	uint16_t write_addr;
	uint16_t write_value;
};
*/
int stage_exec(struct exec_inargs *in, exec_outargs *out)
{
	switch(in->op) {
	case OP_BIT:
		switch(subtype) {
		case 0: /* or */
			break;
		case 1: /* and */
			break;
		case 2: /* nor */
			break;
		case 3: /* xor */
			break;
		}
		break;
	case OP_MATH:
		switch(subtype) {
		case 0: /* add */
			break;
		case 1: /* sub */
			break;
		case 2: /* mul */
			break;
		case 3: /* div */
			break;
		}
		break;
	case OP_UMATH:
		switch(subtype) {
		case 0: /* uadd */
			break;
		case 1: /* usub */
			break;
		case 2: /* umul */
			break;
		case 3: /* udiv */
			break;
		}
		//ureg[arg1] = uz & 0xFFFF;
		write_gpreg(s, arg1, uz & 0xFFFF);
		break;
	case OP_SHIFT:
		switch(subtype) {
		case 0: /* shl */
			break;
		case 1: /* shr */
			break;
		case 2: /* shra */
			break;
		case 3: /* wshr */
			break;
		}
		break;
	case OP_INCI:
		break;
	case OP_DECI:
		break;
	case OP_ANDI:
		break;
	case OP_ORI:
		break;
	case OP_XORI:
		break;
	case OP_LUI:
		break;
	case OP_CPOP: /* TODO implement this */
		break;
	case OP_SHLI:
		break;
	case OP_SHRI:
		break;
	case OP_SHRAI:
		break;
	case OP_WSHRI:
		break;
	case OP_BA:
		break;
	case OP_BAL:
		break;
	case OP_BZ:
		break;
	case OP_BNZ:
		break;
	case OP_JR:
		break;
	case OP_JLR:
		break;
	case OP_LDW:
		break;
	case OP_STW:
		break;
	case OP_LL:
		break;
	case OP_SC:
		break;
	case OP_LDCP: /* TODO implement this */
		break;
	case OP_STCP: /* TODO implement this */
		break;
	case OP_INT: /* TODO implement this */
		break;
	case OP_BRK: /* TODO implement this */
		break;
	case OP_RFI: /* TODO implement this */
		break;
	}

}

void exec_op(int op, int arg1, int arg2, int arg3, int subtype, cpustate_t *s)
{
	int32_t x, y, z;
	uint32_t ux, uy, uz;
	uint16_t uhz;
	int16_t *reg;
	uint16_t *ureg;

	uint16_t nextPC = s->PC + 1;

	reg = (int16_t *) s->reg;
	ureg = s->reg;

	switch(op) {
	case OP_BIT:
		switch(subtype) {
		case 0: /* or */
			//ureg[arg1] =   ureg[arg2] | ureg[arg3];
			write_gpreg(s, arg1, ureg[arg2] | ureg[arg3]);
			break;
		case 1: /* and */
			//ureg[arg1] =   ureg[arg2] & ureg[arg3];
			write_gpreg(s, arg1, ureg[arg2] & ureg[arg3]);
			break;
		case 2: /* nor */
			//ureg[arg1] = ~(ureg[arg2] | ureg[arg3]);
			write_gpreg(s, arg1, ~(ureg[arg2] | ureg[arg3]));
			break;
		case 3: /* xor */
			//ureg[arg1] =   ureg[arg2] ^ ureg[arg3];
			write_gpreg(s, arg1, ureg[arg2] ^ ureg[arg3]);
			break;
		}
		break;
	case OP_MATH:
		x = reg[arg2];
		y = reg[arg3];
		switch(subtype) {
		case 0: /* add */
			z = x + y;
			s->OF = z >> 16;
			break;
		case 1: /* sub */
			z = x - y;
			s->OF = z >> 16;
			break;
		case 2: /* mul */
			z = x * y;
			s->OF = z >> 16;
			break;
		case 3: /* div */
			z = x / y;
			s->OF = x % y;
			break;
		}
		//ureg[arg1] = z & 0xFFFF;
		write_gpreg(s, arg1, z & 0xFFFF);
		break;
	case OP_UMATH:
		ux = ureg[arg2];
		uy = ureg[arg3];
		switch(subtype) {
		case 0: /* uadd */
			uz = ux + uy;
			s->OF = uz >> 16;
			break;
		case 1: /* usub */
			uz = ux - uy;
			s->OF = uz >> 16;
			break;
		case 2: /* umul */
			uz = ux * uy;
			s->OF = uz >> 16;
			break;
		case 3: /* udiv */
			uz = ux / uy;
			s->OF = ux % uy;
			break;
		}
		//ureg[arg1] = uz & 0xFFFF;
		write_gpreg(s, arg1, uz & 0xFFFF);
		break;
	case OP_SHIFT:
		switch(subtype) {
		case 0: /* shl */
			uz = ureg[arg2] << ureg[arg3];
			//ureg[arg1] = uz & 0xFFFF;
			write_gpreg(s, arg1, uz & 0xFFFF);
			s->OF = uz >> 16;
			break;
		case 1: /* shr */
			uz = ureg[arg2] << 16;
			uz >>= ureg[arg3];
			//ureg[arg1] = uz >> 16;
			write_gpreg(s, arg1, uz >> 16);
			s->OF = uz & 0xFFFF;
			break;
		case 2: /* shra */
			z = reg[arg2] << 16;
			z >>= ureg[arg3] & 0x1F;
			//ureg[arg1] = z >> 16;
			write_gpreg(s, arg1, z >> 16);
			s->OF = z & 0xFFFF;
			break;
		case 3: /* wshr */
			uz = s->OF << 16;
			uz |= ureg[arg2];
			uz >>= ureg[arg3] & 0x1F;
			//ureg[arg1] = uz & 0xFFFF;
			write_gpreg(s, arg1, uz & 0xFFFF);
			s->OF = uz >> 16;
			break;
		}
		break;
	case OP_INCI:
		//reg[arg1] += arg2;
		write_gpreg(s, arg1, reg[arg1] + arg2);
		break;
	case OP_DECI:
		//reg[arg1] -= arg2;
		write_gpreg(s, arg1, reg[arg1] - arg2);
		break;
	case OP_ANDI:
		//reg[arg1] &= arg2;
		write_gpreg(s, arg1, reg[arg1] & arg2);
		break;
	case OP_ORI:
		//reg[arg1] |= arg2;
		write_gpreg(s, arg1, reg[arg1] | arg2);
		break;
	case OP_XORI:
		//reg[arg1] ^= arg2;
		write_gpreg(s, arg1, reg[arg1] ^ arg2);
		break;
	case OP_LUI:
		//reg[arg1] = (arg2 & 0xFF) << 8;
		write_gpreg(s, arg1, (arg2 & 0xFF) << 8);
		break;
	case OP_CPOP: /* TODO implement this */
		if(s->USR) {
			/* ACCESS */
		}
		break;
	case OP_SHLI:
		uz = ureg[arg1];
		uz <<= arg2 & 0x1F;
		//ureg[arg1] = uz & 0xFFFF;
		write_gpreg(s, arg1, uz & 0xFFFF);
		s->OF = uz >> 16;
		break;
	case OP_SHRI:
		uz = ureg[arg1] << 16;
		uz >>= arg2 & 0x1F;
		//ureg[arg1] = uz >> 16;
		write_gpreg(s, arg1, uz >> 16);
		s->OF = uz & 0xFFFF;
		break;
	case OP_SHRAI:
		z = reg[arg1] << 16;
		z >>= arg2 & 0x1F;
		//ureg[arg1] = z >> 16;
		write_gpreg(s, arg1, z >> 16);
		s->OF = z & 0xFFFF;
		break;
	case OP_WSHRI:
		uz = s->OF << 16;
		uz |= ureg[arg1];
		uz >>= arg2 & 0x1F;
		//ureg[arg1] = uz & 0xFFFF;
		write_gpreg(s, arg1, uz & 0xFFFF);
		s->OF = uz >> 16;
		break;
	case OP_BA:
		nextPC += arg1 - 1;
		break;
	case OP_BAL:
		//reg[7] = s->PC + 1;
		write_gpreg(s, 7, s->PC + 1);
		nextPC += arg1 - 1;
		break;
	case OP_BZ:
		if(reg[arg1] == 0) {
			nextPC += arg2 - 1;
		}
		break;
	case OP_BNZ:
		if(reg[arg1] != 0) {
			nextPC += arg2 - 1;
		}
		break;
	case OP_JR:
		nextPC = reg[arg1] + arg2;
		break;
	case OP_JLR:
		//ureg[7] = s->PC + 1;
		write_gpreg(s, 7, s->PC + 1);
		nextPC = reg[arg1] + arg2;
		break;
	case OP_LDW:
		//read_mem(s, reg[arg2] + arg3, &(ureg[arg1]));
		read_mem(s, reg[arg2] + arg3, &uhz);
		write_gpreg(s, arg1, uhz);
		break;
	case OP_STW:
		s->LL = 0;
		write_mem(s, reg[arg2] + arg3, ureg[arg1]);
		break;
	case OP_LL:
		s->LL = 1;
		//read_mem(s, reg[arg2] + arg3, &(ureg[arg1]));
		read_mem(s, reg[arg2] + arg3, &uhz);
		write_gpreg(s, arg1, uhz);
		break;
	case OP_SC:
		if(s->LL != 0) {
			write_mem(s, reg[arg2] + arg3, ureg[arg1]);
		}
		s->LL = 0;
		break;
	case OP_LDCP: /* TODO implement this */
		if(s->USR) {
			/* ACCESS */
		}
		break;
	case OP_STCP: /* TODO implement this */
		if(s->USR) {
			/* ACCESS */
		}
		break;
	case OP_INT: /* TODO implement this */
		//ureg[7] = s->PC;
		write_gpreg(s, 7, s->PC);
		s->USR = 0;
		/* SWINT */
		break;
	case OP_BRK: /* TODO implement this */
		/* BRK */
		break;
	case OP_RFI: /* TODO implement this */
		if(s->USR) {
			/* ACCESS */
		}
		s->PC = ureg[7];
		s->USR = 1;
		break;
	}

	s->PC = nextPC;
}

void printf_registers(cpustate_t *s)
{
	int i;

	printf("PC: 0x%4.4x (%d)\n", s->PC, s->PC);
	for(i = 0; i < NUM_GPREGISTERS; i++) {
		printf("r%d: 0x%4.4x (%d, %d)\n",
		i, s->reg[i], s->reg[i], ((int16_t *)s->reg)[i]);
	}
	printf("OF: 0x%4.4x (%d)\n", s->OF, s->OF);
	printf("LL: %d\n", s->LL, s->LL);
	printf("USR: %d\n", s->USR, s->USR);
}

int fetch(struct cpustate *s)
{
	read_mem(s, s->fetch_in.pc, &s->fetch_out.instruction);
	return 0;
}

int decode(struct cpustate *s)
{
	optype_t t;
/*
	decode_instr(s->decode_in.instruction,
	             &t,
	             &s->decode_out.op,
	             &s->decode_out.arg1,
	             &s->decode_out.arg2,
	             &s->decode_out.arg3,
	             &s->decode_out.subtype);
*/
	return 0;
}

int exec(struct cpustate *s)
{
/*
	exec_op(op,
	        arg1,
	        arg2,
	        arg3,
	        subtype,
	        &state);
*/
	return 0;
}

int memio(struct cpustate *s)
{
	return 0;
}

int main(int argc, char *argv[])
{
	cpustate_t state;
	int status;
	int infd;
	int outfd;

	uint16_t instruction;
    optype_t type;
    int op, arg1, arg2, arg3, subtype;

	memset(&state, 0, sizeof(state));
	memset(&opts, 0, sizeof(opts));

	/* parse command line */
	status = parse_cli(argc, argv, &opts);
	if(status != 0) {
		usage(argv[0]);
		return status;
	}
	
	/* load binary image into memory of state */
	infd = open(opts.infile, O_RDONLY);
	if(infd < 0) {
		fprintf(stderr, "error opening image %s (errno=%d, %s)\n",
		        opts.infile, errno, strerror(errno));
		return -1;
	}
	read(infd, state.pmem, sizeof(state.pmem));

	/* run until we reach end-of-program */
	while(state.PC != 0xFFFF) {
		if(opts.trace) {
			printf("%d\n", state.PC);
		}

		/* fetch */
		instruction = state.pmem[state.PC];

		/* decode + execute */

		decode_instr(instruction, &type, &op, &arg1, &arg2, &arg3, &subtype);

		exec_op(op, arg1, arg2, arg3, subtype, &state);

		/* memory i/o... ? */
		//memio();
	}

	/* print register state */
	if(opts.regdump) {
		printf_registers(&state);
	}

	/* dump memory image */
	if(opts.outfile != NULL) {
		outfd = open(opts.outfile, O_CREAT | O_WRONLY, 0666);
		if(outfd < 0) {
			fprintf(stderr, "error opening %s for memdump (errno=%d, %s)\n",
		        	opts.outfile, errno, strerror(errno));
		}
		write(outfd, state.pmem, sizeof(state.pmem));
	}

	return 0;
}
