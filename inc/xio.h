#ifndef _XIO_H
#define _XIO_H

typedef enum {
    XIO_IO_START = -1,
    XIO_IO_READ = 0,
    XIO_IO_WRITE,
    XIO_IO_PREAD,
    XIO_IO_PWRITE,
    XIO_IO_OPEN,
    XIO_IO_CLOSE,
    XIO_IO_DUMMY,
    XIO_IO_END
}XIO_IO_TYPE;


enum {
    XIO_ERR_NONE        = 0,
    XIO_ERR_PARAM       = 0x100,
    XIO_ERR_LOCK_TYPE,
    XIO_ERR_MEMORY_ALLOC,
    XIO_ERR_IO_TYPE,
    XIO_ERR_IO_TYPE_REDEF,
    XIO_ERR_TASK_POOL_EMPTY,
    XIO_ERR_END
    
};

#define XIO_OK          0
#define XIO_ERROR      -1
#define XIO_TRUE        1
#define XIO_FALSE       0

typedef unsigned int                    uint32_t;
typedef unsigned long                   uint64_t;
typedef struct xio_manager_s            xio_manager_t;
typedef struct xio_data_manager_s       xio_data_manager_t;
typedef struct xio_worker_manager_s     xio_worker_manager_t;
typedef struct xio_property_s           xio_property_t;
typedef struct xio_task_s               xio_task_t;
typedef struct xio_err_s                xio_err_t;
typedef struct xio_pool_s               xio_pool_t;


typedef int (*xio_io_exec_handler) (void *data);
typedef int (*xio_io_finish_handler) (void *data);

struct xio_err_s {
    uint32_t    log_errno;
    uint32_t    sys_errno;
};

struct xio_property_s {
    uint32_t        max_task_num;
    uint32_t        max_thread_num;
    uint32_t        min_thread_num;
    uint32_t        run_thread_num;
    uint32_t        idle_time;
};

struct xio_manager_s {
    xio_data_manager_t      *data_mgr;
    xio_worker_manager_t    *worker_mgr;
    xio_pool_t              *pool;
    
};

/*
*user should fill this struct
*/
struct xio_task_s {
    xio_manager_t           *xio_mgr;       //not null
    void                    *user_data;     //may be null
    XIO_IO_TYPE              io_type;
    xio_io_finish_handler    call_back;     //may be null
    xio_err_t                err;
};

xio_manager_t *
xio_init(xio_property_t *property, xio_err_t *err);
void
xio_release(xio_manager_t *xio_mgr, xio_err_t *err);
int
xio_task_submit(xio_task_t *xio_task);
int
xio_reg_exec_func(xio_manager_t *xio_mgr, XIO_IO_TYPE io_type, 
    xio_io_exec_handler io_func, xio_err_t *err);

#endif
