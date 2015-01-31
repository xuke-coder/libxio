#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "xio.h"
#include "xio_thread.h"
#include "xio_signal.h"

void
xio_thread_lock(xio_thread_lock_t *lock)
{
    int error = 0;
    
    if ((error = pthread_mutex_lock(lock)) != 0) {
        printf("pthread mutex lock error %d\n", error);
    }
}

void
xio_thread_unlock(xio_thread_lock_t *lock)
{
    int error = 0;

    if ((error = pthread_mutex_unlock(lock)) != 0) {
        printf("pthread mutex nulock error %d\n", error);
    }
}

int
xio_thread_lock_init(xio_thread_lock_t *lock, xio_err_t *err)
{
    int error = 0;

    if ((error = pthread_mutex_init(lock, NULL)) != 0) {
        err->sys_errno = error;
        return XIO_ERROR;
    }

    return XIO_OK;
}

void
xio_thread_lock_release(xio_thread_lock_t *lock, xio_err_t *err)
{
    int error = 0;

    if ((error = pthread_mutex_destroy(lock)) != 0) {
        err->sys_errno = error;
    }
}

int
xio_thread_create(xio_worker_thread_t *worker_thread, 
    xio_thread_func_t thread_func, void *data, xio_err_t *err)
{
    int         error = 0;
    pthread_t   tid = 0;

    worker_thread->tid = tid;
    worker_thread->release_start = XIO_FALSE;
    worker_thread->release_done = XIO_FALSE;

    if (xio_signal_init(&worker_thread->sub_signal, XIO_TRUE, err) != XIO_OK) {
        return XIO_ERROR;
    }
    
    if ((error = pthread_create(&tid, NULL, thread_func, data)) != 0) {
        err->sys_errno = error;
        goto signal_release;
    }

    return XIO_OK;
    
signal_release:
    xio_signal_release(worker_thread->sub_signal, err);
    
    return XIO_ERROR;
    
}


void
xio_thread_destroy(xio_worker_thread_t *worker_thread, xio_err_t *err)
{
    worker_thread->release_start = XIO_TRUE;

    xio_signal_send(worker_thread->sub_signal, err);
    
    while (1) {
        if (worker_thread->release_done == XIO_TRUE) {
            break;
        }
        usleep(1000);
    }

    xio_signal_release(worker_thread->sub_signal, err);
}


