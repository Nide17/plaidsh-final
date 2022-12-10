/*
 * parser.c
 *
 * Code to parse and tokenize a command line
 *
 * Author: Niyomwungeri Parmenide ISHIMWE <parmenin@andrew.cmu.edu>
 */

#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stddef.h>
#include <glob.h>

#include "parser.h"
#include "command.h"

// THIS SHOULD NOT BE USED BECAUSE WE INCLUDE command.h BUT I DON'T KNOW WHY, command_t IS NOT WORKING
typedef struct command_s
{
  char *in_file;  // if non-NULL, the filename to read input from
  char *out_file; // if non-NULL, the filename to send output to
  int argv_cap;   // current length of argv; different from argc!
  char **argv;    // the actual argv vector
} command_t;

/*
 * Documented in .h file
 */

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
 * Documented in .h file
 */
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
      if ((word[0] == '<' && cmd->in_file) || (word[0] == '>' && cmd->out_file))
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
      // int glob_result = glob(word, GLOB_NOCHECK, NULL, &globbuf);

      char *home = getenv("HOME");
      char *new_word = malloc(sizeof(char) * (strlen(word) + strlen(home) + 1));

      // Handle special characters
      switch (word[0])
      {
      case '~':
        // TILDE EXTENSION
        strcpy(new_word, word);
        glob(new_word, GLOB_TILDE_CHECK, NULL, &globbuf);
        free(new_word);
        break;

      case '{':
        // WILDCARD EXTENSION
        glob(word, GLOB_BRACE, NULL, &globbuf);
        break;

      case '?':
      case '*':
      case '/':
        // WILDCARD EXTENSION
        glob(word, GLOB_NOCHECK, NULL, &globbuf);
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
