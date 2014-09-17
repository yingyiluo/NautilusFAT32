#ifndef __THREAD_H__
#define __THREAD_H__

#ifndef __ASSEMBLER__

#include <spinlock.h>
#include <queue.h>

#define CPU_ANY -1
#define TSTACK_DEFAULT 0

typedef void (*thread_fun_t)(void * input, void ** output);
typedef unsigned stack_size_t;
typedef long thread_id_t;
typedef struct thread thread_t;
typedef struct queue thread_queue_t;


struct thread {
    uint64_t rsp; /* KCH: this cannot change */
    void * stack;
    stack_size_t stack_size;

    queue_entry_t runq_node; // formerly q_node
    queue_entry_t thr_list_node;
    thread_id_t tid;
    struct thread * owner;

    thread_queue_t * waitq;
    queue_entry_t wait_node;

    thread_queue_t * cur_run_q;
    void * exit_status;

    /* thread has finished? */
    uint8_t exited;

    unsigned long refcount;
    int bound_cpu;
};


struct sched_state {
    thread_queue_t * thread_list;
    uint_t num_threads;
};


/* the thread interface */
void yield(void);
void schedule(void);
int sched_init(void);
int sched_init_ap(void);
void exit(void * retval);
void wait(thread_t * t);
void wake_waiters(void);
int join(thread_t * t, void ** retval);

thread_t* 
thread_create (thread_fun_t fun, 
               void * input,
               void ** output,
               uint8_t is_detached,
               stack_size_t stack_size,
               thread_id_t * tid,
               int cpu);
thread_t* 
thread_start (thread_fun_t fun, 
               void * input,
               void ** output,
               uint8_t is_detached,
               stack_size_t stack_size,
               thread_id_t * tid,
               int cpu);

void thread_destroy(thread_t * t);
thread_id_t get_tid(void);
thread_id_t get_parent_tid(void);


#include <percpu.h>

static inline thread_t*
get_cur_thread (void) 
{
    return (thread_t*)per_cpu_get(cur_thread);
}

static inline void
put_cur_thread (thread_t * t)
{
    per_cpu_put(cur_thread, t);
}



#endif /* !__ASSEMBLER */

#endif  /* !__THREAD_H__ */