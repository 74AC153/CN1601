#include <stdint.h>
#include "sim_utils.h"
#include "sim_physmem.h"
#include "sim_core.h"

uint16_t sim_physmem[SIM_PHYSMEM_LEN] = { 0 };

static uint8_t g_zero = 0;

void sim_physmem_io_setup_data(sim_physmem_io_t *pmio, sim_core_io_t *coreio)
{
	int i;

	for(i = 0; i < SIM_CORE_DADDR_LINES && i < SIM_PHYSMEM_ADDR_BITS; i++) {
		pmio->daddr[i] = coreio->daddr[i];
	}

	for( ; i < SIM_CORE_DADDR_LINES; i++) {
		pmio->daddr[i] = &g_zero;
	}

	for(i = 0; i < SIM_CORE_DVAL_LINES && i < SIM_PHYSMEM_DATA_BITS; i++) {
		pmio->dval[i] = coreio->dval[i];
	}

	for( ; i < SIM_CORE_DVAL_LINES; i++) {
		pmio->dval[i] = &g_zero;
	}

	pmio->dreq = coreio->dreq;
	pmio->dread = coreio->dread;
	pmio->dvalid = coreio->dvalid;

	for(i = 0; i < SIM_CORE_IADDR_LINES && i < SIM_PHYSMEM_ADDR_BITS; i++) {
		pmio->iaddr[i] = coreio->iaddr[i];
	}

	for( ; i < SIM_CORE_IADDR_LINES; i++) {
		pmio->iaddr[i] = &g_zero;
	}

	for(i = 0; i < SIM_CORE_IVAL_LINES && i < SIM_PHYSMEM_DATA_BITS; i++) {
		pmio->ival[i] = coreio->ival[i];
	}

	for( ; i < SIM_CORE_IVAL_LINES; i++) {
		pmio->ival[i] = &g_zero;
	}

	pmio->ireq = coreio->ireq;
	pmio->ivalid = coreio->ivalid;
}

int sim_physmem_tick(sim_physmem_io_t *io)
{
	uint32_t addr;
	uint32_t data;

	if(*io->ireq) {
		demux_lines(io->iaddr, SIM_PHYSMEM_ADDR_BITS, &addr);

		data = sim_physmem[addr];
		mux_lines(data, io->ival, SIM_PHYSMEM_DATA_BITS);

		*(io->ivalid) = 1;
	}

	if(*io->dreq) {
		demux_lines(io->daddr, SIM_PHYSMEM_ADDR_BITS, &addr);

		if(*io->dread) {
			data = sim_physmem[addr];
			mux_lines(data, io->dval, SIM_PHYSMEM_DATA_BITS);
		} else {
			demux_lines(io->dval, SIM_PHYSMEM_DATA_BITS, &data);
			sim_physmem[addr] = data;
		}

		*(io->dvalid) = 1;
	}
}
