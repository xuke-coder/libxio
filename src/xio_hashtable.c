#include <stdio.h>
#include <stdlib.h>
#include "xio_hashtable.h"


int
xio_hashtable_init(xio_hashtable_t *htable, uint32_t size, xio_err_t *err)
{
    uint64_t    pool_size = 0;
    
    if (!htable) {
        err->log_errno = XIO_ERR_PARAM;
        return XIO_ERROR;
    }

    htable->max_size = size;
    htable->size = 0;

    pool_size = sizeof(void *) * size + sizeof(xio_hashtable_elem_t) * size * 2;
    
    if (!(htable->pool = (xio_pool_t *)xio_memory_pool_create(
        pool_size, err))) {
        err->log_errno = XIO_ERR_MEMORY_ALLOC;
        return XIO_ERROR;
    }

    if (!(htable->buckets = (xio_hashtable_elem_t **)xio_memory_pool_alloc(
        htable->pool, size, err))) {
        goto pool_release;
    }

    xio_spin_lock_init(&htable->atomic_lock);

    return XIO_OK;

pool_release:
    xio_memory_pool_release(htable->pool, err);
    return XIO_ERROR;
}


void
xio_hashtable_release(xio_hashtable_t *htable, xio_err_t *err)
{
    if (!htable) {
        err->log_errno = XIO_ERR_PARAM;
        return;
    }

    htable->size = 0;
    htable->max_size = 0;
    htable->buckets = NULL;
    xio_spin_lock_release(&htable->atomic_lock);
    xio_memory_pool_release(htable->pool, err);
    htable->pool = NULL;
}


int
xio_hashtable_push(xio_hashtable_t *htable, uint32_t key, uint32_t fd, 
    xio_data_task_node_t *task_node, xio_err_t *err)
{
    xio_hashtable_elem_t    *new_elem = NULL;
    xio_hashtable_elem_t    *elem = NULL;
    xio_pool_t              *pool = NULL;
    xio_link_t              *tail = NULL;
    uint32_t                 index = 0;
    
    if (!htable || !task_node) {
        err->log_errno = XIO_ERR_PARAM;
        return XIO_ERROR;
    }

    pool = htable->pool;

    xio_spin_lock(&htable->atomic_lock);
    
    index = key % htable->max_size;
    elem = htable->buckets[index];

    if (!elem) {
        if (!(new_elem = (xio_hashtable_elem_t *)xio_memory_pool_alloc(pool, 
            sizeof(xio_hashtable_elem_t), err))) {
            err->log_errno = XIO_ERR_MEMORY_ALLOC;
            return XIO_ERROR;
        }

        htable->buckets[index] = new_elem;

        new_elem->next = NULL;
        new_elem->fd = fd;
        xio_queue_init(&new_elem->que, 0);
        xio_queue_single_push(&new_elem->que, tail, &task_node->que_link);
        
    } else if (elem->fd == fd){
        xio_queue_single_push(&elem->que, tail, &task_node->que_link); 

    } else {
        
        while (elem->next) {
            if (elem->next->fd != fd) {
                elem = elem->next;
            } else {
                break;
            }
        }
        
        if (!elem->next) {
            if (!(new_elem = (xio_hashtable_elem_t *)xio_memory_pool_alloc(pool, 
                sizeof(xio_hashtable_elem_t), err))) {
                err->log_errno = XIO_ERR_MEMORY_ALLOC;
                return XIO_ERROR;
            }

            new_elem->next = NULL;
            new_elem->fd = fd;
            xio_queue_init(&new_elem->que, 0);
            xio_queue_single_push(&new_elem->que, tail, &task_node->que_link);

            elem->next = new_elem;
            
        } else {
            elem = elem->next;
            xio_queue_single_push(&elem->que, tail, &task_node->que_link);
        }
    }

    xio_spin_unlock(&htable->atomic_lock);

    return XIO_OK;
}

xio_hashtable_elem_t *
xio_hashtable_pop(xio_hashtable_t *htable, uint32_t key, xio_err_t *err)
{
    xio_hashtable_elem_t    *elem = NULL;
    uint32_t                 index = 0;
    
    if (!htable) {
        if (err) {
            err->log_errno = XIO_ERR_PARAM;
        }
        return NULL;
    }

    xio_spin_lock(&htable->atomic_lock);
    
    index = key % htable->max_size;
    elem = htable[index];

    if (!elem) {
        return NULL;
    } else {
        htable[index] = elem->next;
    }

    xio_spin_unlock(&htable->atomic_lock);
    
    elem->next = NULL;
    
    return elem;

    
}

