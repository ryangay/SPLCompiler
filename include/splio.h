#ifndef SPL_IO_H
#define SPL_IO_H

#include <stdio.h>

/* Contains ANSI codes to change the colour of text output to the terminal */

#define COLOUR_RESET  "\033[0m"
#define BOLD         "\033[1m"
#define BLACK_TEXT   "\033[30;1m"
#define RED_TEXT     "\033[31;1m"
#define GREEN_TEXT   "\033[32;1m"
#define YELLOW_TEXT  "\033[33;1m"
#define BLUE_TEXT    "\033[34;1m"
#define MAGENTA_TEXT "\033[35;1m"
#define CYAN_TEXT    "\033[36;1m"
#define WHITE_TEXT   "\033[37;1m"

/* Macro with variable-length arguments. https://stackoverflow.com/a/679993 */
#ifdef PRINT_INFO
#define INFO(s, ...) fprintf(stderr, WHITE_TEXT "Info: " COLOUR_RESET s, ##__VA_ARGS__);
#define TREE_INFO(s, ...) fprintf(stderr, BLUE_TEXT "Tree: " COLOUR_RESET s, ##__VA_ARGS__);
#else
#define INFO(s, ...)
#define TREE_INFO(s, ...) 
#endif

#define ERROR(line, col, s, ...) fprintf(stderr, RED_TEXT  "Error: " COLOUR_RESET BOLD "(%d,%d): " COLOUR_RESET s, line, col, ##__VA_ARGS__);
#define WARNING(line, col, s, ...) fprintf(stderr, YELLOW_TEXT "Warning: " COLOUR_RESET BOLD "(%d,%d): " COLOUR_RESET s, line, col, ##__VA_ARGS__);

#endif