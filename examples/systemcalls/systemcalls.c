
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include "systemcalls.h"
//#include <stdlib.h>



//@param int count;
//@param char* $cmd[];
//@return bool;


bool do_system(const char *cmd)
{
	int result;
	result = system(cmd);

	if(WIFEXITED(result))
	{
		if(WEXITSTATUS(result)==0)
		{
    			return true;
    		}
    		else
    		{
    			return false;
    		}
	}
	else
	{
		return false;
	}
}


bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

    command[count] = command[count];
    
    va_end(args);
     
     int execResult = 0;
     pid_t pid_for_exec;
     int wait_result = 0;
     
     pid_for_exec = fork();
     
     
     if (pid_for_exec == 0)
     {
     	execResult = execv(command[0], command);
     	exit(-1);
     	
     }
     else
     {
      	return false;
     }
     
     wait_result = waitpid(pid_for_exec, &execResult, 0);  
     if (wait_result < 0)
     {
     	return false;
     }   
     if(WIFEXITED(execResult) && (WEXITSTATUS(execResult)==0))
     {
    	return true;
     }
     
     else
     {
	return false;
     }
}


bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

    command[count] = command[count];
	
    va_end(args);	
	
    pid_t kidpid;
    int execFileResult;
    int fd = 0;
    int fd2 = 0;
    int pidresult = 0;
    
    fd = open(outputfile, O_WRONLY|O_TRUNC|O_CREAT, 0644);

if (fd < 0) 
{ 
	perror("open"); 
	return false; 
}

kidpid = fork();
if (kidpid < 0) 
{
  	//perror("fork"); 
  	return false;
}
else
{
    fd2 = dup2(fd, 1);
    close(fd);
    if (fd2 < 0) 
    { 
    	perror("dup2"); 
    	exit(-1); 
    }
    else
    {
    	//close(fd);
    	execv(command[0], command);
    	perror("execv");
    	exit(-1);
    	pidresult = waitpid(kidpid,&execFileResult,0); 
    	//close(fd);
    }
    if(pidresult == -1)
    {
    	return false;
    }
    if(WIFEXITED(execFileResult))
	{
		 if(WEXITSTATUS(execFileResult)==0)
		 {
    			return true;
    		 }
    		 else
    		 {
    		 	return false;
		 }
	}
    else
	{
		return false;
	}
}
}
