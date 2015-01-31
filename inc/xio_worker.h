#ifndef _XIO_WORKER_H
#define _XIO_WORKER_H

#include <pthread.h>
#include "xio_queue.h"


//typedef struct xio_worker_manager_s         xio_worker_manager_t;
typedef struct xio_worker_thread_s          xio_worker_thread_t;

struct xio_worker_thread_s {
    xio_link_t               link;
    xio_worker_manager_t    *worker_mgr;
    pthread_t                tid;
    int                      release_done;
    int                      release_start;
    int                      sub_signal;
    time_t                   idle_start;
    time_t                   idle_end;
};


struct xio_worker_manager_s {
    xio_manager_t          *xio_mgr;
    xio_pool_t             *pool;
    xio_queue_t             thread_pool;
    xio_queue_t             thread_que;
    uint32_t                max_thread_num;
    uint32_t                min_thread_num;
    uint32_t                run_thread_num;
    uint32_t                idle_time;
    xio_queue_t             wait_queue;
};

int
xio_worker_manager_init(xio_worker_manager_t *worker_mgr, 
    uint32_t max_thread_num, uint32_t min_thread_num, uint32_t run_thread_num, 
    uint32_t idle_time, xio_err_t *err);
void
xio_worker_manager_release(xio_worker_manager_t *worker_mgr, 
    xio_err_t *err);
int
xio_worker_thread_init(xio_worker_manager_t *worker_mgr, xio_err_t *err);
void
xio_worker_thread_release(xio_worker_manager_t *worker_mgr, 
    xio_err_t *err);
void *
xio_worker_thread_cycle(void *data);
void
xio_worker_queue_push(xio_queue_t *que, xio_worker_thread_t *wrk_thread);
xio_worker_thread_t *
xio_worker_queue_pop(xio_queue_t *que);
void
xio_worker_queue_delete(xio_queue_t *que, xio_worker_thread_t *wrk_thread);







#endif

