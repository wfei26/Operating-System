#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <glob.h>

/* maximum length of the command line */
#define MAX_LEN 1024

/* maximum numbers of cmds */
#define MAX_SEP 256

/* max jobs in the shell */
#define MAX_JOBS 100

/* maximum number of pipes */
#define MAX_PIPE 20

/* used for matching internal commands */
char cmd_cd[5] = "cd\0";
char cmd_exit[5] = "exit\0";
char cmd_jobs[5] = "jobs\0";

/* execve's PATH variable */
char *envp[MAX_SEP];
int pathnum = 0;
//char *envp[] = {"PATH=./",0};

/* environment variables */
char *env;
char environ[MAX_LEN];
char environ2[MAX_LEN];
char *phome;
char home[MAX_LEN];
char home2[MAX_LEN];
char *env_arr[MAX_LEN];

/* base directory */
char base_dir[MAX_LEN];
char abs_dir[MAX_LEN];

/* background flag */
int background = 0;

/* store the jobs' pids */
int jobs_pid[MAX_JOBS];
int pid_topptr = 0;

/* search char in string */
int strchar(char *str, char ch)
{
	int i = strlen(str);
	while (i >=0)
	{
		if (str[i]==ch)
			return 1;
		i--;
	}
	return 0;
}

/* signal for catching the child terminated */
void sig_chld(int signo) 
{
	pid_t   pid; 
	int     stat; 

	while((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
       	printf("\n[%d] PID finished\n", pid);    
    }
    return; 
}


/* show the jobs that run in the shell */
void showjobs()
{
	int j;
	/* process's status */
	char status[20];
	/* str2 is the process's name */
	char str1[20],str2[20],str3[20],str4[20];
	char path[60];
	FILE *fin;
	int i;
	fin = fopen("/tmp/pids", "r");
	fscanf(fin, "%d", &pid_topptr);
	for ( i = 0; i < pid_topptr; i++)
		fscanf(fin, "%d", &jobs_pid[i]);
	fclose(fin);
	for (j = 0; j < pid_topptr; j++ )
	{
		/* the status can be found here */
		sprintf(path, "/proc/%d/status", jobs_pid[j]);
		if ( NULL != (fin = fopen(path, "r")) ) 
		{
			fscanf(fin, "%s%s%s%s%s", str1, str2, str3, str4, status);
			for (i = 0; i < strlen(status) - 2; i++)
			{
				status[i] = status[i + 1];
			}
			status[i] = 0;
			/* output the jobs info */
			printf("Process %10s with PID %5d :%10s\n", str2, jobs_pid[j], status);
			//fclose(fin);
		}
	}

}

/* run normal command */
void process_cmds(char *buffer)
{
	int i = 0, j = 0, ri = 0, k = 0, m = 0, n = 0;
	char *p[MAX_SEP];
	char *buf = buffer;
	char *outer_ptr = NULL;
	char *inner_ptr = NULL;
	/* used for execve/execvp as path */
	char cmd[MAX_LEN];
	int flag = 0;

	/* used for job recording */
	int ijob = 0;
	int t_job = 0;
	int jobin = 0;

	/* used for fileIO redirection */
	int fileDescriptor;
	int standardOut;
	int standardIn;
	int fileIO = 0;

	FILE *fin = fopen("/tmp/pids", "w");

	signal(SIGCHLD, sig_chld);

	while( NULL != (p[i] = strtok_r(buf, "|", &outer_ptr)) ) 
	{
		j = i;
		buf = p[i];
		/* get a command */
		while( NULL != (p[i] = strtok_r(buf, " \n", &inner_ptr)) ) 
		{
			//cd_exit_jobs(p[i]);
			i++;
			buf = NULL;
			//printf("%s\n", p[i-1]);
		}
		//printf("sssssss %s %s ssssss\n", p[j], p[j+1]);
		buf = NULL;

		/* handle with 'cd' */
		if ( 0 == strcmp(cmd_cd, p[j]) ) {
			if (p[j+1] == NULL) {
				chdir(home);
				getcwd(base_dir, MAX_LEN);
				return;
			} else {
				if (0!=access(p[j+1], 0)) {
					printf("Error! No such file or direction\n");
					return;
				}
				if (0 == opendir(p[j+1])) {
					printf("Error! %s is not a directory.\n", p[j+1]);
					return ;
				}
				chdir(p[j + 1]);
				getcwd(base_dir, MAX_LEN);
			}
			return;
		}
		/* handle with 'exit' */
		if ( 0 == strcmp(cmd_exit, p[j]) || 0 == strcmp("quit", p[j]))
		{
			printf("bye\n");
			exit(1);
		}
		/* handle with 'jobs' */
		if ( 0 == strcmp(cmd_jobs, p[j]) )
		{
			showjobs();
			return ;
		}
		/* handle with echo */
		if (strcmp(p[j], "echo") == 0) {
			if (strcmp(p[j+1], "$PATH") == 0 ) {
				printf("%s\n", environ);
				return ;
			} else if (strcmp(p[j+1], "$HOME") == 0 ) {
				printf("%s\n", home);
				return ;
			} 
			return;
		}
		/* handle with set */
		if (strcmp(p[j], "set") == 0) {
			if (strncmp(p[j+1], "PATH", 4) == 0 ) {
				sprintf(environ, "%s", p[j+1]);
				setenv("PATH", environ, 1);
				return ;
			} else if (strncmp(p[j+1], "HOME", 4) == 0 ) {
				sprintf(home, "%s", p[j+1]);
				setenv("HOME", home, 1);
				return ;
			} 
			return;
		}
		
		/* handle with > >> < */
		for (ri = 0; ri < i; ri++)
		{
			if (strcmp(p[ri], ">") == 0)
			{
				if (p[ri+1] == NULL){
					printf("Not enough input arguments\n");
					return ;
				}
				fileIO = 1;
				fileDescriptor = open(p[ri+1], O_CREAT | O_TRUNC | O_WRONLY, 0600); 
				// We replace de standard output with the appropriate file
				standardOut = dup(STDOUT_FILENO); 	// first we make a copy of stdout
													// because we'll want it back
				dup2(fileDescriptor, STDOUT_FILENO); 
				close(fileDescriptor);
				//printf("%s$\n", base_dir);
				p[ri] = NULL;
				//printf("\nsssss\n");
				
			} else if (strcmp(p[ri], ">>") == 0)
			{
				if (p[ri+1] == NULL){
					printf("Not enough input arguments\n");
					return ;
				}
				if (access(p[ri+1], 0)!=0) {
					printf("ERROR: no such file to append to (%s)", p[ri+1]);
					return ;
				}
				fileIO = 2;
				fileDescriptor = open(p[ri+1], O_CREAT | O_APPEND | O_WRONLY, 0600); 
				// We replace de standard output with the appropriate file
				standardOut = dup(STDOUT_FILENO); 	// first we make a copy of stdout
													// because we'll want it back
				dup2(fileDescriptor, STDOUT_FILENO); 
				close(fileDescriptor);
				p[ri] = NULL;
			} else if (strcmp(p[ri], "<")==0)
			{
				if (p[ri+1] == NULL){
					printf("Not enough input arguments\n");
					return ;
				}
				if (access(p[ri+1], 0)!=0) {
					printf("ERROR: no such file to input (%s)", p[ri+1]);
					return ;
				}
				fileIO = 3;
				// We open file for read only (it's STDIN)
				fileDescriptor = open(p[ri+1], O_RDONLY, 0600);  
				// We replace de standard input with the appropriate file
				standardIn = dup(STDIN_FILENO); // first, make a copy
												// because we'll want it back
				dup2(fileDescriptor, STDIN_FILENO);
				close(fileDescriptor);
				p[ri] = NULL;
				// Same as before for the output file
				//fileDescriptor = open(outputFile, O_CREAT | O_TRUNC | O_WRONLY, 0600);
				//dup2(fileDescriptor, STDOUT_FILENO);
				//close(fileDescriptor);	
			}
		}
		

		/* handle other single command */

		pid_t pid = fork();/* generate a child process */
		if ( pid < 0 )
			printf("Error! Create child process failed!\n"); 
		else if ( pid == 0 )  /* child process */
		{
			glob_t gl;
			for (k = 0; k < pathnum; k++)
			{
				sprintf(cmd, "%s/%s", envp[k], p[j]);
				if (access(cmd, 0)==0)	{
					flag = 1;
					break;
				}
			}
			if (flag == 0)
			{
				printf("ERROR: command '%s' not found\n", p[j]);
				exit(1);
			}
			
			if (fileIO==0) {
				for (k = 0; k < i; k++)
				{
					if (1==strchar(p[k], '*')) {					
						gl.gl_offs = 0;
						printf("k%s\n", p[k]);
						glob(p[k], GLOB_TILDE, 0, &gl);
						n = k;
        				for (k = n + 1; k <= i; k++)
						{
							p[k+gl.gl_pathc-1] = p[k];
						}
						k = n;
						for (m = 0; m < gl.gl_pathc; m++)
        				{
        					p[k+m] = gl.gl_pathv[m];
        				}
						break;
					}
				}
				//for (k = 0; k < 20; k++)
				//	printf("p[%d]=%s\n", k,p[k]);
			}
			
			//printf("child:%d\n", getpid());
			execve(cmd, &p[j], envp);/* run the child process */
			globfree(&gl);
			exit(1);
		}
		else if ( pid > 0 ) /* father process */
		{
			//printf("father:%d\n", getpid());

			/* record the father process */
			t_job = getpid();
			for (ijob = 0; ijob < pid_topptr; ijob++)
			{
				if ( jobs_pid[ijob] == t_job)
				{
					jobin = 1;
				}
			}
			if ( 0 == jobin )
			{
				jobs_pid[pid_topptr] = t_job;
					pid_topptr++;
					pid_topptr %= MAX_JOBS;
			}

			/* record the child process */
			jobin = 0;
			t_job = pid;
			for (ijob = 0; ijob < pid_topptr; ijob++)
			{
				if ( jobs_pid[ijob] == t_job)
				{
					jobin = 1;
				}
			}
			if ( 0 == jobin )
			{
				jobs_pid[pid_topptr] = t_job;
					pid_topptr++;
					pid_topptr %= MAX_JOBS;
			}

			if ( background == 0)
			{
				/* wait until the child process ends */
				waitpid(pid, NULL, 0);
				/* recover to stdin/stdout */
				if (fileIO==1) {
					dup2(standardOut, STDOUT_FILENO);
				} else if (fileIO==2) {
					dup2(standardOut, STDOUT_FILENO);
				} else if (fileIO==3) {
					dup2(standardIn, STDIN_FILENO);
				}
				for (ijob = 0; ijob < pid_topptr; ijob++)
				{	
					if ( jobs_pid[ijob] == pid)
					{
						jobs_pid[pid_topptr] = 0;
					}
				}
			}
			else
			{
				/* child process runs in background */
				printf("[%d] running in background\n", pid);
				/* recover to stdin/stdout */
				usleep(10000);
				if (fileIO==1) {
					dup2(standardOut, STDOUT_FILENO);
				} else if (fileIO==2) {
					dup2(standardOut, STDOUT_FILENO);
				} else if (fileIO==3) {
					dup2(standardIn, STDIN_FILENO);
				}
				for (ijob = 0; ijob < pid_topptr; ijob++)
				{	
					if ( jobs_pid[ijob] == pid)
					{
						jobs_pid[pid_topptr] = 0;
					}
				}
			}

			
		}

	}

	/* record pids and restore them */
	fprintf(fin, "%d\n", pid_topptr);
	for ( i = 0; i < pid_topptr; i++)
		fprintf(fin, "%d\n", jobs_pid[i]);
	fclose(fin);

}


/* runs commands with pipelines */
void process_cmdsp(char *buffer, int npipe)
{
	int i = 0, j = 0, ri = 0, k = 0, m = 0, n = 0;;
	char *p[MAX_SEP];
	char *buf = buffer;
	char *outer_ptr = NULL;
	char *inner_ptr = NULL;
	char cmd[MAX_LEN];
	int flag = 0;
	
	/* used for pipelines */
	int ipipe = 0;
	int icmd = 0;
	pid_t pid;
	int pipe1[2];
	int pipe2[2];

	/* used to record jobs */
	int t_job = 0;
	int ijob = 0;
	int jobin = 0;

	/* used for fileIO redirection */
	int fileDescriptor;
	int standardOut;
	int standardIn;
	int fileIO = 0;

	FILE *fin = fopen("/tmp/pids", "w");
	/*int stdin_cp;
	int stdout_cp;
	dup2(1, stdout_cp);
	dup2(0, stdin_cp);
	*/
	while( NULL != (p[i] = strtok_r(buf, "|", &outer_ptr)) ) /* divide the commands */
	{
		j = i;
		buf = p[i];
		while( NULL != (p[i] = strtok_r(buf, " \n", &inner_ptr)) ) /* get each command */
		{
			//cd_exit_jobs(p[i]);
			i++;
			buf = NULL;
			//printf("%s\n", p[i-1]);
		}
		//printf("sssssss %s %s ssssss\n", p[j], p[j+1]);
		buf = NULL;

		/* handle with 'cd' */
		if ( 0 == strcmp(cmd_cd, p[j]) ) {
			if (p[j+1] == NULL) {
				chdir(home);
				getcwd(base_dir, MAX_LEN);
				return;
			} else {
				if (0!=access(p[j+1], 0)) {
					printf("Error! No such file or direction\n");
					return;
				}
				if (0 == opendir(p[j+1])) {
					printf("Error! %s is not a directory.\n", p[j+1]);
					return ;
				}
				chdir(p[j + 1]);
				getcwd(base_dir, MAX_LEN);
			}
			return;
		}
		/* handle with 'exit' */
		if ( 0 == strcmp(cmd_exit, p[j]) || 0 == strcmp("quit", p[j]))
		{
			printf("byr\n");
			exit(1);
		}
		/* handle with 'jobs' */
		if ( 0 == strcmp(cmd_jobs, p[j]) )
		{
			showjobs();
			return;
		}
		/* handle with echo */
		if (strcmp(p[j], "echo") == 0) {
			if (strcmp(p[j+1], "$PATH") == 0 ) {
				printf("%s\n", environ);
				return ;
			} else if (strcmp(p[j+1], "$HOME") == 0 ) {
				printf("%s\n", home);
				return ;
			} 
			return;
		}
		/* handle with set */
		if (strcmp(p[j], "set") == 0) {
			if (strncmp(p[j+1], "PATH", 4) == 0 ) {
				sprintf(environ, "%s", p[j+1]);
				setenv("PATH", environ, 1);
				return ;
			} else if (strncmp(p[j+1], "HOME", 4) == 0 ) {
				sprintf(home, "%s", p[j+1]);
				setenv("HOME", home, 1);
				return ;
			} 
			return;
		}

		/* handle with > >> < */
		for (ri = 0; ri < i; ri++)
		{
			if (strcmp(p[ri], ">") == 0)
			{
				if (p[ri+1] == NULL){
					printf("Not enough input arguments\n");
					return ;
				}
				fileIO = 1;
				fileDescriptor = open(p[ri+1], O_CREAT | O_TRUNC | O_WRONLY, 0600); 
				// We replace de standard output with the appropriate file
				standardOut = dup(STDOUT_FILENO); 	// first we make a copy of stdout
													// because we'll want it back
				dup2(fileDescriptor, STDOUT_FILENO); 
				close(fileDescriptor);
				//printf("%s$\n", base_dir);
				p[ri] = NULL;
				//printf("\nsssss\n");
				
			} else if (strcmp(p[ri], ">>") == 0)
			{
				if (p[ri+1] == NULL){
					printf("Not enough input arguments\n");
					return ;
				}
				if (access(p[ri+1], 0)!=0) {
					printf("ERROR: no such file to append to (%s)", p[ri+1]);
					return ;
				}
				fileIO = 2;
				fileDescriptor = open(p[ri+1], O_CREAT | O_APPEND | O_WRONLY, 0600); 
				// We replace de standard output with the appropriate file
				standardOut = dup(STDOUT_FILENO); 	// first we make a copy of stdout
													// because we'll want it back
				dup2(fileDescriptor, STDOUT_FILENO); 
				close(fileDescriptor);
				p[ri] = NULL;
			} else if (strcmp(p[ri], "<")==0)
			{
				if (p[ri+1] == NULL){
					printf("Not enough input arguments\n");
					return ;
				}
				if (access(p[ri+1], 0)!=0) {
					printf("ERROR: no such file to input (%s)", p[ri+1]);
					return ;
				}
				fileIO = 3;
				// We open file for read only (it's STDIN)
				fileDescriptor = open(p[ri+1], O_RDONLY, 0600);  
				// We replace de standard input with the appropriate file
				standardIn = dup(STDIN_FILENO); // first, make a copy
												// because we'll want it back
				dup2(fileDescriptor, STDIN_FILENO);
				close(fileDescriptor);
				p[ri] = NULL;
				// Same as before for the output file
				//fileDescriptor = open(outputFile, O_CREAT | O_TRUNC | O_WRONLY, 0600);
				//dup2(fileDescriptor, STDOUT_FILENO);
				//close(fileDescriptor);	
			}
		}

		/* create pipelines */
		if ( icmd % 2 != 0)
		{
			pipe(pipe1);/* for odd one */
		} 
		else
		{
			pipe(pipe2);/* for even one */
		}
		
		pid = fork();/* create child process */
		if ( pid == -1 )/* create child process failed */
		{
			if ( icmd != npipe )
			{
				if ( icmd % 2 != 0 )
					close(pipe1[1]);
				else
					close(pipe2[1]);
			}
			printf("Error! Create child process failed!\n"); 
			exit(0);
		}
		else if ( pid == 0 )  /* child process */
		{
			/* If we are in the first command */
			if ( icmd == 0)
				dup2(pipe2[1], STDOUT_FILENO);
			/* If we are in the last command, depending on whether it
			   is placed in an odd or even position, we will replace
			   the standard input for one pipe or another. The standard
			   output will be untouched because we want to see the 
			   output in the terminal
			 */
			else if ( icmd == npipe )
			{
				if ( npipe % 2 == 0 )
					dup2(pipe1[0], STDIN_FILENO);
				else
					dup2(pipe2[0], STDIN_FILENO);
			}
			/* If we are in a command that is in the middle, we will
			   have to use two pipes, one for input and another for
			   output. The position is also important in order to choose
			   which file descriptor corresponds to each input/output
			 */
			else
			{
				if ( icmd % 2 != 0)
				{
					dup2(pipe2[0], STDIN_FILENO);
					dup2(pipe1[1], STDOUT_FILENO);
				} 
				else
				{
					dup2(pipe1[0], STDIN_FILENO);
					dup2(pipe2[1], STDOUT_FILENO);
				}
			}
			
			glob_t gl;

			for (k = 0; k < pathnum; k++)
			{
				sprintf(cmd, "%s/%s", envp[k], p[j]);
				if (access(cmd, 0)==0)	{
					flag = 1;
					break;
				}
			}
			if (flag == 0)
			{
				printf("ERROR: command '%s' not found\n", p[j]);
				exit(1);
			}
			if (fileIO==0) {
				for (k = 0; k < i; k++)
				{
					if (1==strchar(p[k], '*')) {					
						gl.gl_offs = 0;
						printf("k%s\n", p[k]);
						glob(p[k], GLOB_TILDE, 0, &gl);
						n = k;
        				for (k = n + 1; k <= i; k++)
						{
							p[k+gl.gl_pathc-1] = p[k];
						}
						k = n;
						for (m = 0; m < gl.gl_pathc; m++)
        				{
        					p[k+m] = gl.gl_pathv[m];
        				}
						break;
					}
				}
				//for (k = 0; k < 20; k++)
				//	printf("p[%d]=%s\n", k,p[k]);
			}

			//printf("child:%d\n", getpid());
			execve(cmd, &p[j], envp);/* run the child process */
			
			exit(1);
		}
		else if ( pid > 0 )/* father process */
		{	
			/* it is similar to the child process, but here
			   we must close the descriptors
			 */
			if ( icmd == 0)
			{
				close(pipe2[1]);
			}
			else if ( icmd == npipe )
			{
				if ( npipe % 2 == 0 )
					close(pipe1[0]);
				else
					close(pipe2[0]);
			}
			else
			{
				if ( icmd % 2 != 0 )
				{
					close(pipe2[0]);
					close(pipe1[1]);
				}
				else
				{
					close(pipe1[0]);
					close(pipe2[1]);
				}
			}	
			//printf("father:%d %d\n", getpid(), icmd);
			
			
			
			/* runs foreground */
			if ( 0 == background )
			{
				waitpid(pid, NULL, 0);
				/* recover to stdin/stdout */
				if (fileIO==1) {
					dup2(standardOut, STDOUT_FILENO);
				} else if (fileIO==2) {
					dup2(standardOut, STDOUT_FILENO);
				} else if (fileIO==3) {
					dup2(standardIn, STDIN_FILENO);
				}
				for (ijob = 0; ijob < pid_topptr; ijob++)
				{	
					if ( jobs_pid[ijob] == pid)
					{
						jobs_pid[pid_topptr] = 0;
					}
				}
				for (ijob = 0; ijob < pid_topptr; ijob++)
				{		
					if ( jobs_pid[ijob] == pid)
					{
						jobs_pid[pid_topptr] = 0;
					}
				}
			}
			/* runs in background */
			else
			{
				/* record the father's pid */
				t_job = getpid();
				for (ijob = 0; ijob < pid_topptr; ijob++)
				{
					if ( jobs_pid[ijob] == t_job)
					{
						jobin = 1;
					}
				}
				if ( 0 == jobin )
				{
					jobs_pid[pid_topptr] = t_job;
						pid_topptr++;
						pid_topptr %= MAX_JOBS;
				}

				/* record the child's pid, beacuse they have different stacks */
				jobin = 0;
				t_job = pid;
				for (ijob = 0; ijob < pid_topptr; ijob++)
				{
					if ( jobs_pid[ijob] == t_job)
					{
						jobin = 1;
					}
				}
				if ( 0 == jobin )
				{
					jobs_pid[pid_topptr] = t_job;
					pid_topptr++;
					pid_topptr %= MAX_JOBS;
				}
				printf("[%d] running in background\n", pid);
				/* recover to stdin/stdout */
				if (fileIO==1) {
					dup2(standardOut, STDOUT_FILENO);
				} else if (fileIO==2) {
					dup2(standardOut, STDOUT_FILENO);
				} else if (fileIO==3) {
					dup2(standardIn, STDIN_FILENO);
				}
				for (ijob = 0; ijob < pid_topptr; ijob++)
				{	
					if ( jobs_pid[ijob] == pid)
					{
						jobs_pid[pid_topptr] = 0;
					}
				}
			}				
		}
		icmd++;
	}
		
	//dup2(stdout_cp, 1);
	//dup2(stdin_cp, 0);
	//close(fileDescriptor);

	/* record pids and restore them */
	fprintf(fin, "%d\n", pid_topptr);
	for ( i = 0; i < pid_topptr; i++)
		fprintf(fin, "%d\n", jobs_pid[i]);
	fclose(fin);

	i = strlen(abs_dir);
	while (abs_dir[i--] != '/');
	sprintf(p[0], "%s", abs_dir); 
	p[1] = NULL;
	for (k = 0; k < pathnum; k++)
	{
		memset(cmd, 0, MAX_LEN);
		sprintf(cmd, "%s/%s", envp[k], p[0]);
		if (access(cmd, 0)==0)	{
			flag = 1;
			break;
		}
	}
	sprintf(cmd, "%s", abs_dir);

	sprintf(home2,"HOME=%s", home);
	sprintf(environ2,"PATH=%s", environ);
	env_arr[0] = environ2;
	env_arr[1] = home2;
	env_arr[2] = NULL;
	execve(cmd, &p[0], env_arr);	
}


int main()
{
	/* store the commands */
	char buf[MAX_LEN];

	/* used to count the numbers of pipes */
	int i = 0, npipe = 0;

	/* pointer for getting the path env */
	char *envptr = NULL;

	/* get the directory of the program */
	readlink("/proc/self/exe", base_dir, MAX_LEN);
	i = strlen(base_dir);
	sprintf(abs_dir, "%s", base_dir);
	while (base_dir[i] != '/')
		base_dir[i--] = 0;
	/* show a prompt */
	printf("%s$", base_dir);
	
	/* get the environment variables */
	env = getenv("PATH");
	sprintf(environ, "%s", env);
	phome = getenv("HOME");
	sprintf(home, "%s", phome);
	
	//printf("%s\n", env);
	pathnum = 0;
	while( NULL != (envp[pathnum] = strtok_r(env, ":", &envptr)) ) 
	{
		pathnum++;
		env = NULL;
		//printf("%s\n", envp[i-1]);
	}
	envp[pathnum] = NULL;

	while (1)
	{
		background = 0;
		/* get a line as the command */
		if (fgets(buf, MAX_LEN, stdin) == NULL) {
			printf("\n");
			return 0;
		}
		buf[strlen(buf)-1] = 0;
		//printf("%s\n", buf);
		if (strcmp(buf, "\n") == 0)
		{
			printf("%s$", base_dir);
			continue;
		}
		for (i = 0; i < strlen(buf); i++)
		{
			/* count the numbers of pipes */
			if ( '|' == buf[i] )
			{
				npipe++;
			}
			/* runs in background when end eith '&'  */
			if ( '&' == buf[i])
			{
				background = 1;
				buf[i] = 0;
			}
		}

		if ( !npipe )/* runs normal command */
			process_cmds(buf);
		else/* runs command with pipes */
		{			
			process_cmdsp(buf, npipe);			
		}
		printf("%s$", base_dir);
	}

	return 0;
}
