#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "sim_cp_nvram.h"

#if ! defined(NOTRACE)
#define TRACE(STATE, ...) do{if((STATE)->trace)printf( __VA_ARGS__ );}while(0)
#define TRACEBLOCK(state, BLOCK) do{if((STATE)->trace) BLOCK}while(0)
#else /* defined (NOTRACE) */
#define TRACE(STATE, ...)
#define TRACEBLOCK(state, BLOCK)
#endif

int sim_cp_nvram_init(
	sim_cp_nvram_state_t *nvram_state,
	char *data_path,
	unsigned int cpnum)
{
	struct stat sb;

	nvram_state->datafd = open(data_path, O_RDWR);
	if(nvram_state->datafd < 0) {
		fprintf(stderr, "nvram: error open %s: %s (errno=%d)\n",
		        data_path, strerror(errno), errno);
		return -1;
	}
	if(fstat(nvram_state->datafd, &sb) < 0) {
		fprintf(stderr, "nvram: error stat %s: %s (errno=%d)\n",
		        data_path, strerror(errno), errno);
		return -1;
	}
	nvram_state->datalen = sb.st_size;
	nvram_state->data = mmap(NULL, nvram_state->datalen,
	                         PROT_READ | PROT_WRITE, 0,
	                         nvram_state->datafd, 0);
	if(nvram_state->data == MAP_FAILED) {
		fprintf(stderr, "nvram: error mmap %s: %s (errno=%d)\n",
		        data_path, strerror(errno), errno);
		return -1;
	}

	nvram_state->cpnum = cpnum;
	nvram_state->trace = false;

	return 0;
}

void sim_cp_nvram_deinit(
	sim_cp_nvram_state_t *nvram_state)
{
	munmap(nvram_state->data, nvram_state->datalen);
	close(nvram_state->datafd);
}

int sim_cp_nvram_reset(
	sim_cp_nvram_state_t *state)
{

}

void sim_cp_nvram_access(
	sim_core_input_t *core_input,
	sim_core_output_t *core_output,
	sim_cp_nvram_state_t *nvram_state)
{

	if(nvram_state->delay) {
		TRACE(nvram_state, "nvram: delay %d\n", nvram_state->delay);
		nvram_state->delay--;
	}

	if(core_output->coproc.select != nvram_state->cpnum) {
		return;
	}

	switch(core_output->coproc.op) {
	case COPROC_OP_READ:
		TRACE(nvram_state, "nvram: read reg %d\n",
		      core_output->coproc.addr);
		core_input->coproc.value =
		  nvram_state->regs.array[core_output->coproc.addr];
		break;

	case COPROC_OP_WRITE:
		TRACE(nvram_state, "nvram: write reg %d\n",
		      core_output->coproc.addr);
		if(core_output->coproc.addr == 0) {
			/* status register is read only */
			TRACE(nvram_state, "nvram: reg %d is read only\n",
			      core_output->coproc.addr);
			break;
		}
		nvram_state->regs.array[core_output->coproc.addr] =
		  core_output->coproc.value;
		break;

	case COPROC_OP_EXEC:
		if(nvram_state->delay != 0) {
			break;
		}

		switch(nvram_state->inst_pend) {
		case CP_NVRAM_INSTR_READ:
			TRACE(nvram_state, "nvram: exec read addr %x\n",
			      nvram_state->addr_pend);
			nvram_state->regs.named.val =
			  ((uint16_t*)nvram_state->data)[nvram_state->addr_pend];
			nvram_state->regs.named.status = NVRAM_STATUS_READ_OK;
			nvram_state->inst_pend = CP_NVRAM_INSTR_NOOP;
			break;
	
		case CP_NVRAM_INSTR_WRITE:
			TRACE(nvram_state, "nvram: exec write addr %x = %x\n",
			      nvram_state->addr_pend, nvram_state->val_pend);
			((uint16_t*)nvram_state->data)[nvram_state->addr_pend] =
			  nvram_state->val_pend;
			nvram_state->regs.named.status = NVRAM_STATUS_WRITE_OK;
			nvram_state->inst_pend = CP_NVRAM_INSTR_NOOP;
			break;
	
		default:
			break;
		}

	
		switch(core_output->coproc.value) {
			uint32_t addr;
			addr = nvram_state->regs.named.addr_lo +
			           (nvram_state->regs.named.addr_hi << 16);
		case CP_NVRAM_INSTR_READ:
			if(nvram_state->regs.named.status != NVRAM_STATUS_IDLE) {
				TRACE(nvram_state, "nvram: queue read but not idle\n");
				break;
			}
			TRACE(nvram_state, "nvram: queue read\n");
			nvram_state->inst_pend = CP_NVRAM_INSTR_READ;
			nvram_state->addr_pend = addr;
			break;
	
		case CP_NVRAM_INSTR_WRITE:
			if(nvram_state->regs.named.status != NVRAM_STATUS_IDLE) {
				TRACE(nvram_state, "nvram: queue writebut not idle\n");
				break;
			}
			TRACE(nvram_state, "nvram: queue write\n");
			nvram_state->inst_pend = CP_NVRAM_INSTR_WRITE;
			nvram_state->addr_pend = addr;
			nvram_state->val_pend = nvram_state->regs.named.val;
			break;

		case CP_NVRAM_INSTR_ACK:
			TRACE(nvram_state, "nvram: ack\n");
			nvram_state->regs.named.status = NVRAM_STATUS_IDLE;
			break;
		default:
			break;
		}
		break;

	default: 
		break;
	}

}

