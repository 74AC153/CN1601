#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>

#include "sim_cp_if.h"
#include "sim_cp_nvram.h"

#define NVRAM_CTL(STATE) ((STATE)->hdr.regs[CP_NVRAM_REG_CTL])
#define NVRAM_STATUS(STATE) ((STATE)->hdr.regs[CP_NVRAM_REG_STATUS])
#define NVRAM_ADDR_LO(STATE) ((STATE)->hdr.regs[CP_NVRAM_REG_ADDR_LO])
#define NVRAM_ADDR_HI(STATE) ((STATE)->hdr.regs[CP_NVRAM_REG_ADDR_HI])
#define NVRAM_VAL(STATE) ((STATE)->hdr.regs[CP_NVRAM_REG_VAL])

int nvram_state_init(sim_cp_state_hdr_t *hdr)
{
	int ch;
	char *data_path;
	struct stat sb;
	sim_cp_nvram_state_t *state = (sim_cp_nvram_state_t *) hdr;

	while((ch = getopt(hdr->argc, hdr->argv, "i:")) != -1) {
		switch(ch) {
		case 'i': data_path = optarg; break;
		default:
			printf("nvram: unknown switch %c\nusage: -i <imagefile>", ch);
			return -1;
		}
	}
	if(data_path == NULL) {
		printf("nvram: -i <imagefile> required");
		return -1;
	}

	state->datafd = open(data_path, O_RDWR);
	if(state->datafd < 0) {
		fprintf(stderr, "nvram: error open %s: %s (errno=%d)\n",
		        data_path, strerror(errno), errno);
		return -1;
	}
	if(fstat(state->datafd, &sb) < 0) {
		fprintf(stderr, "nvram: error stat %s: %s (errno=%d)\n",
		        data_path, strerror(errno), errno);
		return -1;
	}
	state->datalen = sb.st_size;
	if(state->datalen & 1) {
		fprintf(stderr, "nvram: image file len must be even\n");
		return -1;
	}
	state->data = mmap(NULL, state->datalen,
	                         PROT_READ | PROT_WRITE, 0,
	                         state->datafd, 0);
	if(state->data == MAP_FAILED) {
		fprintf(stderr, "nvram: error mmap %s: %s (errno=%d)\n",
		        data_path, strerror(errno), errno);
		return -1;
	}

	printf("nvram: loaded %s (%d words)\n",
	       data_path, state->datalen >> 1);

	return 0;
}

int nvram_state_deinit(sim_cp_state_hdr_t *hdr)
{
	int status;
	sim_cp_nvram_state_t *state = (sim_cp_nvram_state_t *) hdr;
	status = munmap(state->data, state->datalen);
	if(status) {
		fprintf(stderr, "nvram: error munmap: %s (errno=%d)\n",
		        strerror(errno), errno);
		return -1;
	}
	close(state->datafd);
	return 0;
}

int nvram_state_reset(sim_cp_state_hdr_t *hdr)
{
	sim_cp_nvram_state_t *state = (sim_cp_nvram_state_t *) hdr;
	state->delay = 0;
	state->inst_pend = CP_NVRAM_INSTR_NOOP;
	state->addr_pend = 0;
	state->val_pend = 0;
	return 0;
}

int nvram_state_data(sim_cp_state_hdr_t *hdr)
{
	sim_cp_nvram_state_t *state = (sim_cp_nvram_state_t *) hdr;

	if(state->delay) {
		TRACE(hdr, 1, "nvram: delay %d -> %d\n",
		      state->delay, state->delay-1);
		state->delay--;
	}

	return 0;
}

int nvram_state_exec(sim_cp_state_hdr_t *hdr)
{
	sim_cp_nvram_state_t *state = (sim_cp_nvram_state_t *) hdr;
	uint32_t addr;

	/* ignore all requests if an operation is pending */
	if(state->delay != 0) {
		return 0;
	}

	/* if our delay has been decremented to 0 and an instruction is pending,
	   return the result of that instruction */
	switch(state->inst_pend) {
	case CP_NVRAM_INSTR_READ:
		TRACE(hdr, 1, "nvram: exec read addr %x\n",
		      state->addr_pend);
		if(state->addr_pend < state->datalen >> 1) {
			NVRAM_VAL(state) = ((uint16_t*)state->data)[state->addr_pend];
			NVRAM_STATUS(state) = CP_NVRAM_REG_STATUS_READ_OK;
		} else {
			NVRAM_STATUS(state) = CP_NVRAM_REG_STATUS_READ_ERR;
		}
		state->inst_pend = CP_NVRAM_INSTR_NOOP;

		if(NVRAM_CTL(state) & CP_NVRAM_REG_CTL_READ_INT) {
			hdr->core_input->exint_sig[hdr->cpnum] = true;
		}
		break;

	case CP_NVRAM_INSTR_WRITE:
		TRACE(hdr, 1, "nvram: exec write addr %x = %x\n",
		      state->addr_pend, state->val_pend);
		if(state->addr_pend < state->datalen >> 1) {
			((uint16_t*)state->data)[state->addr_pend] = state->val_pend;
			NVRAM_STATUS(state) = CP_NVRAM_REG_STATUS_WRITE_OK;
		} else {
			NVRAM_STATUS(state) = CP_NVRAM_REG_STATUS_WRITE_ERR;
		}
		state->inst_pend = CP_NVRAM_INSTR_NOOP;

		if(NVRAM_CTL(state) & CP_NVRAM_REG_CTL_WRITE_INT) {
			hdr->core_input->exint_sig[hdr->cpnum] = true;
		}
		break;

	default:
		break;
	}

	addr = NVRAM_ADDR_LO(state) + (NVRAM_ADDR_HI(state) << 16);

	switch(hdr->core_output->coproc.value) {
	case CP_NVRAM_INSTR_READ:
		if(NVRAM_STATUS(state) != CP_NVRAM_REG_STATUS_IDLE) {
			TRACE(hdr, 1, "nvram: queue read but not idle\n");
			break;
		}
		TRACE(hdr, 1, "nvram: queue read\n");
		state->inst_pend = CP_NVRAM_INSTR_READ;
		state->addr_pend = addr;
		state->delay = CP_NVRAM_DELAY_CYCLES;
		break;

	case CP_NVRAM_INSTR_WRITE:
		if(NVRAM_STATUS(state) != CP_NVRAM_REG_STATUS_IDLE) {
			TRACE(hdr, 1, "nvram: queue write but not idle\n");
			break;
		}
		TRACE(hdr, 1, "nvram: queue write\n");
		state->inst_pend = CP_NVRAM_INSTR_WRITE;
		state->addr_pend = addr;
		state->val_pend = NVRAM_VAL(state);
		state->delay = CP_NVRAM_DELAY_CYCLES;
		break;

	case CP_NVRAM_INSTR_ACK:
		TRACE(hdr, 1, "nvram: ack\n");
		NVRAM_STATUS(state) = CP_NVRAM_REG_STATUS_IDLE;
		break;

	default:
		break;
	}

	return 0;
}

int nvram_state_print(sim_cp_state_hdr_t *hdr)
{
	sim_cp_nvram_state_t *state = (sim_cp_nvram_state_t *) hdr;
	printf("ctl=%4.4x status=%4.4x addr_lo=%4.4x addr_hi=%4.4x val=%4.4x\n",
	       NVRAM_CTL(state), NVRAM_STATUS(state), NVRAM_ADDR_LO(state),
	       NVRAM_ADDR_HI(state), NVRAM_VAL(state));

	printf("delay=%x inst pend=%d addr pend=%4.4x val pend=%4.4x\n",
	       state->delay, state->inst_pend,
	       state->addr_pend, state->val_pend);
	return 0;
}
