#if !defined(__QUEUE_H__)
#define __QUEUE_H__

#include "slot.h"
#include "ring.h"

/* 队列配置 */
typedef struct
{
    int max;                                /* 单元总数 */
    int size;                               /* 单元大小 */
} queue_conf_t;

/* 加锁队列 */
typedef struct
{
    slot_t *slot;                           /* 内存池 */
    ring_t *ring;                     /* 队列 */
} queue_t;

queue_t *queue_creat(int max, int size);
#define queue_malloc(q) slot_alloc((q)->slot)
#define queue_dealloc(q, p) slot_dealloc((q)->slot, p)
#define queue_push(q, addr) ring_push((q)->ring, addr)
#define queue_pop(q) ring_pop((q)->ring)
#define queue_print(q) ring_print((q)->ring)
void queue_destroy(queue_t *q);

/* 获取队列剩余空间 */
#define queue_space(q) ((q)->ring->max - (q)->ring->num)
#define queue_used(q) ((q)->ring->num)
#define queue_size(q) ((q)->slot->size)

#endif /*__QUEUE_H__*/
