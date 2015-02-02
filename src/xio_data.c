#include <stdio.h>
#include <stdlib.h>
#include "xio.h"
#include "xio_data.h"
#include "xio_worker.h"
#include "xio_memory.h"
#include "xio_queue.h"
#include "xio_signal.h"
#include "xio_spinlock.h"


int
xio_data_manager_init(xio_data_manager_t *data_mgr, uint32_t max_size, 
    xio_err_t *err)
{
    uint32_t        pool_size = 0;
    xio_pool_t     *pool = NULL;
    
    pool_size = sizeof(xio_data_task_node_t) * max_size;
    
    if (!(pool = xio_memory_pool_create(pool_size, SYSPAGE_SIZE, err))) {
        return XIO_ERROR;
    }

    data_mgr->pool = pool;

    xio_data_task_queue_init(data_mgr, max_size, err);
    
    if (xio_data_task_pool_init(data_mgr, max_size, err) != XIO_OK) {
        goto task_queue_release;
    }

    return XIO_OK;
    
task_queue_release:
    xio_data_task_queue_release(data_mgr, err);
    
    xio_memory_pool_release(pool, err);
    
    return XIO_ERROR;
}


void 
xio_data_manager_release(xio_data_manager_t *data_mgr, xio_err_t *err)
{
    xio_data_task_pool_release(data_mgr, err);
    xio_data_task_queue_release(data_mgr, err);
    xio_memory_pool_release(data_mgr->pool, err);
}


int
xio_data_register_exec_handler(xio_data_manager_t *data_mgr, 
    xio_io_exec_handler exec_handler, XIO_IO_TYPE io_type, xio_err_t *err)
{
    if (data_mgr->exec_handler[io_type]) {
        err->log_errno = XIO_ERR_IO_TYPE_REDEF;
        return XIO_ERROR;
    }

    data_mgr->exec_handler[io_type] = exec_handler;

    return XIO_OK;
    
}


int
xio_data_task_push(xio_data_manager_t *data_mgr, void *user_data, 
    XIO_IO_TYPE io_type, xio_io_finish_handler finish_handler, xio_err_t *err)
{
    xio_data_task_node_t    *task_node = NULL;
    xio_queue_t             *task_que = NULL;
    xio_queue_t             *task_pool = NULL;
    xio_worker_manager_t    *worker_mgr = NULL;
    xio_link_t              *head_link = NULL;
    

    task_que = &data_mgr->task_que;
    task_pool = &data_mgr->task_pool;
    worker_mgr = data_mgr->xio_mgr->worker_mgr;
    head_link = &worker_mgr->wait_queue.head;
    
    if (!(task_node = xio_data_queue_pop(task_pool, err))) {
        err->log_errno = XIO_ERR_TASK_POOL_EMPTY;
        return XIO_ERROR;
    }

    task_node->io_type = io_type;
    task_node->finish_handler = finish_handler;
    task_node->user_data = user_data;

    if (xio_data_queue_push(task_que, task_node, err) != XIO_OK) {
        err->log_errno = XIO_ERR_TASK_TOO_MANY;
        return XIO_ERROR;
    }

    xio_spin_lock(&worker_mgr->wait_queue.atomic_lock);
    
    if (worker_mgr->wait_queue.size > 0) {
        xio_signal_send(((xio_worker_thread_t *)head_link->next)->sub_signal, 
            err);
    }

    xio_spin_unlock(&worker_mgr->wait_queue.atomic_lock);
    
    return XIO_OK;
}


int
xio_data_task_pool_init(xio_data_manager_t *data_mgr, uint32_t max_size, 
    xio_err_t *err)
{
    xio_pool_t              *pool = NULL;
    xio_data_task_node_t    *task_node = NULL;
    xio_link_t              *tail = NULL;
    xio_link_t              *link = NULL;
    xio_queue_t             *que = NULL;
    uint32_t                 node_size = 0;
    uint32_t                 i = 0;
    

    node_size = sizeof(xio_data_task_node_t);
    que = &data_mgr->task_pool;
    pool = data_mgr->pool;

    xio_queue_init(que, max_size);
    xio_spin_lock_init(&que->atomic_lock);

    for (i = 0; i < max_size; i++) {
        if (!(task_node = xio_memory_pool_alloc(pool, node_size, err))) {
            goto all_release;
        }
        
        xio_task_node_init(task_node);
        link = &task_node->que_link;
        xio_queue_single_push(que, tail, link);
    }

    return XIO_OK;

all_release:
    xio_spin_lock_release(&que->atomic_lock);
    xio_queue_release(que);
    return XIO_ERROR;
}

void
xio_data_task_pool_release(xio_data_manager_t *data_mgr, xio_err_t *err)
{
    xio_queue_t     *que = NULL;
    
    que = &data_mgr->task_pool;
    
    xio_spin_lock_release(&que->atomic_lock);
    xio_queue_release(que);
}


void
xio_data_task_queue_init(xio_data_manager_t *data_mgr, uint32_t max_size, 
    xio_err_t *err)
{
    xio_queue_t     *que = NULL;

    que = &data_mgr->task_que;
    
    xio_queue_init(que, max_size);
    xio_spin_lock_init(&que->atomic_lock);

}

void
xio_data_task_queue_release(xio_data_manager_t *data_mgr, xio_err_t *err)
{
    xio_queue_t     *que = NULL;

    que = &data_mgr->task_que;

    xio_spin_lock_release(&que->atomic_lock);
    xio_queue_release(que);
}

int
xio_data_queue_push(xio_queue_t *que, xio_data_task_node_t *node, 
    xio_err_t *err)
{
    xio_link_t          *tail = NULL;
    xio_link_t          *link = NULL;
    xio_atomic_lock_t   *lock = NULL;
    int                  ret = XIO_ERROR;

    lock = &que->atomic_lock;
    link = &node->que_link;

    xio_spin_lock(lock);
    
    if (que->release_flag == XIO_FALSE) {
        xio_queue_single_push(que, tail, link);
        ret = XIO_OK;
    }
    
    xio_spin_unlock(lock);

    return ret;
    
}

xio_data_task_node_t *
xio_data_queue_pop(xio_queue_t *que, xio_err_t *err)
{
    xio_link_t          *head = NULL;
    xio_link_t          *link = NULL;
    xio_atomic_lock_t   *lock = NULL;

    lock = &que->atomic_lock;

    xio_spin_lock(lock);
    xio_queue_single_pop(que, head, link);
    xio_spin_unlock(lock);

    return (xio_data_task_node_t *)link;
}


xio_data_task_node_t *
xio_data_queue_pop_n(xio_queue_t *que, uint32_t num, xio_err_t *err)
{
    xio_link_t          *head = NULL;
    xio_link_t          *link = NULL;
    xio_link_t          *ret_link = NULL;
    xio_atomic_lock_t   *lock = NULL;
    uint32_t             i = 0;

    lock = &que->atomic_lock;

    head = &que->head;

    xio_spin_lock(lock);
    
    ret_link = head->next;

    if (ret_link == head) {
        xio_spin_unlock(lock);
        return NULL;
    }
    
    link = head;
    
    for (i = 0; i < num; i++) {
        if (link->next == head) {
            head->prev = head;
            break;
        }
        
        link = link->next;
    }

    head->next = link->next;

    que->size -= i;
    
    xio_spin_unlock(lock);

    link->next = NULL;

    return (xio_data_task_node_t *)ret_link;
}

