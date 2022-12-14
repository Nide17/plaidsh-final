/*
 * plaidsh.c
 *
 * A small shell
 *
 * Author: Howdy Pierce <howdy@cardinalpeak.com>
 * Co-Author: Niyomwungeri Parmenide ISHIMWE <parmenin@andrew.cmu.edu>
 */

#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <assert.h>

#include "parser.h"
#include "command.h"

#define MAX_ARGS 20

/*
 * Handles the exit or quit commands, by exiting the shell. Does not
 * return.
 *
 * Parameters:
 *   argc     The length of the argv vector
 *   argv[]   The argument vector
 */
static int builtin_exit(command_t *cmd)
{
  // ONE ARG AND IS exit OR quit
  if (command_get_argc(cmd) == 1 && (strcmp(command_get_argv(cmd)[0], "exit") == 0 || strcmp(command_get_argv(cmd)[0], "quit") == 0))

    // EXIT THE SHELL
    exit(0);

  return 0;
}

/*
 * Handles the author command, by printing the author of this program
 * to stdout
 *
 * Parameters:
 *   argc     The length of the argv vector
 *   argv[]   The argument vector, which is ignored
 *
 * Returns:
 *   0 on success, 1 on failure
 */
static int builtin_author(command_t *cmd)
{
  // THERE IS ONE ARG AND IS author
  if (command_get_argc(cmd) == 1 && strcmp(command_get_argv(cmd)[0], "author") == 0)
  {
    // DEFINE THE OUTPUT FILENAME
    const char *outFile = command_get_output(cmd);

    // IF FILE DEFINED, REDIRECT STDOUT TO FILE
    if (outFile != NULL)
    {
      freopen(outFile, "w", stdout);
    }

    // ELSE PRINT TO STDOUT
    printf("Niyomwungeri Parmenide ISHIMWE\n");

    // IF FILE DEFINED, REDIRECT STDOUT BACK TO TERMINAL
    if (outFile != NULL)
    {
      freopen("/dev/tty", "w", stdout);
    }

    // SUCCESS
    return 0;
  }

  // FAILURE
  return 1;
}

/*
 * Handles the cd builtin, by setting cwd to argv[1], which must exist.
 *
 * Parameters:
 *   argc     The length of the argv vector
 *   argv[]   The argument vector, which must have either 1 or 2 arguments.
 *
 * Returns:
 *   0 on success, 1 on failure
 */

static int builtin_cd(command_t *cmd)
{
  if (command_get_argc(cmd) == 1 && strcmp(command_get_argv(cmd)[0], "cd") == 0)
  {
    // CHANGE DIRECTORY TO HOME IF NO ARG TO cd
    if (chdir(getenv("HOME")) == 0)
      return 0;
  }
  else if (command_get_argc(cmd) == 2 && strcmp(command_get_argv(cmd)[0], "cd") == 0)
  {
    // CHANGE DIRECTORY TO ARG 1
    if (chdir(command_get_argv(cmd)[1]) == 0)
      return 0;
  }

  // cd COMMAND FAILED
  return 1;
}

/*
 * Handles the pwd builtin, by printing the cwd to the supplied file
 * descriptor
 *
 * Parameters (which are all ignored):
 *   argc     The length of the argv vector
 *   argv[]   The argument vector
 *
 * Returns:
 *   Always returns 0, since it always succeeds
 */
static int builtin_pwd(command_t *cmd)
{
  // THERE IS ONE ARG AND IS THE pwd
  if (command_get_argc(cmd) == 1 && strcmp(command_get_argv(cmd)[0], "pwd") == 0)
  {
    char currentDir[1024];
    // DEFINE THE OUTPUT FILENAME
    const char *outFile = command_get_output(cmd);

    // GET THE CURRENT WORKING DIRECTORY
    getcwd(currentDir, sizeof(currentDir));

    // IF FILE DEFINED, REDIRECT STDOUT TO FILE
    if (outFile != NULL)
    {
      freopen(outFile, "w", stdout);
    }

    // PRINT THE CURRENT WORKING DIRECTORY
    printf("%s \n", currentDir);

    // IF FILE DEFINED, REDIRECT STDOUT BACK TO TERMINAL
    if (outFile != NULL)
    {
      freopen("/dev/tty", "w", stdout);
    }

    return 0;
  }

  // IF ERROR, RETURN 1
  return 1;
}

/*
 * Handles the builtin environment setting, by setting the variable varname to value
 * by using the setenv() function.
 *
 * Parameters:
 *   varname     The variable name to set
 *   value       The value for the variable
 *
 * Returns:
 *   Returns 0 on success, 1 on error
 */
static int builtin_setenv(command_t *cmd)
{

  // THERE IS ONE ARG AND IS THE setenv || THERE IS TWO ARGS - THE setenv & VARNAME - NO VALUE
  if ((command_get_argc(cmd) == 1 && strcmp(command_get_argv(cmd)[0], "setenv") == 0) || (command_get_argc(cmd) == 2 && strcmp(command_get_argv(cmd)[0], "setenv") == 0))
  {
    fprintf(stderr, "Illegal variable name: '%s'\n", command_get_argv(cmd)[1]);
    return 1;
  }

  // THERE IS THREE ARGS AND IS THE setenv - SET THE ENVIRONMENT VARIABLE TO THE THIRD ARG
  else if (command_get_argc(cmd) == 3 && strcmp(command_get_argv(cmd)[0], "setenv") == 0)
  {
    // SET THE ENVIRONMENT VARIABLE
    setenv(command_get_argv(cmd)[1], command_get_argv(cmd)[2], 1);
    return 0;
  }

  // WHEN ARGS ARE GREATER THAN 3
  else if (command_get_argc(cmd) > 3 && strcmp(command_get_argv(cmd)[0], "setenv") == 0)
  {
    fprintf(stderr, "Invalid syntax for: '%s'\n", command_get_argv(cmd)[0]);
    return 1;
  }

  return 0;
}

/*
 * Process an external (non built-in) command, by forking and execing
 * a child process, and waiting for the child to terminate
 *
 * Parameters:
 *   argc      The length of the argv vector
 *   argv[]    The argument vector
 *
 * Returns:
 *   The child's exit value, or -1 on error
 */
static int forkexec_external_cmd(command_t *cmd)
{
  // FORKING THE PROCESS
  pid_t pid = fork();

  // IF PARENT PROCESS - EXECUTE IT
  if (pid == 0)
  {
    // EXECUTE THE COMMAND IF IT EXISTS
    if (execvp(command_get_argv(cmd)[0], command_get_argv(cmd)) == -1)
    {
      fprintf(stderr, "Command not found: '%s'\n", command_get_argv(cmd)[0]);
      exit(-1);
    }

    // EXIT
    exit(0);
  }

  // IF CHILD PROCESS, WAIT FOR IT TO FINISH
  else if (pid > 0)
  {
    int status;

    // WAIT FOR THE CHILD PROCESS TO TERMINATE
    waitpid(pid, &status, 0);
    return WEXITSTATUS(status);
  }

  // IF ERROR, COMMAND FAILED - RETURN -1
  else
  {
    fprintf(stdout, "Child %d exited with status %d \n", pid, WEXITSTATUS(pid));
    return -1;
  }
}

/*
 * Parses one input line, and executes it
 *
 * Parameters:
 *   argc     The length of the argv vector, which must be >= 1
 *   argv[]   The argument vector
 */
void execute_command(command_t *cmd)
{

  // ASSERT THAT THERE IS AT LEAST ONE ARGUMENT
  assert(command_get_argc(cmd) >= 1);

  // EXECUTING THE exit, quit COMMAND
  if (strcmp(command_get_argv(cmd)[0], "exit") == 0 || strcmp(command_get_argv(cmd)[0], "quit") == 0)
    builtin_exit(cmd);

  // EXECUTING THE author COMMAND
  else if (strcmp(command_get_argv(cmd)[0], "author") == 0)
    builtin_author(cmd);

  // EXECUTING cd COMMAND
  else if (strcmp(command_get_argv(cmd)[0], "cd") == 0)
    builtin_cd(cmd);

  // EXECUTING THE pwd COMMAND
  else if (strcmp(command_get_argv(cmd)[0], "pwd") == 0)
    builtin_pwd(cmd);

  // EXECUTING THE setenv COMMAND
  else if (strcmp(command_get_argv(cmd)[0], "setenv") == 0)
    builtin_setenv(cmd);

  // EXECUTING EXTERNAL COMMANDS (NOT BUILT-IN)
  else
    forkexec_external_cmd(cmd);
}

/*
 * The main loop for the shell.
 */
void mainloop()
{
  char argv[MAX_ARGS];
  char *inp = NULL;

  // PRINTING THE WELCOME MESSAGE AND PROMPT TO THE USER ON THE SCREEN
  fprintf(stdout, "Welcome to Plaid Shell!\n");
  const char *userInput = "#> ";

  // CONNECTING THE readline, read_word, AND parse_input IN A LOOP
  while (1)
  {

    // GETTING THE INPUT FROM THE USER
    inp = readline(userInput);

    // SAVING THE INPUT TO HISTORY
    add_history(inp);

    // IF NO INPUT, KEEP PROMPTING
    if (inp == NULL)
      exit(0);

    if (*inp == '\0')
      continue;

    // GETTING AND PARSING THE INPUT, EXECUTING THE COMMAND
    execute_command(parse_input(inp, argv, sizeof(argv)));
  }
}

int main(int argc, char *argv[])
{
  mainloop();
  return 0;
}
