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

/*
 * Documented in .h file
 */

// bring in the read_word function
int read_word(const char *input, char *word, size_t word_len)
{
  assert(input);
  assert(word);

  const char *inpt = input;
  char *w = word;
  bool insideQuotes = false;

  // SKIP THROUGH ANY LEADING WHITESPACE
  while (isspace(*inpt))
  {
    // check if its the end of the input and stop
    if (*inpt == '\0')
      break;
    else
      inpt++;
  }

  while (*inpt)
  {
    // IF WE ARE BETWEEN QUOTES
    if (*inpt == '"')
    {
      // COUNT THE NUMBER OF QUOTES FROM THE FIRST, AND CHECK IF IT IS ODD OR EVEN
      int quotCount = 0;
      for (int i = 0; i < strlen(input); i++)
      {
        if (input[i - 1] != '\\' && input[i] == '"')
          quotCount++;
      }

      // IF IT IS ODD, THEN IT IS NOT TERMINATED
      if (quotCount % 2 != 0)
      {
        sprintf(word, "Unterminated quote");
        return -1;
      }
    }

    // IF WE ARE NOT BETWEEN QUOTES AND WE ENCOUNTER A SPACE, THEN WE ARE DONE
    if (isspace(*inpt) && !insideQuotes)
      break;

    // IF WE ENCOUNTER A QUOTE, HANDLE IT BY TOGGLING THE BETWEEN QUOTES FLAG(IN & OUT)
    else if (*inpt == '"')
    {
      insideQuotes = !insideQuotes;
      inpt++;
    }

    // HANDLING SPECIAL CHARACTERS
    else if (*inpt == '\\')
    {
      switch (*(inpt + 1))
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
        sprintf(word, "Illegal escape character: %c", *(inpt + 1));
        return -1;
      }
      inpt += 2;
    }

    // SUPPORTING VARIABLE SUBSTITUTION IF THE WORD BEGINS WITH $
    else if (*inpt == '$')
    {
      int count = 0;
      char *varContainter = malloc(strlen(inpt) * sizeof(char));

      // CHECK IF THE VARIABLE NAME IS ALPHANUMERIC
      while (isalnum(*(inpt + 1)))
      {
        varContainter[count] = *(inpt + 1);
        inpt++;
        count++;
      }

      varContainter[count] = '\0';

      // GETTING THE VALUE OF THE VARIABLE FROM THE ENVIRONMENT
      char *actValue = getenv(varContainter);

      // IF THE VARIABLE - varContainter - IS NOT DEFINED, RETURN AN ERROR
      if (!actValue)
      {
        sprintf(word, "Undefined variable: \'%s\'", varContainter);
        return -1;
      }

      // COPY THE VALUE OF THE VARIABLE TO THE WORD
      strcpy(w, actValue);


      // MOVE THE POINTER TO THE END OF THE VALUE
      w += strlen(actValue);

      // FREE THE MEMORY ALLOCATED FOR THE VARIABLE
      free(varContainter);

      // MOVE THE POINTER TO THE NEXT CHARACTER
      inpt++;
    }

    // SUPPORTING GLOB PATTERN EXPANSION FOR <, >, >>
    else if (*inpt == '<' || *inpt == '>' || (*inpt == '>' && *(inpt + 1) == '>'))
    {
      // IF WE ARE BETWEEN QUOTES, THEN < OR > ARE NOT REDIRECTION OPERATORS, SO COPY THEM TO THE WORD
      if (insideQuotes)
      {
        *w++ = *inpt++;
        continue;
      }

      // IF SPACE BEFORE OR AFTER REDIRECTION, OR RED.. IS 1st IN THE INPUT, COPY IT
      if (isspace(*(inpt - 1)) || isspace(*(inpt + 1)) || inpt == input)
      {
        *w++ = *inpt++;

        // SKIP WHITESPACES
        while (isspace(*inpt))
          inpt++;

        // IF THE REDIRECTION OPERATOR IS THE LAST CHARACTER IN THE INPUT, THEN RETURN AN ERROR
        if (*inpt == '\0')
        {
          strcpy(word, "Redirection without filename");
          return -1;
        }
      }

      // IF THE REDIRECTION OPERATOR IS THE ONLY CHARACTER IN THE INPUT, RETURN AN ERROR ALSO
      else if (*(inpt + 1) == '\0')
      {
        strcpy(word, "Redirection without filename");
        return -1;
      }

      
      // IF THE REDIRECTION OPERATOR IS NOT PRECEDED BY A SPACE, THEN IT IS NOT A REDIRECTION OPERATOR
      else
        break;
    }

    // COPY THE CHARACTER TO THE WORD
    else
      *w++ = *inpt++;

    // WHEN THE WORD IS TOO LONG, COPY Word too long AND RETURN -1
    if (w >= word + word_len)
    {
      strcpy(word, "Word too long");
      return -1;
    }
  }

  // NULL TERMINATE THE WORD
  *w = '\0';

  // RETURN THE NUMBER OF CHARACTERS PROCESSED
  return inpt - input;
}

/*
 * Documented in .h file
 */
command_t *parse_input(const char *input, char *err_msg, size_t err_msg_len)
{
  int chars_read = 0;
  char word[512];

  // ALLOCATE MEMORY FOR THE NEW COMMAND
  command_t *cmd = command_new();

  // IF THE INPUT IS FINISHED OR NULL, RETURN THE COMMAND
  while (1)
  {
    // READ THE WORDS FROM THE INPUT
    chars_read = read_word(input, word, sizeof(word));
    input += chars_read;

    // IF THERE IS AN ERROR, FREE THE MEMORY ALLOCATED FOR THE COMMAND AND RETURN NULL
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

    // IF THE FIRST CHARACTER OF THE WORD IS < OR >
    if (word[0] == '<' || word[0] == '>' || (word[0] == '>' && word[1] == '>'))
    {
      // ALREADY A VALUE FOR IN_FILE OR OUT_FILE, COPY ERROR - “Multiple redirections not allowed”
      if ((word[0] == '<' && command_get_input(cmd) != NULL) || (word[0] == '>' && command_get_output(cmd) != NULL) || (word[0] == '>' && word[1] == '>' && command_get_output(cmd) != NULL))
      {
        strncpy(err_msg, "Multiple redirections not allowed", err_msg_len);
        return NULL;
      }

      // OTHERWISE, PASS THE PROVIDED FILENAME, STARTING AT word[1]
      if (word[0] == '<')
        command_set_input(cmd, word + 1);

      else if (word[0] == '>' && word[1] == '>')
        command_set_output(cmd, word + 2);

      else
        command_set_output(cmd, word + 1);
    }
    else
    {
      // INITIALIZE THE GLOB
      glob_t globbuf;

      // GLOB THE WORD
      glob(word, GLOB_NOCHECK, NULL, &globbuf);

      // GETTING THE HOME DIRECTORY FROM THE ENVIRONMENT
      char *homeDir = getenv("HOME");
      char *expanded_word = malloc(sizeof(char) * (strlen(word) + strlen(homeDir) + 1));

      // HANDLING THE EXPANSIONS
      switch (word[0])
      {
      case '~':
        // TILDE EXTENSION - REPLACE ~ WITH THE HOME DIRECTORY
        strcpy(expanded_word, word);
        glob(expanded_word, GLOB_TILDE_CHECK, NULL, &globbuf);
        free(expanded_word);
        break;

      case '{':
        // BRACE EXTENSION - eg: REPLACE {a,b,c} WITH a b c
        glob(word, GLOB_BRACE, NULL, &globbuf);
        break;

      case '?':
      case '*':
        // WILDCARD ? AND * - REPLACE ? AND * WITH THE FILENAMES THAT MATCH THE PATTERN
        glob(word, GLOB_NOCHECK, NULL, &globbuf);
        break;

      // OTHERWISE, DO NOTHING
      default:
        break;
      }

      // PROCESSING THE MATCHING GLOB RESULTS - ADDING THEM TO THE COMMAND
      for (int i = 0; i < globbuf.gl_pathc; i++)

        // IF THE WORD ENDS WITH /, ADD IT AS IT IS
        if (word[strlen(word) - 1] == '/')
          command_append_arg(cmd, word);
        else
          command_append_arg(cmd, globbuf.gl_pathv[i]);

      globfree(&globbuf);
    }

    // IF THERE IS NO COMMAND BEFORE THE REDIRECTION, RETURN AN ERROR
    if (command_get_argc(cmd) == 0)
    {
      strncpy(err_msg, "Missing command", err_msg_len);
      return NULL;
    }
  }

  return cmd;
}