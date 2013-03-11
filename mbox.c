#include <assert.h>
#include <sys/time.h>
#include "mbox.h"

msg_t *msg_init(void *p)
{
	msg_t *ret = (msg_t *) p;
	if(ret) {
		dlnode_init(&(ret->hdr));
	}
	return ret;
}

mbox_t *mbox_init(void *p)
{
	mbox_t *ret = (mbox_t *) p;
	if(ret) {
		pthread_mutex_init(&(ret->mux), NULL);
		dlist_init(&(ret->messages));
		ret->len = 0;

		pthread_cond_init(&(ret->cond), NULL);
		ret->waiting = 0;
	}

	return ret;
}

void mbox_msg_post(mbox_t *box, msg_t *msg)
{
	pthread_mutex_lock(&(box->mux));

	dlist_insertlast(&(box->messages), &(msg->hdr));
	box->len++;

	if(box->waiting) {
		pthread_cond_signal(&(box->cond));
	}

	pthread_mutex_unlock(&(box->mux));
}

msg_t *mbox_msg_get(mbox_t *box)
{
	msg_t *ret;
	pthread_mutex_lock(&(box->mux));

	if((ret = (msg_t *) dlist_first(&(box->messages)))) {
		dlnode_remove(&(ret->hdr));
		box->len--;
	}

	pthread_mutex_unlock(&(box->mux));
	return ret;
}

msg_t *mbox_msg_get_wait(mbox_t *box, unsigned long s, unsigned long ns)
{
	msg_t *ret = NULL;
	int status = 0;
	pthread_mutex_lock(&(box->mux));

	if(s == 0 && ns == 0) {
		while(box->len == 0) {
			box->waiting++;
			status = pthread_cond_wait(&(box->cond), &(box->mux));
			box->waiting--;
		}
	} else if(box->len == 0) {
        struct timeval tv;
        struct timespec ts;
        gettimeofday(&tv, NULL);
        ts.tv_sec = tv.tv_sec + s;
        ts.tv_nsec = ns;
		
		box->waiting++;
		status = pthread_cond_timedwait(&(box->cond), &(box->mux), &ts);
		box->waiting--;
	}

	if(!status &&
	   (ret = (msg_t *) dlist_first(&(box->messages)))) {
		dlnode_remove(&(ret->hdr));
		box->len--;
	}

	pthread_mutex_unlock(&(box->mux));
	return ret;
}

size_t mbox_len(mbox_t *box)
{
	return box->len;
}

