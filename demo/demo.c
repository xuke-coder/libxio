#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h> 
#include "xio.h"



typedef struct test_data_open_s {
    xio_task_t  *xio_task;
    char        *pathname;
    int          flags;
    mode_t       mode;
    int          fd;
}test_data_open_t;

typedef struct test_data_write_s {
    xio_task_t  *xio_task;
    int          fd;
    void        *buf;
    size_t       count;
    int          ret;
    
}test_data_write_t;

typedef struct test_data_close_s {
    xio_task_t  *xio_task;
    int          fd;
    int          ret;
}test_data_close_t;


int
test_open(void *data)
{
    test_data_open_t *open_data = NULL;

    open_data = (test_data_open_t *)data;

    open_data->fd = open(open_data->pathname, open_data->flags, open_data->mode);

    return 0;
}

int
test_write(void *data)
{
    test_data_write_t   *write_data = NULL;

    write_data = (test_data_write_t *)data;

    write_data->ret = write(write_data->fd, write_data->buf, write_data->count);

    return 0;
}

int
test_close(void *data)
{
    test_data_close_t   *close_data = NULL;

    close_data = (test_data_close_t *)data;

    close_data->ret = close(close_data->fd);

    return 0;
}

int
open_callback(void *data)
{
    test_data_open_t *open_data = NULL;

    open_data = (test_data_open_t *)data;

    printf("open fd = %d\n", open_data->fd);

    return 0;
}

int
write_callback(void *data)
{
    test_data_write_t   *write_data = NULL;

    write_data = (test_data_write_t *)data;

    printf("write ret = %d\n", write_data->ret);

    return 0;
}

int
close_callback(void *data)
{
    test_data_close_t       *close_data = NULL;

    close_data = (test_data_close_t *)data;

    printf("close ret = %d\n", close_data->ret);

    return 0;
}

int main()
{
    xio_manager_t      *xio_mgr = NULL;
    xio_task_t          xio_task_open;
    xio_task_t          xio_task_write;
    xio_task_t          xio_task_close;
    xio_err_t           err;
    test_data_open_t    open_data;
    test_data_write_t   write_data;
    test_data_close_t   close_data;

    memset(&xio_task_open, 0, sizeof(xio_task_open));
    memset(&xio_task_write, 0, sizeof(xio_task_write));
    memset(&xio_task_close, 0, sizeof(xio_task_close));
    memset(&open_data, 0, sizeof(open_data));
    memset(&write_data, 0, sizeof(write_data));
    memset(&close_data, 0, sizeof(close_data));
    

    xio_mgr = xio_init(NULL, &err);

    xio_reg_exec_func(xio_mgr, XIO_IO_OPEN, test_open, &err);

    xio_reg_exec_func(xio_mgr, XIO_IO_WRITE, test_write, &err);

    xio_reg_exec_func(xio_mgr, XIO_IO_CLOSE, test_close, &err);


    open_data.xio_task = &xio_task_open;
    open_data.pathname = "test_file";
    open_data.flags = O_WRONLY | O_CREAT;
    open_data.mode = 0x444;
    open_data.fd = 0;
    xio_task_open.call_back = open_callback;
    xio_task_open.io_type = XIO_IO_OPEN;
    xio_task_open.user_data = &open_data;
    xio_task_open.xio_mgr = xio_mgr;
    xio_task_submit(&xio_task_open);

    while (1) {
        usleep(10000);
        if (open_data.fd != 0) {
            break;
        }
    }

    
    
    write_data.xio_task = &xio_task_write;
    write_data.fd = open_data.fd;
    write_data.buf = "1234567890";
    write_data.count = 10;
    write_data.ret = 0;
    xio_task_write.call_back = write_callback;
    xio_task_write.io_type = XIO_IO_WRITE;
    xio_task_write.user_data = &write_data;
    xio_task_write.xio_mgr = xio_mgr;
    xio_task_submit(&xio_task_write);


    while (1) {
        usleep(10000);
        if (write_data.ret != 0) {
            break;
        }
    }

    

    close_data.xio_task = &xio_task_close;
    close_data.fd = open_data.fd;
    close_data.ret = -100;
    xio_task_close.call_back = close_callback;
    xio_task_close.io_type = XIO_IO_CLOSE;
    xio_task_close.user_data = &close_data;
    xio_task_close.xio_mgr = xio_mgr;
    xio_task_submit(&xio_task_close);

    
    while (1) {
        usleep(10000);
        if (close_data.ret != -100) {
            break;
        }
    }


    return 0;
    
}