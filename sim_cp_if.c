#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "sim_cp_if.h"

void
sim_cp_hdr_init(
	sim_cp_state_hdr_t *hdr,
	unsigned int cpnum,
	char *argstr,
	sim_core_input_t *core_input,
	sim_core_output_t *core_output)
{
	int i, argnum, argc = 0;
	char **argv = NULL;

	hdr->cpnum = cpnum;
	hdr->tracelevel = 0;
	hdr->core_input = core_input;
	hdr->core_output = core_output;
	
	/* have argstr? */
	if(argstr && strlen(argstr)) {
		/* count args */
		for(i = 0, argc = 1; argstr[i]; i++) {
			if(argstr[i] == ',') {
				argc++;
			}
		}

		argv = malloc(sizeof(char *) * (argc + 1));
		assert(argv);

		/* populate argv */
		argv[0] = argstr;
		for(i = 1, argnum = 1; argstr[i]; i++) {
			if(argstr[i] == ',') {
				argstr[i++] = 0;
				argv[argnum++] = argstr + i;
			}
		}
		argv[argnum] = NULL;
	} else {
		argv = malloc(sizeof(char *));
		assert(argv);
		argv[0] = NULL;
		argc = 0;
	}

	hdr->argv = argv;
	hdr->argc = argc;
}

void sim_cp_hdr_deinit(
	sim_cp_state_hdr_t *hdr)
{
	free(hdr->argv);
}

void sim_cp_hdr_reset(
	sim_cp_state_hdr_t *hdr)
{
	unsigned int i;
	for(i = 0; i < SIM_CORE_NUM_CPREGS; i++) {
		hdr->regs[i] = 0;
	}
}

void sim_cp_read(
	sim_cp_state_hdr_t *hdr)
{
	uint16_t regnum, val;
	regnum = hdr->core_output->coproc.addr;
	val = hdr->regs[regnum];
	TRACE(hdr, 1, "coproc %d: read reg %d -> val %4.4x\n",
	      hdr->cpnum, regnum, val);
	hdr->core_input->coproc.value = val;
}

void sim_cp_write(
	sim_cp_state_hdr_t *hdr)
{
	uint16_t regnum, val;
	regnum = hdr->core_output->coproc.addr;
	val = hdr->core_output->coproc.value;
	TRACE(hdr, 1, "coproc %d: write reg %d <- val %4.4x\n",
	      hdr->cpnum, regnum, val);
	hdr->regs[regnum] = val;
}
