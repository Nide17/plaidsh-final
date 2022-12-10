
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
#include <ctype.h>
#include <stddef.h>
#include <glob.h>

// bring in the command_t type and associated functions
// #define RUN_TESTS         // if defined, turns on all the testing code

#define INIT_ARGV_CAP 5 // When cmds are first created, what is the capacity?

typedef struct command_s
{
    char *in_file;  // if non-NULL, the filename to read input from
    char *out_file; // if non-NULL, the filename to send output to
    int argv_cap;   // current length of argv; different from argc!
    char **argv;    // the actual argv vector
} command_t;

/**********************************************************************
 *
 * Internal versions of malloc, strdup, and free, which keep a count
 * of how many mallocs/strdups have happened in order to guarantee the
 * same number of calls to free
 *
 **********************************************************************/
// #define DEBUG_MALLOC

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

    assert(n_malloc == n_free);
}
#endif // RUN_TESTS

/**********************************************************************
 *
 * Implementations for the command_t calls.  All documentation is in
 * the command.h file.
 *
 **********************************************************************/

command_t *command_new()
{
    command_t *cmd = cint_malloc(sizeof(command_t));
    if (cmd)
    {
        cmd->in_file = NULL;
        cmd->out_file = NULL;

        cmd->argv_cap = INIT_ARGV_CAP;
        cmd->argv = cint_malloc(cmd->argv_cap * sizeof(char *));

        if (!cmd->argv)
        {
            cint_free(cmd);
            return NULL;
        }

        cmd->argv[0] = NULL;
    }

    return cmd;
}

void command_free(command_t *cmd)
{
    if (!cmd)
        return;

    if (cmd->in_file)
    {
        cint_free(cmd->in_file);
        cmd->in_file = NULL;
    }

    if (cmd->out_file)
    {
        cint_free(cmd->out_file);
        cmd->out_file = NULL;
    }

    for (int i = 0; cmd->argv[i]; i++)
    {
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

    if (cmd->in_file)
    {
        // there was already an in_file file here; free and return -1
        cint_free(cmd->in_file);
        cmd->in_file = NULL;
        ret = -1;
    }
    if (in_file)
    {
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

    if (cmd->out_file)
    {
        // there was already an out_file file here; free and return -1
        cint_free(cmd->out_file);
        cmd->out_file = NULL;
        ret = -1;
    }
    if (out_file)
    {
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
    if (!cmd)
    {
        printf("Command is NULL!\n");
        return;
    }

    printf("Command at %p...\n", cmd);
    printf("  < %s\n", cmd->in_file ? cmd->in_file : "stdin");
    printf("  > %s\n", cmd->out_file ? cmd->out_file : "stdout");
    printf("  argc=%d\n", command_get_argc(cmd));

    for (int i = 0; cmd->argv[i]; i++)
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
    for (i = 0; cmd1->argv[i]; i++)
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

    if (idx + 1 == cmd->argv_cap)
    {
        // reallocate argv
        cmd->argv_cap += INIT_ARGV_CAP;
        cmd->argv = realloc(cmd->argv, cmd->argv_cap * sizeof(char *));
    }
    cmd->argv[idx++] = cint_strdup(arg);
    cmd->argv[idx] = NULL;

    return 0;
}

char *const *command_get_argv(command_t *cmd)
{
    if (!cmd)
        return NULL;

    return cmd->argv;
}

// bring in the read_word function
int read_word(const char *input, char *word, size_t word_len)
{
    assert(input);
    assert(word);

    const char *inp = input;
    char *w = word;
    bool betweenQuoteS = false;

    // SKIP THROUGH ANY LEADING WHITESPACE
    while (isspace(*inp))
        inp++;

    // if the end of the input string has been reached, return 0
    if (*inp == '\0')
        return 0;

    while (*inp)
    {
        // IF WE ARE BETWEEN QUOTES
        if (*inp == '"')
        {
            // COUNT THE NUMBER OF QUOTES FROM THE FIRST, AND CHECK IF IT IS ODD OR EVEN
            int quotesNbr = 0;
            for (int i = 0; i < strlen(input); i++)
            {
                if (input[i - 1] != '\\' && input[i] == '"')
                    quotesNbr++;
            }

            // IF IT IS ODD, THEN IT IS NOT TERMINATED
            if (quotesNbr % 2 != 0)
            {
                sprintf(word, "Unterminated quote");
                return -1;
            }
        }

        // IF WE ARE NOT BETWEEN QUOTES AND WE ENCOUNTER A SPACE, THEN WE ARE DONE
        if (isspace(*inp) && !betweenQuoteS)
            break;

        // IF WE ENCOUNTER A QUOTE, HANDLE IT BY TOGGLING THE BETWEEN QUOTES FLAG(IN & OUT)
        else if (*inp == '"')
        {
            betweenQuoteS = !betweenQuoteS;
            inp++;
        }

        // HANDLING SPECIAL CHARACTERS
        else if (*inp == '\\')
        {
            switch (*(inp + 1))
            {
            case 'n':
                *w++ = '\n';
                break;

            case 'r':
                *w++ = '\r';
                break;

            case 't':
                *w++ = '\t';
                break;

            case '\\':
                *w++ = '\\';
                break;

            case ' ':
                *w++ = ' ';
                break;

            case '$':
                *w++ = '$';
                break;

            case '"':
                *w++ = '\"';
                break;

            case '<':
                *w++ = '<';
                break;

            case '>':
                *w++ = '>';
                break;

            default:
                sprintf(word, "Illegal escape character: %c", *(inp + 1));
                return -1;
            }
            inp += 2;
        }

        // ADD SUPPORT FOR VARIABLE SUBSTITUTION
        else if (*inp == '$')
        {
            char *var = malloc(sizeof(char) * 32);
            int i = 0;

            while (isalnum(*(inp + 1)))
            {
                var[i] = *(inp + 1);
                i++;
                inp++;
            }

            var[i] = '\0';
            char *value = getenv(var);

            if (value == NULL)
            {
                sprintf(word, "Undefined variable: \'%s\'", var);
                return -1;
            }

            strcpy(w, value);
            w += strlen(value);
            free(var);
            inp++;
        }
        else if (*inp == '<' || *inp == '>')
        {
            // if either of the characters < or > appear inside double quotes, they do NOT have the special meaning of redirection
            if (betweenQuoteS)
            {
                *w++ = *inp++;
                continue;
            }

            // Check if there is a space before or after the redirection operator, or if the redirection operator is the first character in the input, then it is not preceded by a space
            if (isspace(*(inp - 1)) || isspace(*(inp + 1)) || inp == input)
            {
                *w++ = *inp++;

                // skip any whitespace before redirection
                while (isspace(*inp))
                    inp++;

                if (*inp == '\0')
                {
                    strcpy(word, "Redirection without filename");
                    return -1;
                }
            }

            // if the redirection operator is the only character in the input, then return an error
            else if (*(inp + 1) == '\0')
            {
                strcpy(word, "Redirection without filename");
                return -1;
            }
            // if the redirection operator is not preceded by a space, then it is not a redirection operator
            else
            {
                break;
            }
        }

        // COPY THE CHARACTER TO THE WORD
        else
            *w++ = *inp++;

        // WHEN THE WORD IS TOO LONG, COPY Word too long AND RETURN -1
        if (w >= word + word_len)
        {
            strcpy(word, "Word too long");
            return -1;
        }
    }

    *w = '\0';
    return inp - input;
}

// maximum number of arguments that can be parsed
#define MAX_ARGS 20

command_t *parse_input(const char *input, char *err_msg, size_t err_msg_len)
{
    int chars_read = 0;
    char word[512];

    command_t *cmd = command_new();

    while (1)
    {
        chars_read = read_word(input, word, sizeof(word));
        input += chars_read;

        if (chars_read == -1)
        {
            command_free(cmd);
            strncpy(err_msg, word, err_msg_len);
            return NULL;
        }

        if (chars_read == 0) // end of input string
            break;

        if (word[0] == '\0') // whitespace only
            continue;

        // If the first character of the word is either < or >, then
        if (word[0] == '<' || word[0] == '>')
        {
            // If there was already a value for in_file (or out_file, as appropriate), issue the error “Multiple redirections not allowed”.
            if (word[0] == '<' && cmd->in_file || word[0] == '>' && cmd->out_file)
            {
                strncpy(err_msg, "Multiple redirections not allowed", err_msg_len);
                return NULL;
            }

            // Otherwise, pass the provided filename, starting at word[1], into command_set_input (or command_set_output).
            if (word[0] == '<')
                command_set_input(cmd, word + 1);
            else
                command_set_output(cmd, word + 1);
        }
        else
        {
            glob_t globbuf;
            int glob_result = glob(word, GLOB_NOCHECK, NULL, &globbuf);

            char *home = getenv("HOME");
            char *new_word = malloc(sizeof(char) * (strlen(word) + strlen(home) + 1));

            // Handle special characters
            switch (word[0])
            {
            case '~':
                // TILDE EXTENSION
                strcpy(new_word, word);
                glob_result = glob(new_word, GLOB_TILDE_CHECK, NULL, &globbuf);
                free(new_word);
                break;

            case '{':
                // WILDCARD EXTENSION
                glob_result = glob(word, GLOB_BRACE, NULL, &globbuf);
                break;

            case '?':
            case '*':
            case '/':
                // WILDCARD EXTENSION
                glob_result = glob(word, GLOB_NOCHECK, NULL, &globbuf);
                break;

            default:
                break;
            }

            // Otherwise, add each of the resulting filenames to the command using command_append_arg.
            for (int i = 0; i < globbuf.gl_pathc; i++)
                command_append_arg(cmd, globbuf.gl_pathv[i]);

            globfree(&globbuf);
        }

        if (strncmp(word, "s/", 2) == 0)
        {
            // Skip over the first two characters and treat the rest of the word as the actual argument
            strcpy(word, word + 2);
        }

        // If there is no command before the redirection, return an error
        if (cmd->argv_cap == 0 || cmd->argv[0] == NULL)
        {
            strncpy(err_msg, "Missing command", err_msg_len);
            return NULL;
        }
    }

    return cmd;
}

static int num_parser_tests = 0;

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
    if (cmd == NULL)
    {
        if (exp_result)
            printf("Error [%s]: got error but expected result\n", teststring);
        else
        {
            const char *exp_error = va_arg(valist, const char *);
            if (strcmp(err_msg, exp_error) != 0)
                printf("Error [%s]: Actual error msg did not match expected msg\n", teststring);
            else
                test_result = true;
        }
        goto end;
    }

    if (!exp_result)
    {
        printf("Error [%s]: got result but expected error\n", teststring);
        command_dump(cmd);
        goto end;
    }

    exp_cmd = command_new();
    assert(exp_cmd);
    command_set_input(exp_cmd, exp_in_file);
    command_set_output(exp_cmd, exp_out_file);
    const char *exp_arg = NULL;
    while ((exp_arg = va_arg(valist, const char *)))
        command_append_arg(exp_cmd, exp_arg);

    if (!command_compare(cmd, exp_cmd))
    {
        printf("Error [%s]: Command did not match expected result.\n", teststring);
        printf("Actual result:\n");
        command_dump(cmd);
        printf("Expected result:\n");
        command_dump(exp_cmd);
    }
    else
    {
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

    if (fd == -1)
    {
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
    if (!mkdtemp(tempdir))
    {
        perror("mkdtemp");
        return false;
    }
    char *old_cwd = getcwd(NULL, 0);
    if (chdir(tempdir) != 0)
    {
        perror("cd");
        return false;
    }
    char *files[] = {"one.c", "one.h", "one.o",
                     "two.c", "three.c", "three.h", "three.o", NULL};
    for (int i = 0; files[i]; i++)
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
    passed += test_parser_once("~parmenin", NULL, NULL, true,
                               "/home/parmenin", NULL);
    passed += test_parser_once("~parmenin/tmp", NULL, NULL, true,
                               "/home/parmenin/tmp", NULL);
    passed += test_parser_once("/foo/~/bar/", NULL, NULL, true,
                               "/foo/~/bar/", NULL);
    passed += test_parser_once("sed s/foo/bar/", NULL, NULL, true,
                               "sed", "s/foo/bar/", NULL);
    passed += test_parser_once("~parmenin/tmp", NULL, NULL, true,
                               "/home/parmenin/tmp", NULL);

    // Delete the glob test files plus the tempdir
    for (int i = 0; files[i]; i++)
        if (unlink(files[i]) != 0)
        {
            perror("unlink");
            return false;
        }
    chdir(old_cwd);
    free(old_cwd);
    if (rmdir(tempdir) != 0)
    {
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

    success &= ilse_test_parse_input();

    if (success)
    {
        printf("Excellent work! All tests succeeded!\n");
        return 0;
    }
    else
    {
        printf("NOTE: FAILURES OCCURRED\n");
        return 1;
    }
}
