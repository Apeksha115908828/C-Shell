#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
// #include <sys/types.h>
#include <sys/wait.h>
#define MAX_BUFFER_SIZE 128
#define NUM_BUILTINS 3
char *builtins[NUM_BUILTINS] = {"echo", "exit", "type"};

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
  // if (!found) {
  //   printf("%s: not found\n", command);
  // }
  free(path_copy);
  return found;
}

bool process_input(InputBuffer *input_buffer, char *command) {
  if(command == NULL) {
    input_buffer->is_valid = true; // No command entered, mark as valid
    return true;
  }
  
  if(strncmp(command, "exit", 4) == 0) {
    input_buffer->is_valid = true;
    exit(EXIT_SUCCESS);
  } else if(strncmp(command, "echo", 4) == 0) {
    input_buffer->is_valid = true;
    printf("%s\n", input_buffer->input + strlen(command) + 1); // +1 to skip the space after "echo"
    return true;
  } else if(strncmp(command, "type", 4) == 0) {
    char *command2 = strtok(NULL, "");
    for(int i=0; i<NUM_BUILTINS; i++) {
      if(strncmp(command2, builtins[i], strlen(command2)) == 0) {
        printf("%s is a shell builtin\n", builtins[i]);
        input_buffer->is_valid = true;
        return true;
      }
    }
    // not a buildin command
    if(search_builtin(command2)) {
      input_buffer->is_valid = true;
      return true;
    }
    
    // If we reach here, the command is not a builtin
    printf("%s: not found\n", command2);
    input_buffer->is_valid = true;
    return true;
  }
  return false;
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
    
    char *command = strtok(input_buffer.input, " ");

    process_input(&input_buffer, command);
    if(!input_buffer.is_valid) {
      // Parse the input into arguments
      char *args[MAX_BUFFER_SIZE / 2 + 1];    // considering string/argument length atleast 2 characters
      int arg_count = 1;
      args[0] = command; // First argument is the command itself
      // char *token = strtok(input_buffer.input, " ");
      char* token = strtok(NULL, " ");
      while(token != NULL && arg_count < (MAX_BUFFER_SIZE / 2)) {
        
        args[arg_count] = token;
        arg_count++;
        // printf("Token: %s\n", token); // Debugging line to see the tokens
        // printf("input_buffer.input: %s\n", input_buffer.input); // Debugging line to see the input buffer
        token = strtok(NULL, " ");
      }
      // printf("Number of arguments: %d\n", arg_count); // Debugging line to see the number of arguments
      args[arg_count] = NULL; // Null-terminate the array of arguments

      if(args[0] == NULL) {
        printf("$ ");
        continue; // No command entered, prompt again
      }
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

      // printf("%s: command not found\n", input_buffer.input);
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