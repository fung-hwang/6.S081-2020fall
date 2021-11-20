# Lab4: Traps

### RISC-V assembly
略

### Backtrace 
+ 顺着fp链自顶至底打印每帧中的返回地址即可

`kernel/printf.c`
```c
void
backtrace(void)
{
  uint64 fp;
  uint64 pgup;
  uint64* temp;
  fp=r_fp();
  pgup=PGROUNDUP(fp);
  while(fp<pgup){
    temp=(uint64*)(fp-8);
    printptr(*temp);    //return address
    printf("\n");
    temp=(uint64*)(fp-16);
    fp=*temp;       //s0
  }
}
```

### Alarm
+ 将一次系统调用的动作放到两次系统调用中完成
+ 指定sigalarm返回地址，将原本返回的内容放在sigreturn中完成。需要在sigalarm中保存相关寄存器（sp, ra, s0, a1, epc等），可以完整保存trapframe以简化思考。

`kernel/proc.h`
```c
struct proc {
  
  int ticks;                   // alarm interval
  void (*handler)();           // the pointer to the handler function
  int ticks_pass;              // ticks have passed
  int is_alarming;             // is realarm
  struct trapframe* alarm_trapframe;
};
```

`kernel/sysproc.c`
```c
uint64
sys_sigalarm(void)
{
  int n;
  uint64 addr;
  if(argint(0, &n) < 0 || argaddr(1, &addr) < 0)
   	return -1;
  struct proc* p = myproc();
  //Prevent re-entrant calls to the handler
  if(p->is_alarming == 1)
    return -1;
  p->ticks = n;
  p->handler = (void (*)())addr;
  return 0;
}

uint64
sys_sigreturn(void)
{
  struct proc* p = myproc();   
  p->is_alarming = 0;
  memmove(p->trapframe, p->alarm_trapframe, sizeof(struct trapframe));
  return 0;
}
// ======lab4 section3========
```

`kernel/trap.c/usertrap()`
```c
  // give up the CPU if this is a timer interrupt.
  //if(which_dev == 2)
  //  yield();
  if(which_dev == 2 && p->ticks){
      p->ticks_pass++;
	  if(p->ticks_pass == p->ticks && p->is_alarming == 0){
		memmove(p->alarm_trapframe, p->trapframe, sizeof(struct trapframe));
		p->trapframe->epc = (uint64)p->handler;	//return address
	    p->ticks_pass = 0;
		p->is_alarming = 1;
	  }
      yield();
  }
```

`kernel/proc.c/allocproc()`
```c
  if((p->alarm_trapframe = (struct trapframe*)kalloc()) == 0) {
      freeproc(p);
	  release(&p->lock);
      return 0;
  }
  p->ticks = 0;
  p->ticks_pass = 0;
  p->is_alarming = 0;
  p->handler =(void (*)())0xffffffffff;
```

`kernel/proc.c/freeproc()`
```c
  p->ticks = 0;
  p->ticks_pass = 0;
  p->is_alarming = 0;
  p->handler = 0;
  if(p->alarm_trapframe)
      kfree((void*)p->alarm_trapframe);
  p->alarm_trapframe = 0;
```