/*
 * parser.h
 * 
 * Code to parse and tokenize a command line
 *
 * Author: Howdy Pierce <howdy@cardinalpeak.com>
 */

#ifndef _PARSER_H_
#define _PARSER_H_

#include "command.h"
#include <stddef.h>

/*
 * Returns the first word from input, removing leading whitespace,
 * handling double quotes, and translating escaped characters.
 * 
 * When called, word points to a buffer of word_len length. The
 * read_word function places the translated word from input into the
 * word buffer and returns the number of characters that were
 * processed from input.  In the case where input holds more than one
 * word, it is possible to read the next word by calling read_word
 * again with the pointer input+return_value.
 * 
 * Normally, a word ends with unescaped whitespace or one of the
 * redirection characters ('<' or '>').
 * 
 * However, if an unescaped double quote is encountered, then the
 * characters from that double quote up to the next double quote are
 * included in the word, regardless of whether there is whitespace
 * between the quotes. The quotes themselves are removed.  If input
 * contains an unterminated quote, read_word places the error message
 * “Unterminated quote” into the word buffer and returns -1.
 * 
 * Variables follow the form $varname, where varname is any
 * combination of letters, numbers, or underscores. Variables are
 * expanded via a call to getenv() as they are read. Expansion occurs
 * both inside and outside double quotes. If a variable is not found
 * in the environment, the error message "Undefined variable:
 * '<varname>'" is returned.
 * 
 * The function converts escape sequences as follows:
 *    \n        newline
 *    \t        tab
 *    \r        carriage return
 *    \(space)  a literal space, which does not cause a word break
 *    \"        a literal double quote
 *    \\        a literal backslash
 *    \$        a literal dollar sign (does not start a variable)
 *    \<        a literal less-than symbol (does not indicate redirection)
 *    \>        a literal greater-than symbol (does not indicate redirection)
 *
 * If an escape sequence other than those listed is encountered, the
 * function places the error message “Illegal escape character:
 * <char>” in the word buffer and returns -1.
 * 
 * If the input begins with a '>' or '<' character, then read_word
 * will return a word consisting of the redirection character
 * immediately followed by a filename. If a filename cannot be read
 * following a redirection character, the function places the error
 * message "Redirection without filename" in the word buffer and
 * returns -1.
 * 
 * In the case that the word buffer is not long enough, read_word
 * places the error message “Word too long” into the buffer and
 * returns -1.
 * 
 * Some examples:
 *
 *   'echo'             -> 'echo', returns 4
 *   '   echo'          -> 'echo', returns 7
 *   '   echo   '       -> 'echo', returns 7
 *   '"New York"'       -> 'New York', returns 10
 *   ' New\ York'       -> 'New York', returns 10
 *   '"echo"'           -> 'echo', returns 6
 *   '$SCHOOL'          -> the value of getenv("SCHOOL"), returns 7
 *   '< /from/file'     -> '</from/file', returns 12
 *   '>/to/a/file'      -> '</to/a/file', returns 11
 *
 * Parameters:
 *   input     Unprocessed input line, which must be null terminated
 *   word      Buffer to be filled in with the word read
 *   word_len  Size of word buffer
 * 
 * Returns:
 *   The number of characters from input that were consumed. Parsing
 *   for the next word can start at (input + return_value). Of course,
 *   that may be at the end of input if the entire thing was consumed.
 * 
 *   In case of error, returns -1 and places an error message in
 *   word buffer.
 */
int read_word(const char *input, char *word, size_t word_len);



/*
 * Parses an input line into a newly allocated command_t structure by
 * segmenting the input into words that are bounded by unquoted and
 * unescaped word termination characters (described below).
 *
 * Word termination characters are any unquoted/unescaped whitespace
 * and the redirection characters < and >.
 * 
 * Double quotes are used to group words and are eliminated from the
 * input. For instance, the line
 *       echo one "two three" four
 * is parsed into a command with the four arguments "echo", "one",
 * "two three", and "four".
 *
 * Variables follow the form $varname. Variables are expanded as
 * described in the read_word() documentation.
 * 
 * A single backslash is an escape character and results in
 * substitutions as described in the read_word() documentation.
 *
 * If an unescaped and unquoted > or < is encountered, it will begin
 * the next word. Any whitespace is consumed, followed by a filename.
 * For instance, the following lines:
 *     grep foo<bar
 *     grep foo <bar
 *     grep foo <   bar
 *
 * are all be parsed the same, into a command_t with two arguments
 * "grep" and "foo", and the stdin of the command set to "bar".
 * 
 * For clarity, if either of the characters < or > appears inside
 * double quotes, they do NOT have the special meaning of redirection,
 * and the quote is returned as expected. So for instance:
 *     echo "thirty > twenty"
 *
 * is parsed into a command with stdout as its output, and the two
 * arguments "echo" and "thirty > twenty".
 * 
 * Parameters:
 *   input      Input line as typed by the user
 *   err_msg      In case of error, an error message will be returned 
 *                  in this string
 *   err_msg_len  Length of the err_msg string
 * 
 * Returns:
 *   A newly-allocated command_t structure, which the caller must
 *   deallocate via a call to command_free()
 *  
 *   In case of error, copies a descriptive error message into err_msg
 *   and returns NULL.
 * 
 *   In case the input contains only whitespace, a command_t is
 *   returned, but the command's argc is 0.  However, if the input is
 *   only redirection, with no arguments, then the function returns
 *   the error "Missing command".  Examples of this:
 *      input="     "   -> returns command_t with argc==0
 *      input=">file"   -> returns error "Missing command"  
 *      input="  <file" -> returns error "Missing command"  
 */
command_t *parse_input(const char *input, char *err_msg, size_t err_msg_len);

#endif /* _PARSER_H_ */
