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
