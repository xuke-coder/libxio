#include <stdlib.h>
#include "xio.h"
#include "xio_spinlock.h"

inline uint32_t
xio_atomic_cmp_set(xio_atomic_lock_t *lock, uint32_t old, uint32_t new)
{
    u_char  res;

#ifdef PROCESS_64BIT
    __asm__ volatile (

    "    lock;               "
    "    cmpxchgq  %3, %1;   "
    "    sete      %0;       "

#else
    __asm__ volatile (

    "    lock;               "
    "    cmpxchgl  %3, %1;   "
    "    sete      %0;       "
    
#endif

    : "=a" (res) : "m" (*lock), "a" (old), "r" (new) : "cc", "memory");

    return res;
}


void
xio_spin_lock(xio_atomic_lock_t *lock)
{
    while (!xio_atomic_cmp_set(lock, XIO_LOCK_OFF, XIO_LOCK_ON));
}

void
xio_spin_unlock(xio_atomic_lock_t *lock)
{
    *lock = XIO_LOCK_OFF;
}

void
xio_spin_lock_init(xio_atomic_lock_t *lock)
{
    *lock = XIO_LOCK_OFF;
}

void
xio_spin_lock_release(xio_atomic_lock_t *lock)
{
    *lock = XIO_LOCK_OFF;
}




