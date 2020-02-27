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

/**
 * Initialize a board to a blank state with 0 complexity
 */
void
board_init (struct board *board);


/**
 * Set the value of an element on the board
 * 
 * NOTE: This marks an element as having a decided value
 */
void
board_set (
  struct board *board,
  board_pos x,
  board_pos y,
  element_value value
);

/**
 * Mark a potential value of an element on the board
 *
 * NOTE: Marking an element with a decided value is undefined
 */
void
board_mark (
  struct board *board,
  board_pos x,
  board_pos y,
  element_value value
);


/**
 * Removes a marking of a potential value of an element on the board
 *
 * NOTE: Unmarking an element with a decied value is undefined
 */
void
board_unmark (
  struct board *board,
  board_pos x,
  board_pos y,
  element_value value
);


/**
 * Get whether or not an element has a decided value
 */
bool
board_has_value (
  struct board *board,
  board_pos x,
  board_pos y
);


/**
 * Get definite value of a board element
 *
 * NOTE: Getting the definite value of an element without a value is undefined
 */
element_value
board_get_value (
  struct board *board,
  board_pos x,
  board_pos y
);


/**
 * Get whether or not a board element is marked with a particular value
 *
 * NOTE: Getting the mark state of an element with a value is undefined
 */
bool
board_is_marked (
  struct board *board,
  board_pos x,
  board_pos y,
  element_value value
);


/**
 * Checks if placing the given value at the given position on the board would
 * leave the board in a valid state
 */
bool
board_can_place_value (
  struct board *board,
  board_pos x,
  board_pos y,
  element_value value
);


/**
 * Checks if there are any pairs of board elements that share a value and
 * also share either a row or a column
 */
bool
board_is_valid (struct board *board);
