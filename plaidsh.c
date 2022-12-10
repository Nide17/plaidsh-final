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

typedef struct command_s
{
  char *in_file;  // if non-NULL, the filename to read input from
  char *out_file; // if non-NULL, the filename to send output to
  int argv_cap;   // current length of argv; different from argc!
  char **argv;    // the actual argv vector
} command_t;

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
  if ((cmd->argv_cap == 1 && strcmp(cmd->argv[0], "exit") == 0) || (strcmp(cmd->argv[0], "quit") == 0))
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
  if (cmd->argv_cap == 1 && strcmp(cmd->argv[0], "author") == 0)
  {
    printf("Niyomwungeri Parmenide Parmenide\n");
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
  if (cmd->argv_cap == 2 && strcmp(cmd->argv[0], "cd") == 0)
  {
    // CHANGE DIRECTORY TO ARG 1
    if (chdir(cmd->argv[1]) == 0)
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
  if (cmd->argv_cap == 1 && strcmp(cmd->argv[0], "pwd") == 0)
  {
    char currentDir[1024];
    getcwd(currentDir, sizeof(currentDir));

    // PRINT THE CURRENT WORKING DIRECTORY
    printf("%s \n", currentDir);
    return 0;
  }

  // IF ERROR, RETURN 1
  return 1;
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
    execvp(cmd->argv[0], cmd->argv);
    exit(0);
  }

  // IF CHILD PROCESS, WAIT FOR IT TO FINISH
  else if (pid > 0)
  {
    int status;
    waitpid(pid, &status, 0);
    return WEXITSTATUS(status);
  }
  // IF ERROR, COMMAND FAILED - RETURN -1
  else
  {
    fprintf(stderr, "Child %d exited with status %d \n", pid, WEXITSTATUS(pid));
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
  assert(cmd->argv_cap >= 1);
  // EXECUTING THE exit, quit COMMAND
  if (strcmp(cmd->argv[0], "exit") == 0 || strcmp(cmd->argv[0], "quit") == 0)
    builtin_exit(cmd);

  // EXECUTING THE author COMMAND
  else if (strcmp(cmd->argv[0], "author") == 0)
    builtin_author(cmd);

  // EXECUTING cd COMMAND
  else if (strcmp(cmd->argv[0], "cd") == 0)
    builtin_cd(cmd);

  // EXECUTING THE pwd COMMAND
  else if (strcmp(cmd->argv[0], "pwd") == 0)
    builtin_pwd(cmd);

  // EXECUTING EXTERNAL COMMANDS (NOT BUILT-IN)
  else
    forkexec_external_cmd(cmd);
}

/*
 * The main loop for the shell.
 */
void mainloop()
{
  char *input = NULL;
  char argv[MAX_ARGS];
  command_t *cmd = NULL;

  fprintf(stdout, "Welcome to Plaid Shell!\n");
  const char *prompt = "#> ";

  // CONNECTING THE readline, read_word, AND parse_input IN A LOOP
  while (1)
  {

    // GETTING THE INPUT FROM THE USER
    input = readline(prompt);

    // SAVING THE INPUT TO HISTORY
    add_history(input);

    // IF NO INPUT, KEEP PROMPTING
    if (input == NULL)
      exit(0);

    if (*input == '\0')
      continue;

    // GETTING AND PARSING THE INPUT
    cmd = parse_input(input, argv, MAX_ARGS);

    // ARGUMENTS ERROR
    if (cmd->argv_cap == -1)
      printf(" Error: %s\n", cmd->argv[0]);

    // THE ARGUMENTS ARE OK - RUN THE COMMAND
    else
      execute_command(cmd);
  }
}

int main(int argc, char *argv[])
{
  mainloop();
  return 0;
}
