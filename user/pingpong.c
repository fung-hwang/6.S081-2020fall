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
		wait(0);	//without wait, parent proc can finish write-read by itself
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
