#include "common.h"
#include "thread.h"

static inline size_t min(size_t a, size_t b) { return a < b ? a : b; }
#define N 100010
#define PG_SZ 4096
#define MAGIC 0x6666

struct op {
    void* addr;
    size_t size;
} queue[N];

int n, count = 0;              // 缓冲区的大小，以及当前缓冲区的元素数量
#define MAX_MEM_SZ (96 << 20)  // 96MB

size_t total_sz = 0;
mutex_t lk = MUTEX_INIT();
cond_t cv = COND_INIT();



/**
 * @brief 检查一个地址是否被二次分配:核心方法时通过magic number检查
 * @param ptr 要检查的地址
 * @param size 要检查的大小
 */
void double_alloc_check(void* ptr, size_t size) {
    unsigned int* arr = (unsigned int*)ptr;
    for (int i = 0; (i + 1) * sizeof(unsigned int) <= size; i++) {
        if (arr[i] == MAGIC) {
            PANIC("Double Alloc at addr %p with size %ld!\n", ptr, size);
        }
        arr[i] = MAGIC;
    }
}

/**
 * @brief 清除一个地址的magic number：置 0
 */
void clear_magic(void* ptr, size_t size) {
    unsigned int* arr = (unsigned int*)ptr;
    for (int i = 0; (i + 1) * sizeof(unsigned int) <= size; i++) {
        arr[i] = 0;
    }
}


/**
 * @brief 带有循环次数限制的producer，测试slab分配器
 */
void loop_small_sz_producer(int id) {
    n = (64 << 10);  // 64K
    for (int i = 0; i < n; i++) {
        size_t sz = small_random_sz();
        // debug("%d th Producer trys to alloc %ld\n", id, sz);
        void* ret = pmm->alloc(sz);
        PANIC_ON(ret == NULL, "pmm->alloc(%ld) failed!\n", sz);
        double_alloc_check(ret, sz);
        alignment_check(ret, sz);

        // 将分配的地址和大小放入队列中，如果队列满了则等待
        mutex_lock(&lk);
        while (count == n) {
            cond_wait(&cv, &lk);
        }

        // 将分配的地址和大小放入队列中，并通知消费者，消费者负责进行free
        count++;
        assert(count <= n);
        queue[count].addr = ret;
        queue[count].size = sz;
        cond_broadcast(&cv);
        mutex_unlock(&lk);
    }
}

void loop_small_sz_consumer(int id)
{
    n = (64 << 10); // 64 * 2^10
    for (int i = 0; i < n; i ++)
    {
        mutex_lock(&lk);
        while(count == 0) cond_wait(&cv, &lk);

        // get element;
        void* addr = queue[count].addr;
        size_t sz = queue[count].size;
        count --;
        assert(count >= 0);
        cond_broadcast(&cv);
        mutex_unlock(&lk);
        clear_magic(addr, sz);
        pmm->free(addr);
    }
}


