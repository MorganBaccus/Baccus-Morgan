#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#define MAX 128


char gdir[MAX];
char *dir[64];
int  ndir;

char gpath[MAX];
char *name[64];
int  ntoken;

char *tname[64]; //tail
int ttoken;

int handle_io_redirection(char *cmdnames[], int n_cmdnames);
int find_cmd_and_execute(char *env[],char *name[64]);

int main(int argc, char *argv[], char *env[])
{
  int  i, r;
  int  pid, pid2, status, status2;
  char *s, cmd[64], line[MAX];
  char home[64];
  printf("************* Welcome to morgan's sh **************\n");
  i = 0;
  while (env[i]){

    // Look for home directory
    if (strncmp(env[i], "HOME=", 5)==0){
      strcpy(home, &env[i][5]);
    }
    // Look for PATH=
    if (strncmp(env[i], "PATH=", 5)==0){
      printf("show PATH: %s\n", env[i]);

      printf("decompose PATH into dir strings in gdir[ ]\n");
      strcpy(gdir, &env[i][5]); // gdir will not include PATH=


      dir[0] = gdir; 
      // Directories seperated by colons, replace with newline so that pointer to each can then iterate until null
      ndir = 1;
      for(int s = 0; gdir[s] != '\0'; s++){
	if(gdir[s] == ':'){
	  gdir[s] = '\0';
	  dir[ndir] = gdir + s + 1;
	  ndir++;
	}
      }

      // Print directories
      for(int s=0; s < ndir; s++){
	printf("%s\n", dir[s]);
      }
      
      break;
    }
    i++;
  }
  
  printf("*********** morgan's sh processing loop **********\n");
  
  while(1){
     printf("morgansh: ");

     fgets(line, 128, stdin);
     line[strlen(line)-1] = 0;      // fgets() has \n at end

     if (line[0]==0)
       continue;
     printf("line = %s\n", line);   // Print line to see what you got

     // Check if there is a pipe sumbol in the command
    
     char *pipesplit = strchr(line, '|');

     if(pipesplit){ //Place null character at first pipe symbol
       pipesplit[0] = '\0';
       printf("head = %s\ntail=%s\n", line, pipesplit+1);
     } 
     
     strcpy(gpath, line);
     name[0] = gpath;
     ntoken  = 1;
     for(int i = 0; gpath[i]!='\0'; i++){
       if(gpath[i] == ' '){ 
	   gpath[i] = '\0';
	   if(gpath[i+1]!=' ' && gpath[i+1] !='\0'){ //So extra spaces do not get used as arguments
	     name[ntoken] = gpath + i + 1;
	     ntoken++;
	   }
	 }
     }
  
     name[ntoken]=0;
   
     	  // Print token strings
     for(int i = 0; i <ntoken; i++){
       printf("name[%d]: %s\n", i,name[i]);
     }


     if(strcmp(name[0], "cd")==0){
	 if(name[1]){
	   chdir(name[1]);
	 }else{
	   chdir(home);  
	   
	 }
     }else if(strcmp(name[0], "exit") ==0){
       exit(1);
     }else{
     
     
    
     pid = fork();   // Fork a child sh

     if (pid){
        printf("parent sh %d waits\n", getpid());
        pid = wait(&status);

        printf("child sh %d died : exit status = %04x\n", pid, status);
        continue;
     }
     else{
        printf("child sh %d begins\n", getpid());

	if(pipesplit){
	  int pd[2];
	  pipe(pd); // Create pipe
     
	 pid = fork(); // Fork child for pipe
	 
	  if(pid){ // Parent is pipe reader (tail)
	   
	   close(pd[1]);
	    close(0);
	    dup(pd[0]);
	    
	    close(pd[0]);
	    
	   

	    // First, assume no pipe in tail
	    pipesplit = pipesplit + 2; //  Pipe always followed by space
	    tname[0] = pipesplit;
             ttoken  = 1;
	     for(int i = 0; pipesplit[i]!='\0'; i++){
	       if(pipesplit[i] == ' '){ 
		 pipesplit[i] = '\0';
		 if(pipesplit[i+1]!=' ' && pipesplit[i+1] !='\0'){ // So extra spaces do not get used as arguments
		   tname[ttoken] = pipesplit + i + 1;
		   ttoken++;
		 }
	       }
	     }
  
	     tname[ttoken]=0;
	     //print token strings
	     for(int i = 0; i <ttoken; i++){
	       printf("tname[%d]: %s\n", i,tname[i]);
	     }
	     // printf("before handle io in pipe tail\n");
	      handle_io_redirection(tname, ttoken);
	      // printf("before execute cmd in pipe tail\n");
              find_cmd_and_execute(env,tname);
	    
	  }
	  else{ // Child as pipe writer (head)
	     close(pd[0]);
	    close(1);
	    dup(pd[1]);
	    close(pd[1]);
	    
	    // Continues and executes head like regular command already in name string
	   
	  }

	}



	handle_io_redirection(name, ntoken);
        find_cmd_and_execute(env, name);
     }
     }
  }    
}



int handle_io_redirection(char *cmdnames[], int n_cmdnames){
	// Handle I/O redirection, assume no pipe in command line
	for(int i = 0; i <n_cmdnames; i++){
	  if(strcmp(cmdnames[i], "<") ==0){
	    // Take inputs from name[i + 1]
	    // Check name[i + 1] exists
	    if(cmdnames[i+1]){
	      close(0);
	      int fd=open(cmdnames[i+1], O_RDONLY);
	       cmdnames[i] = 0; // Set name pointer to null so command doesn't try to use < as an argument
	      if(fd == -1){
		printf("Error opening %s as input stream\n",cmdnames[i+1]);
		continue;
	      }
	    }else{
	      printf("no input file specified after <\n");
	      continue;
	    }
	 
	  }else if(strcmp(cmdnames[i], ">")==0){
	    // Send outputs to name[i + 1]
	    // Check name[i + 1] exists
	    if(cmdnames[i+1]){
	      close(1);
	      int fd=open(cmdnames[i+1], O_WRONLY|O_CREAT, 0644);
	      cmdnames[i] = 0; // Set name pointer to null so command doesn't try to use > as an argument
	      if(fd == -1){
		printf("Error opening %s as ouput file\n", cmdnames[i+1]);
		continue;
	      }
	    }else{
	      printf("no output file specified after <\n");
	      continue;
	    }
	  }else if(strcmp(cmdnames[i], ">>")==0){
	    // APPEND outputs to name[i + 1]
	    // Check name[i + 1] exists
	    if(cmdnames[i+1]){
	      close(1);
	      int fd=open(cmdnames[i+1], O_WRONLY|O_APPEND);
	       cmdnames[i] = 0; // Set name pointer to null so command doesn't try to use >> as an argument
	      if(fd == -1){
		printf("Error opening %s as output file to append to\n", cmdnames[i+1]);
		continue;
	      }
	    }else{
	      printf("no output file specified after <\n");
	      continue;
	    }
	  }
	}
}

// ndir and name already parsed before use
int find_cmd_and_execute(char *env[],char *name[64]){
  char cmd[64];
  int r;
    // Find command and execute, passing parameters
  // If name[0] is a binary executable file in the current directory, like a.out
  r = execve(name[0], name, env);
  
  // name[0] is command looking for in various paths
     for (int i=0; i<ndir; i++){	  
      strcpy(cmd, dir[i]);
      strcat(cmd, "/");
      strcat(cmd, name[0]);
	   
       r = execve(cmd, name, env);
      }
  printf("cmd %s not found, child sh exit\n", name[0]);
  exit(123);   // die with value 123
}


