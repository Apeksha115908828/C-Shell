#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
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
bool process_input(InputBuffer *input_buffer) {
  char *command = strtok(input_buffer->input, " ");
  input_buffer->input[input_buffer->input_length - 1] = '\0'; // Remove the trailing newline

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
    input[strlen(input) - 1] = '\0';
    input_buffer.input_length = strlen(input_buffer.input);
    input_buffer.is_valid = false;

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
    process_input(&input_buffer);
    if(!input_buffer.is_valid) {
      printf("%s: command not found\n", input_buffer.input);
    }
    printf("$ ");
  }
  return 0;
}
