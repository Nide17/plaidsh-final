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

/* *************************************************************************************************** */
/*
 * Handles the exit or quit commands, by exiting the shell. Does not
 * return.
 *
 * Parameters:
 *   argc     The length of the argv vector
 *   argv[]   The argument vector
 */
int builtin_exit(command_t *cmd)
{
  // ONE ARG AND IS exit OR quit
  if (command_get_argc(cmd) == 1 && (strcmp(command_get_argv(cmd)[0], "exit") == 0 || strcmp(command_get_argv(cmd)[0], "quit") == 0))
    return 0;

  return 0;
}

// Test for builtin_exit function once
bool test_builtin_exit_once(command_t *cmd, int expected)
{
  // CALL THE FUNCTION BUT DO NOT EXIT THE SHELL
  int actualExit = builtin_exit(cmd);

  // IF THE EXIT CODE IS AS EXPECTED
  if (actualExit == expected)
    return true;

  // ELSE PRINT ERROR MESSAGE
  printf("Test failed for builtin_exit function with exit code %d instead of %d for command %s\n", actualExit, expected, command_get_argv(cmd)[0]);
  return false;
}

// Tests for builtin_exit function
bool test_builtin_exit()
{
  int passed = 0;
  command_t *cmd = command_new();
  command_append_arg(cmd, "exit");
  if (test_builtin_exit_once(cmd, 0))
    passed++;

  command_t *cmd1 = command_new();
  command_append_arg(cmd1, "quit");
  if (test_builtin_exit_once(cmd1, 0))
    passed++;

  return passed == 2;
}

/* *************************************************************************************************** */
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
int builtin_author(command_t *cmd)
{
  // THERE IS ONE ARG AND IS author
  if (command_get_argc(cmd) == 1 && strcmp(command_get_argv(cmd)[0], "author") == 0)
  {
    // DEFINE THE OUTPUT FILENAME
    const char *outFile = command_get_output(cmd);

    // IF FILE DEFINED, REDIRECT STDOUT TO FILE
    if (outFile != NULL)
      freopen(outFile, "a", stdout);

    // ELSE PRINT TO STDOUT
    printf("Niyomwungeri Parmenide ISHIMWE\n");

    // IF FILE DEFINED, REDIRECT STDOUT BACK TO TERMINAL
    if (outFile != NULL)
      freopen("/dev/tty", "w", stdout);

    // SUCCESS
    return 0;
  }

  // FAILURE
  return 1;
}

// Tests one test case of the builtin_author function
bool test_builtin_author_once(command_t *cmd, int expected)
{
  int actualAuthor = builtin_author(cmd);
  if (actualAuthor != expected)
  {
    printf("builtin_author(%s) returned %d, expected %d  \n", command_get_argv(cmd)[0], actualAuthor, expected);
    return false;
  }

  return true;
}

// Tests the builtin_author function
bool test_builtin_author()
{
  int passed = 0;
  command_t *cmd = command_new();
  command_append_arg(cmd, "author");
  if (test_builtin_author_once(cmd, 0))
    passed++;

  command_t *cmd1 = command_new();
  command_append_arg(cmd1, "Niyomwungeri Parmenide ISHIMWE");
  if (test_builtin_author_once(cmd, 0))
    passed++;

  return passed == 2;
}

/* *************************************************************************************************** */
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

int builtin_cd(command_t *cmd)
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

// Tests one test case of the builtin_cd function
bool test_builtin_cd_once(command_t *cmd, int expected)
{
  int actualCd = builtin_cd(cmd);
  if (actualCd != expected)
  {
    printf("builtin_cd(%s) returned %d, expected %d  \n", command_get_argv(cmd)[0], actualCd, expected);
    return false;
  }

  return true;
}

// Tests the builtin_cd function
bool test_builtin_cd()
{
  int passed = 0;

  command_t *cmd1 = command_new();
  command_append_arg(cmd1, "cd");
  command_append_arg(cmd1, "/does/not/exist");
  if (test_builtin_cd_once(cmd1, 1))
    passed++;

  command_t *cmd = command_new();
  command_append_arg(cmd, "cd");
  command_append_arg(cmd, "~");
  if (test_builtin_cd_once(cmd, 1))
    passed++;

  return passed == 2;
}

/* *************************************************************************************************** */
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
int builtin_pwd(command_t *cmd)
{
  // THERE IS ONE ARG AND IS THE pwd
  if (command_get_argc(cmd) >= 1 && strcmp(command_get_argv(cmd)[0], "pwd") == 0)
  {
    char currentDir[1024];
    // DEFINE THE OUTPUT FILENAME
    const char *outFile = command_get_output(cmd);

    // GET THE CURRENT WORKING DIRECTORY
    getcwd(currentDir, sizeof(currentDir));

    // IF FILE DEFINED, REDIRECT STDOUT TO FILE
    if (outFile != NULL)
    {
      freopen(outFile, "a", stdout);
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

// Tests one test case of the builtin_pwd function
bool test_builtin_pwd_once(command_t *cmd, int expected)
{
  int actualPwd = builtin_pwd(cmd);
  if (actualPwd != expected)
  {
    printf("builtin_pwd(%s) returned %d, expected %d  \n", command_get_argv(cmd)[0], actualPwd, expected);
    return false;
  }

  return true;
}

// Tests the builtin_pwd function
bool test_builtin_pwd()
{
  int passed = 0;
  command_t *cmd = command_new();
  command_append_arg(cmd, "pwd");
  if (test_builtin_pwd_once(cmd, 0))
    passed++;

  command_t *cmd1 = command_new();
  command_append_arg(cmd1, "pwd");
  command_append_arg(cmd1, "/check/this/also");
  if (test_builtin_pwd_once(cmd1, 0))
    passed++;

  return passed == 2;
}

/* *************************************************************************************************** */
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
int builtin_setenv(command_t *cmd)
{
  // ILLEGAL NUMBER OF ARGS
  if (command_get_argc(cmd) != 3 && strcmp(command_get_argv(cmd)[0], "setenv") == 0)
  {
    fprintf(stderr, "usage: setenv varname value (%d args provided, expected 3)\n", command_get_argc(cmd));
    return 1;
  }

  // THERE IS THREE ARGS AND IS THE setenv - SET THE ENVIRONMENT VARIABLE TO THE THIRD ARG
  else if (command_get_argc(cmd) == 3 && strcmp(command_get_argv(cmd)[0], "setenv") == 0)
  {
    // SET THE ENVIRONMENT VARIABLE
    setenv(command_get_argv(cmd)[1], command_get_argv(cmd)[2], 1);
    return 0;
  }

  return 0;
}

// Tests one test case of the builtin_setenv function
bool test_builtin_setenv_once(command_t *cmd, int expected)
{
  int actualSetenv = builtin_setenv(cmd);
  if (actualSetenv != expected)
  {
    printf("builtin_setenv(%s, %s) returned %d, expected %d  \n", command_get_argv(cmd)[1], command_get_argv(cmd)[2], actualSetenv, expected);
    return false;
  }

  return true;
}

// Tests the builtin_setenv function
bool test_builtin_setenv()
{
  int passed = 0;
  command_t *cmd = command_new();
  command_append_arg(cmd, "setenv");
  command_append_arg(cmd, "VARNAME");
  command_append_arg(cmd, "value");
  if (test_builtin_setenv_once(cmd, 0))
    passed++;

  command_t *cmd1 = command_new();
  command_append_arg(cmd1, "setenv");
  command_append_arg(cmd1, "VARNAME");
  command_append_arg(cmd1, "");
  if (test_builtin_setenv_once(cmd1, 0))
    passed++;

  return passed == 2;
}

/* *************************************************************************************************** */
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
int forkexec_external_cmd(command_t *cmd)
{
  // FORKING THE PROCESS
  pid_t pid = fork();
  int status;

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
    // WAIT FOR THE CHILD PROCESS TO TERMINATE
    waitpid(pid, &status, 0);
    return WEXITSTATUS(status);
  }

  // CHECK THIS OUT
  if (WEXITSTATUS(status) != 0)
  {
    fprintf(stderr, "Child %d exited with status %d \n", pid, WEXITSTATUS(status));
    return -1;
  }

  // IF ERROR, COMMAND FAILED - RETURN -1
  else
  {
    fprintf(stdout, "Child %d exited with status %d \n", pid, WEXITSTATUS(pid));
    return -1;
  }
}

// Tests one test case of the forkexec_external_cmd function
bool test_forkexec_external_cmd_once(command_t *cmd, int expected)
{
  int actualForkExec = forkexec_external_cmd(cmd);
  if (actualForkExec != expected)
  {
    printf("forkexec_external_cmd(%s) returned %d, expected %d  \n", command_get_argv(cmd)[0], actualForkExec, expected);
    return false;
  }

  return true;
}

// Tests the forkexec_external_cmd function
bool test_forkexec_external_cmd()
{
  int passed = 0;
  command_t *cmd = command_new();
  command_append_arg(cmd, "ls");
  command_append_arg(cmd, "-l");
  if (test_forkexec_external_cmd_once(cmd, 0))
    passed++;

  command_t *cmd1 = command_new();
  command_append_arg(cmd1, "grep");
  command_append_arg(cmd1, "pattern");
  command_append_arg(cmd1, "doesntexisat.txt");
  if (test_forkexec_external_cmd_once(cmd1, 2))
    passed++;

  return passed == 2;
}

/* *************************************************************************************************** */
/*
 * Parses one input line, and executes it
 *
 * Parameters:
 *   argc     The length of the argv vector, which must be >= 1
 *   argv[]   The argument vector
 */
void execute_command(command_t *cmd)
{

  // IF THERE IS AT LEAST ONE ARGUMENT
  if (command_get_argc(cmd) >= 1)
  {

    // EXECUTING THE exit, quit COMMAND
    if (strcmp(command_get_argv(cmd)[0], "exit") == 0 || strcmp(command_get_argv(cmd)[0], "quit") == 0)
    {
      builtin_exit(cmd);
      exit(0);
    }
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

  else
    fprintf(stderr, "Error: Undefined variable \"  \"!\n");
}

bool test_execute_command_once(command_t *cmd)
{
  execute_command(cmd);
  return true;
}

static bool test_execute_command()
{
  int passed = 0;

  // Test external command
  command_t *cmd = command_new();
  command_append_arg(cmd, "ls");
  command_append_arg(cmd, "-l");
  if (test_execute_command_once(cmd))
    passed++;

  // Test another external command
  command_t *cmd1 = command_new();
  command_append_arg(cmd1, "grep");
  command_append_arg(cmd1, "pattern");
  command_append_arg(cmd1, "file.txt");
  if (test_execute_command_once(cmd1))
    passed++;

  // Test built-in command
  command_t *cmd2 = command_new();
  command_append_arg(cmd2, "author");
  if (test_execute_command_once(cmd2))
    passed++;

  return passed == 3;
}

/* *************************************************************************************************** */
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

/* *************************************************************************************************** */
int main(int argc, char *argv[])
{

  printf("RUNNING TESTS FOR BUILTIN FUNCTIONS - IN plaidsh.c\n\n");
  int success = 1;

  success &= test_builtin_author();
  success &= test_builtin_cd();
  success &= test_builtin_pwd();
  success &= test_builtin_setenv();
  success &= test_forkexec_external_cmd();
  success &= test_execute_command();
  success &= test_builtin_exit();

  if (success)
    printf("EXCELLENT WORK! ALL plaidsh.c TESTS SUCCEEDED!\n\n\n");

  else
    printf("NOTE: FAILURES OCCURRED IN plaidsh.c\n\n\n");

  mainloop();
  return 0;
}
