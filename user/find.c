#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"


void
find(char *path,char *filename)
{
	char buf[512];
	char *p;
	int fd;
	struct dirent de;
  	struct stat st;

	if((fd = open(path, 0)) < 0){
    	fprintf(2, "find: cannot open %s\n", path);
    	return;
  	}
	//fstat:get file info
  	if(fstat(fd, &st) < 0){
    	fprintf(2, "find: cannot stat %s\n", path);
    	close(fd);
    	return;
  	}

	switch(st.type){
	//if file
	case T_FILE:
		//find the file name
		for(p=path+strlen(path); p >= path && *p != '/'; p--)
    		;
  		p++;
		if(strcmp(p,filename)==0)
			printf("%s\n",path);
		break;

	//if dir
	case T_DIR:
		//printf("dir: %s\n",path);
		if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
     		printf("find: path too long\n");
      		break;
    	}
		strcpy(buf, path);

   	 	p = buf+strlen(buf);
    	*p++ = '/';
		while(read(fd, &de, sizeof(de)) == sizeof(de)){
			if(de.inum == 0)
        		continue;
      		memmove(p, de.name, DIRSIZ);
      		p[DIRSIZ] = 0;

			//don't enter dir "." and ".."
          	if(strcmp(p,".")==0||strcmp(p,"..")==0)
              	continue;

			if(stat(buf, &st) < 0){
        	printf("find: cannot stat %s\n", buf);
        		continue;
      		}
			find(buf,filename);
		}//while
		break;
	}//switch
	close(fd);


}


int
main(int argc, char *argv[])
{
	if(argc!=3){
		fprintf(2,"Usage:find dir file...\n");
	}

	find(argv[1],argv[2]);
	
	exit(0);
}
