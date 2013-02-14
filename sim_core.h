#ifndef SIM_CORE_H
#define SIM_CORE_H

#include <stdint.h>
#include <stdbool.h>

#define SIM_CORE_NUM_GPREGS 8
#define SIM_CORE_NUM_CTLREGS 32
#define SIM_CORE_NUM_EXTINT 8
#define SIM_CORE_NUM_CPREGS 32

typedef enum
{
	COPROC_OP_NONE = 0,
	COPROC_OP_READ = 1,
	COPROC_OP_WRITE = 2,
	COPROC_OP_EXEC = 3
} coproc_op_t;

typedef struct
{
	uint16_t value;
	bool valid;
	bool fault;
} sim_core_mem_input_t;

typedef struct
{
	uint16_t value;
	uint16_t addr;
	bool read;
	bool request;
} sim_core_mem_output_t;

typedef struct
{
	uint16_t value;
} sim_core_cp_input_t;

typedef struct
{
	uint8_t select;
	coproc_op_t op;
	uint8_t addr;
	uint16_t value;
} sim_core_cp_output_t;

typedef struct
{
	sim_core_mem_input_t data;
	sim_core_mem_input_t instr;

	sim_core_cp_input_t coproc;

	bool exint_sig[SIM_CORE_NUM_EXTINT];

	bool ll_inval;
} sim_core_input_t;

typedef struct
{
	sim_core_mem_output_t data;
	sim_core_mem_output_t instr;

	sim_core_cp_output_t coproc;

	bool user_mode;
} sim_core_output_t;

#define STATUS_GET_SWIARG(STAT) ((STAT) & 0x7FF)
#define STATUS_SET_SWIARG(STAT, VAL) \
	do{ (STAT) = ((STAT) & 0xF800) | ((VAL) & 0x7FF); } while (0)

#define STATUS_GET_UMSAVE(STAT) (((STAT) >> 11) & 0x1)
#define STATUS_SET_UMSAVE(STAT, VAL) \
	do{ (STAT) = ((STAT) & 0xF7FF) | (((VAL) & 0x1) << 11); } while (0)

#define STATUS_GET_LL(STAT) (((STAT) >> 12) & 0x1)
#define STATUS_SET_LL(STAT, VAL) \
	do { (STAT) = ((STAT) & 0xEFFF) | (((VAL) & 0x1) << 12); } while(0)

#define STATUS_GET_RUMODE(STAT) (((STAT) >> 13) & 0x1)
#define STATUS_SET_RUMODE(STAT, VAL) \
	do { (STAT) = ((STAT) & 0xDFFF) | (((VAL) & 0x1) << 13); } while(0)

#define STATUS_GET_GIE(STAT) (((STAT) >> 14) & 0x1)
#define STATUS_SET_GIE(STAT, VAL) \
	do { (STAT) = ((STAT) & 0xBFFF) | (((VAL) & 0x1) << 14); } while(0)

#define STATUS_GET_UM(STAT) (((STAT) >> 15) & 0x1)
#define STATUS_SET_UM(STAT, VAL) \
	do { (STAT) = ((STAT) & 0x7FFF) | (((VAL) & 0x1) << 15); } while(0)

typedef struct
{
	uint16_t pc;
	uint16_t gpr_super[SIM_CORE_NUM_GPREGS];
	union {
		uint16_t array[SIM_CORE_NUM_CTLREGS];
		struct {
			uint16_t gpr_user[SIM_CORE_NUM_GPREGS];
			uint16_t epc;
			uint16_t epc_saved;
			uint16_t status;
			uint16_t ifaulthdl;
			uint16_t dfaulthdl;
			uint16_t swihdl;
			uint16_t illophdl;
			uint16_t privophdl;
			uint16_t exihdl[SIM_CORE_NUM_EXTINT];
			uint16_t ienable;
			uint16_t ipend;
			uint16_t iack;
			uint16_t umcpen;
			uint16_t counter_low;
			uint16_t counter_high;
		} named;
	} ctl;
	bool sleeping;
	
	/* intermediate state between exec and update */
	struct {
		unsigned int regnum;
		uint16_t regval; /* also EPC */
		uint16_t newstat;
		uint16_t nextpc;
		bool instr_stall;
		bool data_stall;
		enum {
			GPRUP, /* single general-purpose register update */
			BRANCH, /* PC just needs updating */
			BRLINK, /* branch & link, PC and R7 need update */
			LOAD,
			LOADLNK, /* load link */
			STORE,
			SCOND,
			CTLUP, /* ctl reg update */
			CPREAD, /* read coproc bus */
			TRAP,
			RFI,
			SLEEP
		} type;
	} inter;

	/* miscellaneous control */
	bool trace;
} sim_core_state_t;


sim_core_output_t *sim_core_output_init(
	sim_core_output_t *output);

sim_core_input_t *sim_core_input_init(
	sim_core_input_t *input);

sim_core_state_t *sim_core_state_init(
	sim_core_state_t *state);

int sim_core_exec(
	sim_core_state_t *state,
	sim_core_output_t *output,
	sim_core_input_t *input);

int sim_core_update(
	sim_core_state_t *state,
	sim_core_output_t *output,
	sim_core_input_t *input);

#endif
