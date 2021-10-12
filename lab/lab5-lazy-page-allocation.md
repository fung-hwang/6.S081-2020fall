## Lazy page allocation 

#### Eliminate allocation from sbrk()
`kernel/sysproc.c`
```c
uint64
sys_sbrk(void)
{
    int addr;
    int n;
  
    if(argint(0, &n) < 0)
      return -1;
    // =======lab5 section1===========
    addr = myproc()->sz;
    myproc()->sz += n;
    //if(growproc(n) < 0)
    //  return -1;
    // =======lab5 section1===========
    return addr;
}
```

#### Lazy allocation
+ 发生页面错误时分配物理页面（kalloc和mappages）

`kernel/trap.c/usertrap()`
```c
  // save user program counter.
  p->trapframe->epc = r_sepc();
  
  // ====lab5 section2======
  if(r_scause() == 13 || r_scause() == 15 ){
	char *mem;
	uint64 va = r_stval();	// page fault va
	va = PGROUNDDOWN(va);
	mem = kalloc();	//pa
    if(mem == 0){
	  //uvmunmap(p->pagetable, va, 1, 0);
	  panic("kalloc fault");
	}
	memset(mem, 0, PGSIZE);
	// map
	if(mappages(p->pagetable, va, PGSIZE, (uint64)mem, PTE_W|PTE_X|PTE_R|PTE_U) != 0){
	  kfree(mem);
	  //uvmunmap(p->pagetable, va, 1, 0);
	  panic("map fault");
	}
  }
  // ====lab5 section2======

  else if(r_scause() == 8){
    // system call
```

`kernel/vm.c/uvmunmap()`
```c
for (a = va; a < va + npages * PGSIZE; a += PGSIZE) {
	if ((pte = walk(pagetable, a, 0)) == 0)
		panic("uvmunmap: walk");
	//if((*pte & PTE_V) == 0)
	//  panic("uvmunmap: not mapped");
	if (PTE_FLAGS(*pte) == PTE_V)
		panic("uvmunmap: not a leaf");
	// ====== lab5 section2 ========
	if ((*pte) & PTE_V) {
		if (do_free) {
			uint64 pa = PTE2PA(*pte);
			kfree((void*)pa);
		}
		*pte = 0;
	}
	// ====== lab5 section2 ========
}
```

#### Lazytests and Usertests
+ 注意细节，注意细节，注意细节
+ 因为惰性分配，需根据最后一级页表的PTE_V核查是否实际分配物理页面

`kernel/sysproc.c`
```c
uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  // ======= lab5 ===========
  struct proc *p = myproc();
  addr = p->sz;
  if(n > 0){
  	p->sz += n;
  }else if(n < 0){
	uint sz;
	sz = p->sz;
	p->sz = uvmdealloc(p->pagetable, sz, sz + n);
  }
  // ======= lab5 ===========
  return addr;
}
```

`kernel/vm.c/uvmcopy()`
```c
for(i = 0; i < sz; i += PGSIZE){
  // ====== lab5 ======
  if((pte = walk(old, i, 0)) == 0)
    continue;
  if((*pte & PTE_V) == 0)
    continue;
  // ====== lab5 ======
  pa = PTE2PA(*pte);
  flags = PTE_FLAGS(*pte);
  if((mem = kalloc()) == 0)
    goto err;
  memmove(mem, (char*)pa, PGSIZE);
  if(mappages(new, i, PGSIZE, (uint64)mem, flags) != 0){
    kfree(mem);
    goto err;
  }
}
```
`kernel/vm.c/uvmunmap()`
```c
for(a = va; a < va + npages*PGSIZE; a += PGSIZE){
	// ====== lab5 ========
	if((pte = walk(pagetable, a, 0)) == 0){
	  continue;
	}
    if((*pte & PTE_V) == 0)
      continue;
	// ====== lab5 ========
    if(PTE_FLAGS(*pte) == PTE_V)
      panic("uvmunmap: not a leaf");
    if(do_free){
      uint64 pa = PTE2PA(*pte);
      kfree((void*)pa);
	}
    *pte = 0;
}
```

`kernel/syscall.c`
```c
int
argaddr(int n, uint64 *ip)
{
  *ip = argraw(n);

  struct proc* p = myproc();
  if(walkaddr(p->pagetable, *ip) == 0){
	if(*ip >= PGROUNDDOWN(p->trapframe->sp) && *ip < p->sz){
	  char *mem;
      mem = kalloc();
      if(mem == 0)
	    return -1;
      memset(mem, 0, PGSIZE);
      if(mappages(p->pagetable, PGROUNDDOWN(*ip), PGSIZE, (uint64)mem, PTE_W|PTE_X|PTE_R|PTE_U) != 0){
	    kfree(mem);
	    return -1;
      }
	}else{
	  return -1;
	}
  }
  return 0;
}
```