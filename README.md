## Plaid Shell(plaidsh) - Code for Assignment 5 for ILSE, 04-801 B2, Fall 2022
    
__INTRODUCTION__

This repo contains an improved version of program called “Plaid Shell” which is a usable Linux shell. The previous program was a simple shell that can execute commands such as "pwd, author, exit, quit, and cd". It was written in C and uses the GNU readline library for command line editing. The program was tested on Ubuntu 20.04.3 LTS and ScottyOne (Linux server installed on AWS).

To create a more fully usable shell, the following features were added to the previous version:
- File globbing and tilde expansion
- Setting and using environment variables
- File redirection for standard input and standard output, via the < and > characters
- Finally, commands can now have an arbitrary number of arguments

__DESCRIPTION__
    
 An implementation of plaidsh for ILSE Assignment 5:
 
 Plaid Shell uses the following functions to work as a shell:
 1. int read_word(const char *input, char *word, size_t word_len)
    - Returns the first word from input, removing leading whitespace, handling double quotes, and translating escaped characters.
    - Variables are also expanded via a call to getenv() as they are read. Expansion occurs both inside and outside double quotes. If a variable is not found in the environment, the error message "Undefined variable: '<varname>'" is returned.

 2. command_t *parse_input(const char *input, char *err_msg, size_t err_msg_len)
   - Parses an input line into a newly allocated command_t structure by segmenting the input into words that are bounded by unquoted and unescaped word termination characters.
   - Word termination characters are any unquoted and unescaped whitespace and the redirection characters < and >.

The plaidsh's main() function calls readline() in a loop. Each time readline returns, it prints the result via printf and the left and right arrow keys work and the tab completion of filenames works as well. In addition, the up and down arrow keys works by calling the add_history function from the readline library.


__IMPORTANCE__

Someone may need to use this program if they want to use a shell that can execute commands such as "pwd, author, exit, quit, and cd and others" and for exploration of the GNU readline library.

__GETTING STARTED__

- Clone this repository.
- Run the make command from its containing directory to get the better of it.
- Run the plaidsh executable to start the shell.
- Run the make clean command to clean up the directory.
- Check if it has effects.
- Happy exploration!!
  


 __KEYWORDS__

<mark>ILSE</mark>     <mark>CMU</mark>     <mark>Assignment5</mark>     <mark>plaidsh</mark>     <mark>C Programming</mark>     <mark>Strings</mark> 



  __AUTHOR__
    
 Written by parmenin (Niyomwungeri Parmenide ISHIMWE) at CMU-Africa - MSIT 
    

    
 __DATE__
    
 December 14, 2022" 
