#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
// #include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>

#include <readline/readline.h>
#include <readline/history.h>
// #include <readline/keymaps.h>

#define MAX_BUFFER_SIZE 128
#define NUM_BUILTINS 6
#define CMD_PATH_MAX 100
#define MAX_COMMANDS 100    // piped commands
#define MAX_COMMAND_LENGTH 50
#define MAX_HISTORY_COMMANDS 100
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

int history_index = 0;
int last_append_ind = 0;
char *builtins[NUM_BUILTINS] = {"echo", "exit", "type", "pwd", "cd", "history"};
char **history;

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
  // free(path_copy);
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
    char *path = getenv("HISTFILE");
    FILE *file = fopen(path, "a");
    if(file != NULL) {
      for(int i=min(last_append_ind, history_index-1); i<history_index; i++) {
      // for(int i=0; i<history_index; i++) {
        fputs(history[i], file);
        fputs("\n", file);
      }
      last_append_ind = history_index;
      fclose(file);
      // for(int i=0; i<history_index; i++) {
      //   free(history[i]);
      // }
      // free(history);
    }
    exit(EXIT_SUCCESS);
  } else if(strncmp(command, "echo", 4) == 0) {
    char *echo_text = input_buffer + strlen(command);
    while (*echo_text == ' ') echo_text++;
    if(strchr(echo_text, '\'') == NULL && strchr(echo_text, '"') == NULL) {
      // char* token = strtok(echo_text, " ");
      bool first = true;
      while(*echo_text != '\0') {
        if(*echo_text == ' ') {
          if(!first) {
            printf(" ");
          }
          first = false;
          while(*echo_text != '\0' && *echo_text == ' ') {
            echo_text++;
          }
          while(*echo_text != '\0' && *echo_text != ' ') {
            printf("%c", *echo_text);
            echo_text++;
          }
        } else {
          if(!first) {
            printf(" ");
          }
          first = false;
          while(*echo_text != '\0' && *echo_text != ' ') {
            printf("%c", *echo_text);
            echo_text++;
          }
        }
        // remove quotes from arguments
        // if(!first) {
        //   printf(" ");
        // }
        // first = false;
        // printf(token);
        // token = strtok(NULL, " ");
      }
      printf("\n");
      return true;
    }
    // handle 'abc'
    if(*echo_text == '\'') {
      echo_text++;
      char *echo_text_copy = echo_text;
      int count = strlen(echo_text);
      while(echo_text_copy[count-1] == '\0' || echo_text_copy[count-1] == ' ' || echo_text_copy[count-1] == '1') {
        count--;
        echo_text_copy--;
      }
      echo_text[count-1] = '\0';
    }
    // handle "abc"
    if(*echo_text == '"') {
      int i=0, j=1;
      int count_quotes = 1;
      // char *echo_text_copy = echo_text;
      // remove front spaces
      // while(echo_text[j] == ' ') {
      //   j++;
      // }
      while(echo_text[j] != '\0') {
        if(echo_text[j] == '"') {
          count_quotes++;
          if(count_quotes%2 == 0) {
            while(echo_text[j] != '\0' && echo_text[j] == ' ') {
              j++;
            }
            continue;
          } else {
            echo_text[i] = ' ';
            i++;
          }
        } else {
          echo_text[i] = echo_text[j];
          i++;
        }
        j++;
      }
      echo_text[i] = '\0';
      i++;
      while(i<j) {
        echo_text[i] = '\0';
        i++;
      }
    }
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
  } else if(strncmp(command, "history", 7) == 0) {
    // check for the read from file
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
    if(arg_count <= 2) {
      // not reading from file
      char *count = NULL;
      if(arg_count > 1) {
        count = args[1];
      }
      if(count == NULL) {
        for(int i=0; i<history_index; i++) {
          printf("%d %s\n", i+1, history[i]);
        }
      } else {
        int num_hist = atoi(count);
        for(int i=max(history_index - num_hist, 0); i<history_index; i++) {
          printf("%d %s\n", i+1, history[i]);
        }
      }
      return true;
    }
    
    if(strncmp(args[1], "-r", 2) == 0) {
      // read from the file
      FILE *file = fopen(args[2], "r");
      if(file == NULL) {
        printf("errored while openeing th efile");
        perror("error opening the file");
        return true;
      }
      char line[100];
      while(fgets(line, sizeof(line), file)) {
        // printf("reading line = %s\n", line);
        line[strcspn(line, "\n")] = '\0';
        history[history_index] = malloc(strlen(line) * sizeof(char));
        strcpy(history[history_index], line);
        history_index++;
      }
      // printf("read from the file");
      fclose(file);
    } else if(strncmp(args[1], "-w", 2) == 0) {
      // write to the file
      FILE *file = fopen(args[2], "w");
      if(file == NULL) {
        perror("error opening the file");
        return true;
      }
      // char line[100];
      for(int i=0; i<history_index; i++) {
        fputs(history[i], file);
        fputs("\n", file);
      }
      fclose(file);
    } else if(strncmp(args[1], "-a", 2) == 0) {
      // append to the file
      FILE *file = fopen(args[2], "a");
      if(file == NULL) {
        perror("error opening the file");
        return true;
      }
      for(int i=min(last_append_ind, history_index-1); i<history_index; i++) {
      // for(int i=0; i<history_index; i++) {
        fputs(history[i], file);
        fputs("\n", file);
      }
      last_append_ind = history_index;
      fclose(file);
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
  // free(input_copy);
}

int hist_cursor = -1;  // -1 means at newest line (empty)

int my_up_arrow_handler(int count, int key) {
  if (history_index == 0) return 0;  // no history

  if (hist_cursor == -1)
    hist_cursor = history_index - 1;
  else if (hist_cursor > 0)
    hist_cursor--;

  rl_replace_line(history[hist_cursor], 1);
  rl_point = rl_end;
  return 0;
}

int my_tab_handler(int count, int key) {
  if(count == 1) {
    const char* curr = rl_line_buffer;
    int curr_len = strlen(rl_line_buffer);
    for(int i=0; i<NUM_BUILTINS; i++) {
      if(strlen(builtins[i]) > curr_len && strncmp(builtins[i], curr, strlen(curr)) == 0) {
        const char* new_char = " ";
        char *new_val = malloc(sizeof(char) * (strlen(builtins[i]) + 5));
        strcpy(new_val, builtins[i]);
        strcat(new_val, new_char);
        rl_replace_line(new_val, 1);
        rl_point = rl_end;
        return 0;
      }
    }
  }
  rl_point = rl_end;
  return 0;
}

int my_down_arrow_handler(int count, int key) {
  if (hist_cursor == -1) return 0;

  hist_cursor++;
  if (hist_cursor >= history_index) {
    hist_cursor = -1;
    rl_replace_line("", 0);
  } else {
    rl_replace_line(history[hist_cursor], 1);
  }
  rl_point = rl_end;
  return 0;
}
int saved_stdout = -1;
void saveStdout() {
  saved_stdout = dup(STDOUT_FILENO);
  if (saved_stdout == -1) {
    perror("dup");
  }
}

void restoreStdout() {
  if (saved_stdout != -1) {
    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);
    saved_stdout = -1;
  }
}

int saved_stderr = -1;
void saveStderr() {
  saved_stderr = dup(STDERR_FILENO);
  if (saved_stderr == -1) {
    perror("dup");
  }
}

void restoreStderr() {
  if (saved_stderr != -1) {
    dup2(saved_stderr, STDERR_FILENO);
    close(saved_stderr);
    saved_stderr = -1;
  }
}
// void restoreStdout() { freopen("/dev/tty", "w", stdout); }
int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);
  int curr_hist_ind = 0;
  
  history = malloc(MAX_HISTORY_COMMANDS * sizeof(char*));
  char *path = getenv("HISTFILE");
  FILE *file = fopen(path, "r");
  if(file != NULL) {
    char line[100];
    while(fgets(line, sizeof(line), file)) {
      // printf("reading line = %s\n", line);
      line[strcspn(line, "\n")] = '\0';
      history[history_index] = malloc(sizeof(line) * sizeof(char));
      strcpy(history[history_index], line);
      history_index++;
    }
    last_append_ind = history_index;
    // printf("read from the file");
    fclose(file);
  }
  

  // Uncomment this block to pass the first stage
  // printf("$ ");

  // Wait for user input
  char input[100];
  rl_bind_keyseq("\\e[A", my_up_arrow_handler);
  rl_bind_keyseq("\\e[B", my_down_arrow_handler);
  rl_bind_keyseq("\t", my_tab_handler);
  while(1) {
    InputBuffer input_buffer = CreateInputBuffer();
    // fgets(input_buffer.input, MAX_BUFFER_SIZE, stdin);
    char *line = readline("$ ");
    if (line == NULL) {
      break;  // Ctrl+D
    }
    strcpy(input_buffer.input, line);
    input_buffer.input_length = strlen(line);
    // input_buffer.input_length = strlen(input_buffer.input);
    // input_buffer.input[input_buffer.input_length] = '\0'; // Remove the trailing newline
    input_buffer.is_valid = false;
    history[history_index] = malloc(input_buffer.input_length * sizeof(char));
    strcpy(history[history_index++], input_buffer.input);

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
      // for(int i=0; i<num_cmds; i++) {
      //   free(commands[i]);
      // }
      // free(commands);
    } else if(strchr(input_buffer.input, '>') != NULL) {
      int orig_command_len = 0;
      char *input_buffer_copy = input_buffer.input;
      // while (*echo_text == ' ') echo_text++;
      char *next_ptr = input_buffer_copy;
      while (*next_ptr != '>') {
        next_ptr++;
        orig_command_len++;
      }
      //double >>
      if(*(next_ptr+1) == '>') {
        if(*(next_ptr-1) == '1') {
          char *orig =malloc(orig_command_len * sizeof(char));
          strncpy(orig, input_buffer.input, orig_command_len-1);
          orig[orig_command_len-1] = '\0';
          
          while(*next_ptr == '>' || *next_ptr == ' ') next_ptr++;
          char *filename = next_ptr;
          saveStdout();
          FILE *file = fopen(filename, "a");
          dup2(fileno(file), STDOUT_FILENO);
          processCommands(orig, true);
          fclose(file);
          restoreStdout();
          continue;
          // free(orig);
        } if(*(next_ptr-1) == '2') {
          char *orig =malloc(orig_command_len * sizeof(char));
          strncpy(orig, input_buffer.input, orig_command_len-1);
          orig[orig_command_len-1] = '\0';
          
          while(*next_ptr == '>' || *next_ptr == ' ') next_ptr++;
          char *filename = next_ptr;
          saveStderr();
          FILE *file = fopen(filename, "a");
          dup2(fileno(file), STDERR_FILENO);
          processCommands(orig, true);
          fclose(file);
          restoreStderr();
          free(orig);
        } else {
          char *orig =malloc(orig_command_len+1 * sizeof(char));
          strncpy(orig, input_buffer.input, orig_command_len);
          orig[orig_command_len] = '\0';
          
          while(*next_ptr == '>' || *next_ptr == ' ') next_ptr++;
          char *filename = next_ptr;
          saveStdout();
          FILE *file = fopen(filename, "a");
          dup2(fileno(file), STDOUT_FILENO);
          processCommands(orig, true);
          fclose(file);
          restoreStdout();
          free(orig);
          // case >>
        }
      } else {
        //single >
        if(*(next_ptr-1) == '1') {
          // case 1>
          char *orig =malloc(orig_command_len * sizeof(char));
          strncpy(orig, input_buffer.input, orig_command_len-1);
          orig[orig_command_len-1] = '\0';
          
          while(*next_ptr == '>' || *next_ptr == ' ') next_ptr++;
          char *filename = next_ptr;
          saveStdout();
          FILE *file = fopen(filename, "w");
          dup2(fileno(file), STDOUT_FILENO);
          processCommands(orig, true);
          fclose(file);
          restoreStdout();
          free(orig);
        } else if(*(next_ptr-1) == '2') {
          // case 2>
          char *orig =malloc(orig_command_len * sizeof(char));
          strncpy(orig, input_buffer.input, orig_command_len-1);
          orig[orig_command_len-1] = '\0';
          
          while(*next_ptr == '>' || *next_ptr == ' ') next_ptr++;
          char *filename = next_ptr;
          saveStderr();
          FILE *file = fopen(filename, "w");
          dup2(fileno(file), STDERR_FILENO);
          processCommands(orig, true);
          fclose(file);
          restoreStderr();
          free(orig);
        } else {
          char *orig =malloc(orig_command_len+1 * sizeof(char));
          strncpy(orig, input_buffer.input, orig_command_len);
          orig[orig_command_len] = '\0';
          while(*next_ptr == '>' || *next_ptr == ' ') next_ptr++;
          char *filename = next_ptr;
          saveStdout();
          FILE *file = fopen(filename, "w");
          dup2(fileno(file), STDOUT_FILENO);
          processCommands(orig, true);
          fclose(file);
          restoreStdout();
          free(orig);
        }
      }
    } else {
      processCommands(input_buffer.input, true);
    }
    // if(line != NULL) {
    //   free(line);
    // }
  }
  return 0;
}