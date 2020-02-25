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

/**
 * Convert any valid board position to a quadrant base position (lowest index)
 */
#define TO_QUAD(pos) (((pos) / 3) * 3)

typedef unsigned short int board_pos;
typedef unsigned char element_value;

/**
 * Simple definition of a board element. Vaild values range: 0-8
 * Complexity describes how many possible values would be valid for an instance
 * of a board element. For example, if it could be either 1 or 5, it would have
 * a complexity of 2. If it could hold 4, 5 or 8, it would have a complexity
 * of 3
 */
struct board_element {
  bool has_value : 1;                 /* Whether element has a decided value */

  union {
    element_value    value      : 4;  /* Value of element */
    struct {
      unsigned short potential  : 9;  /* Bitfield of possible values */
      unsigned char  complexity : 4;  /* Complexity tracker */
    };
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
  const struct board *board,
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
  const struct board *board,
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
  const struct board *board,
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


/**
 * Check row and column of given position and mark potential values that could
 * be set at that position and still leave the board in a valid state
 */
void
board_update_marks (
  struct board *board,
  board_pos x,
  board_pos y
);


/**
 * Check if setting a value at a given position would prevent a quadrant from
 * setting a certain value
 */
bool
board_can_quad_set_value (
  struct board *board,
  board_pos x,
  board_pos y,
  element_value value
);


/**
 * Structure describing result of a value search in a quadrant
 */
struct count_result
{
  bool is_set;
  unsigned char count : 4;
  struct board_element *unique;
};


/**
 * Compute amount of unset board elements in a quadrant with a given potential
 * bit field value 
 */
struct count_result
board_count_quad_potentials (
  struct board *board,
  board_pos x,
  board_pos y,
  element_value value
);


/**
 * Update potential values by analysing other potential values in quadrant
 */
bool
board_update_marks_by_quad (
  struct board *board,
  board_pos x,
  board_pos y
);


/**
 * Update potential values by analysing if it would prevent other quadrants
 * from setting a certain value
 */
bool
board_update_marks_by_excl (
  struct board *board,
  board_pos x,
  board_pos y
);


/**
 * Refreshes marks of all board elements without a value
 */
void
board_update_all_marks (struct board *board);


/**
 * Attempt to set value at the given position
 * If value can be placed, this updates all elements without a value on the
 * same row or column that do not have a value so that their markers reflect
 * the addition of this value and returns true
 * If value cannot be placed, just return false
 */
bool
board_place (
  struct board *board,
  board_pos x,
  board_pos y,
  element_value value
);


/**
 * Place a speculative value. This allocates a duplicate board with the element
 * placed at the given location, if possible, while leaving the given board
 * untouched
 *
 * NOTE: If element cannot be placed, this returns NULL
 */
struct board *
board_place_speculative (
  const struct board *board,
  board_pos x,
  board_pos y,
  element_value value
);


/**
 * Recomputes board complexity by searching all elements on board for the
 * lowest complexity
 */
bool
board_refresh_complexity (struct board *board);


/**
 * Copy a board layout to another board
 */
void
board_copy (const struct board *board_from, struct board *board_to);
