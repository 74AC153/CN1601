#ifndef SIM_CP_IF
#define SIM_CP_IF

#include <stdbool.h>
#include <stdint.h>
#include "sim_core.h"

#if ! defined(NOTRACE)
#define TRACE(HDR, LEVEL, ...) \
	do { if((HDR)->tracelevel & (LEVEL)) printf(__VA_ARGS__); } while(0)
#else /* defined (NOTRACE) */
#define TRACE(S, L, ...)
#endif

typedef struct {
	unsigned int cpnum;
	uint32_t tracelevel;
	int argc;
	char **argv;
	void *priv;
	sim_core_input_t *core_input;
	sim_core_output_t *core_output;
	uint16_t regs[SIM_CORE_NUM_CPREGS];
} sim_cp_state_hdr_t;

/* FIXME: do we need a return value? */
typedef int(*state_fn_t)(sim_cp_state_hdr_t *hdr);

typedef struct {
	char *name;
	char *usage;
	sim_cp_state_hdr_t *state;
	state_fn_t init; /* called when starting up sim, hdr setup before */
	state_fn_t deinit; /* called when shutting down sim, hdr torn down after */
	state_fn_t reset; /* called when cpu reset is done */
	state_fn_t data; /* called after exec stage */
	state_fn_t exec; /* called when cpop matches */
	state_fn_t fetch; /* called after update stage */
	state_fn_t print; /* called when print <name> */
} sim_cp_info_t;


void sim_cp_hdr_init(
	sim_cp_state_hdr_t *hdr,
	unsigned int cpnum,
	char *argstr,
	sim_core_input_t *core_input,
	sim_core_output_t *core_output);

void sim_cp_hdr_deinit(
	sim_cp_state_hdr_t *hdr);

void sim_cp_hdr_reset(
	sim_cp_state_hdr_t *hdr);

void sim_cp_read(
	sim_cp_state_hdr_t *hdr);

void sim_cp_write(
	sim_cp_state_hdr_t *hdr);

#endif
