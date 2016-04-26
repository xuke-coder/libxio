#include <stdio.h>
#include <stdlib.h>
#include "xio.h"
#include "xio_data.h"
#include "xio_worker.h"
#include "xio_memory.h"




xio_manager_t *
xio_init(xio_property_t *property, xio_err_t *err)
{
    xio_manager_t           *xio_mgr = NULL;
    xio_data_manager_t      *data_mgr = NULL;
    xio_worker_manager_t    *worker_mgr = NULL;
    xio_pool_t              *pool = NULL;
    uint32_t                 pool_size = 0;
    uint32_t                 max_task_num = 0;
    uint32_t                 max_thread_num = 0;
    uint32_t                 min_thread_num = 0;
    uint32_t                 run_thread_num = 0;
    uint32_t                 idle_time = 0;
    
    
    if (!err) {
        return NULL;
    }

    #ifdef __x86_64__
    	#define PROCESS_64BIT
    #endif

    if (property) {
        max_task_num = property->max_task_num;
        max_thread_num = property->max_thread_num;
        min_thread_num = property->min_thread_num;
        run_thread_num = property->run_thread_num;
        idle_time = property->idle_time;
        
    } else {
        max_task_num = 1024;
        max_thread_num = 8;
        min_thread_num = 2;
        run_thread_num = 4;
        idle_time = 5;
    }

    //pool create
    pool_size = sizeof(xio_manager_t) + sizeof(xio_data_manager_t) + 
        sizeof(xio_worker_manager_t);
    
    if (!(pool = xio_memory_pool_create(pool_size, SYSPAGE_SIZE, err))) {
        return NULL;
    }

    if (!(xio_mgr = (xio_manager_t *)xio_memory_pool_alloc(pool, 
        sizeof(xio_manager_t), err))) {
        goto memory_release;
    }

    
    xio_mgr->pool = pool;

    //data init
    if (!(data_mgr = (xio_data_manager_t *)xio_memory_pool_alloc(pool, 
        sizeof(xio_data_manager_t), err))) {
        goto memory_release;
    }
    
    xio_mgr->data_mgr = data_mgr;

    data_mgr->xio_mgr = xio_mgr;
    
    if (xio_data_manager_init(data_mgr, max_task_num, err) 
        != XIO_OK) {
        goto memory_release;
    }

    //worker init
    if (!(worker_mgr = (xio_worker_manager_t *)xio_memory_pool_alloc(pool,
        sizeof(xio_worker_manager_t), err))) {
        goto data_release;
    }

    xio_mgr->worker_mgr = worker_mgr;

    worker_mgr->xio_mgr = xio_mgr;
    
    if (xio_worker_manager_init(worker_mgr, max_thread_num, min_thread_num, 
        run_thread_num, idle_time, err) != XIO_OK) {
        goto data_release;
    }

    return xio_mgr;
            
data_release:
    xio_data_manager_release(data_mgr, err);
    
memory_release:
    xio_memory_pool_release(pool, err);

    return NULL;
}

void
xio_release(xio_manager_t *xio_mgr, xio_err_t *err)
{
    if (!err) {
        return;
    }

    if (!xio_mgr) {
        err->log_errno = XIO_ERR_PARAM;
        return;
    }
    
    xio_worker_manager_release(xio_mgr->worker_mgr, err);

    xio_data_manager_release(xio_mgr->data_mgr, err);
    
    xio_memory_pool_release(xio_mgr->pool, err);
}


int
xio_task_submit(xio_task_t *xio_task)
{    
    if (!xio_task) {
        return XIO_ERROR;
    }

    if (!xio_task->xio_mgr || (xio_task->io_type <= XIO_IO_START || 
        xio_task->io_type >= XIO_IO_END)) {
        xio_task->err.log_errno = XIO_ERR_PARAM;
        return XIO_ERROR;
    }

    if (xio_data_task_push(xio_task->xio_mgr->data_mgr, xio_task->user_data, 
        xio_task->io_type, xio_task->call_back, &xio_task->err) != XIO_OK) {
        return XIO_ERROR;
    }

    return XIO_OK;
}

int
xio_reg_exec_func(xio_manager_t *xio_mgr, XIO_IO_TYPE io_type, 
    xio_io_exec_handler io_func, xio_err_t *err)
{
    xio_data_manager_t      *data_mgr = NULL;

    if (!err) {
        return XIO_ERROR;
    }
    
    if (!xio_mgr || !io_func || (io_type <= XIO_IO_START || 
        io_type >= XIO_IO_END)) {
        err->log_errno = XIO_ERR_PARAM;
        return XIO_ERROR;
    }

    data_mgr = xio_mgr->data_mgr;
    
    return xio_data_register_exec_handler(data_mgr, io_func, io_type, err); 
}

