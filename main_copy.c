#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
// #include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>

#define MAX_BUFFER_SIZE 128
#define NUM_BUILTINS 5
#define PATH_MAX 100
#define MAX_COMMANDS 100

char *builtins[NUM_BUILTINS] = {"echo", "exit", "type", "pwd", "cd"};

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

bool is_valid(char* path, int path_len) {

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
  // char *command = strtok(input_buffer, " ");
  // printf("input_buffer = %s command = %s", input_buffer, command);
// bool process_input(InputBuffer *input_buffer, char *command) {
  if(command == NULL) {
    // input_buffer->is_valid = true; // No command entered, mark as valid
    return true;
  }
  
  if(strncmp(command, "exit", 4) == 0) {
    // input_buffer->is_valid = true;
    exit(EXIT_SUCCESS);
  } else if(strncmp(command, "echo", 4) == 0) {
    // input_buffer->is_valid = true;
    // printf("%s\n", input_buffer + strlen(command) + 1); // +1 to skip the space after "echo"
    char *echo_text = input_buffer + strlen(command);
    while (*echo_text == ' ') echo_text++;
    printf("%s\n", echo_text);
    return true;
    // printf("executed echo");
    return true;
  } else if(strncmp(command, "type", 4) == 0) {
    handleType();
    // input_buffer->is_valid = true;
    return true;
  } else if(strcmp(command, "pwd") == 0) {
    char cwd[PATH_MAX];
    if(getcwd(cwd, sizeof(cwd)) != NULL) {
      printf("%s\n", cwd);
      // input_buffer->is_valid = true;
      return true;
    }
  } else if(strcmp(command, "cd") == 0) {
    char *path = strtok(NULL, "");
    if(path == NULL || strcmp(path, "~") == 0) {
      // should go to home path
      chdir(getenv("HOME"));
      // input_buffer->is_valid = true;
      return true;
    }
    if(chdir(path) != 0) {
      printf("cd: %s: No such file or directory\n", path);
    }
    // input_buffer->is_valid = true;
    return true;
  }
  // printf("returning false for command = %s", command);
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
  // printf("processCommands = %s\n", input_buffer);
  // process_input(&input_buffer, command);
  // if(!input_buffer.is_valid) {
  char *input_copy = strdup(input_buffer);  // safe, modifiable copy
  char *command = strtok(input_copy, " ");
  // char *command = strtok(input_buffer, " ",);
  if(!process_input(input_buffer, command)) {
    // printf("came to fork and execute....%s", input_buffer);
    
    // Parse the input into arguments
    char *args[MAX_BUFFER_SIZE / 2 + 1];    // considering string/argument length atleast 2 characters
    int arg_count = 1;
    args[0] = command; // First argument is the command itself
    // char *token = strtok(input_buffer.input, " ");
    char* token = strtok(NULL, " ");
    while(token != NULL && arg_count < (MAX_BUFFER_SIZE / 2)) {
      args[arg_count] = token;
      arg_count++;
      token = strtok(NULL, " ");
    }
    args[arg_count] = NULL; // Null-terminate the array of arguments

    if(args[0] == NULL) {
      // printf("$ ");
      // continue; // No command entered, prompt again
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
      perror("execvp failed");
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
    // Remove the trailing newline
    // input[strlen(input) - 1] = '\0';
    // printf("Input: %s\n strlen()=%d", input_buffer.input, strlen(input_buffer.input)); // Debugging line to see the input
    input_buffer.input_length = strlen(input_buffer.input);
    input_buffer.input[input_buffer.input_length - 1] = '\0'; // Remove the trailing newline
    input_buffer.is_valid = false;

    if(strchr(input_buffer.input, '|') != NULL) {
      int num_cmds = 0;
      char **commands = getPipedCommands(input_buffer.input, &num_cmds);
      // printf("num_cmds = %d", num_cmds);
      // for(int i=0; i<num_cmds; i++) {
      //   printf("cmd[%d] = %s", i, commands[i]);
      // }
      int pipefd[2 * (num_cmds-1)];
      //create the necessary pipes, num_cmds-1
      for(int i=0; i<num_cmds-1; i++) {
        if(pipe(pipefd + i*2) < 0) {
          perror("pipe");
          exit(EXIT_FAILURE);
        }
      }

      // now fork the processes
      for(int i=0; i<num_cmds; i++) {
        // printf("forking for comd[%d] = %s\n", i, commands[i]);
        pid_t pid = fork();
        if(pid == 0) {
          // child process
          if(i != 0) {
            dup2(pipefd[(i - 1)*2], STDIN_FILENO);
          }
          if(i != num_cmds-1) {
            dup2(pipefd[i*2 + 1], STDOUT_FILENO);
          }

          //closing all the pipes
          for(int j=0; j<(num_cmds-1)*2; j++) {
            close(pipefd[j]);
          }

          // call the actual execution with commands[i]
          // printf("calling process comand for i=%d\n", i);
          processCommands(commands[i], false);
        }
      }

      // closing all the pipes in the parent
      for(int i=0; i<(num_cmds-1)*2; i++) {
        close(pipefd[i]);
      }

      // wait for all the children
      for(int i=0; i<num_cmds; i++) {
        wait(NULL);
      }
    } else {
      // printf("came here......");
      processCommands(input_buffer.input, true);
    }
    
    
    printf("$ ");
  }
  return 0;
}


// Print the input back to the user
// if(strncmp(input, "exit", 4) == 0) {
//   // printf("Exiting...\n");
//   break;
// }

// // handling the commands
// // echo command
// // If the input starts with "echo ", print the rest of the input
// if(strncmp(input, "echo ", 5) == 0) {
//   printf("%s\n", input + 5);
//   continue;
// }
// // type command
// // If the input starts with "type ", print the rest of the input
// if(strncmp(input, "type ", 5) == 0) {
//   // if(argc < 2) {
//   //   printf("Usage: %s <filename>\n", argv[0]);
//   //   continue;
//   // }
//   for(int i=0; i<2; i++) {
//     if(strcmp(input + 5, builtins[i]) == 0) {
//       printf("%s is a shell builtin\n", builtins[i]);
//       continue;
//     }
//   }
// }
// printf("%s: command not found\n", input);



// int my_up_arrow_handler(int count, int key) {
//   // move up in history
//   HIST_ENTRY *entry = previous_history();
//   if (entry) {
//     rl_replace_line(entry->line, 1);  // replace current line
//     rl_point = rl_end;                // move cursor to end
//   }
//   return 0;
// }

// int my_down_arrow_handler(int count, int key) {
//   // move down in history
//   HIST_ENTRY *entry = next_history();
//   if (entry) {
//     rl_replace_line(entry->line, 1);
//   } else {
//     rl_replace_line("", 0);  // clear line if no forward history
//   }
//   rl_point = rl_end;
//   return 0;
// }



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


char **tokenize_echo_args(const char *input_line, int *argc_out) {
  char **tokens = malloc(sizeof(char*) * MAX_BUFFER_SIZE);
  int token_index = 0;

  const char *p = input_line;
  char buffer[1024];
  int buf_index = 0;
  bool in_single = false, in_double = false;

  while (*p) {
    char c = *p;
    if (in_single) {
      if (c == '\'' && *(p+1) == '\'') {
        buffer[buf_index++] = '\'';
        p++;
      } else if (c == '\\' && *(p+1) == '\'') {
        buffer[buf_index++] = '\'';
        p++;
      } else if (c == '\'') {
        in_single = false;
      } else {
        buffer[buf_index++] = c;
      }
    } else if (in_double) {
      if (c == '\\') {
        p++;
        if (*p == '$' || *p == '`' || *p == '"' || *p == '\\' || *p == '\n') {
          buffer[buf_index++] = *p;
        } else {
          buffer[buf_index++] = '\\';
          buffer[buf_index++] = *p;
        }
      } else if (c == '"') {
        in_double = false;
      } else {
        buffer[buf_index++] = c;
      }
    } else {
      if (c == '\'') {
        in_single = true;
      } else if (c == '"') {
        in_double = true;
      } else if (c == '\\') {
        p++;
        if (*p) buffer[buf_index++] = *p;
      } else if (isspace(c)) {
        if (buf_index > 0) {
          buffer[buf_index] = '\0';
          tokens[token_index++] = strdup(buffer);
          buf_index = 0;
        }
      } else {
        buffer[buf_index++] = c;
      }
    }
    p++;
  }
  if (buf_index > 0) {
    buffer[buf_index] = '\0';
    tokens[token_index++] = strdup(buffer);
  }

  tokens[token_index] = NULL;
  *argc_out = token_index;
  return tokens;
}



another impelementation for quotes:

int argc;
    char **argv = tokenize_echo_args(echo_text, &argc);
    for (int i = 0; i < argc; i++) {
      if (i > 0) printf(" ");
      printf("%s", argv[i]);
      free(argv[i]);
    }
    free(argv);
    printf("\n");
    return true;

-- in the process_input funcxtion


char **tokenize_echo_args(const char *input_line, int *argc_out) {
    char **tokens = malloc(sizeof(char*) * MAX_BUFFER_SIZE);
    int token_index = 0;

    const char *p = input_line;
    char buffer[1024];
    int buf_index = 0;
    bool in_single = false, in_double = false;

    while (*p) {
        char c = *p;

        if (in_single) {
            if (c == '\'') {
                in_single = false;
            } else {
                buffer[buf_index++] = c;
            }
        } else if (in_double) {
            if (c == '\\') {
                p++;
                if (*p == '$' || *p == '`' || *p == '"' || *p == '\\' || *p == '\n') {
                    buffer[buf_index++] = *p;
                } else {
                    buffer[buf_index++] = '\\';
                    buffer[buf_index++] = *p;
                }
            } else if (c == '"') {
                in_double = false;
            } else {
                buffer[buf_index++] = c;
            }
        } else {
            if (c == '\'') {
                in_single = true;
            } else if (c == '"') {
                in_double = true;
            } else if (c == '\\') {
                p++;
                if (*p) buffer[buf_index++] = *p;
            } else if (isspace(c)) {
                if (buf_index > 0) {
                    buffer[buf_index] = '\0';
                    tokens[token_index++] = strdup(buffer);
                    buf_index = 0;
                }
            } else {
                buffer[buf_index++] = c;
            }
        }
        p++;
    }

    if (buf_index > 0) {
        buffer[buf_index] = '\0';
        tokens[token_index++] = strdup(buffer);
    }

    tokens[token_index] = NULL;
    *argc_out = token_index;
    return tokens;
}
