#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include "xio.h"
#include "xio_data.h"
#include "xio_worker.h"
#include "xio_thread.h"
#include "xio_signal.h"
#include "xio_queue.h"
#include "xio_memory.h"
#include "xio_spinlock.h"


int
xio_worker_manager_init(xio_worker_manager_t *worker_mgr, 
    uint32_t max_thread_num, uint32_t min_thread_num, uint32_t run_thread_num, 
    uint32_t idle_time, xio_err_t *err)
{
    xio_pool_t          *pool = NULL;
    xio_worker_thread_t *worker_thread = NULL;
    xio_queue_t         *thread_pool = NULL;
    xio_queue_t         *thread_que = NULL;
    xio_queue_t         *wait_queue = NULL;
    uint32_t             pool_size = 0;
    uint32_t             i = 0;
    uint32_t             max_size = 0;

    thread_que = &worker_mgr->thread_que;
    thread_pool = &worker_mgr->thread_pool;
    wait_queue = &worker_mgr->wait_queue;
    pool_size = sizeof(xio_worker_thread_t) * max_thread_num;

    worker_mgr->max_thread_num = max_thread_num;
    worker_mgr->min_thread_num = min_thread_num;
    worker_mgr->run_thread_num = run_thread_num;
    worker_mgr->idle_time = idle_time;
    
    xio_queue_init(thread_que, max_size);
    xio_queue_init(thread_pool, max_size);
    xio_queue_init(wait_queue, max_size);
    
    if (xio_thread_lock_init(&thread_que->thread_lock, err) != XIO_OK) {
        goto que_release;   
    }

    if (xio_thread_lock_init(&thread_pool->thread_lock, err) != XIO_OK) {
        goto que_lock_release;   
    }    

    if (!(pool = xio_memory_pool_create(pool_size, SYSPAGE_SIZE, err))) {
        goto pool_lock_release;
    }

    worker_mgr->pool = pool;

    for (i = 0; i < worker_mgr->max_thread_num; i++) {
        if (!(worker_thread = (xio_worker_thread_t *)xio_memory_pool_alloc(pool, 
            sizeof(xio_worker_thread_t), err))) {
            goto memory_pool_release;
        }

        worker_thread->link.next = NULL;
        worker_thread->link.prev = NULL;

        xio_worker_queue_push(thread_pool, worker_thread);
    }

    
    if (xio_worker_thread_init(worker_mgr, err) != XIO_OK) {
        goto memory_pool_release;
    }
    

    return XIO_OK;
    
memory_pool_release:
    xio_memory_pool_release(pool, err);
    
pool_lock_release:
    xio_thread_lock_release(&thread_pool->thread_lock, err);
    
que_lock_release:
    xio_thread_lock_release(&thread_que->thread_lock, err);
    
que_release:
    xio_queue_release(thread_pool);
    xio_queue_release(thread_que);
    return XIO_ERROR;
    
}

void
xio_worker_manager_release(xio_worker_manager_t *worker_mgr, 
    xio_err_t *err)
{
    xio_queue_t     *thread_pool = NULL;
    xio_queue_t     *thread_que = NULL;

    thread_pool = &worker_mgr->thread_pool;
    thread_que = &worker_mgr->thread_que;
    
    xio_worker_thread_release(worker_mgr, err);
    xio_memory_pool_release(worker_mgr->pool, err);
    xio_thread_lock_release(&worker_mgr->thread_pool.thread_lock, err);
    xio_thread_lock_release(&worker_mgr->thread_que.thread_lock, err);
    xio_queue_release(thread_pool);
    xio_queue_release(thread_que);
}

int
xio_worker_thread_init(xio_worker_manager_t *worker_mgr, xio_err_t *err)
{
    uint32_t                 i = 0;
    xio_worker_thread_t     *worker_thread = NULL;
    xio_queue_t             *thread_que = NULL;
    xio_queue_t             *thread_pool = NULL;
    xio_pool_t              *pool = NULL;

    pool = worker_mgr->pool;
    thread_que = &worker_mgr->thread_que;
    thread_pool = &worker_mgr->thread_pool;
    
    for (i = 0; i < worker_mgr->run_thread_num; i++) {
        
        worker_thread = xio_worker_queue_pop(thread_pool);
        
        worker_thread->worker_mgr = worker_mgr;

        xio_worker_queue_push(thread_que, worker_thread);

        if (xio_thread_create(worker_thread, xio_worker_thread_cycle, 
            worker_thread, err) != XIO_OK) {
            goto thread_release;
        }
    }

    return XIO_OK;
    
thread_release:

    worker_thread = (xio_worker_thread_t *)thread_que->head.next;
    
    while (worker_thread) {
        xio_thread_destroy(worker_thread, err);
        worker_thread = (xio_worker_thread_t *)worker_thread->link.next;
    }
    return XIO_ERROR;
}

void
xio_worker_thread_release(xio_worker_manager_t *worker_mgr, xio_err_t *err)
{
    xio_worker_thread_t     *worker_thread = NULL;
    xio_queue_t             *que = NULL;

    que = &worker_mgr->thread_pool;
    worker_thread = (xio_worker_thread_t *)que->head.next;
    
    while (worker_thread) {
        xio_thread_destroy(worker_thread, err);
        worker_thread = (xio_worker_thread_t *)worker_thread->link.next;
    }
}



void *
xio_worker_thread_cycle(void *data)
{
    xio_worker_manager_t        *worker_mgr = NULL;
    xio_data_manager_t          *data_mgr = NULL;
    xio_manager_t               *xio_mgr = NULL;
    xio_worker_thread_t         *worker_thread = NULL;
    xio_queue_t                 *task_queue = NULL;
    xio_queue_t                 *thread_queue = NULL;
    xio_queue_t                 *wait_queue = NULL;
    xio_data_task_node_t        *task_node = NULL;
    int                          i = 0;
    time_t                       idle_start_time = 0;
    time_t                       idle_end_time = 0;
    time_t                       idle_time = 0;
    xio_link_t                  *head_link = NULL;

    worker_thread = (xio_worker_thread_t *)data;
    worker_mgr = worker_thread->worker_mgr;
    xio_mgr = worker_mgr->xio_mgr;
    data_mgr = xio_mgr->data_mgr;
    task_queue = &data_mgr->task_que;
    thread_queue = &worker_mgr->thread_que;
    wait_queue = &worker_mgr->wait_queue;
    idle_time = worker_mgr->idle_time;
    head_link = &thread_queue->head;


    while (task_queue->size != 0 || worker_thread->release_start == XIO_FALSE) {
        if (!(task_node = xio_data_queue_pop_n(task_queue, 10, NULL))) {
            
            if (i < 3) {
                i++;
                continue;
            }

            xio_spin_lock(&thread_queue->atomic_lock);
            if (thread_queue->size <= worker_mgr->min_thread_num) {
                xio_spin_unlock(&thread_queue->atomic_lock);
                xio_signal_recv(worker_thread->sub_signal, NULL);
                
            } else {
                xio_queue_double_remove(thread_queue, 
                    ((xio_link_t *)worker_thread));
                
                xio_spin_unlock(&thread_queue->atomic_lock);

                xio_worker_queue_push(wait_queue, worker_thread);

                idle_start_time = time(NULL);
                
                xio_signal_recv(worker_thread->sub_signal, NULL);

                xio_worker_queue_delete(wait_queue, worker_thread);
                
                idle_end_time = time(NULL);

                if (idle_end_time - idle_start_time >= idle_time) {
                    xio_signal_send(
                        ((xio_worker_thread_t *)head_link->next)->sub_signal, 
                        NULL);
                    break;
                }

                xio_worker_queue_push(thread_queue, worker_thread);
            }
           
        }

        while (task_node) {
            if (data_mgr->exec_handler[task_node->io_type]) {
                data_mgr->exec_handler[task_node->io_type](task_node->user_data);
            }

            if (task_node->finish_handler) {
                task_node->finish_handler(task_node->user_data);
            }
            
            task_node = (xio_data_task_node_t *)task_node->que_link.next;
        }

        i = 0;
        
    }

    worker_thread->release_done = XIO_TRUE;

    return NULL;
}


void
xio_worker_queue_push(xio_queue_t *que, xio_worker_thread_t *wrk_thread)
{
    xio_link_t      *tail = NULL;
    
    xio_spin_lock(&que->atomic_lock);
    xio_queue_double_push(que, tail, ((xio_link_t *)wrk_thread));
    xio_spin_unlock(&que->atomic_lock);

}

xio_worker_thread_t *
xio_worker_queue_pop(xio_queue_t *que)
{
    xio_link_t      *head = NULL;
    xio_link_t      *link = NULL;
    
    xio_spin_lock(&que->atomic_lock);
    xio_queue_double_pop(que, head, link);
    xio_spin_unlock(&que->atomic_lock);

    return (xio_worker_thread_t *)link;
}

void
xio_worker_queue_delete(xio_queue_t *que, xio_worker_thread_t *wrk_thread)
{
    xio_spin_lock(&que->atomic_lock);
    xio_queue_double_remove(que, ((xio_link_t *)wrk_thread));
    xio_spin_unlock(&que->atomic_lock);

}


