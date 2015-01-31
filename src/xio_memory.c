
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include "xio.h"
#include "xio_memory.h"

xio_pool_t *
xio_memory_pool_create(size_t size, size_t syspagesize, xio_err_t *err)
{
    xio_pool_t          *pool = NULL;
    xio_normal_pool_t   *normal_pool = NULL;

    if (size == 0) {
        err->log_errno = XIO_ERR_PARAM;
        return NULL;
    }

    if (!(pool = xio_memory_alloc(sizeof(xio_pool_t), err))) {
        return NULL;
    }

    if (size < sizeof(xio_normal_pool_t)) {
        size += sizeof(xio_normal_pool_t);
    }

    if (!(normal_pool = xio_memory_alloc(size, err))) {
        goto fuck_pool_release;
    }

    pool->cur_normal_pool = normal_pool;
    pool->normal_pool = normal_pool;
    pool->large_pool = NULL;

    normal_pool->cur = (char *)normal_pool + sizeof(xio_normal_pool_t);
    normal_pool->end = (char *)normal_pool + size;
    normal_pool->next = NULL;

    size -= sizeof(xio_pool_t);

    if (size < syspagesize) {
        normal_pool->max_size = size;
    } else {
        normal_pool->max_size = syspagesize;
    }

    return pool;

fuck_pool_release:
    xio_memory_free(pool, err);
    return NULL;
}

void
xio_memory_pool_release(xio_pool_t *pool, xio_err_t *err)
{
    xio_normal_pool_t *normal = NULL;
    xio_normal_pool_t *temp_normal = NULL;
    xio_large_pool_t  *large = NULL;
    xio_large_pool_t  *temp_large = NULL;
    
    if (!pool) {
        err->log_errno = XIO_ERR_PARAM;
        return;
    }

    large = pool->large_pool;
    
    while (large) {
        temp_large = large->next;
        xio_memory_free(large->data, err);
        large = temp_large;
    }

    normal = pool->normal_pool;

    while (normal) {
        temp_normal = normal->next;
        xio_memory_free(normal, err);
        normal = temp_normal;
    }

    xio_memory_free(pool, err);
}

void *
xio_memory_pool_alloc(xio_pool_t *pool, size_t size, xio_err_t *err)
{
    xio_normal_pool_t   *normal_pool = NULL;
    char                    *align_ptr = NULL;
    
    if (!pool || size == 0) {
        err->sys_errno = XIO_ERR_PARAM;
        return NULL;
    }

    normal_pool = pool->cur_normal_pool;
    
    if (size <= normal_pool->max_size) {
        while (normal_pool) {
            align_ptr = (char *)(((uintptr_t)normal_pool->cur + 
                (uintptr_t)(sizeof(char *) - 1)) & ~((uintptr_t)sizeof(char *) - 1));
            if (align_ptr < normal_pool->end && (size_t)(normal_pool->end - align_ptr)
                > size) {
                normal_pool->cur = align_ptr + size;
                return align_ptr;
            }

            normal_pool = normal_pool->next;
        }

        return xio_memory_pool_create_next(pool, size, err);    
    }

    return xio_memory_pool_create_large(pool, size, err);
}


void *
xio_memory_pool_create_next(xio_pool_t *pool, size_t size, 
    xio_err_t *err)
{
    xio_normal_pool_t   *new_pool = NULL;
    xio_normal_pool_t   *cur_pool = NULL;
    xio_normal_pool_t   *p = NULL;
    char                *align_ptr = NULL;
    size_t               pool_size = 0;
    
    if (!pool || size == 0) {
        err->log_errno = XIO_ERR_PARAM;
        return NULL;
    }

    cur_pool = pool->cur_normal_pool;
    
    pool_size = (size_t)((char *)cur_pool->end - (char *)cur_pool);
    
    if (!(new_pool = xio_memory_alloc(pool_size, err))) {
        return NULL;
    }
    

    new_pool->end = (char *)new_pool + pool_size;
    new_pool->cur = (char *)new_pool + sizeof(xio_normal_pool_t);
    new_pool->next = NULL;
    align_ptr = (char *)(((uintptr_t)new_pool->cur + (uintptr_t)(sizeof(char *) - 1)) 
                &~ ((uintptr_t)sizeof(char *) - 1));
    new_pool->cur = align_ptr + size;

    for (p = cur_pool; p->next; p = p->next) {
        if ((size_t)(p->end - p->cur) < sizeof(char *)) {
            pool->cur_normal_pool = p->next;
        }
    }

    p->next = new_pool;
    
    return align_ptr;
}


void *
xio_memory_pool_create_large(xio_pool_t *pool, size_t size, 
    xio_err_t *err)
{
    char                *p = NULL;
    xio_large_pool_t    *large_pool = NULL;
    
    if (!pool || size == 0) {
        err->log_errno = XIO_ERR_PARAM;
        return NULL;
    }

    if (!(p = xio_memory_alloc(size, err))) {
        return NULL;
    }

    if (!(large_pool = xio_memory_pool_alloc(pool, 
        sizeof(xio_large_pool_t), err))) {
        goto fuck_memory_release;
    }

    large_pool->data = p;
    large_pool->size = size;
    large_pool->next = pool->large_pool;
    pool->large_pool = large_pool;

    return p;

fuck_memory_release:
    xio_memory_free(p, err);
    return NULL;
}

void *
xio_memory_alloc(uint32_t size, xio_err_t *err)
{
    void *p = NULL;
    
    if (size == 0) {
        err->log_errno = XIO_ERR_PARAM;
        return NULL;
    }
    
    if (!(p = malloc(size))) {
        err->sys_errno = errno;
        return NULL;
    }

    return p;
}

void *
xio_memory_calloc(uint32_t size, xio_err_t *err)
{
    void *p = NULL;
    
    if (size == 0)
        err->log_errno = XIO_ERR_PARAM;
        return NULL;

    if (!(p = xio_memory_alloc(size, err))) {
        return NULL;
    }

    xio_memory_set(p, 0, size);
    return p;
}

void
xio_memory_free(void *p, xio_err_t *err)
{
    if (!p) {
        err->log_errno = XIO_ERR_PARAM;
        return;
    }
    free(p);
}

void *
xio_memory_memalign(uint32_t alignment, uint32_t size, 
    xio_err_t *err)
{
    void *p = NULL;
    int   ret = 0;
    
    if (size == 0) {
        err->log_errno = XIO_ERR_PARAM;
        return NULL;
    }

    if ((ret = posix_memalign(&p, alignment, size)) != 0) {
        err->sys_errno = ret;
        return NULL;
    }

    return p;
}


void
xio_memory_set(void *s, int c, size_t n)
{
    memset(s, c, n);
}

