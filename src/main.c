#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
// #include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>

#define MAX_BUFFER_SIZE 128
#define NUM_BUILTINS 6
#define CMD_PATH_MAX 100
#define MAX_COMMANDS 100

char *builtins[NUM_BUILTINS] = {"echo", "exit", "type", "pwd", "cd", "history"};

typedef struct {
  char *input;
  int8_t input_length;
  bool is_valid;
} InputBuffer;

InputBuffer CreateInputBuffer() {
  InputBuffer buffer;
  buffer.input = calloc(MAX_BUFFER_SIZE, sizeof(char));
  buffer.input_length = 0;
  buffer.is_valid = false;
  return buffer;
}

bool search_builtin(const char* command) {
  char *path = getenv("PATH");
  if (path == NULL) {
    printf("%s: not found\n", command);
    return false;
  }
  char *path_copy = strdup(path);
  bool found = false;
  char *dir = strtok(path_copy, ":");
  while (dir != NULL && !found) {
    char full_path[1000];
    snprintf(full_path, sizeof(full_path), "%s/%s", dir, command);

    // Check if it is accessible
    if (access(full_path, X_OK) == 0) {
      printf("%s is %s\n", command, full_path);
      found = true;
    }
    // Get the next token
    dir = strtok(NULL, ":");
  }
  free(path_copy);
  return found;
}

void handleType() {
  char *command2 = strtok(NULL, "");
  for(int i=0; i<NUM_BUILTINS; i++) {
    if(strncmp(command2, builtins[i], strlen(command2)) == 0) {
      printf("%s is a shell builtin\n", builtins[i]);
      return;
    }
  }
  // not a buildin command
  if(search_builtin(command2)) {
    return;
  }
  
  // If we reach here, the command is not a builtin
  printf("%s: not found\n", command2);
}
bool process_input(char* input_buffer, char* command) {
  if(command == NULL) {
    return true;
  }
  
  if(strncmp(command, "exit", 4) == 0) {
    exit(EXIT_SUCCESS);
  } else if(strncmp(command, "echo", 4) == 0) {
    char *echo_text = input_buffer + strlen(command);
    while (*echo_text == ' ') echo_text++;
    printf("%s\n", echo_text);
    return true;
  } else if(strncmp(command, "type", 4) == 0) {
    handleType();
    return true;
  } else if(strcmp(command, "pwd") == 0) {
    char cwd[CMD_PATH_MAX];
    if(getcwd(cwd, sizeof(cwd)) != NULL) {
      printf("%s\n", cwd);
      return true;
    }
  } else if(strcmp(command, "cd") == 0) {
    char *path = strtok(NULL, "");
    if(path == NULL || strcmp(path, "~") == 0) {
      // should go to home path
      chdir(getenv("HOME"));
      return true;
    }
    if(chdir(path) != 0) {
      printf("cd: %s: No such file or directory\n", path);
    }
    return true;
  }
  return false;
}

char* trim(char* token) {
  while (isspace(*token)) {   // ctype.h
    token++;
  }
  if (*token == 0) {
    return token;
  }

  char *end = token + strlen(token) - 1;
  while (end > token && isspace(*end)) {
    end--;
  }
  *(end + 1) = '\0';
  return token;
}

char** getPipedCommands(char* input_buffer, int *num_cmds) {
  char **cmds = malloc(MAX_COMMANDS * sizeof(char *));
  if (!cmds) {
    perror("malloc failed");
    exit(EXIT_FAILURE);
  }
  int ind = 0;
  char *token = strtok(input_buffer, "|");
  while(token != NULL) {
    cmds[ind++] = trim(token);
    token = strtok(NULL, "|");
  }
  *num_cmds = ind;
  return cmds;
}

void processCommands(char *input_buffer, bool toFork) {
  char *input_copy = strdup(input_buffer);  // safe, modifiable copy
  char *command = strtok(input_copy, " ");
  if(!process_input(input_buffer, command)) {    
    // Parse the input into arguments
    char *args[MAX_BUFFER_SIZE / 2 + 1];    // considering string/argument length atleast 2 characters
    int arg_count = 1;
    args[0] = command; // First argument is the command itself
    char* token = strtok(NULL, " ");
    while(token != NULL && arg_count < (MAX_BUFFER_SIZE / 2)) {
      // remove quotes from arguments
      if ((token[0] == '"' && token[strlen(token) - 1] == '"')) {
        token[strlen(token) - 1] = '\0'; // remove trailing quote
        token++;                         // skip leading quote
      }
      args[arg_count] = token;
      arg_count++;
      token = strtok(NULL, " ");
    }
    args[arg_count] = NULL; // Null-terminate the array of arguments

    if(args[0] == NULL) {
      free(input_copy);
      return;
    }
    if(toFork) {
      pid_t pid = fork();
      if(pid == 0) {
        // Child Process
        execvp(args[0], args);
        printf("%s: command not found\n", args[0]);
        exit(EXIT_FAILURE); // Exit child process if exec fails
      } else if(pid > 0) {
        // Parent Process
        int status;
        waitpid(pid, &status, 0);
      } else {
        perror("fork");
      }
    } else {
      execvp(args[0], args);
      // perror("execvp failed");
      printf("%s: command not found\n", args[0]);
      exit(EXIT_FAILURE);
    }
  }
  free(input_copy);
}
int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);

  // Uncomment this block to pass the first stage
  printf("$ ");

  // Wait for user input
  char input[100];
  InputBuffer input_buffer = CreateInputBuffer();
  while(1) {
    fgets(input_buffer.input, MAX_BUFFER_SIZE, stdin);
    input_buffer.input_length = strlen(input_buffer.input);
    input_buffer.input[input_buffer.input_length - 1] = '\0'; // Remove the trailing newline
    input_buffer.is_valid = false;

    if(strchr(input_buffer.input, '|') != NULL) {
      int num_cmds = 0;
      char **commands = getPipedCommands(input_buffer.input, &num_cmds);
      int pipefd[2], prev_fd = -1;
      for(int i=0; i<num_cmds; i++) {
        if (i < num_cmds - 1) {
          if (pipe(pipefd) < 0) {
            perror("pipe failed");
            exit(1);
          }
        }
        pid_t pid = fork();
        if(pid == 0) {
          // if not first command, read from the pipe
          if(prev_fd != -1) {
            dup2(prev_fd, STDIN_FILENO);
            close(prev_fd);
          }
          // if not the last command, write to the pipe
          if(i < num_cmds-1) {
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[0]);
            close(pipefd[1]);
          }
          // printf("processing command =%s", commands[i]);
          processCommands(commands[i], false);
          exit(0);  // check if its needed
        }
        // parent, clean up
        if(prev_fd != -1) {
          close(prev_fd);
        }
        if(i < num_cmds-1) {
          close(pipefd[1]);
          prev_fd = pipefd[0];
        }
      }
      // wait for all the children
      for(int i=0; i<num_cmds; i++) {
        wait(NULL);
      }
    } else {
      processCommands(input_buffer.input, true);
    }
    printf("$ ");
  }
  return 0;
}


// int pipefd[2 * (num_cmds-1)];
//       //create the necessary pipes, num_cmds-1
//       for(int i=0; i<num_cmds-1; i++) {
//         if(pipe(pipefd + i * 2) < 0) {
//           perror("pipe");
//           exit(EXIT_FAILURE);
//         }
//       }
//       printf("start loop for calling fork for the piped commands num = %d\n", num_cmds);
//       // now fork the processes
//       for(int i=0; i<num_cmds; i++) {
//         printf("fork count = %d\n", i);
//         pid_t pid = fork();
//         if(pid == 0) {
//           printf("in child process for i=%d\n", i);
//           // child process
//           if(i != 0) {
//             // dup2(pipefd[(i - 1)*2], STDIN_FILENO);
//             printf("stdin calling dup2 for i = %d\n", i);
//             if (dup2(pipefd[(i - 1) * 2], STDIN_FILENO) == -1) {
//               // perror("dup2 stdin");
//               printf("error in stdin dup2 for i=%d\n", i);
//               // exit(EXIT_FAILURE);
//             }
//             // close(pipefd[(i - 1) * 2]); // close after dup2
//             printf("dup2 returned for i = %d\n", i);
//           }
//           if(i != num_cmds-1) {
//             printf("stdout calling dup2 for i = %d\n", i);
//             fflush(stdout);
//             if (dup2(pipefd[i * 2 + 1], STDOUT_FILENO) == -1) {
//               perror("dup2 stdout");
//               printf("error in stdout dup2 for i=%d\n", i);
//               // exit(EXIT_FAILURE);
//             }
//             // close(pipefd[i * 2 + 1]); // close after dup2
//             printf("dup2 returned for i = %d\n", i);
//           }
//           printf("get past the duplication command for i=%d\n", i);
//           //closing all the pipes
//           for(int j=0; j<(num_cmds-1)*2; j++) {
//             if(j != (i * 2 + 1) && j != ((i - 1) * 2)) {
//               printf("calling close pipe for i=%d j=%d\n", i, j);
//               close(pipefd[j]);
//             }
//             // close(pipefd[j]);
//           }

//           // call the actual execution with commands[i]
//           processCommands(commands[i], false);
//         }
//       }

//       // closing all the pipes in the parent
//       for(int i=0; i<(num_cmds-1)*2; i++) {
//         close(pipefd[i]);
//       }

//       // wait for all the children
//       for(int i=0; i<num_cmds; i++) {
//         wait(NULL);
//       }