#include "co.h"
#include <stdlib.h>
#include <setjmp.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#define STACK_SIZE 4 * 1024 * 8 // uintptr_t ---->  8 in x86_64

static inline void stack_switch_call(void* sp, void* entry, uintptr_t arg){    
    asm volatile(
#if __x86_64__
            "movq %%rsp,-8(%0); leaq -16(%0), %%rsp; movq %2, %%rdi ; call *%1; movq -8(%0) ,%%rsp;"
            :
            : "b"((uintptr_t)sp), "d"(entry), "a"(arg)
            : "memory"
#else
            "movl %%esp, -0x8(%0); leal -0xC(%0), %%esp; movl %2, -0xC(%0); call *%1;movl -0x8(%0), %%esp"
            :
            : "b"((uintptr_t)sp), "d"(entry), "a"(arg)
            : "memory"
#endif
            ); 
}

enum co_status
{
    CO_NEW = 1,	
    CO_RUNNING,
    CO_WAITING, 
    CO_DEAD,	
};

struct co
{
    struct co *next;
    struct co *waiter;			   
    void (*func)(void *);
    void *arg;
    char name[50];
    enum co_status status;		   
    jmp_buf context;			   
    uint8_t stack[STACK_SIZE + 1]; // 协程的堆栈						   // uint8_t == unsigned char
};

struct co *current;
struct co *co_start(const char *name, void (*func)(void *), void *arg)
{   

    // init current if current is NULL
    if (current == NULL) 
    {
        current = (struct co*)malloc(sizeof(struct co));
        current->status = CO_RUNNING; // BUG !! 写成了 current->status==8CO_RUNNING;
        current->waiter = NULL;
        strcpy(current->name, "main");
        current->next = current;
    }

    struct co* new_node = (struct co*)malloc(sizeof(struct co));
    new_node->arg = arg;
    new_node->func = func;
    new_node->status = CO_NEW;
    strcpy(new_node->name, name);

    struct co *temp = current;
    while (temp)
    {
        if (temp->next == current)
            break;

        temp = temp->next;
    }
    assert(temp);
    temp->next = new_node;
    new_node->next = current;
    return new_node;
}

void co_wait(struct co *co)
{
    current->status = CO_WAITING;
    co->waiter = current;
    while (co->status != CO_DEAD)
    {
        co_yield ();
    }

    assert(co->status == CO_DEAD);
    current->status = CO_RUNNING;
    struct co* temp = current;

    while (temp)
    {
        if (temp->next == co) break;
        temp = temp->next;
    }

    temp->next = temp->next->next;
    free(co);
}

void co_yield ()
{
    assert(current);
    int ret = setjmp(current->context);
    if (ret == 0) 
    {
        // choose coroutine
        struct co *coroutine = current;
        do
        {
            coroutine = coroutine->next;
        } while (coroutine->status == CO_DEAD || coroutine->status == CO_WAITING);

        current = coroutine;
        assert(current == coroutine);
        if (current->status == CO_NEW)
        {
            assert(current->status == CO_NEW);
            ((struct co volatile *)current)->status = CO_RUNNING;
            stack_switch_call(&current->stack[STACK_SIZE], current->func, (uintptr_t)current->arg);
            ((struct co volatile *)current)->status = CO_DEAD;
            if (current->waiter) current = current->waiter;
        }
        else
        {
            longjmp(current->context, 1);
        }
    }
}

