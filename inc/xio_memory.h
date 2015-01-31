#ifndef _XIO_MEMORY_H
#define _XIO_MEMORY_H


#if defined(_SC_PAGESIZE)
#define SYSPAGE_SIZE  sysconf(_SC_PAGESIZE)
#elif defined(_SC_PAGE_SIZE)
#define SYSPAGE_SIZE sysconf(_SC_PAGE_SIZE)
#else
#define SYSPAGE_SIZE 4096
#endif


//typedef struct xio_pool_s           xio_pool_t;
typedef struct xio_normal_pool_s    xio_normal_pool_t;
typedef struct xio_large_pool_s     xio_large_pool_t;

struct xio_pool_s {
    xio_normal_pool_t   *normal_pool;
    xio_normal_pool_t   *cur_normal_pool;
    xio_large_pool_t    *large_pool;
};

struct xio_normal_pool_s {
    size_t                   max_size;
    char                    *cur;
    char                    *end;
    xio_normal_pool_t       *next;
};

struct xio_large_pool_s {
    char                *data;
    size_t               size;
    xio_large_pool_t    *next;
};

    
xio_pool_t *
xio_memory_pool_create(size_t size, size_t syspagesize, xio_err_t *err);
void
xio_memory_pool_release(xio_pool_t *pool, xio_err_t *err);
void *
xio_memory_pool_alloc(xio_pool_t *pool, size_t size, xio_err_t *err);
void *
xio_memory_pool_create_next(xio_pool_t *pool, size_t size, 
    xio_err_t *err);
void *
xio_memory_pool_create_large(xio_pool_t *pool, size_t size, 
    xio_err_t *err);
void *
xio_memory_alloc(uint32_t size, xio_err_t *err);
void *
xio_memory_calloc(uint32_t size, xio_err_t *err);
void
xio_memory_free(void *p, xio_err_t *err);
void *
xio_memory_memalign(uint32_t alignment, uint32_t size, 
    xio_err_t *err);
void
xio_memory_set(void *s, int c, size_t n);

#endif

