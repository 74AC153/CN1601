#ifndef SIM_CP_NVRAM_H
#define SIM_CP_NVRAM_H

#include <stdint.h>
#include "sim_cp_if.h"
#include "sim_core.h"

#define CP_NVRAM_REG_CTL 0
#define CP_NVRAM_REG_CTL_READ_INT 0x1
#define CP_NVRAM_REG_CTL_WRITE_INT 0x2

#define CP_NVRAM_REG_STATUS 1
#define CP_NVRAM_REG_STATUS_IDLE 0
#define CP_NVRAM_REG_STATUS_READ_PENDING 1
#define CP_NVRAM_REG_STATUS_READ_OK 2
#define CP_NVRAM_REG_STATUS_READ_ERR 3
#define CP_NVRAM_REG_STATUS_WRITE_PENDING 4
#define CP_NVRAM_REG_STATUS_WRITE_OK 5
#define CP_NVRAM_REG_STATUS_WRITE_ERR 6

#define CP_NVRAM_REG_ADDR_LO 2
#define CP_NVRAM_REG_ADDR_HI 3
#define CP_NVRAM_REG_VAL 4

#define CP_NVRAM_INSTR_NOOP 0
#define CP_NVRAM_INSTR_READ 1
#define CP_NVRAM_INSTR_WRITE 2
#define CP_NVRAM_INSTR_ACK 3
#define CP_NVRAM_INSTR_GETLEN_LO 4
#define CP_NVRAM_INSTR_GETLEN_HI 5


#define CP_NVRAM_DELAY_CYCLES 3


typedef struct {
	sim_cp_state_hdr_t hdr;

	void *data;
	unsigned int datalen;
	int datafd;

	unsigned int delay;
	uint16_t inst_pend;
	uint16_t addr_pend;
	uint16_t val_pend;
} sim_cp_nvram_state_t;

int nvram_state_init(sim_cp_state_hdr_t *hdr);
int nvram_state_deinit(sim_cp_state_hdr_t *hdr);
int nvram_state_reset(sim_cp_state_hdr_t *hdr);
int nvram_state_data(sim_cp_state_hdr_t *hdr);
int nvram_state_exec(sim_cp_state_hdr_t *hdr);
int nvram_state_print(sim_cp_state_hdr_t *hdr);

#endif
