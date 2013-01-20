#ifndef SIM_MEMIF_H
#define SIM_MEMIF_H

#include <stdlib.h>
#include "sim_core.h"

/* cycle delay for memory accesses */
#define MEMIF_DELAY 1

typedef struct {
	uint16_t *physmem;
	size_t memlen;
	uint8_t delay;
	bool quiescent;

	char *name;
	bool trace;
} sim_memif_state_t;

void sim_memif_init(
	sim_memif_state_t *state,
	uint16_t *physmem,
	size_t memlen,
	char *name);

void sim_memif_access(
	sim_core_mem_input_t *core_input,
	sim_core_mem_output_t *core_output,
	sim_memif_state_t *mem_state);

void sim_memif_tick(sim_memif_state_t *mem_state);

#endif
