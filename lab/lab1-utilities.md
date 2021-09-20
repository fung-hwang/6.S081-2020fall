## Lab1: Xv6 and Unix utilities

#### Boot xv6
+ 跑通就算胜利，不废话

#### sleep
+ 调用sleep系统调用即可

``` C
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{

  if(argc != 2){
    fprintf(2, "Usage: sleep time...\n");
    exit(1);
  }

  int n=atoi(argv[1]);

  sleep(n);

  exit(0);
}
```

#### pingpong
+ pipe的简单应用
+ 一个要点是父进程中的wait(0)

``` C
 #include "kernel/types.h"
 #include "kernel/stat.h"
 #include "user/user.h"

 int
 main(int argc, char *argv[])
 {
      int p[2];
      char buf[2];
      pipe(p);
      //child
      if(fork()==0)
      {
          if(read(p[0],buf,1)<=0)
          {
              fprintf(2,"child read error...\n");
              exit(1);
          }
          //printf("%c",buf[0]);
          close(p[0]);
          printf("%d: received ping\n",getpid());
          if(write(p[1],buf,1)<=0)
          {
               fprintf(2,"child write error...\n");
               exit(1);
          }
          close(p[1]);
      }
      //parent
      else
        {
            buf[0]='x';
            if(write(p[1],buf,1)<=0)
            {
                fprintf(2,"parent write error...\n");
                exit(1);
            }
            close(p[1]);
            wait(0);    //without wait, parent proc can finish write-read by itself
            if(read(p[0],buf,1)<=0)
            {
                fprintf(2,"parent read error...\n");
                exit(1);
            }
            printf("%d: received pong\n",getpid());
            close(p[0]);
        }

        exit(0);
    }
```

#### primes
+ 解析在Noiton中

``` C
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void redirect(int k, int *p) {
	close(k);
	dup(p[k]);
	close(p[0]);
	close(p[1]);
}

//从左边pipe读，过滤，再写入右边pipe
void sieve(int p) {
	int n;
	while (read(0, &n, sizeof(int))) {
		if (n % p != 0) {
			write(1, &n, sizeof(int));
		}
	}
}

//左边管道读取的第一个数是本轮的筛（素数）
//子进程用得到的筛prime处理本轮的筛选
//父进程开启下一轮筛选
void sink() {
	int p[2];
	int prime;
	if (read(0, &prime, sizeof(int))) {
		printf("prime %d\n", prime);
		pipe(p);
		if (fork()) {
			redirect(0, p);
			sink();
		}
		else {
			redirect(1, p);
			sieve(prime);
		}
	}
}

int
main(int argc, char *argv[]) {
	int p[2];
	pipe(p);
	if (fork() > 0) {
		redirect(0, p);
		sink();
	}
	else { //读入2-35
		redirect(1, p);
		for (int i = 2; i < 36; i++) {
			write(1, &i, sizeof(int));
		}
	}

	exit(0);
}
```

#### find
+ 对ls.c的拙劣模仿
+ 递归查看目录，递归边界是文件
+ 不得进入目录"."和".."

``` C
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void
find(char *path, char *filename)
{
	char buf[512];
	char *p;
	int fd;
	struct dirent de;
	struct stat st;

	if ((fd = open(path, 0)) < 0) {
		fprintf(2, "find: cannot open %s\n", path);
		return;
	}
	//fstat:get file info
	if (fstat(fd, &st) < 0) {
		fprintf(2, "find: cannot stat %s\n", path);
		close(fd);
		return;
	}

	switch (st.type) {
		//if file
	case T_FILE:
		//find the file name
		for (p = path + strlen(path); p >= path && *p != '/'; p--)
			;
		p++;
		if (strcmp(p, filename) == 0)
			printf("%s\n", path);
		break;

		//if dir
	case T_DIR:
		//printf("dir: %s\n",path);
		if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
			printf("find: path too long\n");
			break;
		}
		strcpy(buf, path);

		p = buf + strlen(buf);
		*p++ = '/';
		while (read(fd, &de, sizeof(de)) == sizeof(de)) {
			if (de.inum == 0)
				continue;
			memmove(p, de.name, DIRSIZ);
			p[DIRSIZ] = 0;

			//don't enter dir "." and ".."
			if (strcmp(p, ".") == 0 || strcmp(p, "..") == 0)
				continue;

			if (stat(buf, &st) < 0) {
				printf("find: cannot stat %s\n", buf);
				continue;
			}
			find(buf, filename);
		}//while
		break;
	}//switch

	close(fd);
}


int
main(int argc, char *argv[])
{
	if (argc != 3) {
		fprintf(2, "Usage:find dir file...\n");
	}

	find(argv[1], argv[2]);

	exit(0);
}

```

#### xargs
+ 逻辑非常简单：从stdin中收集每次exec的参数，在每个子进程中exec即可
+ C的字符串写的太差了，纯纯的摆烂了，有时间重写 **（务必回顾C字符串）**

``` C
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

#define N 32

int read_argument(int argc,char (*new_argv)[N])
{
	char *p=new_argv[argc];
	while(read(0,p,sizeof(char))>0){

		if(*p=='\n'){
			*p=0;
			return ++argc;
		}
		if(*p==' '){
			*p=0;
			p=new_argv[++argc];
        }
		else{
			p++;
		}
	}

	//if fail to read
	return -1;
}

int
main(int argc, char *argv[])
{

	if(argc<2)
		fprintf(2,"Usage:xargs [cmd]...");

	char new_argv[MAXARG][N];
	int num=0;
	for(;num<argc-1;++num){
		strcpy(new_argv[num],argv[num+1]);
	}
	int cmd_num;
	while((cmd_num=read_argument(num,new_argv))>0){
    //另做一个char**以满足exec的参数需要
		char *new_argv2[MAXARG];
		for(int i=0;i<cmd_num;i++){
			new_argv2[i]=new_argv[i];
		}


		if(fork()==0){
			exec(new_argv2[0],new_argv2);
			fprintf(2,"exec failed...");
			exit(1);
		}
		wait(0);
	}


	exit(0);
}
```
