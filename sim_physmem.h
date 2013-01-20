#ifndef SIM_PHYSMEM_H
#define SIM_PHYSMEM_H

#include <stdint.h>

#include "sim_core.h"

#define SIM_PHYSMEM_ADDR_BITS 16
#define SIM_PHYSMEM_DATA_BITS 16

extern uint16_t sim_physmem[];
#define SIM_PHYSMEM_LEN (0x1 << (SIM_PHYSMEM_ADDR_BITS-1))

typedef struct {
	uint8_t *daddr[SIM_PHYSMEM_ADDR_BITS];
	uint8_t *dval[SIM_PHYSMEM_DATA_BITS];
	uint8_t *dreq;
	uint8_t *dread;
	uint8_t *dvalid;
	uint8_t *iaddr[SIM_PHYSMEM_ADDR_BITS];
	uint8_t *ival[SIM_PHYSMEM_DATA_BITS];
	uint8_t *ireq;
	uint8_t *ivalid;
} sim_physmem_io_t;

void sim_physmem_io_setup_data(sim_physmem_io_t *pmio, sim_core_io_t *coreio);
void sim_physmem_io_setup_inst(sim_physmem_io_t *pmio, sim_core_io_t *coreio);
int sim_physmem_tick(sim_physmem_io_t *io);

#endif
