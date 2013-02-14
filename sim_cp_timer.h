#ifndef SIM_CP_TIMER_H
#define SIM_CP_TIMER_H

#include <stdint.h>
#include "sim_core.h"

#define TIMER_CTL_GET_ENABLED(CTL) ((CTL) & 0x1)
#define TIMER_CTL_SET_ENABLED(CTL, VAL) \
	do{ (CTL) = ((CTL) & 0xFFFE) | ((VAL) & 0x1); } while (0)

typedef enum {
	CP_TIMER_INSTR_NOOP,
	CP_TIMER_INSTR_IACK
} cp_timer_instr_t;

typedef struct {
	union {
		uint16_t array[SIM_CORE_NUM_CPREGS];
		struct {
			uint16_t ctl;
			uint16_t cycles_lo;
			uint16_t cycles_hi;
		} named;
	} regs;

	unsigned int cpnum; //
	bool trace;
} sim_cp_timer_state_t;

void sim_cp_timer_init(
	sim_cp_timer_state_t *state,
	unsigned int cpnum);

void sim_cp_timer_deinit(
	sim_cp_timer_state_t *state);

void sim_cp_timer_reset(
	sim_cp_timer_state_t *state);

void sim_cp_timer_access(
	sim_core_input_t *core_input,
	sim_core_output_t *core_output,
	sim_cp_timer_state_t *timer_state);

#endif
