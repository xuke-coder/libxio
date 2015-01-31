#ifndef _XIO_SIGNAL_H
#define _XIO_SIGNAL_H

int
xio_signal_init(int *evfd, int block_flag, xio_err_t *err);
void
xio_signal_release(int evfd, xio_err_t *err);
int
xio_signal_send(int evfd, xio_err_t *err);
int
xio_signal_recv(int evfd, xio_err_t *err);


#endif
