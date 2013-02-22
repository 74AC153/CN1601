#ifndef SIM_CP_TIMER_H
#define SIM_CP_TIMER_H

#include <stdint.h>
#include "sim_cp_if.h"
#include "sim_core.h"


#define CP_TIMER_INSTR_NOOP 0
#define CP_TIMER_INSTR_IACK 1

#define CP_TIMER_REG_CTL 0
#define CP_TIMER_REG_CTL_EN_START 0
#define CP_TIMER_REG_CTL_EN_LEN 1

#define CP_TIMER_REG_CYCLE_LO 1
#define CP_TIMER_REG_CYCLE_HI 2

typedef struct {
	sim_cp_state_hdr_t hdr;
} sim_cp_timer_state_t;

int timer_state_data(sim_cp_state_hdr_t *hdr);
int timer_state_exec(sim_cp_state_hdr_t *hdr);
int timer_state_print(sim_cp_state_hdr_t *hdr);

#if 0
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

#endif
