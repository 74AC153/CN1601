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

#if 0

#if ! defined(NOTRACE)
#define TRACE(STATE, ...) do{if((STATE)->trace)printf( __VA_ARGS__ );}while(0)
#define TRACEBLOCK(state, BLOCK) do{if((STATE)->trace) BLOCK}while(0)
#else /* defined (NOTRACE) */
#define TRACE(STATE, ...)
#define TRACEBLOCK(state, BLOCK)
#endif

void sim_cp_timer_init(
	sim_cp_timer_state_t *state,
	unsigned int cpnum)
{
	state->cpnum = cpnum;
}

void sim_cp_timer_deinit(
	sim_cp_timer_state_t *state)
{
	return;
}

void sim_cp_timer_reset(
	sim_cp_timer_state_t *state)
{
	unsigned int i;
	for(i = 0; i < sizeof(state->regs.array)/sizeof(state->regs.array[0]); i++)
	{
		state->regs.array[i] = 0;
	}
}

void sim_cp_timer_access(
	sim_core_input_t *core_input,
	sim_core_output_t *core_output,
	sim_cp_timer_state_t *timer_state)
{
	if(TIMER_CTL_GET_ENABLED(timer_state->regs.named.ctl)) {
		timer_state->regs.named.cycles_lo --;
		if(! timer_state->regs.named.cycles_lo) {
			if(! timer_state->regs.named.cycles_hi) {
				/* timer underflow, raise interrupt */
				TRACE(timer_state, "timer: raising interrupt\n");
				TIMER_CTL_SET_ENABLED(timer_state->regs.named.ctl, false);
				core_input->exint_sig[timer_state->cpnum] = true;
			} else {
				timer_state->regs.named.cycles_hi --;
			}
		}
	}

	if(core_output->coproc.select == timer_state->cpnum) {
		switch(core_output->coproc.op) {
		case COPROC_OP_READ:
			TRACE(timer_state, "timer: read reg %d\n",
			      core_output->coproc.addr);
			core_input->coproc.value =
			    timer_state->regs.array[core_output->coproc.addr];
			break;
		case COPROC_OP_WRITE:
			TRACE(timer_state, "timer: write reg %d\n",
			      core_output->coproc.addr);
			timer_state->regs.array[core_output->coproc.addr] =
			    core_output->coproc.value;
			break;
		case COPROC_OP_EXEC:
			switch(core_output->coproc.value) {
			case CP_TIMER_INSTR_IACK:
				TRACE(timer_state, "timer: exec iack\n");
				core_input->exint_sig[timer_state->cpnum] = false;
				break;
			default:
				TRACE(timer_state, "timer: exec UNKNOWN\n");
				break;
			}
			break;
		default: 
			break;
		}
	}
}
#endif

