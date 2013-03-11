#ifndef MBOX_H
#define MBOX_H

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include "dlist.h"

typedef struct
{
	dlnode_t hdr;
} msg_t;

typedef struct
{
	dlist_t messages;
	size_t len;
	pthread_mutex_t mux;

	pthread_cond_t cond;
	size_t waiting;
} mbox_t;

msg_t *msg_init(void *p);

mbox_t *mbox_init(void *p);

void mbox_msg_post(mbox_t *box, msg_t *msg);
msg_t *mbox_msg_get(mbox_t *box);
/* if s == 0 and ns == 0, wait forever */
msg_t *mbox_msg_get_wait(mbox_t *box, unsigned long s, unsigned long ns);

size_t mbox_len(mbox_t *box);


#endif
