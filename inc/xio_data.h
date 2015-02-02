#ifndef _XIO_DATA_H
#define _XIO_DATA_H

#include "xio_queue.h"



#define xio_task_node_init(task)            \
            task->user_data = NULL;         \
            task->finish_handler = NULL;    \
            task->que_link.next = NULL;


//typedef struct xio_queue_s             xio_queue_t;
//typedef struct xio_data_manager_s      xio_data_manager_t;
typedef struct xio_data_task_node_s    xio_data_task_node_t;

//typedef int (*xio_io_finish_handler) (void *data);
//typedef int (*xio_io_exec_handler) (void *data);


struct xio_data_task_node_s {
    xio_link_t               que_link;
    xio_io_finish_handler    finish_handler;
    void                    *user_data;
    XIO_IO_TYPE              io_type;
};

struct xio_data_manager_s {
    xio_manager_t           *xio_mgr;
    xio_pool_t              *pool;
    xio_queue_t              task_que;
    xio_queue_t              task_pool;
    uint32_t                 que_max_size;
    xio_io_exec_handler      exec_handler[XIO_IO_END];
    
};

int
xio_data_manager_init(xio_data_manager_t *data_mgr, uint32_t max_size, 
    xio_err_t *err);
void 
xio_data_manager_release(xio_data_manager_t *data_mgr, xio_err_t *err);
int
xio_data_task_pool_init(xio_data_manager_t *data_mgr, uint32_t max_size, 
    xio_err_t *err);
void
xio_data_task_pool_release(xio_data_manager_t *data_mgr, xio_err_t *err);
void
xio_data_task_queue_init(xio_data_manager_t *data_mgr, uint32_t max_size, 
    xio_err_t *err);
void
xio_data_task_queue_release(xio_data_manager_t *data_mgr, xio_err_t *err);
int
xio_data_task_push(xio_data_manager_t *data_mgr, void *user_data, 
    XIO_IO_TYPE io_type, xio_io_finish_handler finish_handler, xio_err_t *err);

int
xio_data_register_exec_handler(xio_data_manager_t *data_mgr, 
    xio_io_exec_handler exec_handler, XIO_IO_TYPE io_type, xio_err_t *err);
int
xio_data_queue_push(xio_queue_t *que, xio_data_task_node_t *node, 
    xio_err_t *err);
xio_data_task_node_t *
xio_data_queue_pop(xio_queue_t *que, xio_err_t *err);
xio_data_task_node_t *
xio_data_queue_pop_n(xio_queue_t *que, uint32_t num, xio_err_t *err);







#endif
