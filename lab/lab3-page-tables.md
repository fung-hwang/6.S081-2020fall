## Lab3: Page tables

#### Print a page table
+ 对freewalk的拙劣模仿

kernel/vm.c
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

#### A kernel page table per process
+ lab给的思路略微反人类，猜测是为了让我们写一遍页表分配的相关操作
+ 之前读文档时看的代码是xv6-riscv，一度不能理解这个section
+ 需要给每个进程配一个内核页表，理论上需要像进程页表一样（创建时分配、结束时销毁），还要把每个进程的kstack在其内核页表中做映射。如果仅为了使进程持有内核页表，为什么不直接复制kernel_pagetable呢？
+ 如果之后发现这个思路有缺陷，再进行修改，目前是完全可以的

kernel/proc.c `allocproc`
```c
p->kpagetable = proc_kpagetable();
```

kernel/proc.c `freeproc()`
```c
  if(p->kpagetable)
  	proc_freekpagetable(p->kpagetable);
}

```

kernel/proc.c `proc_kpagetable()`
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

kernel/proc.c `proc_freekpagetable()`
```c
// Free a process's kernel page table
void
proc_freekpagetable(pagetable_t kpagetable)
{
  kfree((void*)kpagetable);
}

```

kernel/proc.c `scheduler()`
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
