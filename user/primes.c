#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
 
 
void sieve(int p) {
  int n;
  while (read(0, &n, sizeof(int))) {
    if (n % p != 0) {
      write(1, &n, sizeof(int));
    }
  }
}
 
void redirect(int k, int *p) {
  close(k);
  dup(p[k]);
  close(p[0]);
  close(p[1]);
}
 
void sink() {
  int p[2];
  int prime;
  if (read(0, &prime, sizeof(int))) {
    printf("prime %d\n", prime);
    pipe(p);
    if (fork()) {
      redirect(0, p);
      sink();
    } else {
      redirect(1, p);
      sieve(prime);
    }
  }
}
 
int main( ) {
	int p[2];
	pipe(p);
	if (fork()>0) { 
    	redirect(0, p);
    	sink();
  	} 
	else {
    	redirect(1, p);
		for (int i = 2; i < 36; i++) {
    	write(1, &i, sizeof(int));
     }

  }
  exit(0);
}
