#ifndef _XIO_SPINLOCK_H
#define _XIO_SPINLOCK_H

#define XIO_LOCK_ON     1
#define XIO_LOCK_OFF    0

#define xio_atomic_lock_t   unsigned int

void
xio_spin_lock(xio_atomic_lock_t *lock);
void
xio_spin_unlock(xio_atomic_lock_t *lock);
void
xio_spin_lock_init(xio_atomic_lock_t *lock);
void
xio_spin_lock_release(xio_atomic_lock_t *lock);

#endif
