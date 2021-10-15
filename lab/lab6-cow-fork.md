## Copy-on-Write Fork

+ 注意细节，注意细节，注意细节

`kernel/riscv.h`
```c
#define PTE_COW (1L << 8)
```

`kernel/memlayout.h`
```c
#define REFER_INDEX(pa) (((uint64)pa-KERNBASE)/PGSIZE)
```

`kernel/vm.c`
```c
int refer[(PHYSTOP-KERNBASE)/4096];

int 
uvmcopy(pagetable_t old, pagetable_t new, uint64 sz)
{
	pte_t *pte;
	uint64 pa, i;    
	uint flags;    

	for(i = 0; i < sz; i += PGSIZE){         
		if((pte = walk(old, i, 0)) == 0)    
			panic("uvmcopy: pte should exist"); 
		if((*pte & PTE_V) == 0)
			panic("uvmcopy: page not present"); 
		pa = PTE2PA(*pte);
		*pte = (*pte & ~PTE_W) | PTE_COW;   // new flags
		flags = PTE_FLAGS(*pte);
		if(mappages(new, i, PGSIZE, pa, flags) != 0){
			return -1;
		}
		refer[REFER_INDEX(pa)]++;       // reference num
	}
	return 0;
}


// copyout
int
copyout(pagetable_t pagetable, uint64 dstva, char *src, uint64 len)
{
	uint64 n, va0, pa0;
	pte_t *pte;

	while(len > 0){
		va0 = PGROUNDDOWN(dstva);
		pa0 = walkaddr(pagetable, va0);
		if (pa0 == 0) {
			return -1;
		}
		pte = walk(pagetable, va0, 0);
		if (*pte & PTE_COW)
		{
			// allocate a new page
			uint64 ka = (uint64) kalloc(); // newly allocated physical address

			if (ka == 0){
				struct proc *p = myproc();
				p->killed = 1; // there's no free memory
			} else {
				memmove((char*)ka, (char*)pa0, PGSIZE); // copy the old page to the new page
					 uint flags = PTE_FLAGS(*pte);
					 uvmunmap(pagetable, va0, 1, 1);
				 *pte = PA2PTE(ka) | flags | PTE_W;
				 *pte &= ~PTE_COW;
				 pa0 = ka;
			}
		}
		n = PGSIZE - (dstva - va0);
		if(n > len)
			n = len;
		memmove((void *)(pa0 + (dstva - va0)), src, n);

		len -= n;
		src += n;
		dstva = va0 + PGSIZE;
	}
	return 0;
}
```

`kernel/trap.c`
```c
if( r_scause() == 15){ 
		uint64 old_pa;
		uint64 new_pa;
		pte_t *pte;
		uint64 va = r_stval();	// page fault va
		va = PGROUNDDOWN(va);
		old_pa = walkaddr(p->pagetable, va);  // cow page
		if(old_pa == 0){
			panic("lack cow page");
		}
		new_pa = (uint64)kalloc();	//new pa
		if(new_pa == 0){
			p->killed = 1;
			goto kill;
		}
		// copy mem page content
		memmove((void*)new_pa, (void*)old_pa ,PGSIZE);
		// before map, let PTE_V=0
		pte = walk(p->pagetable, va, 0);
		*pte = *pte & ~PTE_V;
		mappages(p->pagetable, va, PGSIZE, new_pa ,PTE_W|PTE_R|PTE_X|PTE_U);
		// reference num
		kfree((void*)old_pa);
    }
```