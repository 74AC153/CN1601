#include "stdio.h"
#include "sim_memif.h"

#if ! defined(NOTRACE)
#define TRACE(STATE, LEVEL, ...) \
	do{ \
		if((STATE)->tracelevel & (LEVEL)) \
			printf( __VA_ARGS__ ); \
	} while(0)
#define TRACEBLOCK(state, BLOCK) \
	do{ \
		if((STATE)->tracelevel & (LEVEL)) \
			BLOCK \
	} while(0)
#else /* defined (NOTRACE) */
#define TRACE(STATE, ...)
#define TRACEBLOCK(state, BLOCK)
#endif

void sim_memif_init(
	sim_memif_state_t *state,
	uint16_t *physmem,
	size_t memlen,
	char *name)
{
	state->physmem = physmem;
	state->memlen = memlen;
	state->delay = 0;
	state->quiescent = true;
	state->name = name;
}

void sim_memif_access(
	sim_core_mem_input_t *core_input,
	sim_core_mem_output_t *core_output,
	sim_memif_state_t *mem_state)
{
	core_input->fault = false;
	core_input->valid = false;

	if(core_output->request) {
		if(mem_state->quiescent) {
			TRACE(mem_state, 1, "mem %s: quiescent\n", mem_state->name);
			mem_state->delay = MEMIF_DELAY;
			mem_state->quiescent = false;
		}
		if(mem_state->delay) {
			TRACE(mem_state, 1, "mem %s: delay\n", mem_state->name);
			mem_state->delay--;
			return;
		}
	} else {
		TRACE(mem_state, 1, "mem %s: idle\n", mem_state->name);
		return;
	}

	if(core_output->addr > mem_state->memlen) {
		TRACE(mem_state, 1, "mem %s: addr fault\n", mem_state->name);
		core_input->valid = false;
		core_input->fault = true;
	} else if(core_output->read) {
		core_input->value = mem_state->physmem[core_output->addr];
		TRACE(mem_state, 1, "mem %s: read addr %4.4x val %4.4x\n",
		      mem_state->name, core_output->addr, core_input->value);
		core_input->valid = true;
		core_input->fault = false;
	} else {
		TRACE(mem_state, 1, "mem %s: write addr %4.4x val %4.4x\n",
		      mem_state->name, core_output->addr, core_output->value);
		mem_state->physmem[core_output->addr] = core_output->value;
		core_input->valid = true;
		core_input->fault = false;
	}
	mem_state->quiescent = true;
}

void sim_memif_tick(sim_memif_state_t *mem_state)
{
	if(! mem_state->quiescent)
		mem_state->delay--;
}

