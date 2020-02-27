/**
 * Sudoku board header file
 *
 * Created by Gabriel Tofvesson
 */

#include <stdbool.h>

/**
 * Get a `struct board_element`-entry from a specified location on the board
 */
#define BOARD_ELEM(board_ptr, x, y) (&(board_ptr)->elements[((y) * 9) + (x)])

typedef unsigned short int board_pos;
typedef unsigned char element_value;

/**
 * Simple definition of a board element. Vaild values range: 0-8
 */
struct board_element {
  bool has_value : 1;                 /* Whether element has a decided value */

  union {
    element_value  value     : 4;     /* Value of element */
    unsigned short potential : 9;     /* Bitfield of possible values */
  };
};

/**
 * Board structure representing the state of a Sudoku game
 * Complexity describes 
 *
 * TODO: Replace elements with packed structure
 */
struct board {
  struct board_element elements[81];  /* Game board */
  unsigned char complexity;           /* Complexity of simplest element */
};
