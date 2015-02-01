#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/eventfd.h>
#include "xio.h"

int
xio_signal_init(int *evfd, int block_flag, xio_err_t *err)
{
    int    ret = 0;

    if (!err) {
        return XIO_ERROR;
    }
    
    if (!evfd) {
        err->log_errno = XIO_ERR_PARAM;
        return XIO_ERROR;
    }

    if (block_flag == XIO_FALSE) {
        if ((ret = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)) == -1) {
            err->sys_errno = errno;
            return XIO_ERROR;
        }
    } else {
        if ((ret = eventfd(0, EFD_CLOEXEC)) == -1) {
            err->sys_errno = errno;
            return XIO_ERROR;
        }
    }

    *evfd = ret;

    return XIO_OK;
}

void
xio_signal_release(int evfd, xio_err_t *err)
{
    if (close(evfd) == -1) {
        err->sys_errno = errno;
    }
    
}

int
xio_signal_send(int evfd, xio_err_t *err)
{
    uint64_t    val = 0x555;

    if (write(evfd, &val, sizeof(val)) == -1) {
        if (err) {
            err->sys_errno = errno;
        }
        return XIO_ERROR;
    }
    return XIO_OK;
}

int
xio_signal_recv(int evfd, xio_err_t *err)
{
    uint64_t    val = 0;

    if (read(evfd, &val, sizeof(val)) == -1) {
        if (err) {
            err->sys_errno = errno;
        }
        return XIO_ERROR;
    }

    return XIO_OK;
    
}

