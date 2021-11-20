# Lab2: system calls

### trace
+ 根据lab提示完成声明、数据等的补充
+ struct proc中增加掩码mask
+ 执行系统调用trace时从a0处获得mask
+ 每次系统调用结束前打印，打印前需判断该系统调用是否被掩码mask掩蔽
+ 子进程需继承父进程的mask

`kernel/proc.h`
```c
struct proc {
  //已有内容省略
  //add mask
  int mask;
};
```

`kernel/sysproc.c`
```c
uint64
sys_trace(void)
{
  int mask;
  //read mask from p->trapframe->a0
  if(argint(0, &mask) < 0)
    return -1;
  	myproc()->mask = mask;
  return 0;
}

```

`kernel/syscall.c`
```c
void
syscall(void)
{
  const char *syscall_name[]={
  " ", //占位
  "fork",
  "exit",
  "wait",
  "pipe",
  "read",
  "kill",
  "exec",
  "fstat",
  "chdir",
  "dup",
  "getpid",
  "sbrk",
  "sleep",
  "uptime",
  "open",
  "write",
  "mknod",
  "unlink",
  "link",
  "mkdir",
  "close",
  "trace"
  };

  int num;
  struct proc *p = myproc();

  num = p->trapframe->a7;
  if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    p->trapframe->a0 = syscalls[num]();
	//print
	if(p->mask&(1<<num))
		printf("%d: syscall %s -> %d\n",p->pid,syscall_name[num],p->trapframe->a0);
  }
  else {
    printf("%d %s: unknown sys call %d\n",
            p->pid, p->name, num);
    p->trapframe->a0 = -1;
  }
}

```

`kernel/proc.c`
```c
int
fork(void)
{
  //已有内容省略
  np->mask=p->mask

  return pid;
}
```

### sysinfo
+ 和trace一样完成声明、数据等的补充
+ 在kalloc.c中遍历空闲块链freelist，统计空闲块数量
+ 在proc.c中统计进程数量
+ 在sys_sysinfo()中使用copyout()，将内核变量传递到用户空间

`kernel/sysproc.c`
```c
uint64
sys_sysinfo()
{
  uint64 info_addr;	//user pointer to struct sysinfo
  if(argaddr(0,&info_addr)<0)
    return -1;
  uint64 nfm,nproc;
  if((nfm=freemem())<0)
  	return -1;

  if((nproc=proc_num())<0)
  	return -1;

  struct proc *p = myproc();
  struct sysinfo sysi;
  sysi.freemem=nfm;
  sysi.nproc=nproc;

  if(copyout(p->pagetable,info_addr,(char *)&sysi,sizeof(sysi))<0)
  	return -1;
  //printf("deliver success--------\n");
  return 0;
}

```

`kernel/kalloc.c`
```c
uint64
freemem()
{
  struct run *r;
  uint64 num = 0;
  for (r = kmem.freelist; r != 0; r = r->next)
    num++;
  return num*PGSIZE;
}
```

`kernel/proc.c`
```c
uint64
proc_num(void)
{
  uint64 num = 0;
  struct proc *p;
  for (p = proc; p < &proc[NPROC]; p++) {
    if (p->state == UNUSED)
      continue;
    else {
      num++;
    }
  }
  return num;
}
```
