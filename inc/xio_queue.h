#ifndef _XIO_QUEUE_H
#define _XIO_QUEUE_H

//#include "xio_spinlock.h"
//#include "xio_thread.h"

#ifndef xio_atomic_lock_t
#define xio_atomic_lock_t   unsigned int
#endif

#ifndef xio_thread_lock_t
#define xio_thread_lock_t  pthread_mutex_t
#endif

typedef enum {
    LOCK_TYPE_ATOMIC,
    LOCK_TYPE_THREAD,
    LOCK_TYPE_END
} LOCK_TYPE;
    
   
typedef struct xio_link_s           xio_link_t;
typedef struct xio_queue_s          xio_queue_t;

struct xio_link_s {
    xio_link_t      *prev;
    xio_link_t      *next;
};


struct xio_queue_s {
    xio_link_t          head;
    xio_atomic_lock_t   atomic_lock;
    xio_thread_lock_t   thread_lock;
    uint32_t            size;
    uint32_t            max_size;
};


#define     xio_queue_init(que, max_size)           \
                que->head.next = &que->head;        \
                que->head.prev = &que->head;        \
                que->size = 0;                      \
                que->max_size = max_size;

#define     xio_queue_release(que)                  \
                que->head.next = &que->head;        \
                que->head.prev = &que->head;        \
                que->size = 0;                      \
                que->max_size = 0;

#define     xio_queue_single_pop(que, head, node)   \
                head = &que->head;                  \
                if (head->next == head) {           \
                    node = NULL;                    \
                } else {                            \
                    node = head->next;              \
                    head->next = node->next;        \
                    node->next = NULL;              \
                    que->size--;                    \
                }

#define     xio_queue_single_push(que, tail, node)  \
                tail = que->head.prev;              \
                tail->next = node;                  \
                node->next = &que->head;            \
                que->head.prev = node;              \
                que->size++;

#define     xio_queue_single_get(que, head, node, temp, size, i)            \
                head = &que->head;                                          \
                if (head->next == head) {                                   \
                    node = NULL;                                            \
                } else {                                                    \
                    node = head->next;                                      \
                    temp = node;                                            \
                    for (i = 1; (temp->next != head) && (i < size); i++) {  \
                        temp = temp->next;                                  \
                    }                                                       \
                    if (temp->next != head) {                               \
                        head->next = temp->next;                            \
                        que->size -= size;                                  \
                    } else {                                                \
                        head->next = head;                                  \
                        head->prev = head;                                  \
                        que->size = 0;                                      \
                    }                                                       \
                }
                    
                
                


#define     xio_queue_double_pop(que, head, node)   \
                head = &que->head;                  \
                if (head->next == head) {           \
                    node = NULL;                    \
                } else {                            \
                    node = head->next;              \
                    head->next = node->next;        \
                    head->next->prev = head;        \
                    que->size--;                    \
                }

#define     xio_queue_double_push(que, tail, node)  \
                tail = que->head.prev;              \
                tail->next = node;                  \
                node->prev = tail;                  \
                node->next = &que->head;            \
                que->head.prev = node;              \
                que->size++;
                

#define     xio_queue_double_remove(que, link)      \
                link->prev->next = link->next;      \
                link->next->prev = link->prev;      \
                link->prev = NULL;                  \
                link->next = NULL;                  \
                que->size--;
#endif
