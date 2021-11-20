# Lab3: Page tables

### Print a page table
+ 对freewalk的拙劣模仿

`kernel/vm.c`
```c
void
vmprint_PTE(pagetable_t pagetable,int level)
{
	for(int i=0;i<512;i++){
		pte_t pte = pagetable[i];
      	if(pte & PTE_V){
			for(int j=0;j<-level+3;j++){
				printf("..");
				if(j!=-level+2) printf(" ");
			}
			printf("%d: pte %p pa %p\n",i,pte,PTE2PA(pte));
			if(level>0)
				vmprint_PTE((pagetable_t)(PTE2PA(pte)),level-1);
		}
    }

}

void
vmprint(pagetable_t pagetable)
{
	printf("page table %p\n",pagetable);

   	vmprint_PTE(pagetable,2);
}
```

### A kernel page table per process
+ 文档3.8(code:exec)中暗示，在旧版本的xv6中，用户地址空间也包含内核（这种做法存在安全风险）。我们要做的正是复现旧版本的xv6。
+ 需要给每个进程配一个内核页表，理论上需要像进程页表一样（创建时分配、结束时销毁），还要把每个进程的kstack在其内核页表中做映射。如果仅为了使进程持有内核页表，为什么不直接完全复制kernel_pagetable呢？这个思路目前是完全可以的（PS：在第3题中需要稍做修改，但整体思路不变）
+ 之前读文档时看的代码是xv6-riscv，一度不能理解此题的思路提示hints为什么这么复杂。
+ 直接使用kernel_pagetable的行为纯属偷鸡。不过，建立在理解上的偷鸡不是很好吗？

`kernel/proc.c/allocproc()`
```c
p->kpagetable = proc_kpagetable();
```

`kernel/proc.c/freeproc()`
```c
  if(p->kpagetable)
  	proc_freekpagetable(p->kpagetable);
}

```

`kernel/proc.c/proc_kpagetable()`
```c
//Create a kernel pagetable for a given process
pagetable_t
proc_kpagetable(void)
{
  pagetable_t kpagetable;
  kpagetable = uvmcreate();
  if(kpagetable == 0)
    return 0;
  //just copy kernel_pagetable
  for(int i=0;i<512;i++){
    kpagetable[i]=kernel_pagetable[i];
  }
  return kpagetable;
}

```

`kernel/proc.c/proc_freekpagetable()`
```c
// Free a process's kernel page table
void
proc_freekpagetable(pagetable_t kpagetable)
{
  kfree((void*)kpagetable);
}

```

`kernel/proc.c/scheduler()`
```c
p->state = RUNNING;
c->proc = p;

// ==========section 2==================
w_satp(MAKE_SATP(p->kpagetable));
sfence_vma();
// ==========section 2==================
swtch(&c->context, &p->context);

// Process is done running for now.
// It should have changed its p->state before coming back.
c->proc = 0;

// ==========section 2==================
kvminithart();
// ==========section 2==================

found = 1;
```

+ 因为section 3的要求，上面的思路出现了缺陷。体现在：进程的内核地址0~PLIC中需要放入用户地址空间，这部分内核地址就不能直接等于kernel_pagetable中对应内存地址
+ 每个进程的内核地址0~PLIC都需要各自的内存块，而其他内核地址还可以复制kernel_pagetable的内容
+ 结束进程释放空间的时候不要使用uvmunmap来对应ukvmmap，因为用户映射已经发生改变
+ **修改部分如下所示：**

`kernel/proc.c/proc_kpagetable()`
```c
//Create a kernel pagetable for a given process
pagetable_t
proc_kpagetable(void)
{
	pagetable_t kpagetable;
  kpagetable = uvmcreate();
  if(kpagetable == 0)
    return 0;
  //just copy kernel_pagetable
  for(int i=1;i<512;i++){
    kpagetable[i]=kernel_pagetable[i];
  }

  // uart registers
  ukvmmap(kpagetable, UART0, UART0, PGSIZE, PTE_R | PTE_W);
  // virtio mmio disk interface
  ukvmmap(kpagetable, VIRTIO0, VIRTIO0, PGSIZE, PTE_R | PTE_W);
  // CLINT
  //kvmmapkern(kpagetable, CLINT, CLINT, 0x10000, PTE_R | PTE_W);
  // PLIC
  ukvmmap(kpagetable, PLIC, PLIC, 0x400000, PTE_R | PTE_W);

  return kpagetable;
}

```

kernel/proc.c `proc_freekpagetable()`
```c
// Free a process's kernel page table
void
proc_freekpagetable(pagetable_t kpagetable)
{
	//don't use uvmunmap
  pte_t pte = kpagetable[0];
  pagetable_t level1 = (pagetable_t) PTE2PA(pte);
  for (int i = 0; i < 512; i++) {
   	pte_t pte = level1[i];
    if (pte & PTE_V) {
      uint64 level2 = PTE2PA(pte);
      kfree((void *) level2);
      level1[i] = 0;
    }
  }
  kfree((void *) level1);
  kfree((void*)kpagetable);
}

```

### Simplify copyin/copyinstr
+ 在用户映射发生改变时，将进程的用户页表变化同步到进程的内核页表中

`kernel/vm.c/pgtbl_to_kpgtbl()`
```c
//add user mappings to kernel page table
void
pgtbl_to_kpgtbl(pagetable_t kpagetable,pagetable_t pagetable,uint64 oldsz,uint64 newsz)
{
  if(newsz>=PLIC)
    panic("overlap PLIC");
  pte_t *upte,*kpte;
  uint64 va;
  if(oldsz<newsz){
    for(va=oldsz;va<newsz;va+=PGSIZE){
      upte = walk(pagetable, va, 0);
      kpte = walk(kpagetable, va, 1);
      // A page without PTE_U set can be accessed in kernel mode
      *kpte = *upte & ~(PTE_U|PTE_X);
    }
  }//if
  else if(oldsz>newsz){
    for(va=newsz;va<oldsz;va+=PGSIZE){
	  upte = walk(pagetable, va, 0);
	  kpte = walk(kpagetable, va, 0);
	  if(kpte==0)
	    panic("pgtbl_to_kpgtbl");
	  *kpte=0;
	}
  }//else if
}
```

`kernel/proc.c/userinit()`
```c
// Set up first user process.
void
userinit(void)
{
  //其余部分省略

	p->state = RUNNABLE;

  // ==========section 3==================
  pgtbl_to_kpgtbl(p->kpagetable,p->pagetable,0,p->sz);
  // ==========section 3==================

  release(&p->lock);
}
```

`kernel/proc.c/growproc()`
```c
int
growproc(int n)
{
  uint sz;
  struct proc *p = myproc();

  sz = p->sz;
  if(n > 0){
    if((sz = uvmalloc(p->pagetable, sz, sz + n)) == 0) {
      return -1;
    }
  } else if(n < 0){
    sz = uvmdealloc(p->pagetable, sz, sz + n);
  }

  // ===========section 3===========
  pgtbl_to_kpgtbl(p->kpagetable,p->pagetable,p->sz,sz);
  // ===========section 3===========

  p->sz = sz;
  return 0;
}
```

`kernel/proc.c/fork()`
```c
int
fork(void)
{
  //其余部分省略

  pid = np->pid;

  // ===========section 3===========
  pgtbl_to_kpgtbl(np->kpagetable,np->pagetable,0,np->sz);
  // ===========section 3===========

  np->state = RUNNABLE;

  release(&np->lock);

  return pid;
}
```

`kernel/exec.c/exec()`
```c
int
exec(char *path, char **argv)
{
	//其余部分省略
	pgtbl_to_kpgtbl(p->kpagetable,p->pagetable,0,p->sz);
}
```
