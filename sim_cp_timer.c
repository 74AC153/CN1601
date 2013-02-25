#include <stdio.h>
#include <stdlib.h>
#include "sim_cp_timer.h"
#include "utils.h"

#define TIMER_GET_ENABLED(STATE) \
	field_getbits16((STATE)->hdr.regs[CP_TIMER_REG_CTL],  \
	              CP_TIMER_REG_CTL_EN_START, \
	              CP_TIMER_REG_CTL_EN_LEN)

#define TIMER_SET_ENABLED(STATE, EN_VAL) \
	do { \
		(STATE)->hdr.regs[CP_TIMER_REG_CTL] =  \
		  field_setbits16((STATE)->hdr.regs[CP_TIMER_REG_CTL], \
		                  CP_TIMER_REG_CTL_EN_START, \
		                  CP_TIMER_REG_CTL_EN_LEN, \
		                  EN_VAL); \
	} while(0)

#define TIMER_CYCLES_LO(STATE) \
	((STATE)->hdr.regs[CP_TIMER_REG_CYCLE_LO])

#define TIMER_CYCLES_HI(STATE) \
	((STATE)->hdr.regs[CP_TIMER_REG_CYCLE_HI])

int timer_state_data(sim_cp_state_hdr_t *hdr)
{
	sim_cp_timer_state_t *state = (sim_cp_timer_state_t *) hdr;

	if(TIMER_GET_ENABLED(state)) {

		TIMER_CYCLES_LO(state) --;
		TRACE(hdr, 1, "timer: cycles_lo -> %4.4x\n", 
		      TIMER_CYCLES_LO(state));

		if(! TIMER_CYCLES_LO(state)) {
			if(! TIMER_CYCLES_HI(state)) {
				/* timer underflow, raise interrupt */
				TRACE(hdr, 1, "timer: raising interrupt\n");
				TIMER_SET_ENABLED(state, false);
				hdr->core_input->exint_sig[hdr->cpnum] = true;
			} else {
				TIMER_CYCLES_HI(state) --;
				TRACE(hdr, 1, "timer: cycles_hi -> %4.4x\n",
				      TIMER_CYCLES_LO(state));
			}
		}
	}
	return 0;
}

int timer_state_exec(sim_cp_state_hdr_t *hdr)
{
	switch(hdr->core_output->coproc.value) {
	case CP_TIMER_INSTR_IACK:
		TRACE(hdr, 1, "timer: exec iack\n");
		hdr->core_input->exint_sig[hdr->cpnum] = false;
		break;
	default:
		TRACE(hdr, 1, "timer: exec UNKNOWN\n");
		break;
	}

	return 0;
}

int timer_state_print(sim_cp_state_hdr_t *hdr)
{
	sim_cp_timer_state_t *state = (sim_cp_timer_state_t *) hdr;

	printf("enabled: %d\n", TIMER_GET_ENABLED(state));
	printf("cycles_lo: %x\n", TIMER_CYCLES_LO(state));
	printf("cycles_hi: %x\n", TIMER_CYCLES_HI(state));

	return 0;
}
