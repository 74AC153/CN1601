#ifndef SIM_CP_UART_H
#define SIM_CP_UART_H

#include <poll.h>
#include <pthread.h>
#include <stdbool.h>

#include "sim_cp_if.h"
#include "mbox.h"

#define FIFO_LEN 16

#define CP_UART_REG_CTL 0
#define CP_UART_REG_CTL_TX_SPACE_INT 1
#define CP_UART_REG_CTL_RX_AVAIL_INT 2

#define CP_UART_REG_STATUS 1
#define CP_UART_REG_STATUS_TX_SPACE 1
#define CP_UART_REG_STATUS_RX_AVAIL 2

#define CP_UART_REG_VAL 2

#define CP_UART_INSTR_NOOP 0
#define CP_UART_INSTR_SEND 1
#define CP_UART_INSTR_RECV 2
#define CP_UART_INSTR_TXUSED 3
#define CP_UART_INSTR_RXUSED 4
#define CP_UART_INSTR_RESET_TX 5
#define CP_UART_INSTR_RESET_RX 6
#define CP_UART_INSTR_CLEAR 7
#define CP_UART_INSTR_FIFOS_ON 8
#define CP_UART_INSTR_FIFOS_OFF 9
#define CP_UART_INSTR_INT_ACK 10

typedef struct {
	sim_cp_state_hdr_t hdr;

	int txfd, rxfd;
	char *rxpath, *txpath;

	bool fifo_en;
	size_t fifo_len;
	mbox_t in_box;
	mbox_t out_box;
	bool tx_raised, rx_raised;

	pthread_t rx_thr, tx_thr;
	bool run_threads;

	pthread_mutex_t status_mux;
} sim_cp_uart_state_t;


int uart_state_init(sim_cp_state_hdr_t *hdr);
int uart_state_deinit(sim_cp_state_hdr_t *hdr);
int uart_state_reset(sim_cp_state_hdr_t *hdr);
int uart_state_data(sim_cp_state_hdr_t *hdr);
int uart_state_exec(sim_cp_state_hdr_t *hdr);
int uart_state_print(sim_cp_state_hdr_t *hdr);

#endif
