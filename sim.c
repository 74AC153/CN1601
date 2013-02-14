#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

#include "sim_core.h"
#include "sim_memif.h"
#include "sim_cp_timer.h"
#include "sim_cp_nvram.h"
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


#define TIMER_CPNUM 0
sim_cp_timer_state_t timer_state;
#define NVRAM_CPNUM 1
sim_cp_nvram_state_t nvram_state;

bool g_continue = true;

void unset_run_sim(int arg)
{
	arg = arg;
	printf("stopped\n");
	signal(SIGINT, SIG_DFL);
	g_continue = false;
}

typedef struct {
	char *infile;
	bool debug;
	int startaddr;
	int loadoff;
} cli_args_t;

void usage(char *progname)
{
	printf("usage: %s [-i <image>] [-o <offset>] [-s <startaddr>] [-d]\n", 
	       progname);
	printf("<offset> defaults to 0\n");
	printf("<startaddr> defaults to 0\n");
	printf("-d: start in command mode\n");
	printf("if <image> is not specified, -d is implied\n");
}

int parse_cli(int argc, char *argv[], cli_args_t *args)
{
	int ch;
	args->infile = NULL;
	args->loadoff = 0;
	args->startaddr = 0;
	args->debug = false;

	while((ch = getopt(argc, argv, "i:o:s:d")) != -1) {
		switch(ch) {
		case 'i': args->infile = optarg; break;
		case 'o': args->loadoff = strtol(optarg, NULL, 0); break;
		case 's': args->startaddr = strtol(optarg, NULL, 0); break;
		case 'd': args->debug = true; break;
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
	printf("delay=%d quiescent=%s\n", state->delay, state->quiescent ? "true " : "false");
}

void print_timer_state(sim_cp_timer_state_t *timer_state)
{
	printf("enabled: %d\n", TIMER_CTL_GET_ENABLED(timer_state->regs.named.ctl));
	printf("cycles_lo: %x\n", timer_state->regs.named.cycles_lo);
	printf("cycles_hi: %x\n", timer_state->regs.named.cycles_hi);
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

void cycle(void)
{
	/* fetch */
	sim_memif_access(&core_input.instr, &core_output.instr, &imem_state);
	/* decode + exec */
	sim_core_exec(&core_state, &core_output, &core_input);
	/* coprocessors */
	sim_cp_timer_access(&core_input, &core_output, &timer_state);
	
	/* mem */
	sim_memif_access(&(core_input.data), &(core_output.data), &dmem_state);
	/* update */
	sim_core_update(&core_state, &core_output, &core_input);
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

void reset(void)
{
	sim_cp_timer_init(&timer_state, 0);
	reset_core();
	reset_mem();
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
	int numsteps;
	if(argc == 1) {
		numsteps = strtol(argv[0], NULL, 0);
	} else if(argc == 0) {
		numsteps = 1;
	} else {
		printf("error: usage step [<numsteps>] (default 1)\n");
		return 0;
	}

	printf("running %u cycles\n", numsteps);
	while(numsteps--) {
		cycle();
	}
	return 0;
}

int do_interp_continue(void)
{
	signal(SIGINT, unset_run_sim);
	printf("continuing\n");
	while(g_continue) {
		cycle();
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
	} else if(strcmp(argv[0], "timer") == 0) {
		printf("*** TIMER ***\n");
		print_timer_state(&timer_state);
	} else {
		printf("error: usage: show [state|output|input|instr|data|timer]\n");
	}
	return 0;
}

int do_interp_trace(int argc, char *argv[])
{
	bool s, i, d, timer;
	s = core_state.trace;
	d = dmem_state.trace;
	i = imem_state.trace;
	timer = timer_state.trace;
	if(argc == 0) {
		
	} else if(argc == 1) {
		if(!strcmp(argv[0], "on")) {
			s = i = d = timer = true;
		} else if(!strcmp(argv[0], "off")) {
			s = i = d = timer = false;
		} else if(!strcmp(argv[0], "state")) {
			s = true;
		} else if(!strcmp(argv[0], "imem")) {
			i = true;
		} else if(!strcmp(argv[0], "dmem")) {
			d = true;
		} else if(!strcmp(argv[0], "timer")) {
			timer = true;
		} else {
			printf("unknown trace command: %s\n", argv[0]);
			goto error;
		}
	} else if(argc == 2) {
		if(strcmp(argv[1], "on") && strcmp(argv[1], "off")) {
			printf("unknown trace command: %s\n", argv[1]);
			goto error;
		}
		if(!strcmp(argv[0], "state")) {
			s = strcmp(argv[1], "on") == 0;
		} else if(!strcmp(argv[0], "imem")) {
			i = strcmp(argv[1], "on") == 0;
		} else if(!strcmp(argv[0], "dmem")) {
			d = strcmp(argv[1], "on") == 0;
		} else if(!strcmp(argv[0], "timer")) {
			timer = strcmp(argv[1], "on");
		} else {
			printf("unknown trace command: %s\n", argv[0]);
			goto error;
		}	
	} else {
		goto error;
	}

	core_state.trace = s;
	dmem_state.trace = d;
	imem_state.trace = i;
	timer_state.trace = timer;
	printf("trace state=%d imem=%d dmem=%d timer=%d\n",
	       s, i, d, timer);

	return 0;

error:
	printf("usage: trace {on | off}\n");
	printf("             {state | imem | dmem | timer}\n");
	printf("             {state | imem | dmem | timer} {on | off}\n");
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
	cli_args_t args;
	char linebuf[256];
	
	/* read cli */

	status = parse_cli(argc, argv, &args);
	if(status) {
		return -1;
	}

	/* initialize */

	reset();

	/* go */

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

	while(!status) {
		printf("> ");
		fflush(stdout);
		fgets(linebuf, sizeof(linebuf), stdin);
		linebuf[sizeof(linebuf) - 1] = 0;
		if(strchr(linebuf, '\n')) *strchr(linebuf, '\n') = 0;
interp:
		status = interpret(linebuf);
	}

	return 0;
}
