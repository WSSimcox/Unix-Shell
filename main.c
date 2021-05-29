#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <wait.h>
#include <fcntl.h>
#include <stdlib.h>

// Created by William Simcox

#define MAX_LINE 80 // The maximum length command

/* Function for creating a new process */
void createProcess(char* args[], int ampersand)
{

  pid_t pid; // process id created
  pid = fork();

  if (pid == 0) // Checking if child process
  {
    execvp(args[0], args); // Runs input command
  }
  else if (pid > 0)
  {
    if (ampersand == 0) // If no & present
    {
      wait(NULL);
    }
  }
  else
  {
    printf("ERROR: Failed to Fork.\n");
  }
}

/* Main which simulates a simple UNIX shell */
int main(void)
{
  int check = 0; // Used for breaking out of while loop when token is NULL
  char *args[MAX_LINE/2 + 1] = {}; // command line arguments
  char input[MAX_LINE + 1];
  char input_history[MAX_LINE + 1] = {};
  int should_run = 1; // flag to determine when to exit program
  char* token;
  int ampersand = 0; // int that acts as bool if & is present

  while (should_run)
  {
    printf("OSH> ");
    fflush(stdout);

    fgets(input, MAX_LINE, stdin); // Taking in user input
    //printf("input: %s\n", input); // Printing user cmds

    input[strlen(input) - 1] = '\0'; // Setting NULL terminating character

    if ((strcmp(input, "!!") == 0) && input_history[0] != '\0') // If history is present
    {
      memcpy(input, input_history, MAX_LINE + 1); // Change !! to old arg
      printf("%s\n", input); // Printing user cmds // Print old arg
    }
    else
    {
      if (strcmp(input, "!!") != 0)
      {
        memcpy(input_history, input, MAX_LINE + 1); // input_history updates
      }
      if ((strcmp(input, "!!") == 0) && input_history[0] == '\0') // If no history present
      {
        printf("No commands in history.\n");
      }
    }

    token = strtok(input, " "); // Creating first token
    int i = 0;

    int ampersand_index = -1; // For

    while (token != NULL)  // Parsing input
    {
      if (strcmp(token, "exit") == 0) // If exit, then program ends
      {
        should_run = 0;
        break;
      }

      if (strcmp(token, ">") == 0) // Checking for output
      {
        token = strtok(NULL, " "); // Step forward
        char* output = strdup(token); // Output file name

        int file = open(token, O_WRONLY | O_CREAT, 0777);
        if (file == -1) // error check
        {
          return 2; // returns error
        }
        else
        {
          dup2(file, STDOUT_FILENO); // Creates copy of a file descriptor
          close(file); // Closes file
        }
        break;
      }

      if (strcmp(token, "<") == 0) // Checking for input
      {
        token = strtok(NULL, " "); // Step forward
        int file_input = dup(STDIN_FILENO);

        int file = open(token, O_RDONLY);
        if (file == -1)
        {
          return 2; // returns error
        }
        else
        {
          dup2(file_input, STDOUT_FILENO); // Creates copy of a file descriptor
          close(file); // Closes file
        }
      }

      if (strcmp(token, "|") == 0) // Checking for pipe
      {
        token = strtok(NULL, " "); // Step foward

        int fd[2]; // file descriptor
        int stdIn = -1;
        int stdOut = -1;

        if (pipe(fd) == -1) // Creating pipe
        {
          printf("ERROR: Unable to create pipe!");
          exit(0);
        }

        char* first[MAX_LINE/2 + 1] = {}; // First arg
        char* second[MAX_LINE/2 + 1] = {}; // Second arg

        int pipe_location = i; // Pipe's location in array

        for (int j = 0; j < pipe_location; j++) // Setting for arg
        {
          first[j] = args[j];
        }

        for (int j = 0; j < pipe_location; j++) // Setting second arg
        {
          second[j] = token; // Set second to token arg
          token = strtok(NULL, " "); // Step forward
          if (token == NULL) // Checking if there is a next arg
          {
            break;
          }
        }

        pid_t pid; // process id created
        pid = fork();

        if (pid < 0)
        {
          printf("ERROR: Failed to Fork.\n");
          return 2; // Return error
        }
        else if (pid == 0) // Child process
        {
          stdOut = dup(STDOUT_FILENO);
          close(fd[0]);
          dup2(fd[1], STDOUT_FILENO);
          close(fd[1]);
          createProcess(first, ampersand);
          exit(0);
        }
        else // Parent process
        {
          stdIn = dup(STDIN_FILENO);
          wait(NULL);
          close(fd[1]);
          dup2(fd[0], STDIN_FILENO);
          close(fd[0]);
          createProcess(second, ampersand);
        }
        check = 1;
        close(fd[0]);
        close(fd[1]);
        dup2(stdIn, STDIN_FILENO);
        close(stdIn);
        dup2(stdOut, STDIN_FILENO);
        close(stdOut);
      }

      if (check != 0)
      {
        break;
      }

      if (strcmp(token, ";") == 0) // Disabling &
      {
          ampersand = 0; // & = false
          break;
      }

      if (strcmp(token, "&") == 0) // Checking for '&'
      {
        ampersand = 1; // Ampersand = true
        ampersand_index = i; // Index recorded
      }
      else
      {
        args[i] = strdup(token);
      }
      token = strtok(NULL, " ");
      i++;
    }

    if (check != 0)
    {
      check = 0;
      memset(input, NULL, MAX_LINE + 1);
      memset(args, NULL, MAX_LINE/2 + 1); // Clears args
      continue;
    }

    if (ampersand_index != -1)
    {
      createProcess(args + ampersand_index + 1, ampersand);
    }
    createProcess(args, ampersand);

    memset(args, NULL, MAX_LINE/2 + 1); // Clears args
  }
  return 0;
}
