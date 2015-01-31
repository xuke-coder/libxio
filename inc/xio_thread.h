#ifndef _XIO_THREAD_H
#define _XIO_THREAD_H

#include <pthread.h>
#include "xio_worker.h"

#define xio_thread_lock_t  pthread_mutex_t

typedef void * (*xio_thread_func_t) (void *data);

int
xio_thread_lock_init(xio_thread_lock_t *lock, xio_err_t *err);
void
xio_thread_lock_release(xio_thread_lock_t *lock, xio_err_t *err);
void
xio_thread_lock(xio_thread_lock_t *lock);
void
xio_thread_unlock(xio_thread_lock_t *lock);
int
xio_thread_create(xio_worker_thread_t *worker_thread, 
    xio_thread_func_t thread_func, void *data, xio_err_t *err);
void
xio_thread_destroy(xio_worker_thread_t *worker_thread, xio_err_t *err);



#endif
