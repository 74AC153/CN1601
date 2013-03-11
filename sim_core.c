#include "sim_core.h"
#include <assert.h>
#include <string.h>
#include "sim_core.h"
#include "instructions.h"
#include <stdio.h>

#if ! defined(NOTRACE)
#define TRACE(STATE, LEVEL, ...) do{if((STATE)->tracelevel & (LEVEL))printf( __VA_ARGS__ );}while(0)
#define TRACEBLOCK(STATE, LEVEL, BLOCK) do{if((STATE)->tracelevel & (LEVEL) ) BLOCK}while(0)
#else /* defined (NOTRACE) */
#define TRACE(STATE, LEVEL ...)
#define TRACEBLOCK(STATE, LEVEL, BLOCK)
#endif

sim_core_output_t *sim_core_output_init(
	sim_core_output_t *output)
{
	if(output)
		memset(output, 0, sizeof(*output));
	return output;
}

sim_core_input_t *sim_core_input_init(
	sim_core_input_t *input)
{
	if(input)
		memset(input, 0, sizeof(*input));
	return input;
}

sim_core_state_t *sim_core_state_init(
	sim_core_state_t *state)
{
	if(state)
		memset(state, 0, sizeof(*state));
	return state;
}

typedef enum
{
	ALU_OP_OR,
	ALU_OP_NAND,
	ALU_OP_AND,
	ALU_OP_XOR,
	ALU_OP_ADD,
	ALU_OP_SUB,
	ALU_OP_ADDC,
	ALU_OP_SUBC,
	ALU_OP_SHL,
	ALU_OP_SHR,
	ALU_OP_SHRA,
	ALU_OP_LHSWAP,
	ALU_OP_ROT,
	ALU_OP_SHL8OR,
} alu_op_t;

uint16_t alu_exec(alu_op_t op, uint16_t arg0, uint16_t arg1 )
{
	switch(op) {
		case ALU_OP_OR: return arg0 | arg1;
		case ALU_OP_NAND: return ~( arg0 & arg1 );
		case ALU_OP_AND: return arg0 & arg1;
		case ALU_OP_XOR: return arg0 ^ arg1;
		case ALU_OP_ADD: return arg0 + arg1;
		case ALU_OP_SUB: return arg0 - arg1;
		case ALU_OP_ADDC:
			return ( ((uint32_t) arg0 + (uint32_t) arg1) > 0x0000FFFF ?
			        0x1 : 0x0 );
		case ALU_OP_SUBC:
			return ( ((uint32_t) arg0 - (uint32_t) arg1) > 0x0000FFFF ?
			        0x1 : 0x0 );
		case ALU_OP_SHL: return arg0 << arg1;
		case ALU_OP_SHR: return arg0 >> arg1;
		case ALU_OP_SHRA: return (int16_t) arg0 >> arg1;
		case ALU_OP_LHSWAP:
			return ((arg0 & 0x00FF) << 8) | ((arg0 & 0xFF00) >> 8);
		case ALU_OP_ROT:
			arg1 &= 0xF;
			return (arg0 << arg1) | (arg0 >> (16-arg1));
		case ALU_OP_SHL8OR:
			return ((arg0 & 0x00FF) << 8) | (arg1 & 0x00FF);
		default: assert(0); break;
	}
	return 0;
}

uint16_t gpreg_read(sim_core_state_t *state, unsigned int regnum)
{
	if(STATUS_GET_UM(state->ctl.named.status)) {
		return state->ctl.named.gpr_user[regnum];
	} else {
		return state->gpr_super[regnum];
	}
}

void gpreg_write(sim_core_state_t *state, unsigned int regnum, uint16_t regval)
{
	if(STATUS_GET_UM(state->ctl.named.status)) {
		state->ctl.named.gpr_user[regnum] = regval;
	} else {
		state->gpr_super[regnum] = regval;
	}
}

void status_trap(uint16_t *stat)
{
	STATUS_SET_LL(*stat, false);                        /* clear load link */
	STATUS_SET_UMSAVE(*stat, STATUS_GET_RUMODE(*stat)); /* rumsave <- rumode */
	STATUS_SET_RUMODE(*stat, STATUS_GET_UM(*stat));     /* rumode <- umode */
	STATUS_SET_UM(*stat, false);                        /* umode <- false */
	STATUS_SET_GIE(*stat, false);                       /* disable interrupts */
}

void status_rfi(uint16_t *stat)
{
	STATUS_SET_LL(*stat, false);                        /* clear load link */
	STATUS_SET_UM(*stat, STATUS_GET_RUMODE(*stat));     /* umode <- rumode */
	STATUS_SET_RUMODE(*stat, STATUS_GET_UMSAVE(*stat)); /* rumode <- rumsave */
	STATUS_SET_GIE(*stat, true);                        /* enable interrupts */
}

#define TEST_BIT(FIELD, BIT) \
	((FIELD) & (0x1 << (BIT)))

#define SET_BIT(FIELD, BIT, VAL) \
	((FIELD) = ((FIELD) & ~(1 << (BIT))) | (((VAL) ? 1 : 0) << (BIT)))

int check_pending_interrupt(
	sim_core_state_t *state,
	sim_core_input_t *input)
{
	int i;
	for(i = 0; i < SIM_CORE_NUM_EXTINT; i++) {
		if(input->exint_sig[i] &&
		   TEST_BIT(state->ctl.named.ienable, i) &&
		   STATUS_GET_GIE(state->ctl.named.status)) {
			return i;
		}
	}
	return -1;
}


int sim_core_exec(
	sim_core_state_t *state,
	sim_core_output_t *output,
	sim_core_input_t *input)
{
	int i;
	uint16_t instr_value;
	int opcode, subcode, fields[3];

	uint16_t alu_arg0, alu_arg1;
	alu_op_t alu_op;

	TRACE(state, 1, "core: exec pc %x\n", state->pc);
	/* check for data stalls */
	if(state->inter.data_stall) {
		TRACE(state, 1, "core: exec data stall\n");
		return 0;
	}

	/*
	 * fetch
	 */

	/* check for sleep state */
	if(state->sleeping) {
		TRACE(state, 1, "core: exec sleeping\n");
		/* trap on pending hardware interrupts */
		i = check_pending_interrupt(state, input);
		if(i >= 0) {
			state->inter.nextpc = state->ctl.named.exihdl[i];
			TRACE(state, 1, "core: exec pending interrupt %d\n", i);
			goto trap;
		}
		goto finish;
	}

	if(! input->instr.valid) {
		if(! input->instr.fault) {
			/* stall if instruction isn't ready yet */
			state->inter.instr_stall = true;
			TRACE(state, 1, "core: exec instr stall\n");
			return 0;
		} else {
			/* trap if we have an instr read fault */
			state->inter.nextpc = state->ctl.named.ifaulthdl;
			state->inter.regval = state->pc;
			TRACE(state, 1, "core: exec instr read fault at %x\n", state->pc);
			goto trap;
		}
	} else {
		state->inter.instr_stall = false;
	}
	instr_value = input->instr.value;

	/* trap on pending hardware interrupts */
	i = check_pending_interrupt(state, input);
	if(i >= 0) {
		state->inter.nextpc = state->ctl.named.exihdl[i];
		TRACE(state, 1, "core: exec pending interrupt %d\n", i);
		goto trap;
	}


	/*
	 * decode
	 */

	if(decode_instruction(&opcode, &subcode, &fields[0], instr_value)) {
		state->inter.nextpc = state->ctl.named.illophdl;
		state->inter.regval = state->pc;
		TRACE(state, 1, "core: exec instr decode fault at %x\n", state->pc);
		goto trap;
	}

	/*
	 * execute
	 */

	state->inter.newstat = state->ctl.named.status;
	state->inter.nextpc = state->pc + 1;

	switch(opcode) {
		case 0x00: /* alu bit */
			switch(subcode) {
			case 0x0:
				alu_op = ALU_OP_OR;
				TRACE(state, 1, "core: exec or\n");
				break;
			case 0x1:
				alu_op = ALU_OP_NAND;
				TRACE(state, 1, "core: exec nand\n");
				break;
			case 0x2:
				alu_op = ALU_OP_AND;
				TRACE(state, 1, "core: exec and\n");
				break;
			case 0x3:
				alu_op = ALU_OP_XOR;
				TRACE(state, 1, "core: exec xor\n");
				break;
			}
			alu_arg0 = gpreg_read(state, fields[1]);
			alu_arg1 = gpreg_read(state, fields[2]);
			state->inter.regnum = fields[0];
			state->inter.regval = alu_exec(alu_op, alu_arg0, alu_arg1);
			state->inter.type = GPRUP;
			break;
		case 0x01: /* alu math */
			switch(subcode) {
			case 0x0:
				alu_op = ALU_OP_ADD;
				TRACE(state, 1, "core: exec add\n");
				break;
			case 0x1:
				alu_op = ALU_OP_SUB;
				TRACE(state, 1, "core: exec sub\n");
				break;
			case 0x2:
				alu_op = ALU_OP_ADDC;
				TRACE(state, 1, "core: exec addc\n");
				break;
			case 0x3:
				alu_op = ALU_OP_SUBC;
				TRACE(state, 1, "core: exec subc\n");
				break;
			}
			alu_arg0 = gpreg_read(state, fields[1]);
			alu_arg1 = gpreg_read(state, fields[2]);
			state->inter.regnum = fields[0];
			state->inter.regval = alu_exec(alu_op, alu_arg0, alu_arg1);
			state->inter.type = GPRUP;
			break;
		case 0x02: /* alu shift */
			switch(subcode) {
			case 0x0:
				alu_op = ALU_OP_SHL;
				TRACE(state, 1, "core: exec shl\n");
				break;
			case 0x1:
				alu_op = ALU_OP_SHR;
				TRACE(state, 1, "core: exec shr\n");
				break;
			case 0x2:
				alu_op = ALU_OP_SHRA;
				TRACE(state, 1, "core: exec shra\n");
				break;
			case 0x3:
				alu_op = ALU_OP_ROT;
				TRACE(state, 1, "core: exec rot\n");
				break;
			}
			alu_arg0 = gpreg_read(state, fields[1]);
			alu_arg1 = gpreg_read(state, fields[2]);
			state->inter.regnum = fields[0];
			state->inter.regval = alu_exec(alu_op, alu_arg0, alu_arg1);
			state->inter.type = GPRUP;
			break;
		case 0x03: /* alu shift immed */
			switch(subcode) {
			case 0x0:
				alu_op = ALU_OP_SHL;
				TRACE(state, 1, "core: exec shli\n");
				break;
			case 0x1:
				alu_op = ALU_OP_SHR;
				TRACE(state, 1, "core: exec shri\n");
				break;
			case 0x2:
				alu_op = ALU_OP_SHRA;
				TRACE(state, 1, "core: exec shrai\n");
				break;
			case 0x3:
				alu_op = ALU_OP_ROT;
				TRACE(state, 1, "core: exec roti\n");
				break;
			}
			alu_arg0 = gpreg_read(state, fields[0]);
			alu_arg1 = fields[1];
			state->inter.regnum = fields[0];
			state->inter.regval = alu_exec(alu_op, alu_arg0, alu_arg1);
			state->inter.type = GPRUP;
			break;
		case 0x04: /* li */
			TRACE(state, 1, "core: exec li\n");
			state->inter.regnum = fields[0];
			state->inter.regval = fields[1];
			state->inter.type = GPRUP;
			break;
		case 0x05: /* shlli */
			TRACE(state, 1, "core: exec shlli\n");
			alu_arg0 = gpreg_read(state, fields[0]);
			alu_arg1 = fields[1];
			alu_op = ALU_OP_SHL8OR;
			state->inter.regnum = fields[0];
			state->inter.regval = alu_exec(alu_op, alu_arg0, alu_arg1);
			state->inter.type = GPRUP;
			break;
		case 0x06: /* inci */
			TRACE(state, 1, "core: exec inci\n");
			alu_arg0 = gpreg_read(state, fields[0]);
			alu_arg1 = fields[1];
			alu_op = ALU_OP_ADD;
			state->inter.regnum = fields[0];
			state->inter.regval = alu_exec(alu_op, alu_arg0, alu_arg1);
			state->inter.type = GPRUP;
			break;
		case 0x07: /* deci */
			TRACE(state, 1, "core: exec deci\n");
			alu_arg0 = gpreg_read(state, fields[0]);
			alu_arg1 = fields[1];
			alu_op = ALU_OP_SUB;
			state->inter.regnum = fields[0];
			state->inter.regval = alu_exec(alu_op, alu_arg0, alu_arg1);
			state->inter.type = GPRUP;
			break;
		case 0x08: /* ori */
			TRACE(state, 1, "core: exec ori\n");
			alu_arg0 = gpreg_read(state, fields[0]);
			alu_arg1 = fields[1];
			alu_op = ALU_OP_OR;
			state->inter.regnum = fields[0];
			state->inter.regval = alu_exec(alu_op, alu_arg0, alu_arg1);
			state->inter.type = GPRUP;
			break;
		case 0x09: /* andi */
			TRACE(state, 1, "core: exec andi\n");
			alu_arg0 = gpreg_read(state, fields[0]);
			alu_arg1 = fields[1] | (0xFF << 8);
			alu_op = ALU_OP_AND;
			state->inter.regnum = fields[0];
			state->inter.regval = alu_exec(alu_op, alu_arg0, alu_arg1);
			state->inter.type = GPRUP;
			break;
		case 0x0A: /* xori */
			TRACE(state, 1, "core: exec xori\n");
			alu_arg0 = gpreg_read(state, fields[0]);
			alu_arg1 = fields[1];
			alu_op = ALU_OP_XOR;
			state->inter.regnum = fields[0];
			state->inter.regval = alu_exec(alu_op, alu_arg0, alu_arg1);
			state->inter.type = GPRUP;
			break;
		case 0x0B: /* orih */
			TRACE(state, 1, "core: exec orih\n");
			alu_arg0 = gpreg_read(state, fields[0]);
			alu_arg1 = fields[1] << 8;
			alu_op = ALU_OP_OR;
			state->inter.regnum = fields[0];
			state->inter.regval = alu_exec(alu_op, alu_arg0, alu_arg1);
			state->inter.type = GPRUP;
			break;
		case 0x0C: /* andih */
			TRACE(state, 1, "core: exec andih\n");
			alu_arg0 = gpreg_read(state, fields[0]);
			alu_arg1 = (fields[1] << 8) | 0xFF;
			alu_op = ALU_OP_AND;
			state->inter.regnum = fields[0];
			state->inter.regval = alu_exec(alu_op, alu_arg0, alu_arg1);
			state->inter.type = GPRUP;
			break;
		case 0x0D: /* xorih */
			TRACE(state, 1, "core: exec xorih\n");
			alu_arg0 = gpreg_read(state, fields[0]);
			alu_arg1 = fields[1] << 8;
			alu_op = ALU_OP_XOR;
			state->inter.regnum = fields[0];
			state->inter.regval = alu_exec(alu_op, alu_arg0, alu_arg1);
			state->inter.type = GPRUP;
			break;
		case 0x0E: /* sleep */
			TRACE(state, 1, "core: exec sleep\n");
			if(STATUS_GET_UM(state->ctl.named.status)) {
				TRACE(state, 1, "core: exec insufficient priviledge\n");
				state->inter.nextpc = state->ctl.named.privophdl;
				goto trap;
			}
			state->inter.type = SLEEP;
			break;
		case 0x0F: /* ba */
			TRACE(state, 1, "core: exec ba\n");
			alu_arg0 = state->pc;
			alu_arg1 = fields[0];
			state->inter.type = BRANCH;
			state->inter.nextpc = alu_exec(ALU_OP_ADD, alu_arg0, alu_arg1);
			break;
		case 0x10: /* bal */
			TRACE(state, 1, "core: exec bal\n");
			alu_arg0 = state->pc;
			alu_arg1 = fields[0];
			state->inter.regnum = 7;
			state->inter.regval = state->pc + 1;
			state->inter.nextpc = alu_exec(ALU_OP_ADD, alu_arg0, alu_arg1);
			state->inter.type = GPRUP;
			break;
		case 0x11: /* bz */
			TRACE(state, 1, "core: exec bz\n");
			if(gpreg_read(state, fields[0]) == 0) {
				TRACE(state, 1, "core: exec branch taken\n");
				alu_arg0 = fields[1];
				alu_arg1 = state->pc;
				state->inter.nextpc = alu_exec(ALU_OP_ADD, alu_arg0, alu_arg1);
			}
			state->inter.type = BRANCH;
			break;
		case 0x12: /* bnz */
			TRACE(state, 1, "core: exec bnz\n");
			if(gpreg_read(state, fields[0]) != 0) {
				TRACE(state, 1, "core: exec branch taken\n");
				alu_arg0 = fields[1];
				alu_arg1 = state->pc;
				state->inter.nextpc = alu_exec(ALU_OP_ADD, alu_arg0, alu_arg1);
			}
			state->inter.type = BRANCH;
			break;
		case 0x13: /* jr */
			TRACE(state, 1, "core: exec jr\n");
			alu_arg0 = gpreg_read(state, fields[0]);
			alu_arg1 = fields[1];
			state->inter.nextpc = alu_exec(ALU_OP_ADD, alu_arg0, alu_arg1);
			state->inter.type = BRANCH;
			break;
		case 0x14: /* jlr */
			TRACE(state, 1, "core: exec jlr\n");
			alu_arg0 = gpreg_read(state, fields[0]);
			alu_arg1 = fields[1];
			state->inter.regval = state->pc + 1;
			state->inter.regnum = 7;
			state->inter.nextpc = alu_exec(ALU_OP_ADD, alu_arg0, alu_arg1);
			state->inter.type = BRLINK;
			break;
		case 0x15: /* ll */
			TRACE(state, 1, "core: exec ll\n");
			alu_arg0 = gpreg_read(state, fields[1]);
			alu_arg1 = fields[2];
			output->data.addr = alu_exec(ALU_OP_ADD, alu_arg0, alu_arg1);
			output->data.read = true;
			output->data.request = true;
			state->inter.regnum = fields[0];
			STATUS_SET_LL(state->inter.newstat, true);
			state->inter.type = LOADLNK;
			break;
		case 0x16: /* ldw */
			TRACE(state, 1, "core: exec ldw\n");
			alu_arg0 = gpreg_read(state, fields[1]);
			alu_arg1 = fields[2];
			output->data.addr = alu_exec(ALU_OP_ADD, alu_arg0, alu_arg1);
			output->data.read = true;
			output->data.request = true;
			state->inter.regnum = fields[0];
			state->inter.type = LOAD;
			break;
		case 0x17: /* stw */
			TRACE(state, 1, "core: exec stw\n");
			alu_arg0 = gpreg_read(state, fields[1]);
			alu_arg1 = fields[2];
			output->data.addr = alu_exec(ALU_OP_ADD, alu_arg0, alu_arg1);
			output->data.value = gpreg_read(state, fields[0]);
			output->data.read = false;
			output->data.request = true;
			state->inter.type = STORE;
			break;
		case 0x18: /* sc */
			TRACE(state, 1, "core: exec sc\n");
			if(STATUS_GET_LL(state->ctl.named.status)) {
				TRACE(state, 1, "core: exec sc succeed\n");
				alu_arg0 = gpreg_read(state, fields[1]);
				alu_arg1 = fields[2];
				output->data.addr = alu_exec(ALU_OP_ADD, alu_arg0, alu_arg1);
				output->data.value = gpreg_read(state, fields[0]);
				output->data.read = false;
				output->data.request = true;
				state->inter.regnum = fields[0];
				state->inter.regval = 1;
				state->inter.type = SCOND;
			} else {
				TRACE(state, 1, "core: exec sc fail\n");
				state->inter.regnum = fields[0];
				state->inter.regval = 0;
				state->inter.type = GPRUP;
			}
			break;
		case 0x19: /* mfcp */
			TRACE(state, 1, "core: exec mfcp\n");
			if(STATUS_GET_UM(state->ctl.named.status)) {
				uint8_t cpmask = 0x1 << fields[1];
				if(! cpmask & state->ctl.named.umcpen) {
					TRACE(state, 1, "core: exec insufficient priviledge\n");
					state->inter.nextpc = state->ctl.named.privophdl;
					goto trap;
				}
			} 
			output->coproc.select = fields[1];
			output->coproc.op = COPROC_OP_READ;
			output->coproc.addr = fields[2];
			state->inter.type = CPREAD;
			state->inter.regnum = fields[0];
			break;
		case 0x1A: /* mtcp */
			TRACE(state, 1, "core: exec mtcp\n");
			if(STATUS_GET_UM(state->ctl.named.status)) {
				uint8_t cpmask = 0x1 << fields[1];
				if(! cpmask & state->ctl.named.umcpen) {
					TRACE(state, 1, "core: exec insufficient priviledge\n");
					state->inter.nextpc = state->ctl.named.privophdl;
					goto trap;
				}
			}
			output->coproc.select = fields[1];
			output->coproc.op = COPROC_OP_WRITE;
			output->coproc.addr = fields[2];
			output->coproc.value = gpreg_read(state, fields[0]);
			state->inter.type = BRANCH;
			break;
		case 0x1B: /* cpop */
			TRACE(state, 1, "core: exec cpop\n");
			if(STATUS_GET_UM(state->ctl.named.status)) {
				uint8_t cpmask = 0x1 << fields[1];
				if(! cpmask & state->ctl.named.umcpen) {
					TRACE(state, 1, "core: exec insufficient priviledge\n");
					state->inter.nextpc = state->ctl.named.privophdl;
					goto trap;
				}
			} 
			output->coproc.select = fields[1];
			output->coproc.op = COPROC_OP_EXEC;
			output->coproc.value = gpreg_read(state, fields[0]);
			state->inter.type = BRANCH;
			break;
		case 0x1C: /* int */
			TRACE(state, 1, "core: exec int\n");
			state->inter.regval = state->pc + 1;
			STATUS_SET_SWIARG(state->inter.newstat, fields[0]);
			status_trap(&(state->inter.newstat)); /* c.f. "goto trap;" */
			state->inter.nextpc = state->ctl.named.swihdl;
			state->inter.type = TRAP;
			break;
		case 0x1D: /* rfi */
			TRACE(state, 1, "core: exec rfi\n");
			if(STATUS_GET_UM(state->ctl.named.status)) {
				TRACE(state, 1, "core: exec insufficient priviledge\n");
				state->inter.nextpc = state->ctl.named.privophdl;
				goto trap;
			}
			status_rfi(&(state->inter.newstat));
			state->inter.nextpc = state->ctl.named.epc;
			state->inter.type = RFI;
			break;
		case 0x1E: /* mfctl */
			TRACE(state, 1, "core: exec mfctl\n");
			if(STATUS_GET_UM(state->ctl.named.status)) {
				TRACE(state, 1, "core: exec insufficient priviledge\n");
				state->inter.nextpc = state->ctl.named.privophdl;
				goto trap;
			}
			state->inter.regnum = fields[0];
			state->inter.regval = state->ctl.array[fields[1]];
			state->inter.type = GPRUP;
			break;
		case 0x1F: /* mtctl */
			TRACE(state, 1, "core: exec mtctl\n");
			if(STATUS_GET_UM(state->ctl.named.status)) {
				TRACE(state, 1, "core: exec insufficient priviledge\n");
				state->inter.nextpc = state->ctl.named.privophdl;
				goto trap;
			}
			state->inter.regnum = fields[1];
			state->inter.regval = gpreg_read(state, fields[0]);
			state->inter.type = CTLUP;
			break;
		default: /* ??? */
			TRACE(state, 1, "core: ERROR: exec fallthrough\n");
			return -1;
	}
	goto finish;

trap:
	status_trap(&(state->inter.newstat));
	state->inter.regval = state->pc;
	state->inter.type = TRAP;

finish:
#if 0
	TRACEBLOCK(state, 1, {
		char *type;
		printf("core: exec done\n");
		printf("core: inter regnum=%d regval=%x\n",
		       state->inter.regnum, state->inter.regval);
		printf("core: inter status %d us=%c ll=%c rum=%c gie=%c um=%c\n",
		       state->inter.newstat.swiarg,
		       state->inter.newstat.ret_um_save,
		       state->inter.newstat.load_link,
		       state->inter.newstat.ret_umode,
		       state->inter.newstat.glob_int_en,
		       state->inter.newstat.in_um);
		printf("core: inter nextpc=%x\n", state->inter.nextpc);
		switch(core->inter.type) {
			case GPRUP:   type = "GPRUP";   break;
			case BRANCH:  type = "BRANCH";  break;
			case BRLINK:  type = "BRLINK";  break;
			case LOAD:    type = "LOAD";    break;
			case LOADLNK: type = "LOADLNK"; break;
			case STORE:   type = "STORE";   break;
			case SCOND:   type = "SCOND";   break;
			case CTLUP:   type = "CTLUP";   break;
			case CPREAD:  type = "CPREAD";  break;
			case TRAP:    type = "TRAP";    break;
			case RFI:     type = "RFI";     break;
		}
		printf("core: inter type=%s\n", type);
	});
#endif
	return 0;
}

static void update_propagate_signals(
	sim_core_state_t *state,
	sim_core_output_t *output,
	sim_core_input_t *input)
{
	int i;
	/* propagate appropriate ctl register bits to output pins */
	output->user_mode = STATUS_GET_UM(state->ctl.named.status);

	/* propagate input pins to appropriate ctl register bits */
	for(i = 0; i < SIM_CORE_NUM_EXTINT; i++) {
		TRACEBLOCK(state, 1, {
			if((TEST_BIT(state->ctl.named.ipend, i) != 0) !=
			   input->exint_sig[i]) {
				if(input->exint_sig[i]) {
					printf("core: update ipend %d set\n", i);
				}
				else {
					printf("core: update ipend %d reset\n", i);
				}
			}
		});
		SET_BIT(state->ctl.named.ipend, i, input->exint_sig[i]);
	}
	SET_BIT(state->ctl.named.ipend, 14, input->instr.fault);
	SET_BIT(state->ctl.named.ipend, 15, input->data.fault);
}

int sim_core_update(
	sim_core_state_t *state,
	sim_core_output_t *output,
	sim_core_input_t *input)
{
	int status = 0;

	/* propagate input to state, state to output */
	update_propagate_signals(state, output, input);

	/* don't do anything if we've stalled at the exec stage */
	if(state->inter.instr_stall) {
		TRACE(state, 1, "core: update instr stall\n");
		goto counter;
	}

	/* sleeping and we aren't in a trap? noop. */
	if(state->sleeping && state->inter.type != TRAP) {
		TRACE(state, 1, "core: update sleeping\n");
		return 0;
	}

	state->ctl.named.status = state->inter.newstat;

	switch(state->inter.type) {
	case GPRUP:
		TRACE(state, 1, "core: update GPRUP\n");
		gpreg_write(state, state->inter.regnum, state->inter.regval);
		break;
	case BRANCH:
		/* just jump to nextpc (done by default) */
		TRACE(state, 1, "core: update BRANCH\n");
		break;
	case BRLINK:
		/* go to nextpc, but also save pc into R7 */
		TRACE(state, 1, "core: update BRLINK\n");
		gpreg_write(state, 7, state->inter.regval);
		break;
	case LOAD:
		if(! input->data.valid && ! input->data.fault) {
			TRACE(state, 1, "core: update LOAD data stall\n");
			state->inter.data_stall = true;
			output->instr.request = false;
			goto counter;
		} else if(input->data.fault) {
			TRACE(state, 1, "core: update LOAD data fault\n");
			status_trap(&(state->inter.newstat));
			state->inter.nextpc = state->ctl.named.dfaulthdl;
		} else {
			TRACE(state, 1, "core: update LOAD\n");
			STATUS_SET_LL(state->ctl.named.status, false);
			gpreg_write(state, state->inter.regnum, input->data.value);
			output->data.request = false;
		}
		state->inter.data_stall = false;
		break;
	case LOADLNK:
		if(! input->data.valid && ! input->data.fault) {
			TRACE(state, 1, "core: update LOADLNK data stall\n");
			state->inter.data_stall = true;
			output->instr.request = false;
			goto counter;
		} else if(input->data.fault) {
			TRACE(state, 1, "core: update LOADLNK data fault\n");
			status_trap(&(state->inter.newstat));
			state->inter.nextpc = state->ctl.named.dfaulthdl;
		} else {
			TRACE(state, 1, "core: update LOADLNK\n");
			STATUS_SET_LL(state->ctl.named.status, true);
			gpreg_write(state, state->inter.regnum, input->data.value);
			output->data.request = false;
		}
		state->inter.data_stall = false;
		break;
	case STORE:
		if(! input->data.valid && ! input->data.fault) {
			TRACE(state, 1, "core: update STORE data stall\n");
			state->inter.data_stall = true;
			output->instr.request = false;
			goto counter;
		} else if(input->data.fault) {
			TRACE(state, 1, "core: update STORE data fault\n");
			status_trap(&(state->inter.newstat));
			state->inter.nextpc = state->ctl.named.dfaulthdl;
		} else {
			TRACE(state, 1, "core: update STORE\n");
			STATUS_SET_LL(state->ctl.named.status, false);
			output->data.request = false;
		}
		state->inter.data_stall = false;
		break;
	case SCOND:
		if(! input->data.valid && ! input->data.fault) {
			TRACE(state, 1, "core: update SCOND data stall\n");
			state->inter.data_stall = true;
			output->instr.request = false;
			goto counter;
		}
		else if(input->data.fault) {
			TRACE(state, 1, "core: update SCOND data fault\n");
			status_trap(&(state->inter.newstat));
			state->inter.nextpc = state->ctl.named.dfaulthdl;
		} else {
			TRACE(state, 1, "core: update SCOND\n");
			STATUS_SET_LL(state->ctl.named.status, false);
			output->data.request = false;
			gpreg_write(state, state->inter.regnum, 1);
		}
		state->inter.data_stall = false;
		break;
	case CTLUP:
		TRACE(state, 1, "core: update CTLUP reg %d val %x\n",
		      state->inter.regnum, state->inter.regval);
		state->ctl.array[state->inter.regnum] = state->inter.regval;
		break;
	case CPREAD:
		TRACE(state, 1, "core: update CPREAD\n");
		gpreg_write(state, state->inter.regnum, input->coproc.value);
		break;
	case TRAP:
		TRACE(state, 1, "core: update TRAP\n");
		state->ctl.named.epc_saved = state->ctl.named.epc;
		state->ctl.named.epc = state->inter.regval;
		state->sleeping = false;
		break;
	case RFI:
		TRACE(state, 1, "core: update RFI\n");
		state->ctl.named.epc = state->ctl.named.epc_saved;
		break;
	case SLEEP:
		TRACE(state, 1, "core: update SLEEP\n");
		state->sleeping = true;
		// when the trap comes to disloge this, we need to continue at the
		// next instruction
		state->pc = state->inter.nextpc;
		output->instr.request = false;
		// indicate if we've gone into an unwakeable sleep
		if(! STATUS_GET_GIE(state->ctl.named.status) ||
		   ! state->ctl.named.ienable) {
			TRACE(state, 1, "core: unwakeable SLEEP\n");
			status = -1;
		}

		goto counter;
	}

	state->pc = state->inter.nextpc;
	output->instr.addr = state->pc;
	output->instr.request = true;

counter:
	/* clear coprocessor bus */
	output->coproc.op = COPROC_OP_NONE;

	state->ctl.named.counter_low++;
	if(! state->ctl.named.counter_low) {
		state->ctl.named.counter_high++;
	}

	return status;
}

int sim_core_cp_op_pending(
	coproc_op_t op,
	sim_core_output_t *output)
{
	if(output->coproc.op == op) {
		return output->coproc.select;
	}
	return -1;
}
