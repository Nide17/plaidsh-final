/*
 * command.h
 * 
 * Data structure to hold the result of parsing user input
 *
 * Author: Howdy Pierce <howdy@cardinalpeak.com>
 */
#ifndef _COMMAND_H_
#define _COMMAND_H_

#include <stdbool.h>

typedef struct command_s command_t;

/*
 * Allocates and initializes a command_t object
 *
 * Returns: A new command_t, which must be freed by calling
 *    command_free().  If no memory is available, returns NULL.
 */
command_t *command_new();

/*
 * Deletes a previously-allocated command_t object.
 *
 * Parameters:
 *   cmd   The command to be freed
 */
void command_free(command_t *cmd);

/*
 * Updates the command with a new input file, which should be either a
 * filename or NULL to set the input destination to stdin
 *
 * Parameters:
 *   cmd     The command to be updated
 *   input   The new input filename, or NULL for stdin
 *   
 * Returns:
 *   0 on success
 *  -1 if setting the input to a filename, and the input was already
 *      set to a filename; this is probably an error
 */
int command_set_input(command_t *cmd, const char *input);

/*
 * Updates the command with a new output file, which should be either a
 * filename or NULL to set the output destination to stdout
 *
 * Parameters:
 *   cmd      The command to be updated
 *   output   The new output filename, or NULL for stdout
 *   
 * Returns:
 *   0 on success
 *  -1 if setting the output to a filename, and the output was already
 *      set to a filename; this is probably an error
 */
int command_set_output(command_t *cmd, const char *output);

/*
 * Get the current input or output for the command.
 *
 * Parameters:
 *   cmd      The command
 * 
 * Returns: 
 *   The current input or output filename; NULL indicates stdin
 *      or stdout 
 */
const char *command_get_input(command_t *cmd);
const char *command_get_output(command_t *cmd);


/*
 * Print the contents of a command to stdout
 *
 * Parameters:
 *   cmd    The command to print
 */
void command_dump(command_t *cmd);

/*
 * Compares two commands
 *
 * Parameters:
 *   cmd1, cmd2   The commands to compare
 * 
 * Returns: True if the two commands match fully, and false
 *   otherwise. To "match fully", the two commands must have the same
 *   input, the same output, the same number of arguments, and all
 *   arguments must match.
 */
bool command_compare(command_t *cmd1, command_t *cmd2);

/*
 * Returns true if this command is empty, false otherwise.  An "empty" command has:
 *    input = stdin
 *    output = stdout
 *    no arguments
 *
 * Parameters:
 *   cmd          The command to evaluate
 * 
 * Returns:
 *   True if cmd is empty, false otherwise
 */
bool command_is_empty(command_t *cmd);

/*
 * Return the count of arguments on this command
 *
 * Parameters:
 *   cmd    The command to examine
 * 
 * Returns:
 *   The number of arguments, which could be 0
 */
int command_get_argc(command_t *cmd);

/*
 * Append a new argument to this command.
 *
 * Parameters:
 *   cmd      The command
 *   arg      The argument to append (which will be copied aside)
 * 
 * Returns:
 *   0 on success, -1 on failure (which could only be "out of memory")
 */
int command_append_arg(command_t *cmd, const char *arg);

/*
 * Get a pointer to the NULL-terminated argv vector for this command
 *
 * Parameters:
 *   cmd     The command
 * 
 * Returns:
 *   A pointer to a NULL-terminated argv vector for this command. The
 *   vector will be valid until a subsequent call to command_append_arg()
 *   or command_free()
 */
char * const * command_get_argv(command_t *cmd);


#endif /* _COMMAND_H_ */
