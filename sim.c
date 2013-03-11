#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "sim_core.h"
#include "sim_memif.h"
#include "sim_cp_timer.h"
#include "sim_cp_nvram.h"
#include "sim_cp_uart.h"
#include "utils.h"

/******************************************
 * core simulator state structures & data *
 ******************************************/
sim_core_state_t core_state;
sim_core_output_t core_output;
sim_core_input_t core_input;

/**********************************
 * memory state structures & data *
 **********************************/
sim_memif_state_t dmem_state;
sim_memif_state_t imem_state;

#define PHYSMEM_NUMWORDS 65535
uint16_t physmem[PHYSMEM_NUMWORDS];

sim_cp_timer_state_t timer_state;
sim_cp_nvram_state_t nvram_state;
sim_cp_uart_state_t uart_state;

bool g_continue = true;

void unset_run_sim(int arg)
{
	arg = arg;
	printf("stopped\n");
	signal(SIGINT, SIG_DFL);
	g_continue = false;
}

sim_cp_info_t coproc_info[] = {
	{
		"timer",
		"(no argugments)",
		(sim_cp_state_hdr_t *) &timer_state, 
		NULL, /*init*/
		NULL, /*deinit*/
		NULL, /*reset*/
		timer_state_data,
		timer_state_exec,
		NULL, /*fetch*/
		timer_state_print
	},
	{
		"nvram",
		"-i <datafile>",
		(sim_cp_state_hdr_t *) &nvram_state, 
		nvram_state_init,
		nvram_state_deinit,
		nvram_state_reset,
		nvram_state_data,
		nvram_state_exec,
		NULL, /*fetch*/
		nvram_state_print
	},
	{
		"uart",
		"-r <rxfifo> -t <txfifo>",
		(sim_cp_state_hdr_t *) &uart_state, 
		uart_state_init,
		uart_state_deinit,
		uart_state_reset,
		uart_state_data,
		uart_state_exec,
		NULL, /*fetch*/
		uart_state_print
	}
};
/* FIXME: change this to NUM_COPROCS */
#define ARRLEN(ARR) (sizeof(ARR) / sizeof(ARR[0]))

typedef struct {
	char *infile;
	bool debug;
	int startaddr;
	int loadoff;
	char *cparg[8];
} cli_args_t;

void usage(char *progname)
{
	unsigned int i;
	printf("usage: %s [-i <img>] [-o <off>] [-s <start>] [-d] [-# <args>]\n", 
	       progname);
	printf("<img>: initial memory image, if not specified, -d implied\n");
	printf("<off>: load offset for <img>, defaults to 0\n");
	printf("<start>: starting PC address, defaults to 0\n");
	printf("-d: start in command mode\n");
	printf("-# <args>: # from 0..%d, <args> comma delimited with no spaces\n",
	       (int) ARRLEN(coproc_info) - 1);
	for(i = 0; i < ARRLEN(coproc_info); i++) {
		printf("coproc %d (%s) usage: %s\n",
		       i, coproc_info[i].name, coproc_info[i].usage);
	}
	
}

int parse_cli(int argc, char *argv[], cli_args_t *args)
{
	int ch;
	memset(args, 0, sizeof(*args));

	while((ch = getopt(argc, argv, "i:o:s:d0:1:2:3:4:5:6:7:")) != -1) {
		switch(ch) {
		case 'i': args->infile = optarg; break;
		case 'o': args->loadoff = strtol(optarg, NULL, 0); break;
		case 's': args->startaddr = strtol(optarg, NULL, 0); break;
		case 'd': args->debug = true; break;
		case '0': 
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7': args->cparg[ch - '0'] = optarg; break;
		default:
			printf("error: unknown switch %c\n", ch);
			usage(argv[0]);
			return -1;
		}
	}

	if(!args->infile) {
		args->debug = true;
	}
	return 0;
}

void print_state(sim_core_state_t *state)
{
	int i;
	printf("pc=%4.4x\n", state->pc);
	for(i = SIM_CORE_NUM_GPREGS-1; i >= 0; i--)
		printf("s%d=%4.4x ", i, state->gpr_super[i]);	
	printf("\n");
	for(i = SIM_CORE_NUM_GPREGS-1; i >= 0; i--)
		printf("r%d=%4.4x ", i, state->ctl.named.gpr_user[i]);	
	printf("\n");
	printf("epc=%4.4x epc_s=%4.4x\n",
	       state->ctl.named.epc, state->ctl.named.epc_saved);
	printf("swiarg=%4.4x ru_s=%d ll=%d ru=%d gie=%d um=%d\n",
	       STATUS_GET_SWIARG(state->ctl.named.status),
	       STATUS_GET_UMSAVE(state->ctl.named.status),
	       STATUS_GET_LL(state->ctl.named.status),
	       STATUS_GET_RUMODE(state->ctl.named.status),
	       STATUS_GET_GIE(state->ctl.named.status),
	       STATUS_GET_UM(state->ctl.named.status));
	printf("ifhdl=%4.4x dfhdl=%4.4x swihdl=%4.4x illhdl=%4.4x prvhdl=%4.4x\n",
	       state->ctl.named.ifaulthdl, state->ctl.named.dfaulthdl,
	       state->ctl.named.swihdl, state->ctl.named.illophdl,
	       state->ctl.named.privophdl);
	printf("exinthdl[%d..0]: ", SIM_CORE_NUM_EXTINT-1);
	for(i = SIM_CORE_NUM_EXTINT-1; i >= 0; i--)
		printf("%4.4x ", state->ctl.named.exihdl[i]);
	printf("\n");
	printf(" ienable[%d..0]: ", SIM_CORE_NUM_EXTINT-1);
	for(i = SIM_CORE_NUM_EXTINT-1; i >= 0; i--)
		printf("%d ", (state->ctl.named.ienable & (0x1 << i)) != 0);
	printf("\n");
	printf("ipending[%d..0]: ", SIM_CORE_NUM_EXTINT-1);
	for(i = SIM_CORE_NUM_EXTINT-1; i >= 0; i--)
		printf("%d ", (state->ctl.named.ipend & (0x1 << i)) != 0);
	printf("\n");
	printf("    iack[%d..0]: ", SIM_CORE_NUM_EXTINT-1);
	for(i = SIM_CORE_NUM_EXTINT-1; i >= 0; i--)
		printf("%d ", (state->ctl.named.iack & (0x1 << i)) != 0);
	printf("\n");
	printf("  umcpen[%d..0]: ", SIM_CORE_NUM_EXTINT-1);
	for(i = SIM_CORE_NUM_EXTINT-1; i >= 0; i--)
		printf("%d ", (state->ctl.named.umcpen & (0x1 << i)) != 0);
	printf("\n");
	printf("counter hi=%4.4x lo=%4.4x\n",
	       state->ctl.named.counter_high, state->ctl.named.counter_low);
	printf("inter regnum=%d regval=%4.4x\n",
	       state->inter.regnum, state->inter.regval);
	printf("inter swiarg=%4.4x ru_s=%d ll=%d ru=%d gie=%d um=%d\n",
	       STATUS_GET_SWIARG(state->inter.newstat),
	       STATUS_GET_UMSAVE(state->inter.newstat),
	       STATUS_GET_LL(state->inter.newstat),
	       STATUS_GET_RUMODE(state->inter.newstat),
	       STATUS_GET_GIE(state->inter.newstat),
	       STATUS_GET_UM(state->inter.newstat));
	printf("inter nextpc=%4.4x\n", state->inter.nextpc);
	printf("inter instr_stall: %s data_stall: %s\n",
	       state->inter.instr_stall ? "true " : "false",
	       state->inter.data_stall ? "true " : "false");
	printf("inter type: ");
	switch(state->inter.type) {
	case GPRUP: printf("GPRUP\n"); break;
	case BRANCH: printf("BRANCH\n"); break;
	case BRLINK: printf("BRLINK\n"); break;
	case LOAD: printf("LOAD\n"); break;
	case LOADLNK: printf("LOADLNK\n"); break;
	case STORE: printf("STORE\n"); break;
	case SCOND: printf("SCOND\n"); break;
	case CTLUP: printf("CTLUP\n"); break;
	case CPREAD: printf("CPREAD\n"); break;
	case TRAP: printf("TRAP\n"); break;
	case RFI: printf("RFI\n"); break;
	case SLEEP: printf("SLEEP\n"); break;
	}
}

void print_output(sim_core_output_t *output)
{
	printf(" data value=%4.4x addr=%4.4x read=%s req=%s\n",
	       output->data.value, output->data.addr,
	       output->data.read ? "true " : "false",
	       output->data.request ? "true " : "false");
	printf("instr value=%4.4x addr=%4.4x read=%s req=%s\n",
	       output->instr.value, output->instr.addr,
	       output->instr.read ? "true " : "false",
	       output->instr.request ? "true " : "false");
	printf("copoc sel=%d ", output->coproc.select);
	switch(output->coproc.op) {
	case COPROC_OP_NONE: printf("op=none"); break;
	case COPROC_OP_READ: printf("op=read"); break;
	case COPROC_OP_WRITE: printf("op=write"); break;
	case COPROC_OP_EXEC: printf("op=exec"); break;
	}
	printf("\n");
#if 0
	printf("exint_ack[%d..0]: ", SIM_CORE_NUM_EXTINT-1);
	for(i = SIM_CORE_NUM_EXTINT-1; i >= 0; i--)
		printf("%d ", output->exint_ack[i]);
#endif
	printf("\n");
	printf("user mode: %s\n", output->user_mode ? "true " : "false");
}

void print_input(sim_core_input_t *input)
{
	int i;
	printf("  data value=%4.4x valid=%s fault=%s\n",
	       input->data.value,
	       input->data.valid ? "true " : "false",
	       input->data.fault ? "true " : "false");
	printf(" instr value=%4.4x valid=%s fault=%s\n",
	       input->instr.value,
	       input->instr.valid ? "true " : "false",
	       input->instr.fault ? "true " : "false");
	printf("coproc value=%4.4x\n", input->coproc.value);
	printf("exint_sig[%d..0]: ", SIM_CORE_NUM_EXTINT-1);
	for(i = SIM_CORE_NUM_EXTINT-1; i >= 0; i--)
		printf("%d ", input->exint_sig[i]);
	printf("\n");
	printf("ll invalidate=%s\n", input->ll_inval ? "true " : "false");
}

void print_mem_state(sim_memif_state_t *state)
{
	printf("delay=%d quiescent=%s\n",
	       state->delay, state->quiescent ? "true " : "false");
}

char *strchrs(char *str, char *chars, bool endmatch)
{
	char *off;
	for(; *chars; chars++)
		if((off = strchr(str, *chars)))
			return off;
	if(endmatch)
		return strchr(str, 0);
	return NULL;
}

/* str is modified */
int split(char *str, char *seps, bool zerolen,
          char **args, int argslen,
          int *numargs, char **lineout)
{
	char *cursor, *end;
	cursor = str;
	*numargs = 0;
	*lineout = NULL;
	
	if(!cursor) return 0;

	while(true) {
		end = strchrs(cursor, seps, true);
		if((end - cursor) || zerolen) {
			if(!argslen) {
				if(*cursor || zerolen) {
					*lineout = cursor;
					return -1;
				}
				return 0;
			}

			*args = cursor;
			(*numargs)++;
			args++;
			argslen--;
		}
		if(!*end) break;
		*end = 0;
		cursor = end+1;
	}

	return 0;
}

int cycle(void)
{
	unsigned int i;
	int status, cpnum;

	/* fetch */
	for (i = 0; i < ARRLEN(coproc_info); i++) {
		if(coproc_info[i].fetch) {
			/* FIXME: deal with errors here */
			status = coproc_info[i].fetch(coproc_info[i].state);
		}
	}

	sim_memif_access(&core_input.instr, &core_output.instr, &imem_state);

	/* decode + exec */
	sim_core_exec(&core_state, &core_output, &core_input);

	if(0 <= (cpnum = sim_core_cp_op_pending(COPROC_OP_EXEC, &core_output))) {
		if(cpnum < (int) ARRLEN(coproc_info) && coproc_info[cpnum].exec) {
			coproc_info[cpnum].exec(coproc_info[cpnum].state);
		}
	}
	
	/* mem */
	for (i = 0; i < ARRLEN(coproc_info); i++) {
		if(coproc_info[i].data) {
			/* FIXME: deal with errors here */
			status = coproc_info[i].data(coproc_info[i].state);
		}
	}

	sim_memif_access(&(core_input.data), &(core_output.data), &dmem_state);

	/* update */
	if(0 <= (cpnum = sim_core_cp_op_pending(COPROC_OP_READ, &core_output))) {
		if(cpnum < (int) ARRLEN(coproc_info)) {
			sim_cp_read(coproc_info[cpnum].state);
		}
	}

	if(0 <= (cpnum = sim_core_cp_op_pending(COPROC_OP_WRITE, &core_output))) {
		if(cpnum < (int) ARRLEN(coproc_info)) {
			sim_cp_write(coproc_info[cpnum].state);
		}
	}

	status = sim_core_update(&core_state, &core_output, &core_input);

	return status;
}

void reset_core(void)
{
	printf("resetting core state to power-on values\n");
	sim_core_state_init(&core_state);
	sim_core_input_init(&core_input);
	sim_core_output_init(&core_output);
	sim_memif_init(&dmem_state, physmem, PHYSMEM_NUMWORDS, "data");
	sim_memif_init(&imem_state, physmem, PHYSMEM_NUMWORDS, "inst");
}

void reset_mem(void)
{
	printf("clearing memory\n");
	memset(physmem, 0, sizeof(physmem));
}

/* FIXME: handle nonzero return values */
int reset(void)
{
	unsigned int i;
	int status;
	for(i = 0; i < ARRLEN(coproc_info); i++) {
		sim_cp_hdr_reset(coproc_info[i].state);
		if(!coproc_info[i].reset) {
			continue;
		}
		if((status = coproc_info[i].reset(coproc_info[i].state))) {
			printf("error: coproc %s (%d) reset failed (%d)\n",
		           coproc_info[i].name, i, status);
			return -1;
		}
	}
	reset_core();
	reset_mem();
	return 0;
}

int do_interp_reset(int argc, char *argv[])
{
	if(argc == 0) {
		reset_core();
		return 0;
	} else if(argc == 1) {
		if(!strcmp(argv[0], "core")) {
			reset_core();
		} else if(!strcmp(argv[0], "mem")) {
			reset_mem();
		} else if(!strcmp(argv[0], "all")) {
			reset();
		}
	} else {
		printf("usage: reset {core | mem | all}\n");
	}
	return 0;
}

int do_interp_load(int argc, char *argv[])
{
	char *image;
	size_t image_bytes;
	size_t image_words;
	uint16_t offset = 0;
	int status;
	if(argc < 1 || argc > 2) {
		printf("error: usage: load <file> [<offset>]\n");
		return 0;
	}
	if(argc == 2) {
		offset = strtol(argv[1], NULL, 0);
	}
	printf("reading %s\n", argv[0]);
	status = load_file(argv[0], &image, &image_bytes);
	if(status) {
		return 0;
	}
	image_words = image_bytes / sizeof(physmem[0]);
	if(offset + image_words > PHYSMEM_NUMWORDS) {
		printf("error: offset + image len exceeds max addr\n");
		return 0;
	}
	memcpy(physmem + offset, image, image_bytes);
	printf("%lu words loaded at offset 0x%x\n", image_words, offset);
	return 0;
}

int do_interp_goto(int argc, char *argv[])
{
	uint16_t startaddr;
	if(argc != 1) {
		printf("error: usage: goto <addr>\n");
		return 0;
	}
	printf("setting pc to 0x%4.4x\n", startaddr);
	startaddr = strtol(argv[0], NULL, 0);
	core_output.instr.request = true;
	core_output.instr.read = true;
	core_output.instr.addr = startaddr;
	core_state.pc = startaddr;
	return 0;
}

int do_interp_step(int argc, char *argv[])
{
	int numsteps, status;
	if(argc == 1) {
		numsteps = strtol(argv[0], NULL, 0);
	} else if(argc == 0) {
		numsteps = 1;
	} else {
		printf("error: usage step [<numsteps>] (default 1)\n");
		return 0;
	}

	printf("running %u cycles\n", numsteps);
	status = 0;
	while(numsteps-- && !status) {
		status = cycle();
	}
	if(status < 0) {
		return -1;
	}
	return 0;
}

int do_interp_continue(void)
{
	int status;
	signal(SIGINT, unset_run_sim);
	printf("continuing\n");
	status = 0;
	while(g_continue && !status) {
		status = cycle();
	}
	if(status < 0) {
		return -1;
	}
	return 0;
}

int do_interp_read(int argc, char *argv[])
{
	int i;
	int addr;
	uint16_t val;
	if(argc == 0) {
		printf("error: usage: read <addr> [<addr> ...]\n");
		return 0;
	}

	for(i = 0; i < argc; i++) {
		addr = strtol(argv[i], NULL, 0);
		if(addr > PHYSMEM_NUMWORDS) {
			printf("0x%4.4x: (inval)\n", addr);
		} else {
			val = physmem[addr];
			printf("0x%4.4x: 0x%4.4x\n", addr, val);
		}
	}
	return 0;
}

int do_interp_show(int argc, char *argv[])
{
	if(argc == 0) {
		printf("*** STATE ***\n");
		print_state(&core_state);
		printf("*** OUTPUT ***\n");
		print_output(&core_output);
		printf("*** INPUT ***\n");
		print_input(&core_input);
		printf("*** INSTR MEM ***\n");
		print_mem_state(&imem_state);
		printf("*** DATA MEM ***\n");
		print_mem_state(&dmem_state);
	} else if(strcmp(argv[0], "state") == 0) {
		printf("*** STATE ***\n");
		print_state(&core_state);
	} else if(strcmp(argv[0], "output") == 0) {
		printf("*** OUTPUT ***\n");
		print_output(&core_output);
	} else if(strcmp(argv[0], "input") == 0) {
		printf("*** INPUT ***\n");
		print_input(&core_input);
	} else if(strcmp(argv[0], "instr") == 0) {
		printf("*** INSTR MEM ***\n");
		print_mem_state(&imem_state);
	} else if(strcmp(argv[0], "data") == 0) {
		printf("*** DATA MEM ***\n");
		print_mem_state(&dmem_state);
	} else {
		unsigned int i;
		for(i = 0; i < ARRLEN(coproc_info); i++) {
			if(!strcmp(argv[0], coproc_info[i].name)) {
				printf("*** COPROCESSOR: %s ***\n", coproc_info[i].name);
				coproc_info[i].print(coproc_info[i].state);
				break;
			}
		}
		if(i == ARRLEN(coproc_info)) {
			printf("error: usage: show [state|output|input|instr|data");
			for(i = 0; i < ARRLEN(coproc_info); i++) {
				printf("|%s", coproc_info[i].name);
			}
			printf("]\n");
		}
	}
	return 0;
}

int do_interp_trace(int argc, char *argv[])
{
	unsigned int i;
	uint32_t level;

	switch(argc) {
	default:
		goto error;

	case 0:
		printf("core %8.8x\n", core_state.tracelevel);
		printf("imem %8.8x\n", imem_state.tracelevel);
		printf("dmem %8.8x\n", dmem_state.tracelevel);
		for(i = 0; i < ARRLEN(coproc_info); i++) {
			printf("%s %8.8x\n",
			       coproc_info[i].name,
			       coproc_info[i].state->tracelevel);
		}
		break;

	case 2:
		errno = 0;
		level = strtol(argv[1], NULL, 0);
		if(errno) {
			printf("invalid tracelevel: %s\n", argv[1]);
			goto error;
		}

		if(!strcmp(argv[0], "core")) {
			core_state.tracelevel = level;
			printf("core trace level set to %8.8x\n", core_state.tracelevel);
		} else if(!strcmp(argv[0], "imem")) {
			imem_state.tracelevel = level;
			printf("imem trace level set to %8.8x\n", imem_state.tracelevel);
		} else if(!strcmp(argv[0], "dmem")) {
			dmem_state.tracelevel = level;
			printf("dmem trace level set to %8.8x\n", dmem_state.tracelevel);
		} else {
			for(i = 0; i < ARRLEN(coproc_info); i++) {
				if(! strcmp(coproc_info[i].name, argv[0])) {
					coproc_info[i].state->tracelevel = level;
					printf("%s trace level set to %8.8x\n",
					       coproc_info[i].name,
					       coproc_info[i].state->tracelevel);
					break;
				}
			}
			if(i == ARRLEN(coproc_info)) {
				printf("unknown trace target: %s\n", argv[0]);
				goto error;
			}
		}
		break;
	}

	return 0;

error:
	printf("usage: trace\n");
	printf("usage: trace [<target> <level>]\n");
	printf("<target> is one of:\n");
	printf("core\ndmem\nimem\n");
	for(i = 0; i < ARRLEN(coproc_info); i++) {
		printf("%s\n", coproc_info[i].name);
	}
	
	return 0;
}

int interpret(char *cmdstr)
{
	char *argv[16];
	char *commands[16];
	int status = 0;
	int argc, numcommands;
	char *cursor;
	char *cmdcursor = cmdstr;
	int cmd;

	status = split(cmdcursor, ";", false,
	               commands, sizeof(commands) / sizeof(commands[0]),
	               &numcommands, &cmdcursor);

	for(cmd = 0; cmd < numcommands && ! status; cmd++) {
		cursor = commands[cmd];
		status = split(cursor, " \t", false,
		               argv, sizeof(argv) / sizeof(argv[0]),
		               &argc, &cursor);

#if 0
		{
			int arg;
			for(arg = 0; arg < argc; arg++)
				printf("arg %d: %s\n", arg, argv[arg]);
		}
#endif

		if(!strcmp(argv[0], "quit")) {
			return -1;
		} else if(!strcmp(argv[0], "reset")) {
			status = do_interp_reset(argc-1, &(argv[1]));
		} else if(!strcmp(argv[0], "load")) {
			status = do_interp_load(argc-1, &(argv[1]));
		} else if(!strcmp(argv[0], "goto")) {
			status = do_interp_goto(argc-1, &(argv[1]));
		} else if(!strcmp(argv[0], "cycle")) {
			status = do_interp_step(argc-1, &(argv[1]));
		} else if(!strcmp(argv[0], "continue")) {
			status = do_interp_continue();
		} else if(!strcmp(argv[0], "read")) {
			status = do_interp_read(argc-1, &(argv[1]));
		} else if(!strcmp(argv[0], "disassemble")) {
			printf("unimplemented\n"); status = 0;
		} else if(!strcmp(argv[0], "write")) {
			printf("unimplemented\n"); status = 0;
		} else if(!strcmp(argv[0], "assemble")) {
			printf("unimplemented\n"); status = 0;
		} else if(!strcmp(argv[0], "show")) {
			status = do_interp_show(argc-1, &(argv[1]));
		} else if(!strcmp(argv[0], "trace")) {
			status = do_interp_trace(argc-1, &(argv[1]));
		} else {
			printf("available commands: load goto cycle continue read"
			       " disassemble write assemble show trace\n");
			status = 0;
		}
	}
	return status;
}


int main(int argc, char *argv[])
{
	int status;
	unsigned int i;
	cli_args_t args;
	char linebuf[256], lastline[256];
	
	/* read cli */

	status = parse_cli(argc, argv, &args);
	if(status) {
		return -1;
	}

	/* setup coprocessor states */

	for(i = 0; i < ARRLEN(coproc_info); i++) {
#if 0
		if(!coproc_info[i].init && args.cparg[i]) {
			printf("error: coproc %s (%d) has no init callback\n",
			       coproc_info[i].name, i);
			return -1;
		}
#endif
		sim_cp_hdr_init(coproc_info[i].state, i, args.cparg[i],
		                &core_input, &core_output);
		if(coproc_info[i].init) {
			status = coproc_info[i].init(coproc_info[i].state);
			if(status) {
				printf("error: coproc %s (%d) init failed (%d)\n",
			           coproc_info[i].name, i, status);
				return -1;
			}
		}
	}

	/* initialize */

	reset();

	/* go */
	memset(linebuf, 0, sizeof(linebuf));
	memset(lastline, 0, sizeof(lastline));

	if(args.infile) {
		snprintf(linebuf, sizeof(linebuf), "load %s %d; goto %d;",
		         args.infile, args.loadoff, args.startaddr);
		status = interpret(linebuf);
		if(status) {
			printf("initialization error\n");
			return -1;
		}
	}

	if(!args.debug) {
		snprintf(linebuf, sizeof(linebuf), "continue");
		goto interp;
	}

	status = 0;
	while(!status) {
		printf("> ");
		fflush(stdout);
		fgets(linebuf, sizeof(linebuf), stdin);
		linebuf[sizeof(linebuf) - 1] = 0;
		if(strchr(linebuf, '\n')) *strchr(linebuf, '\n') = 0;
		if(strlen(linebuf) == 0) {
			memcpy(linebuf, lastline, sizeof(linebuf));
			printf("%s\n", linebuf);
		}
interp:
		memcpy(lastline, linebuf, sizeof(lastline));
		status = interpret(linebuf);
	}

	/* de-init */
	for(i = 0; i < ARRLEN(coproc_info); i++) {
		if(coproc_info[i].deinit) {
			coproc_info[i].deinit(coproc_info[i].state);
		}
	}

	return 0;
}
