#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);

  // Uncomment this block to pass the first stage
  // printf("$ ");

  // Wait for user input
  char input[100];

  char builtins[2][10] = {"echo", "exit"};
  while(1) {

    printf("$ ");
    fgets(input, 100, stdin);
    // Remove the trailing newline
    input[strlen(input) - 1] = '\0';
    // Print the input back to the user
    if(strncmp(input, "exit", 4) == 0) {
      // printf("Exiting...\n");
      break;
    }

    // handling the commands
    // echo command
    // If the input starts with "echo ", print the rest of the input
    if(strncmp(input, "echo ", 5) == 0) {
      printf("%s\n", input + 5);
      continue;
    }
    // type command
    // If the input starts with "type ", print the rest of the input
    if(strncmp(input, "type ", 5) == 0) {
      if(argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        continue;
      }
      for(int i=0; i<2; i++) {
        if(strcmp(input + 5, builtins[i]) == 0) {
          printf("%s is a built-in command\n", builtins[i]);
          continue;
        }
      }
    }
    printf("%s: command not found\n", input);
  }
  return 0;
}
