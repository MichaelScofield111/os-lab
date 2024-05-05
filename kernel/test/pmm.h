
#include <common.h>
#include <list.h>
#include <thread.h>

typedef struct object {
    struct object* next;
} object_t;

typedef struct slab {
    struct slab* next;
    object_t* free_objects; // 指向空闲对象链表
    size_t num_free_objects; // 空闲对象数
    lock_t slb_lock; // 保护slab
    size_t size;
} slab_t;

typedef struct cache {
    slab_t* slabs;
    size_t obj_size; // 对象大小
    lock_t cache_lock;
} cache_t;
