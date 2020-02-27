/**
 * Sudoku board implementation
 *
 * Created by Gabriel Tofvesson
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "board.h"


/**
 * Raises an error by printing it to stderr and stopping execution
 */
#define ERROR(reason)                     \
{                                         \
  fprintf(stderr, "ERROR: %s\n", reason); \
  abort();                                \
}


/**
 * Check if a given xy-pair is in bounds of a Sudoku board
 */
static inline bool
is_in_bounds (board_pos x, board_pos y)
{
  return  x >= 0 &&
          x <  9 &&
          y >= 0 &&
          y <  9 ;
}

/**
 * Check if a given element value is within acceptable bounds
 */
static inline bool
is_valid_value (element_value value)
{
  return  value >= 0 &&
          value <  9 ;
}


void
board_init (struct board *board)
{
  memset (board, 0, sizeof board->elements);
  board->complexity = 0;
}


void
board_set (
  struct board *board,
  board_pos x,
  board_pos y,
  element_value value
)
{
  if (is_in_bounds (x, y) && is_valid_value (value))
  {
    struct board_element *elem = BOARD_ELEM (board, x, y);
    elem->has_value = true;
    elem->value = value;
  }
  else ERROR("Invalid parameters to function board_set()");
}


void 
board_mark (
  struct board *board,
  board_pos x,
  board_pos y,
  element_value value
)
{
  if (is_in_bounds (x, y) && is_valid_value (value))
  {
    struct board_element *elem = BOARD_ELEM (board, x, y);
    elem->potential |= 1 << value;
  }
  else ERROR("Invalid parameters to function board_mark()");
}


void 
board_unmark (
  struct board *board,
  board_pos x,
  board_pos y,
  element_value value
)
{
  if (is_in_bounds (x, y) && is_valid_value (value))
  {
    struct board_element *elem = BOARD_ELEM (board, x, y);

    /* Shift bit to correct place and then invert first 9 bits */
    elem->potential &= (1 << value) ^ 0x1FF;
  }
  else ERROR("Invalid parameters to function board_unmark()");
}


bool
board_has_value (
  struct board *board,
  board_pos x,
  board_pos y
)
{
  if (is_in_bounds (x, y))
  {
    return BOARD_ELEM (board, x, y)->has_value;
  }
  else ERROR("Invalid parameters to function board_has_value()");
}


element_value
board_get_value (
  struct board *board,
  board_pos x,
  board_pos y
)
{
  if (is_in_bounds (x, y))
  {
    return BOARD_ELEM (board, x, y)->value;
  }
  else ERROR("Invalid parameters to function board_get_value()");
}


bool
board_is_marked (
  struct board *board,
  board_pos x,
  board_pos y,
  element_value value
)
{
  if (is_in_bounds (x, y) && is_valid_value (value))
  {
    return BOARD_ELEM (board, x, y)->potential & (1 << value);
  }
  else ERROR("Invalid parameters to function board_is_marked()");
}
