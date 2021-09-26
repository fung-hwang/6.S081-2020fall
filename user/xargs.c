#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

#define N 32

int read_argument(int argc,char (*new_argv)[N])
{	
	char *p=new_argv[argc];
	//read eof can cause failuse???!!!
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
