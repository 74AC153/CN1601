#include <stdio.h>
#include "sim_cp_timer.h"

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
