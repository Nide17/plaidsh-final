/*
 * test_tokenize.c
 * 
 * Test functions for tokenize.c
 *
 * Author: Howdy Pierce <howdy@cardinalpeak.com>
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "command.h"
#include "parser.h"

#define MAX_ARGS 20


/*
 * Tests the read_word function
 *
 * Returns:
 *   True if all test cases pass, false otherwise.
 */
static bool
ilse_test_read_word()
{
  typedef struct {
    const char *input;
    const char *exp_word;
    const int exp_pos;
  } test_matrix_t;

  setenv("TESTVAR", "Scotty Dog", 1);
  
  char word_buf[32];
  test_matrix_t tests[] =
    {
      // from assignment writeup
      {"   echo ", "echo", 7},
      {"grep", "grep", 4},
      {"echo one two three", "echo", 4},
      {"", "", 0},
      {"  ", "", 2},
      {"\"one two\" three", "one two", 9},
      {"One\\ Two Three", "One Two", 8},
      {"ec\"ho\" HELLO", "echo", 6},
      {"echo\\ ", "echo ", 6},
      {"echo\\\\ ", "echo\\", 6},
      {"echo\t", "echo", 4},
      {"function() one", "function()", 10},
      {" \"\\\"\"  ", "\"", 5},
      {"\\\"", "\"", 2},

      // other worthwhile test cases
      {" ", "", 1},
      {" \n", "", 2},
      {"  ", "", 2},
      {"\t\n\r   ", "", 6},

      {"     12.34", "12.34", 10},
      {"\techo", "echo", 5},
      {"\t \techo", "echo", 7},
      {"echo", "echo", 4},
      {"echo   ", "echo", 4},
      {"echo\n", "echo", 4},
      {"\"one two\"  ", "one two", 9},
      {"\\\"one\\ two\\\"  ", "\"one two\"", 12},
      {"\\\"one two\\\"  ", "\"one", 5},
      {" two\\\"  ", "two\"", 6},
      {" ec\"ho\"  ", "echo", 7},
      {"ec\"ho \"  ", "echo ", 7},
      {"  echo\\t   ", "echo\t", 8},
      {"  echo\\n   ", "echo\n", 8},
      {"  echo\\r   ", "echo\r", 8},
      {"  echo\\\\   ", "echo\\", 8},
      {"  echo\\   ", "echo ", 8},
      {"  echo\\\"   ", "echo\"", 8},
      {"  echo\\g  ", "Illegal escape character: g", -1},
      {"  \"\\t\"   ", "\t", 6},
      {"  \"\\n\"   ", "\n", 6},
      {"  \"\\r\"   ", "\r", 6},
      {" \\\\", "\\", 3},
      {" \\$", "$", 3},
      {"\\ ", " ", 2},
      {"\\\"", "\"", 2},
      {" one\\<two  ", "one<two", 9},
      {" two\\>one!", "two>one!", 10},


      {"x\\n\\t\\r\\\\\\ \\\"   ", "x\n\t\r\\ \"", 13},
      {" supercalifragilisticexpialidocious ", "Word too long", -1},
      {" snake_case ", "snake_case", 11},
      {" CamelCase@20", "CamelCase@20", 13},
      {"\tfunction() one", "function()", 11},

      // unterminated quotes
      {"\"unterminated quote!", "Unterminated quote", -1},
      {"untermin\"ated quote!", "Unterminated quote", -1},
      {"\"untermina\\\"ted quote!", "Unterminated quote", -1},
      {"unterminated_quote!\"", "Unterminated quote", -1},

      // variable substitution
      {"$TESTVAR", "Scotty Dog", 8},
      {"x$TESTVAR", "xScotty Dog", 9},
      {"x$TESTVAR ", "xScotty Dog", 9},
      {"x$TESTVAR- ", "xScotty Dog-", 10},
      {"\"$TESTVAR\"", "Scotty Dog", 10},
      {"$TESTVARx", "Undefined variable: 'TESTVARx'", -1},
      {"\"$TESTVARx\"", "Undefined variable: 'TESTVARx'", -1},
      {"\"$-TESTVAR\"", "Undefined variable: ''", -1},
      {"$$", "Undefined variable: ''", -1},
      {"x\"$TESTVAR\"x", "xScotty Dogx", 12},
      {"\\$TESTVAR", "$TESTVAR", 9},
      {"\"\\$TESTVAR\"", "$TESTVAR", 11},

      // redirection
      {"< /path/to/file  $TESTVAR", "</path/to/file", 15},
      {"<    /path/to/file  $TESTVAR", "</path/to/file", 18},
      {"</path/to/file  $TESTVAR", "</path/to/file", 14},
      {">file1 ", ">file1", 6},
      {"cat<foo", "cat", 3},
      {"cat\\<foo", "cat<foo", 8},
      {"<$TESTVAR", "<Scotty Dog", 9},
      {"< $TESTVAR", "<Scotty Dog", 10},
      {"> /path/to/file  $TESTVAR", ">/path/to/file", 15},
      {">    /path/to/file  $TESTVAR", ">/path/to/file", 18},
      {">/path/to/file  $TESTVAR", ">/path/to/file", 14},
      {"cat>foo", "cat", 3},
      {"cat\\>foo", "cat>foo", 8},
      {">$TESTVAR", ">Scotty Dog", 9},
      {"\"five > three\"", "five > three", 14},
      {"\"five>\"", "five>", 7},
      {"\"<html>\"", "<html>", 8},
      {"\"5 < 7\"", "5 < 7", 7},
      {">>", "Redirection without filename", -1},
      {">   ", "Redirection without filename", -1},
      {">", "Redirection without filename", -1},
      {"<<", "Redirection without filename", -1},
      {"<   ", "Redirection without filename", -1},
      {"<", "Redirection without filename", -1},
      {"\"<this isn't redirection>\"", "<this isn't redirection>", 26}
    };
  const int num_tests = sizeof(tests) / sizeof(test_matrix_t);
  int tests_passed = 0;

  for(int i=0; i<num_tests; i++) {

    int act_pos = read_word(tests[i].input, word_buf, sizeof(word_buf));
    if (act_pos == tests[i].exp_pos && strcmp(word_buf, tests[i].exp_word) == 0) {
      tests_passed++;
    } else {
      printf("  FAILED: read_word(\"%s\" ...) returned %d, \"%s\"\n",
        tests[i].input, act_pos, word_buf);
    }
    
  }

  printf("%s: PASSED %d/%d\n", __FUNCTION__, tests_passed, num_tests);
  return (tests_passed == num_tests);
}


static int num_parser_tests=0;

/*
 * Tests one test case of the parse_input function. 
 *
 * Parameters:
 *   exp_in_file      Expected input file, or NULL for stdin
 *   exp_out_file     Expected output file, or NULL for stdout
 *   exp_result       true if a result is expected, false if an error is expected
 *   argv, argv, ...  NULL terminated list of expected output arguments
 * 
 *   Note: if exp_result is false, then exactly one argv should be
 *   specified, which should contain the expected error message.
 * 
 * Returns:
 *   True if test passes, false otherwise. 
 */
static bool
test_parser_once(const char *teststring,
    const char *exp_in_file, const char *exp_out_file, bool exp_result, ...)
{
  va_list valist;
  char err_msg[128];
  bool test_result = false;
  command_t *exp_cmd = NULL;

  num_parser_tests++;
  va_start(valist, exp_result);


  command_t *cmd = parse_input(teststring, err_msg, sizeof(err_msg));
  if (cmd == NULL) {
    if (exp_result)
      printf("Error [%s]: got error but expected result\n", teststring);
    else {
      const char *exp_error = va_arg(valist, const char *);
      if (strcmp(err_msg, exp_error) != 0) 
        printf("Error [%s]: Actual error msg did not match expected msg\n", teststring);
      else
        test_result = true;
    }
    goto end;
  }

  if (!exp_result) {
    printf("Error [%s]: got result but expected error\n", teststring);
    goto end;
  }

  exp_cmd = command_new();
  assert(exp_cmd);
  command_set_input(exp_cmd, exp_in_file);
  command_set_output(exp_cmd, exp_out_file);
  const char *exp_arg = NULL;
  while ((exp_arg = va_arg(valist, const char *)))
    command_append_arg(exp_cmd, exp_arg);
  
  if (!command_compare(cmd, exp_cmd)) {
    printf("Error [%s]: Command did not match expected result.\n", teststring);
    printf("Actual result:\n");
    command_dump(cmd);
    printf("Expected result:\n");
    command_dump(exp_cmd);
  } else {
    test_result = true;
  }

 end:
  command_free(cmd);
  command_free(exp_cmd);
  return test_result;
}


/*
 * Equivalent to the 'touch' command line utility: Creates the
 * specified file in the cwd. File is created with perms 600.
 *
 * Parameters:
 *   filename   The file to be created
 * 
 * Returns:
 *   true on success, false on error
 *
 * Shamelessly stolen from 
 *  https://codereview.stackexchange.com/questions/277101/touch-command-in-c
 */
static bool
touch(const char *filename)
{
  int fd = open(filename, O_CREAT, S_IRUSR | S_IWUSR);

  if (fd == -1) {
    perror("open");
    return false;
  }

  close(fd);
  return true;
}


/*
 * Tests the parse_input function
 *
 * Returns:
 *   True if all test cases pass, false otherwise.
 */
static bool
ilse_test_parse_input()
{
  int passed = 0;

  // for all tests, the environment will have the variable FOO set to
  // "Carnegie Mellon"
  setenv("FOO", "Carnegie Mellon", 1);

  // empty command string
  passed += test_parser_once("", NULL, NULL, true, NULL);
  passed += test_parser_once("   ", NULL, NULL, true, NULL);
  passed += test_parser_once("   \n", NULL, NULL, true, NULL);

  // basic commands
  passed += test_parser_once("echo this is a test", NULL, NULL, true, 
      "echo", "this", "is", "a", "test", NULL);
  passed += test_parser_once("echo  this is  a test   ", NULL, NULL, true,
      "echo", "this", "is", "a", "test", NULL);
  passed += test_parser_once("echo \tthis  is\ta\ntest   \n", NULL, NULL, true,
      "echo", "this", "is", "a", "test", NULL);
  passed += test_parser_once("echo one\\n two\\r", NULL, NULL, true,
      "echo", "one\n", "two\r", NULL);
  passed += test_parser_once("echo \"one two\"", NULL, NULL, true,
      "echo", "one two", NULL);
  passed += test_parser_once("echo one\\ two", NULL, NULL, true,
      "echo", "one two", NULL);
  passed += test_parser_once("echo \\\"one   two", NULL, NULL, true,
      "echo", "\"one", "two", NULL);
  passed += test_parser_once("echo one \"# no comment\"", NULL, NULL, true,
      "echo", "one", "# no comment", NULL);
  passed += test_parser_once("one \"two three\" four", NULL, NULL, true,
      "one", "two three", "four", NULL);
  passed += test_parser_once("\\n\\t\\r\\\\\\ \\\"   ", NULL, NULL, true,
      "\n\t\r\\ \"", NULL);

  // variable substitution
  passed += test_parser_once("echo $FOO", NULL, NULL, true,
      "echo", "Carnegie Mellon", NULL);
  passed += test_parser_once("echo -$FOO-", NULL, NULL, true,
      "echo", "-Carnegie Mellon-", NULL);
  passed += test_parser_once("echo \"$FOO\"", NULL, NULL, true,
      "echo", "Carnegie Mellon", NULL);
  passed += test_parser_once("echo \"-$FOO-\"", NULL, NULL, true,
      "echo", "-Carnegie Mellon-", NULL);

  // input/output redirection
  passed += test_parser_once("cat < /etc/passwd > /tmp/a_file", "/etc/passwd",
      "/tmp/a_file", true, "cat", NULL);
  passed += test_parser_once("cat </etc/passwd >/tmp/a_file", "/etc/passwd",
      "/tmp/a_file", true, "cat", NULL);
  passed += test_parser_once("cat </etc/passwd ", "/etc/passwd", NULL,
      true, "cat", NULL);
  passed += test_parser_once("cat \"</etc/passwd\" ", "/etc/passwd", NULL,
      true, "cat", NULL);
  passed += test_parser_once("cat >/tmp/afile   ", NULL, "/tmp/afile",
      true, "cat", NULL);
  passed += test_parser_once("cat \">/tmp/afile \"   ", NULL, "/tmp/afile ",
      true, "cat", NULL);
  passed += test_parser_once("grep foobar < /tmp/afile   ", "/tmp/afile", NULL,
      true, "grep", "foobar", NULL);
  passed += test_parser_once("grep $FOO < /tmp/afile   ", "/tmp/afile", NULL,
      true, "grep", "Carnegie Mellon", NULL);
  passed += test_parser_once("grep $FOO</tmp/afile   ", "/tmp/afile", NULL,
      true, "grep", "Carnegie Mellon", NULL);

  // not redirection or a pipe, but has the escaped/quoted characters in it
  passed += test_parser_once("grep $FOO\\> ", NULL, NULL, true,
      "grep", "Carnegie Mellon>", NULL);
  passed += test_parser_once("echo $FOO\\< ", NULL, NULL, true,
      "echo", "Carnegie Mellon<", NULL);

  // ................. start of globbing tests .....................
  // to test globbing, we need to set up a test directory with some
  // known files in it
  char tempdir[128];
  strcpy(tempdir, "test_parser_dir_XXXXXX");
  if (!mkdtemp(tempdir)) {
    perror("mkdtemp");
    return false;
  }
  char *old_cwd = getcwd(NULL, 0);
  if (chdir(tempdir) != 0) {
    perror("cd");
    return false;
  }
  char *files[] = {"one.c", "one.h", "one.o",
    "two.c", "three.c", "three.h", "three.o", NULL};
  for (int i=0; files[i]; i++) 
    if (!touch(files[i]))
      return false;

  // note, globbed filenames come back sorted by default
  passed += test_parser_once("   *", NULL, NULL, true,
      "one.c", "one.h", "one.o", "three.c", "three.h", "three.o",
      "two.c", NULL);
  passed += test_parser_once("ls *.c", NULL, NULL, true,
      "ls", "one.c", "three.c", "two.c", NULL);
  passed += test_parser_once("ls *.g", NULL, NULL, true,
      "ls", "*.g", NULL);
  passed += test_parser_once("  one.*\n", NULL, NULL, true,
      "one.c", "one.h", "one.o", NULL);
  passed += test_parser_once("ls one.[ch]", NULL, NULL, true,
      "ls", "one.c", "one.h", NULL);
  passed += test_parser_once("*.[chg]", NULL, NULL, true,
      "one.c", "one.h", "three.c", "three.h", "two.c", NULL);
  passed += test_parser_once("ls {one,two}.c", NULL, NULL, true,
      "ls", "one.c", "two.c", NULL);
  passed += test_parser_once("ls {one,three}.[ch]", NULL, NULL, true,
      "ls", "one.c", "one.h", "three.c", "three.h", NULL);
  passed += test_parser_once("ls ~ > file1", NULL, "file1", true,
      "ls", getenv("HOME"), NULL);
  passed += test_parser_once("~howdy", NULL, NULL, true,
      "/home/howdy", NULL);
  passed += test_parser_once("~howdy/tmp", NULL, NULL, true,
      "/home/howdy/tmp", NULL);
  passed += test_parser_once("/foo/~/bar/", NULL, NULL, true,
      "/foo/~/bar/", NULL);
  passed += test_parser_once("sed s/foo/bar/", NULL, NULL, true,
      "sed", "s/foo/bar/", NULL);
  passed += test_parser_once("~howdy/tmp", NULL, NULL, true,
      "/home/howdy/tmp", NULL);

  // Delete the glob test files plus the tempdir
  for (int i=0; files[i]; i++) 
    if (unlink(files[i]) != 0) {
      perror("unlink");
      return false;
    }
  chdir(old_cwd);
  free(old_cwd);
  if (rmdir(tempdir) != 0) {
    perror("rmdir");
    return false;
  }
  // ................. end of globbing tests ......................


  // error cases
  passed += test_parser_once("grep $FOO <    ", NULL, NULL, false,
      "Redirection without filename");
  passed += test_parser_once("grep $FOO <", NULL, NULL, false,
      "Redirection without filename");
  passed += test_parser_once("grep $FOO >", NULL, NULL, false, 
      "Redirection without filename");
  passed += test_parser_once("grep $FOO > ", NULL, NULL, false,
      "Redirection without filename");
  passed += test_parser_once("grep $FOO> ", NULL, NULL, false, 
      "Redirection without filename");

  passed += test_parser_once("cat < /a/file </a/different/file", NULL,
      NULL, false, "Multiple redirections not allowed");
  passed += test_parser_once("cat > /a/file >/a/different/file", NULL,
      NULL, false, "Multiple redirections not allowed");

  passed += test_parser_once("<foo", "foo", NULL, false, "Missing command");
  passed += test_parser_once("  < foo", "foo", NULL, false, "Missing command");
  passed += test_parser_once(">  foo", NULL, "foo", false, "Missing command");

  passed += test_parser_once("grep 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19",
      NULL, NULL, true, "grep", "1", "2", "3", "4", "5", "6", "7", "8",
      "9", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", NULL);
  passed += test_parser_once("grep 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21",
      NULL, NULL, true, "grep", "1", "2", "3", "4", "5", "6", "7", "8",
      "9", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20", "21", NULL);

  printf("%s: PASSED %d/%d\n", __FUNCTION__, passed, num_parser_tests);
  return (passed == num_parser_tests);
}


int main(int argc, char *argv[])
{
  int success = 1;

  success &= ilse_test_read_word();
  success &= ilse_test_parse_input();

  if (success) {
    printf("Excellent work! All tests succeeded!\n");
    return 0;
  } else {
    printf("NOTE: FAILURES OCCURRED\n");
    return 1;
  }
}
