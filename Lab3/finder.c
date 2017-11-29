#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <errno.h>
#include <sys/wait.h>

#define BSIZE 256

#define BASH_EXEC  "/bin/bash"
#define FIND_EXEC  "/bin/find"
#define XARGS_EXEC "/usr/bin/xargs"
#define GREP_EXEC  "/bin/grep"
#define SORT_EXEC  "/bin/sort"
#define HEAD_EXEC  "/usr/bin/head"

int main(int argc, char *argv[])
{
  int status;
  pid_t pid_1, pid_2, pid_3, pid_4;

  int pip1[2];
  int pip2[2];
  int pip3[2];

  pipe(pip1);
  pipe(pip2);
  pipe(pip3);

  if (argc != 4) {
    printf("usage: finder DIR STR NUM_FILES\n");
    exit(0);
  }

  pid_1 = fork();
  if (pid_1 == 0) 
	{
    	/* First Child */

	char buf[BSIZE];
	bzero(buf, BSIZE);
	sprintf(buf, "%s %s -name \'*\'.[ch]", FIND_EXEC, argv[1]);

	close(pip1[0]);
	close(pip2[0]);
	close(pip2[1]);
	close(pip3[0]);
	close(pip3[1]);

	dup2(pip1[1], 1);

	if ( (execl(BASH_EXEC, BASH_EXEC, "-c", buf, (char *) 0)) < 0) 
	{
    		fprintf(stderr, "Process encountered an error. ERROR%d", errno);
		return EXIT_FAILURE;
	}
        
	close(pip1[1]);
    	exit(0);
  	}

  pid_2 = fork();
  if (pid_2 == 0) 
	{
    	/* Second Child */

	char buf[BSIZE];
	bzero(buf, BSIZE);
	sprintf(buf, "%s %s -c %s", XARGS_EXEC, GREP_EXEC, argv[2]);

	close(pip1[1]);
	close(pip2[0]);
	close(pip3[0]);
	close(pip3[1]);
	
	dup2(pip1[0],0);
  	dup2(pip2[1],1);

	if ( (execl(BASH_EXEC, BASH_EXEC, "-c", buf, (char *) 0)) < 0) 
	{
		fprintf(stderr, "\nError execing find. ERROR#%d\n", errno);
		return EXIT_FAILURE;
	}
		  
	close(pip1[0]);
	close(pip2[1]);
	exit(0);
  	}

  pid_3 = fork();
  if (pid_3 == 0) 
	{
    	/* Third Child */

	char buf[BSIZE];
	bzero(buf, BSIZE);  

	close(pip1[1]);
    	close(pip1[0]);
	close(pip2[1]);
	close(pip3[0]);
    
    	dup2(pip2[0],0);
    	dup2(pip3[1],1);
    	
	execl(SORT_EXEC, SORT_EXEC, "-t", ":", "+1.0", "-2.0", "--numeric", "--reverse", NULL);

	close(pip2[0]);
        close(pip3[1]);
	exit(0);
  	}

  pid_4 = fork();
  if (pid_4 == 0) 
	{
    	/* Fourth Child */
    	char buf[BSIZE];
	sprintf(buf, "--lines=%s", argv[3]);

    	close(pip1[1]);
    	close(pip1[0]);
	close(pip2[1]);
	close(pip2[0]);
	close(pip3[1]);
	
	dup2(pip3[0],0);

	execl(HEAD_EXEC, HEAD_EXEC, buf, ((char *) 0));
	
	close(pip3[0]);
  	exit(0);
  	}
  
    	close(pip1[0]);
    	close(pip1[1]);
	close(pip2[0]);
  	close(pip2[1]);
	close(pip3[0]);
  	close(pip3[1]);

  if ((waitpid(pid_1, &status, 0)) == -1) {
    fprintf(stderr, "Process 1 encountered an error. ERROR%d", errno);
    return EXIT_FAILURE;
  }
  if ((waitpid(pid_2, &status, 0)) == -1) {
    fprintf(stderr, "Process 2 encountered an error. ERROR%d", errno);
    return EXIT_FAILURE;
  }
  if ((waitpid(pid_3, &status, 0)) == -1) {
    fprintf(stderr, "Process 3 encountered an error. ERROR%d", errno);
    return EXIT_FAILURE;
  }
  if ((waitpid(pid_4, &status, 0)) == -1) {
    fprintf(stderr, "Process 4 encountered an error. ERROR%d", errno);
    return EXIT_FAILURE;
  }

  return 0;
}
