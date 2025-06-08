#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);

  // Uncomment this block to pass the first stage
  printf("$ ");

  // Wait for user input
  char input[100];
  while(1) {
    fgets(input, 100, stdin);
    // Remove the trailing newline
    input[strlen(input) - 1] = '\0';
    // Print the input back to the user
    printf("%s: command not found\n", input);
  }
  return 0;
}
