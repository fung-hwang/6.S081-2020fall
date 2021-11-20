# Lab7: Multithreading

### Uthread: switching between threads
+ 用户级线程实现
+ 每个用户级线程都拥有其自己的线程栈stack
+ 用户级线程只在进程环境中切换，不涉及内核
+ 只需要保存和恢复少量与执行密切相关的寄存器，其实和内核中的进程切换是一样的

`user/uthread`
```c
struct ucontext{
	uint64 ra;
	uint64 sp;
	// callee-saved
	uint64 s0;
	uint64 s1;
	uint64 s2;
	uint64 s3;
	uint64 s4;
	uint64 s5;
	uint64 s6;
	uint64 s7;
	uint64 s8;
	uint64 s9;
	uint64 s10;
	uint64 s11;
};

struct thread {
  char stack[STACK_SIZE]; /* the thread's stack */
  int state;             /* FREE, RUNNING, RUNNABLE */
  struct ucontext context;
};

void 
thread_schedule(void)
{
  // 其余省略
  if (current_thread != next_thread) {         /* switch threads?  */
    next_thread->state = RUNNING;
    t = current_thread;
    current_thread = next_thread;
    /* YOUR CODE HERE
     * Invoke thread_switch to switch from t to next_thread:
     * thread_switch(??, ??);
     */
    thread_switch((uint64)&(t->context),(uint64)&(current_thread->context));
  } else
    next_thread = 0;
}

void 
thread_create(void (*func)())
{
  struct thread *t;

  for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
    if (t->state == FREE) break;
  }
  t->state = RUNNABLE;
  // YOUR CODE HERE
	(t->context).ra = (uint64)func;
	(t->context).sp = (uint64)t->stack + STACK_SIZE;
}
```

### Using threads
+ 简单使用线程库pthread
+ 因为只有put()和insert()修改hashtable，而insert()由put()调用，所以对put()的内容加锁即可
+ 如果单从题目的角度而言，修改value不影响get_thread中判断key是否存在，可以只在insert()前后加锁

`notxv6/ph.c`
```c
pthread_mutex_t mutex[NBUCKET];

static 
void put(int key, int value)
{
  int i = key % NBUCKET;

  // is the key already present?
  struct entry *e = 0;
  pthread_mutex_lock(&mutex[i]);
  for (e = table[i]; e != 0; e = e->next) {
    if (e->key == key)
      break;
  }
  if(e){
    // update the existing key.
    e->value = value;
  } else {
    // the new is new.
    insert(key, value, &table[i], table[i]);
  }
  pthread_mutex_unlock(&mutex[i]);
}
// 在main中 pthread_mutex_init
```

### Barrier
+ 简单使用线程库pthread
+ 如果该轮还有进程没结束，就wait等待
+ 如果此进程是该轮最后一个进程，则处理轮次信息并broadcast，唤醒其他所有进程开始下一轮

`notxv6/barrier.c`
```c
static void
barrier()
{
	// YOUR CODE HERE
	//
	// Block until all threads have called barrier() and
	// then increment bstate.round.
	//
	pthread_mutex_lock(&bstate.barrier_mutex);
	bstate.nthread++;
	if (bstate.nthread != nthread)
		pthread_cond_wait(&bstate.barrier_cond, &bstate.barrier_mutex);
	else {
		bstate.round++;
		bstate.nthread = 0;
		pthread_cond_broadcast(&bstate.barrier_cond);
	}
	pthread_mutex_unlock(&bstate.barrier_mutex);
}
```