#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

void parseInput();
void handleCommand();
void grsherror();
void changePath();
void validate_commands();
void split_args();
void execute_args();
void runShell();

char *path = "/bin";
int redir = 0;

//MAIN METHOD
int main(int argc, char *argv[]){
    runShell(argc, argv);
}

//RUN SHELL
void runShell(int argc, char *argv[]){
    char * command = 0;
    size_t len = 0;
    if (argc == 1) {
        while (1) {
            printf("grsh> ");
            getline(&command,&len,stdin);
            if(strncmp(command,"\n",1)==0){
                continue;
            }
            parseInput(command);
        }

    }
    else if (argc == 2) {
        FILE *batchFile;
        batchFile = fopen(argv[1], "r");
        if (batchFile == NULL) { exit(1); }
        while (getline(&command, &len, batchFile) != -1) {
            parseInput(command);
        }
        exit(0);
    }
    else {
        exit(1);
    }
}

//HANDLE ERRORS

void grsherror(){
  char error_message[30] = "An error has occurred\n";
  write(STDERR_FILENO, error_message, strlen(error_message));
}

//PARSES LINE SEPEATING PARALLEL COMMANDS
void parseInput(char* line){
  // remove new-line char
  if (line[strlen(line) - 1] == '\n'){
    line[strlen(line) - 1] = '\0';

  }
  if (strncmp(line, "exit",4) == 0){
    exit(0);
  }

  // seperation of parallel args
  if(strstr(line, "&")){
    // copy of line to keep original unadultured for safety
    char linecopy[strlen(line)];
    strcpy(linecopy, line);

    // array initialization
    char* cmds[strlen(line)];
    char *curr =  (char *)malloc(strlen(line));

    // setting first argument
    curr = strtok(linecopy, "&");
    cmds[0] = (char *)malloc(strlen(line));


    // setting remaining commands in array
    int i = 0;
    while(curr != NULL){
      strcpy(cmds[i], curr);
      curr = strtok(NULL, "&");
      i++;
      cmds[i] = (char *)malloc(strlen(line));
    }
    cmds[i] = NULL;

    // calling all commands
    i = 1;
    handleCommand(cmds[0]);
    while(cmds[i] != NULL){
      handleCommand(cmds[i]);
      i++;
    }
  }  else { handleCommand(line); }
}

//HANDLES BUILT IN COMMANDS AND REDIRECTION

void handleCommand(char* line){
  // retrieve base command
  char *args = (char *)malloc(sizeof(line));
  strcpy(args, line);
  char *cmd = strtok(args, " ");

  // handle command == cd
  if (strncmp(cmd, "cd",2) == 0){
    cmd = strtok(NULL, " ");
    if (strtok(NULL, " ") != NULL){
      grsherror();
      return;
    }
    else {
      chdir(cmd);
      return;
    }
  }
  // handle cmd == path
  if (strncmp(cmd, "path",4) == 0){
    changePath(line);
    return;
  }
  // check for redirect
  if(strstr(line, ">")){
    redir = 1;
    // copy of line to work with
    char* hold = (char *)malloc(strlen(line));
    char* cmd = (char *)malloc(strlen(line));
    char* output = (char *)malloc(strlen(line));
    strcpy(hold, line);

    strcpy(cmd, strtok(hold, ">"));
    output = strtok(NULL, ">");

    if (output == NULL){
      grsherror();
      return;
    }
    validate_commands(cmd, output);
  }else{ validate_commands(line, ""); }
}
void changePath(char* line){
  path = (char *)malloc(strlen(line));
  char* cmd = strtok(line, " ");
  cmd = strtok(NULL, "");
  strcat(path, cmd);
}

//CHECKS IF COMMAND IS PRESENT IN ANY PATH
void validate_commands(char *line, char* output){
  // copies of path and the arguments
  //printf("current path: %s\n", path);
  char path_copy[strlen(path)];
  char args_copy[strlen(line)];
  strcpy(path_copy, path);
  strcpy(args_copy, line);

  // retrieving the command to execute and the first path
  // to try
  char* cmd = strtok(args_copy, " ");
  char* curr_path = strtok(path_copy, " ");
  char cmdpath[strlen(path)];

  // boolean check for if we found command
  int cmd_found = 1;

  // try all combinations of bin paths and commands
  while((cmd_found == 1) && (curr_path != NULL)){

    // combine bin path with command
    strcpy(cmdpath, curr_path);
    strcat(cmdpath, "/");
    strcat(cmdpath, cmd);

    // try command path
    if(access(cmdpath, X_OK) == 0){
      cmd_found =0;
    }
    // move to next path
    curr_path = strtok(NULL, " ");
  }
  if(cmd_found == 0){

    split_args(cmdpath, line, output);
    return;
  }else{
    grsherror();
    return;
  }
  return;
}

//SEPERATES COMMANDS FROM ARGUMENTS
void split_args(char* cmdpath, char* line, char* output){
  // needed variables
  char* args[strlen(line)];
  char* curr_arg;
  int i = 1;

  // setting the path to first arg
  args[0] = (char *)malloc(strlen(cmdpath));
  strcpy(args[0], cmdpath);
  strtok(line, " ");
  curr_arg = strtok(NULL, " ");

  // copying remaining args into arg list
  while(curr_arg != NULL){
    args[i] = (char *)malloc(strlen(line));
    strcpy(args[i], curr_arg);
    i += 1;
    curr_arg = strtok(NULL, " ");
  }

  // set last arg to NULL
  args[i] = NULL;
 // printf("arg array created!\n");

  execute_args(args, output);
  return;
}

//EXECUTES COMMAND USING FORK, EXECV, AND WAIT
void execute_args(char* args[], char* output){
  int rc;
  // CREATES CHILD PROCESS(ES) AND EXECUTES
  rc = fork();
  if(rc < 0){
   //FORK FAILED
  }
    else if(rc == 0){
 
    if (redir == 0){
      execv(args[0], args);

    }
    else if(redir == 1){
      int fd = open(output, O_TRUNC | O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
      dup2(fd, 1);
      dup2(fd, 2);
      close(fd);
      execv(args[0], args);

    }
  }
  else{
    int wc = wait(NULL);
    assert(wc >= 0);
  }
    
  redir = 0;
}
