#include <common.h>
#include <pmm.h>

static cache_t g_caches[MAX_CACHES];

void slab_allocator_init()
{
   size_t obj_sz = 8;
   // obj_sz 8 -> 2048
   for (int i = 0; i < MAX_CACHES; i ++)
   {
       g_caches[i].slab = NULL;
       g_caches[i].obj_size = obj_sz;
       g_caches[i].cache_lock = NULL;
       obj_sz = obj_sz << 1;
   }
}

static cache_t* find_cache(size_t size)
{
    for (int i = 0; i < MAX_CACHES; i ++)
    {
        if(g_caches[i].obj_size >= size) return &g_caches[i];
    }
    return NULL;
}

static slab_t* allocate_slab(cache_t* cache)
{
    // 如果我们从cache最开始的从左到右都没有找到object
    // 1. new page;
    // 初始化页面上的一些slab信息
    // 将页面空闲区域的obj以链表方式进行连接
    uintptr_t slab_addr = buddy_alloc(&g_buddy_pool, PAGE_SIZE);
    assert(slab_addr % PAGE_SIZE == 0); //对其到PAGE_SIZE的
    
    slab_t* new_slab = (slab_t *)slab_addr;
    // 有疑问?
    slab_addr += sizeof(slab_t);
    slab_addr = ALIGN(slab_addrm cache->obj_size);

    size_t num_obj = (PAGE_SIZE - (slab_addr - (uintptr_t)new_slab)) / cache->obj_size;
    object_t* obj = (object_t*)slab_addr;
    new_slab->free_objects = obj;
    new_slab->num_free_objects = num_obj;
    new_slab->size = cache->obj_size;

    for (int i = 0; i < num_obj - 1; i ++) {
        obj->next = (object_t *)((uintptr_t)obj + new_slab->size);
        obj = obj->next;
        assert((uintptr_t)obj + new_slab->size <= (uintptr_t)new_slab + PAGE_SIZE);
    }

    obj->next = NULL;
    return new_slab;

}

void *slab_alloc(size_t size)
{
    if (size == 0 || size >= PAGE_SIZE){
        return NULL;
    }


    cache_t* cache = find_cache(size);
    if (cache == NULL) {
        PANIC("slab alloc error");
        return NULL;
    }

    slab_t* slab = cache->slabs;
    while(slab != NULL)
    {   
        if (slab->num_free_objects > 0)
        {
            object_t* obj = slab->free_objects;
            free_objects = obj->next;
            slab->num_free_objects --;
            return obj;
        }
        else
        {
            slab = slab->next;
        }
    }

    if (slab == NULL) {
        // build new slab
        slab = allocate_slab(cache);
        slab->next = cache->next;
        cache->slabs = slab;
    }

    object_t* obj = slab->free_objects;
    slab->free_objects = obj->next;
    slab->num_free_objects --;

    return obj;
}

// slab free
void slab_free(void* ptr)
{
    // 如果对ptr的低12位全部清0的话就可以得到slab()开始的地址(储存元信息的结构体)
    if (ptr == NULL) return;
    object_t* obj = (object_t *)ptr;
    slab_t* slab = ((slab_t *)ptr & ~(PAGE_SIZE - 1));

    obj->next = slab->free_objects;
    slab->free_objects = obj;
    slab->num_free_objects ++;

}

static void *kalloc(size_t size) {
    // TODO
    // You can add more .c files to the repo.
    void* ret = NULL;
    size = align_size(size);

    ret = slab_alloc(size);

    return ret;
}

static void kfree(void *ptr) {
    // TODO
    // You can add more .c files to the repo.
    

}

static void pmm_init() {
    uintptr_t pmsize = (
        (uintptr_t)heap.end
        - (uintptr_t)heap.start
    );

    printf(
        "Got %d MiB heap: [%p, %p)\n",
        pmsize >> 20, heap.start, heap.end
    );
}

MODULE_DEF(pmm) = {
    .init  = pmm_init,
    .alloc = kalloc,
    .free  = kfree,
};
