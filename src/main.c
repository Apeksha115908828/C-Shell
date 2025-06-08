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
  while(1) {

    printf("$ ");
    fgets(input, 100, stdin);
    // Remove the trailing newline
    input[strlen(input) - 1] = '\0';
    // Print the input back to the user
    if(strcmp(input, "exit") == 0) {
      printf("Exiting...\n");
      break;
    }

    // handling the commands
    // echo command
    // If the input starts with "echo ", print the rest of the input
    if(strncmp(input, "echo ", 5) == 0) {
      printf("%s\n", input + 5);
      continue;
    }
    printf("%s: command not found\n", input);
  }
  return 0;
}
