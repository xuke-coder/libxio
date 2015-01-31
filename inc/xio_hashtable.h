#ifndef _XIO_HASHTABLE_H
#define _XIO_HASHTABLE_H

#include "xio_queue.h"

typedef struct xio_hashtable_elem_s     xio_hashtable_elem_t;
typedef struct xio_hashtable_s          xio_hashtable_t;

struct xio_hashtable_elem_s {
    uint32_t         fd;
    xio_queue_t      que;
    xio_queue_t     *next;
};

struct xio_hashtable_s {
    unsigned int                max_size;
    unsigned int                size;
    xio_hashtable_elem_t      **buckets;
    xio_pool_t                 *pool;
    xio_thread_lock_t           thread_lock;
    xio_atomic_lock_t           atomic_lock;
};


int
xio_hashtable_init(xio_hashtable_t *htable, uint32_t size, xio_err_t *err);
void
xio_hashtable_release(xio_hashtable_t *htable, xio_err_t *err);
int
xio_hashtable_push(xio_hashtable_t *htable, uint32_t key, uint32_t fd, 
    xio_data_task_node_t *task_node, xio_err_t *err);
xio_hashtable_elem_t *
xio_hashtable_pop(xio_hashtable_t *htable, uint32_t key, xio_err_t *err);

#endif
