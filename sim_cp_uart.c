#include <stdio.h>
#include <util.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "sim_cp_uart.h"

#define POLL_TIMEOUT_MS 100

typedef struct {
	msg_t hdr;
	char c;
} charmsg_t;

charmsg_t *charmsg_new(char c)
{
	charmsg_t *m = malloc(sizeof(charmsg_t));
	if(m) {
		msg_init(m);
		m->c = c;
	}
	return m;
}

void *rx_thr_fun(void *p)
{
	struct pollfd poll_io;
	int status;
	char c;
	size_t used;
	charmsg_t *m;
	sim_cp_uart_state_t *state = (sim_cp_uart_state_t *) p;

	fprintf(stderr, "uart: rx thr start\n");

	poll_io.fd = state->rxfd;
	poll_io.events = POLLIN;
	while(state->run_threads) {
		poll_io.revents = 0;
		status = poll(&poll_io, 1, POLL_TIMEOUT_MS);
		if(status < 0) {
			fprintf(stderr, "uart: poll error: %s (errno=%d)\n",
			       strerror(errno), errno);
			if(errno != EINTR && errno != EAGAIN) {
				break;
			}
			continue;
		}
		if(poll_io.revents & (POLLERR | POLLNVAL) ) {
			fprintf(stderr, "uart: poll returned err=%d nval=%d\n",
				    (poll_io.revents & POLLERR) != 0,
			        (poll_io.revents & POLLNVAL) != 0);
			continue;
		}
		if(status == 0) {
			continue;
		}

		status = read(state->rxfd, &c, 1);
		if(status < 0) {
			printf("uart: read error %s (errno=%d)\n",
			       strerror(errno), errno);
			if(errno != EINTR) {
				break;
			}
		}
		if(status == 0) {
			continue;
		}
		TRACE(&state->hdr, 2, "uart: received char %c\n", c);

		used = mbox_len(&(state->in_box));
		m = NULL;
		if(state->fifo_en && used < state->fifo_len) {
			TRACE(&state->hdr, 2, "uart: fifo rx space avail\n");
			assert(m = charmsg_new(c));
		} else if(!used) {
			TRACE(&state->hdr, 2, "uart: rx space avail\n");
			assert(m = charmsg_new(c));
		}
		if(m) {
			TRACE(&state->hdr, 2, "uart: post msg\n");
			mbox_msg_post(&(state->in_box), &(m->hdr));
		}
	}

	fprintf(stderr, "uart: rx thr finish\n");

	return NULL;
}

void *tx_thr_fun(void *p)
{
	charmsg_t *m;
	sim_cp_uart_state_t *state = (sim_cp_uart_state_t *) p;

	fprintf(stderr, "uart: tx thr start\n");

	while(state->run_threads) {
		m = (charmsg_t *) mbox_msg_get_wait(&(state->out_box),
		                                    POLL_TIMEOUT_MS/1000,
		                                    (POLL_TIMEOUT_MS%1000) * 1000000);
		if(m) {
			write(state->txfd, &(m->c), 1);
			TRACE(&state->hdr, 2, "uart: sent char %c\n", m->c);
			free(m);
		}
	}

	fprintf(stderr, "uart: tx thr finish\n");

	return NULL;
}

int uart_state_init(sim_cp_state_hdr_t *hdr)
{
	int status, ch;
	sim_cp_uart_state_t *state = (sim_cp_uart_state_t *) hdr;


	optind = 0;
	while((ch = getopt(hdr->argc, hdr->argv, "r:t:")) != -1) {
		switch(ch) {
		case 'r':
			state->rxpath = optarg;
			break;
		case 't':
			state->txpath = optarg;
			break;
		default:
			fprintf(stderr, "uart: unknown switch -%c", ch);
			return -1;
		}
	}
	if(state->rxpath) {
		fprintf(stderr, "uart: opening Rx FIFO at %s...\n", state->rxpath);
		state->rxfd = open(state->rxpath, O_RDONLY);
		if(state->rxfd < 0) {
			fprintf(stderr, "uart: error opening %s: %s (errno=%d)\n",
			        state->rxpath, strerror(errno), errno);
			return -1;
		}
		fprintf(stderr, "uart: Rx FIFO at %s opened\n", state->rxpath);
	} else {
		state->rxfd = -1;
	}
	if(state->txpath) {
		fprintf(stderr, "uart: opening Tx FIFO at %s...\n", state->txpath);
		state->txfd = open(state->txpath, O_WRONLY);
		if(state->txfd < 0) {
			fprintf(stderr, "uart: error opening %s: %s (errno=%d)\n",
			        state->txpath, strerror(errno), errno);
			return -1;
		}
		fprintf(stderr, "uart: Tx FIFO at %s opened\n", state->txpath);
	} else {
		state->txfd = -1;
	}

	mbox_init(&(state->in_box));
	mbox_init(&(state->out_box));
	state->fifo_len = FIFO_LEN;
	state->run_threads = 1;

	if(state->rxfd >= 0) {
		pthread_create(&(state->rx_thr), NULL, rx_thr_fun, state);
	}
	if(state->txfd >= 0) {
		pthread_create(&(state->tx_thr), NULL, tx_thr_fun, state);
	}

	return 0;
}

int uart_state_deinit(sim_cp_state_hdr_t *hdr)
{
	sim_cp_uart_state_t *state = (sim_cp_uart_state_t *) hdr;
	state->run_threads = false;
	if(state->rxfd >= 0) {
		pthread_join(state->rx_thr, NULL);
	}
	if(state->txfd >= 0) {
		pthread_join(state->tx_thr, NULL);
	}
	close(state->rxfd);
	close(state->txfd);
	return 0;
}

int uart_state_reset(sim_cp_state_hdr_t *hdr)
{
	size_t i, n;
	charmsg_t *m;
	sim_cp_uart_state_t *state = (sim_cp_uart_state_t *) hdr;

	for(i = 0, n = mbox_len(&state->in_box); i < n; i++) {
		m = (charmsg_t *) mbox_msg_get(&state->in_box);
		free(m);
	}

	for(i = 0, n = mbox_len(&state->out_box); i < n; i++) {
		m = (charmsg_t *) mbox_msg_get(&state->out_box);
		free(m);
	}

	return 0;
}

int uart_state_data(sim_cp_state_hdr_t *hdr)
{
	size_t n;
	sim_cp_uart_state_t *state = (sim_cp_uart_state_t *) hdr;

	/* check for data in Rx queue, set status, raise interrupt if requested */
	n = mbox_len(&state->in_box);
	if(n) {
		TRACE(&state->hdr, 1, "uart: rx data available\n");
		state->hdr.regs[CP_UART_REG_STATUS] |= CP_UART_REG_STATUS_RX_AVAIL;
		if(!state->rx_raised && 
		   state->hdr.regs[CP_UART_REG_CTL] & CP_UART_REG_CTL_RX_AVAIL_INT){
			TRACE(&state->hdr, 1, "uart: raise rx data interrupt\n");
			state->hdr.core_input->exint_sig[state->hdr.cpnum] = true;
			state->rx_raised = true;
		}
	} else {
		TRACE(&state->hdr, 1, "uart: no rx data\n");
		state->hdr.regs[CP_UART_REG_STATUS] &= ~CP_UART_REG_STATUS_RX_AVAIL;
	}

	/* check for space in Tx queue, set status, raise interrupt if requested */
	n = mbox_len(&state->out_box);
	if(n < state->fifo_len) {
		TRACE(&state->hdr, 1, "uart: tx space available\n");
		state->hdr.regs[CP_UART_REG_STATUS] |= CP_UART_REG_STATUS_TX_SPACE;
		if(!state->tx_raised &&
		   state->hdr.regs[CP_UART_REG_CTL] & CP_UART_REG_CTL_TX_SPACE_INT){
			TRACE(&state->hdr, 1, "uart: raise tx space interrupt\n");
			state->hdr.core_input->exint_sig[state->hdr.cpnum] = true;
			state->tx_raised = true;
		}
	} else {
		TRACE(&state->hdr, 1, "uart: no tx space\n");
		state->hdr.regs[CP_UART_REG_STATUS] &= ~CP_UART_REG_STATUS_TX_SPACE;
	}

	return 0;
}

int uart_state_exec(sim_cp_state_hdr_t *hdr)
{
	size_t i, n;
	charmsg_t *m;
	sim_cp_uart_state_t *state = (sim_cp_uart_state_t *) hdr;

	switch(hdr->core_output->coproc.value) {
	case CP_UART_INSTR_NOOP:
		TRACE(&state->hdr, 1, "uart: exec noop\n");
		break;

	case CP_UART_INSTR_SEND:
		TRACE(&state->hdr, 1, "uart: exec send\n");
		if(mbox_len(&state->out_box) < state->fifo_len) {
			TRACE(&state->hdr, 1, "uart: send space avail\n");
			assert(m = charmsg_new(hdr->regs[CP_UART_REG_VAL]));
			mbox_msg_post(&(state->out_box), &(m->hdr));
		}
		break;

	case CP_UART_INSTR_RECV:
		TRACE(&state->hdr, 1, "uart: exec recv\n");
		if(mbox_len(&state->in_box)) {
			m = (charmsg_t *) mbox_msg_get(&state->in_box);
			hdr->regs[CP_UART_REG_VAL] = m->c;
			TRACE(&state->hdr, 1, "uart: recv data avail (%x)\n",
			      hdr->regs[CP_UART_REG_VAL]);
			free(m);
		}
		break;
		
	case CP_UART_INSTR_TXUSED:
		hdr->regs[CP_UART_REG_VAL] = mbox_len(&state->out_box);
		TRACE(&state->hdr, 1, "uart: exec txused (%u)\n",
		      hdr->regs[CP_UART_REG_VAL]);
		break;

	case CP_UART_INSTR_RXUSED:
		hdr->regs[CP_UART_REG_VAL] = mbox_len(&state->in_box);
		TRACE(&state->hdr, 1, "uart: exec rxused (%u)\n",
		      hdr->regs[CP_UART_REG_VAL]);
		break;

	case CP_UART_INSTR_RESET_TX:
		TRACE(&state->hdr, 1, "uart: exec reset tx\n");
		state->tx_raised = false;
		break;

	case CP_UART_INSTR_RESET_RX:
		TRACE(&state->hdr, 1, "uart: exec reset rx\n");
		state->rx_raised = false;
		break;

	case CP_UART_INSTR_CLEAR:
		TRACE(&state->hdr, 1, "uart: exec clear\n");
		for(i = 0, n = mbox_len(&state->in_box); i < n; i++) {
			m = (charmsg_t *) mbox_msg_get(&state->out_box);
			free(m);
		}
		break;

	case CP_UART_FIFOS_ON:
		TRACE(&state->hdr, 1, "uart: exec fifos on\n");
		state->fifo_en = true;
		break;

	case CP_UART_FIFOS_OFF:
		TRACE(&state->hdr, 1, "uart: exec fifos off\n");
		state->fifo_en = false;
		break;

	case CP_UART_INT_ACK:
		TRACE(&state->hdr, 1, "uart: exec int ack\n");
		state->hdr.core_input->exint_sig[state->hdr.cpnum] = false;
		break;
	}
	return 0;
}

int uart_state_print(sim_cp_state_hdr_t *hdr)
{
	sim_cp_uart_state_t *state = (sim_cp_uart_state_t *) hdr;
	printf("fifo_en=%d incnt=%u rx_raised=%d outcnt=%u tx_raised=%d\n",
	       state->fifo_en,
	       (unsigned int) mbox_len(&state->in_box), state->rx_raised,
	       (unsigned int) mbox_len(&state->out_box), state->tx_raised);
	return 0;
}
