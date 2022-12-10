
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

/*
 * Tests the read_word function
 *
 * Returns:
 *   True if all test cases pass, false otherwise.
 */
static bool
ilse_test_read_word()
{
    typedef struct
    {
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
            {"cat<foo", "cat", 3}, // TODO:
            {"cat\\<foo", "cat<foo", 8},
            {"<$TESTVAR", "<Scotty Dog", 9},
            {"< $TESTVAR", "<Scotty Dog", 10},
            {"> /path/to/file  $TESTVAR", ">/path/to/file", 15},
            {">    /path/to/file  $TESTVAR", ">/path/to/file", 18},
            {">/path/to/file  $TESTVAR", ">/path/to/file", 14},
            {"cat>foo", "cat", 3}, // TODO:
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
            {"\"<this isn't redirection>\"", "<this isn't redirection>", 26}};
    const int num_tests = sizeof(tests) / sizeof(test_matrix_t);
    int tests_passed = 0;

    for (int i = 0; i < num_tests; i++)
    {

        int act_pos = read_word(tests[i].input, word_buf, sizeof(word_buf));
        if (act_pos == tests[i].exp_pos && strcmp(word_buf, tests[i].exp_word) == 0)
        {
            tests_passed++;
        }
        else
        {
            printf("  FAILED: read_word(\"%s\" ...) returned %d, \"%s\"\n",
                   tests[i].input, act_pos, word_buf);
        }
    }

    printf("%s: PASSED %d/%d\n", __FUNCTION__, tests_passed, num_tests);
    return (tests_passed == num_tests);
}

// main function to test read_word by calling ilse_test_read_word()
int main()
{
    setenv("TESTVAR", "Scotty Dog", 1);
    return ilse_test_read_word();
}