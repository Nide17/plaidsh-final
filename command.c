/*
 * command.c
 * 
 * Code to manipulate commands, used by plaidsh
 *
 * Author: Howdy Pierce <howdy@cardinalpeak.com>
 */

#include <assert.h>             // assert
#include <stdlib.h>             // free/malloc
#include <stdio.h>              // printf
#include <string.h>             // strcmp

#include "command.h"

//#define RUN_TESTS         // if defined, turns on all the testing code

#define INIT_ARGV_CAP 5     // When cmds are first created, what is the capacity?

typedef struct command_s {
  char *in_file;      // if non-NULL, the filename to read input from
  char *out_file;     // if non-NULL, the filename to send output to
  int argv_cap;       // current length of argv; different from argc!
  char **argv;        // the actual argv vector
} command_t;
  

/**********************************************************************
 * 
 * Internal versions of malloc, strdup, and free, which keep a count
 * of how many mallocs/strdups have happened in order to guarantee the
 * same number of calls to free
 *
 **********************************************************************/
//#define DEBUG_MALLOC

static unsigned int n_malloc = 0;
static unsigned int n_free = 0;


static void *cint_malloc(size_t size)
{
  n_malloc++;
  void *ptr = malloc(size);

#ifdef DEBUG_MALLOC
  printf("DEBUG_MALLOC %p: malloc(%lu)\n", ptr, size);
#endif
  
  return ptr;
}

static char *cint_strdup(const char *s1)
{
  n_malloc++;

  char *str = strdup(s1);

#ifdef DEBUG_MALLOC
  printf("DEBUG_MALLOC %p: strdup(%s)\n", str, s1);
#endif
  
  return str;
}

static void cint_free(void *ptr)
{
#ifdef DEBUG_MALLOC
  printf("DEBUG_MALLOC %p: free\n", ptr);
#endif
  
  assert(++n_free <= n_malloc);
  free(ptr);
}

#ifdef RUN_TESTS
/*
 * When this function is called, the caller is asserting that there
 * should be no outstanding allocated memory blocks. Function asserts
 * on failure, so if it returns, all is well.
 */
static void cint_assert_all_free()
{
#ifdef DEBUG_MALLOC
  printf("n_malloc=%u n_free=%u\n", n_malloc, n_free);
#endif

  assert (n_malloc == n_free);
}
#endif   // RUN_TESTS


/**********************************************************************
 * 
 * Implementations for the command_t calls.  All documentation is in
 * the command.h file.
 *
 **********************************************************************/

command_t *command_new()
{
  command_t *cmd = cint_malloc(sizeof(command_t));
  if (cmd) {
    cmd->in_file = NULL;
    cmd->out_file = NULL;

    cmd->argv_cap = INIT_ARGV_CAP;
    cmd->argv = cint_malloc(cmd->argv_cap * sizeof(char *));

    if (!cmd->argv) {
      cint_free(cmd);
      return NULL;
    }

    cmd->argv[0] = NULL;
  }

  return cmd;
}


void
command_free(command_t *cmd)
{
  if (!cmd)
    return;

  if (cmd->in_file) {
    cint_free(cmd->in_file);
    cmd->in_file = NULL;
  }
  
  if (cmd->out_file) {
    cint_free(cmd->out_file);
    cmd->out_file = NULL;
  }

  for (int i=0; cmd->argv[i]; i++) {
    cint_free(cmd->argv[i]);
    cmd->argv[i] = NULL;
  }
    
  cint_free(cmd->argv);
  cmd->argv = NULL;

  cint_free(cmd);
}

int command_set_input(command_t *cmd, const char *in_file)
{
  if (!cmd)
    return -1;

  int ret = 0;

  if (cmd->in_file) {
    // there was already an in_file file here; free and return -1
    cint_free(cmd->in_file);
    cmd->in_file = NULL;
    ret = -1;
  }
  if (in_file) {
    cmd->in_file = cint_strdup(in_file);
    if (cmd->in_file == NULL)
      ret = -1;
  }

  return ret;
}


int command_set_output(command_t *cmd, const char *out_file)
{
  if (!cmd)
    return -1;

  int ret = 0;

  if (cmd->out_file) {
    // there was already an out_file file here; free and return -1
    cint_free(cmd->out_file);
    cmd->out_file = NULL;
    ret = -1;
  }
  if (out_file) {
    cmd->out_file = cint_strdup(out_file);
    if (cmd->out_file == NULL)
      ret = -1;
  }

  return ret;
}


const char *command_get_input(command_t *cmd)
{
  if (!cmd)
    return NULL;
  return cmd->in_file;
}


const char *command_get_output(command_t *cmd)
{
  if (!cmd)
    return NULL;
  return cmd->out_file;
}


void command_dump(command_t *cmd)
{
  if (!cmd) {
    printf("Command is NULL!\n");
    return;
  }
    
  printf("Command at %p...\n", cmd);
  printf("  < %s\n", cmd->in_file ? cmd->in_file : "stdin");
  printf("  > %s\n", cmd->out_file ? cmd->out_file : "stdout");
  printf("  argc=%d\n", command_get_argc(cmd));

  for (int i=0; cmd->argv[i]; i++) 
    printf("    argv[%d] = %s\n", i, cmd->argv[i]);

}


bool command_compare(command_t *cmd1, command_t *cmd2)
{
  if (cmd1 == NULL || cmd2 == NULL)
    return (cmd1 == cmd2);

  if (strcmp(cmd1->in_file ? cmd1->in_file : "null",
          cmd2->in_file ? cmd2->in_file : "null") != 0)
    return false;

  if (strcmp(cmd1->out_file ? cmd1->out_file : "null",
          cmd2->out_file ? cmd2->out_file : "null") != 0)
    return false;

  int i;
  for (i=0; cmd1->argv[i]; i++) 
    if (cmd2->argv[i] == 0 || strcmp(cmd1->argv[i], cmd2->argv[i]) != 0)
      return false;

  if (cmd2->argv[i] != NULL)
    return false;

  return true;
}

bool command_is_empty(command_t *cmd)
{
  if (!cmd)
    return true;

  if (cmd->in_file || cmd->out_file)
    return false;

  if (cmd->argv[0] == NULL)
    return true;

  return false;
}

int command_get_argc(command_t *cmd)
{
  if (!cmd)
    return -1;

  int cnt;
  for (cnt = 0; cmd->argv[cnt]; cnt++)
    ;

  return cnt;
}

int command_append_arg(command_t *cmd, const char *arg)
{
  if (!cmd || !arg)
    return -1;

  // find the terminal NULL
  int idx;
  for (idx = 0; cmd->argv[idx]; idx++)
    ;

  assert(idx < cmd->argv_cap);

  if (idx + 1 == cmd->argv_cap) {
    // reallocate argv
    cmd->argv_cap += INIT_ARGV_CAP;
    cmd->argv = realloc(cmd->argv, cmd->argv_cap * sizeof(char*));
  }
  cmd->argv[idx++] = cint_strdup(arg);
  cmd->argv[idx] = NULL;

  return 0;
}


char * const * command_get_argv(command_t *cmd)
{
  if (!cmd)
    return NULL;

  return cmd->argv;
}



/**********************************************************************
 * 
 * Test code below
 *
 **********************************************************************/
#ifdef RUN_TESTS

void test_command()
{
  command_t *cmd;

  
  assert( (cmd = command_new()) );

  // test initial conditions
  assert( command_get_input(cmd) == NULL );
  assert( command_get_output(cmd) == NULL );
  assert( command_get_argc(cmd) == 0 );
  assert( command_get_argv(cmd)[0] == NULL );
  assert( command_is_empty(cmd) );

  // play with input
  const char *infile = "/tmp/foo";
  assert( command_set_input(cmd, infile) == 0 );
  assert( strcmp(command_get_input(cmd), infile) == 0 );
  assert( !command_is_empty(cmd) );
  assert( command_set_input(cmd, NULL) == -1 );
  assert( command_get_input(cmd) == NULL );
  assert( command_is_empty(cmd) );
  assert( command_set_input(cmd, infile) == 0 );
  assert( command_set_input(cmd, infile) == -1 );

  // play with output
  const char *outfile = "/tmp/foo";
  assert( command_set_output(cmd, outfile) == 0 );
  assert( strcmp(command_get_output(cmd), outfile) == 0 );
  assert( command_set_output(cmd, NULL) == -1 );
  assert( command_get_output(cmd) == NULL );
  assert( command_set_output(cmd, outfile) == 0 );
  assert( command_set_output(cmd, outfile) == -1 );

  // add some args -- enough to force a realloc
  char *test_args[] = {"zero", "one", "two", "three", "four", "five", "six", NULL};
  for (int i=0; test_args[i]; i++) {
    assert( command_append_arg(cmd, test_args[i]) == 0 );
    assert( command_get_argc(cmd) == i+1 );
  }

  // get the argv vector and ensure it's correct
  char *const *argv = command_get_argv(cmd);
  assert( argv );
  for (int i=0; test_args[i]; i++) {
    assert( argv[i] != NULL );
    assert( strcmp(argv[i], test_args[i]) == 0 );
  }
  int argc = command_get_argc(cmd);
  assert( argc == sizeof(test_args) / sizeof(test_args[0]) - 1 );
  assert( argv[argc] == NULL );

  assert( !command_is_empty(cmd) );

  // dump the command
  command_dump(cmd);

  // now free it
  command_free(cmd);
  cint_assert_all_free();
}


int main(int argc, char *argv[])
{
  test_command();
  fprintf(stderr, "test_command: All tests succeeded!\n");
  return 0;
}

#endif   // RUN_TESTS
