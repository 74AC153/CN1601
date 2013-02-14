#ifndef SIM_CP_NVRAM_H
#define SIM_CP_NVRAM_H

#include <stdint.h>
#include "sim_core.h"

typedef enum {
	CP_NVRAM_INSTR_NOOP,
	CP_NVRAM_INSTR_READ,
	CP_NVRAM_INSTR_WRITE,
	CP_NVRAM_INSTR_ACK,
} cp_nvram_instr_t;

#define NVRAM_STATUS_IDLE          0
#define NVRAM_STATUS_READ_PENDING  1
#define NVRAM_STATUS_READ_OK       2
#define NVRAM_STATUS_READ_ERR      3
#define NVRAM_STATUS_WRITE_PENDING 4
#define NVRAM_STATUS_WRITE_OK      5
#define NVRAM_STATUS_WRITE_ERR     6

#define NVRAM_CTL_READ_OK_INT 0x1
#define NVRAM_CTL_READ_ERR_INT 0x2
#define NVRAM_CTL_WRITE_OK_INT 0x4
#define NVRAM_CTL_WRITE_ERR_INT 0x8

typedef struct {
	union {
		uint16_t array[SIM_CORE_NUM_CPREGS];
		struct {
			uint16_t status;
			uint16_t ctl;
			uint16_t addr_lo;
			uint16_t addr_hi;
			uint16_t val;
		} named;
	} regs;

	unsigned int cpnum;
	bool trace;

	void *data;
	size_t datalen;
	int datafd;

	unsigned int delay;
	cp_nvram_instr_t inst_pend;
	uint16_t addr_pend;
	uint16_t val_pend;
} sim_cp_nvram_state_t;

int sim_cp_nvram_init(
	sim_cp_nvram_state_t *state,
	char *data_path,
	unsigned int cpnum);

void sim_cp_nvram_deinit(
	sim_cp_nvram_state_t *nvram_state);

int sim_cp_nvram_reset(
	sim_cp_nvram_state_t *state);

void sim_cp_nvram_access(
	sim_core_input_t *core_input,
	sim_core_output_t *core_output,
	sim_cp_nvram_state_t *nvram_state);

	
#endif
