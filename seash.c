#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>


int fork_and_wait(const char* command, const char* arg) {
    // TODO: Fork a child process
    // Child: execute command with arg using execlp
	// Parent: wait for child to complete
    // Return 0 on success, -1 on fork error
    pid_t pid = fork();
	if(pid < 0){
		perror("fork error");
        exit(1);
		return -1;
    }
    printf("pid = %d\n",pid);
    if(pid == 0){
		
        if(!arg || arg == NULL){
            execlp(command,command,NULL);
        }
        else{
			execlp(command,command,arg,NULL);
		}
        perror("execlp");

        exit(0);
    }
    else{
        int status;
        waitpid(pid, &status,0);
    //    printf("pid = %d; status = %d\n",pid,status);

    }
    return 0;
}
int write_history(const char *filepath, const char *command)
 {
     // TODO: Open file in append mode
     // If fopen fails, call perror("open") and return -1
     // Write command to file with fprintf
     // Close file
     // Return 0 on success
      FILE* f = fopen(filepath, "a");
      if(f == NULL || ferror(f)){
          perror("open");
          return -1;
      }
     fprintf(f,"%s\n",command);
     fclose(f); 
     return 0;
 }
 int read_history(const char *filepath)
   {
       // TODO: Open file in read mode
     // If fopen fails, call perror("open") and return -1
     // Read lines with fgets and print with line numbers
     // Format: "line_number command"
     // Close file
     // Return 0 on success
     FILE* f = fopen(filepath,"r");
     if(f == NULL || ferror(f)){
         perror("open");
         return -1;
     }
     char buffer[2048];
     char *line;
     int i = 0;
     while(f && !ferror(f) && !feof(f)){
         line = fgets(buffer,sizeof(buffer),f);
         if(!line){
             break;
         }
         printf("%d %s",i,line);
         i++;
     }
     return 0;
 }
 int run_cd(int argc, char *path)
   {
       // TODO: Check if argc == 2 (cd + path)
       // If wrong: print "usage: cd <path>" to stderr and return -1
     // If correct: call chdir(path)
     // If chdir fails: call perror("cd") and return -1
     // If success: return 0
     if(argc == 2 && path != NULL){
         if(chdir(path) != 0){
             perror("cd");
             return -1;
         }
         return 0;
     }
     fprintf(stderr,"usage: cd <path>\n");
     return -1;
 }
  int run_exit(int argc)
   {
       // TODO: Check if argc == 1 (just "exit", no arguments)
       // If correct: call exit(EXIT_SUCCESS)
       // If wrong: print "usage: exit" to stderr and return -1
      int status;
      pid_t pid;
      waitpid(pid,&status, 0);
      if(argc != 1){
          fprintf(stderr,"usage: exit\n");
          return -1;
     }

     if(WIFEXITED(status)&& WEXITSTATUS(status) == 0){
        exit(EXIT_SUCCESS);
         return 0;
     }
      fprintf(stderr,"usage: exit\n");
      return -1;
 }
  void prompt_command_loop(){
       // implement this function
       char buffer[1000];
       char *input;
       char *factor;
      int len;
     while(input != "exit"){
          input = fgets(buffer,sizeof(buffer),stdin);
          if(input == NULL){
             break;
          }
          len = strlen(input);
          if(len >= 0 && input[len-1] == '\n'){
            len--;
              input[len] = '\0';
          }
          factor = strtok(input," \t");
          if(factor != NULL){
             printf("%s\n",factor);
         }
     }
 };
int fork_run_command(const char command[], char *args[]){
    // Implement the run_ls function here!
   pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        
        exit(1);
        return 0;
    } 
    if (pid == 0) {
        // 1. Child calls exec
        execvp(args[0], args);
        
    // 2. Fallback exit (only reached if exec fails, e.g., bad command)
        perror(command);
        return -1; 
    } else {
        // 3. Parent waits for the exec'd child to finish
        int status;
        waitpid(pid, &status, 0); 
    }

}
int run_command(const char command[],char *args[]){
	execvp(args[0], args);
	// 2. Fallback exit (only reached if exec fails, e.g., bad command)
	perror("execvp");
		
	return -1;
}
int main(int argc, char *argv[]) {
    char buffer[1001];
	int isShell;
	if(argc == 1){
		isShell = 0;
	}
	else{
		if(argc != 2)
			return -1;
		if(argv[1] != "./seash")
			return -1;
		isShell = 1;
	}
	char *prompt;
	if(isShell)
		prompt = "student@🐚 /tmp/seash-sandbox/";
	else
		prompt = "you@🐚 /home/you/";
	char cwd[120];
	
	if(getcwd(cwd,sizeof(cwd)) == NULL){
		if(run_exit(1) == -1)
			return -1;
	}
	printf("%s%s$",prompt,strstr(cwd,"cse29/"));
    char old[1001];
	while(fgets(buffer, sizeof(buffer), stdin) != NULL) {
		strcpy(old,buffer);
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') buffer[len-1] = '\0';
        char* argv[64];
        int argc = 0;
        char *token = strtok(buffer, " \t");
        while (token != NULL && argc < 64) {
            argv[argc++] = token;
            token = strtok(NULL, " \t");
        	
		}
		argv[argc] = NULL;
        if (argc > 0) {
            const char* command = argv[0];
            char arg[1001];
			int rightform = 1;
			
			
			if(strcmp(command,"cd") == 0){
				if(argc == 1 ||  (isShell == 1 && argc > 0 && strcmp(argv[0],"&") != 0 ) || (isShell == 0 && argc != 2 && strcmp(argv[2],"&")!= 0)){
					rightform = 0;
					perror("cd");

				}
				else{
					strcpy(arg,argv[1]);
					if(run_cd(argc,arg) != 0)
						perror("cd");
				}
				if(write_history("./history.txt",old) == -1)
					perror("history");
			}
			else if (strcmp(command,"history") == 0){
					

				if(argc > 2 && strcmp(argv[2],"&") != 0){

						
					perror("history");
					rightform = 0;
				}
				else if (argc == 2){

					if(strcmp(argv[1],"-c") == 0){
						FILE *f = fopen("./history.txt","w");
						if(f == NULL || ferror(f)){

							perror("open");
							rightform = 0;
							perror("history");
						 }
						fclose(f);
							
					

					}
					else if (strcmp(argv[1],"&") != 0){
						perror("history");
						rightform = 0;
					}
					else if(read_history("./history.txt") == -1){
						perror("history");
						rightform = 0;
					}
				}
				if(write_history("./history.txt",old) == -1)
					perror("history");
			}
		
			else if(strcmp(command,"exit") == 0){
				if(run_exit(argc) == -1){
					perror("exit");
					rightform = 0;
				}
				
				if(write_history("./history.txt",old) == -1)
					perror("history");
			}
			else if (strcmp(command,"pwd") == 0){
				if(argc != 1){

					perror("pwd");
					rightform = 0;
				}
				else{

					printf("%s%s\n",prompt,strstr(cwd,"cse29"));
				}

				if(write_history("./history.txt",old) == -1)
					perror("history");
			}
			else{
				if(fork_run_command(command,argv) == -1)
					perror(command);
				if(write_history("./history.txt",old) == -1)
					perror("history");
			}

			
		}
		if(getcwd(cwd,sizeof(cwd)) == NULL){
			if(run_exit(1) == -1)
				return -1;
		}
		printf("%s%s$",prompt,strstr(cwd,"cse29"));
    }

	
    return 0;
}

