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


bool
board_can_place_value (
  struct board *board,
  board_pos x,
  board_pos y,
  element_value value
)
{
  if (is_in_bounds (x, y) && is_valid_value (value))
  {
    /* Check for x-axis */
    for (board_pos compare_x = 0; compare_x < 9; ++compare_x)
    {
      if (compare_x == x)
        continue;
      else if (
                board_has_value (board, compare_x, y) &&
                board_get_value (board, compare_x, y) == value
              )
        return false;
    }

    /* Check for y-axis */
    for (board_pos compare_y = 0; compare_y < 9; ++compare_y)
    {
      if (compare_y == y)
        continue;
      else if (
                board_has_value (board, x, compare_y) &&
                board_get_value (board, x, compare_y) == value
      )
        return false;
    }
    
    /* No matches */
    return true;
  }
  else ERROR("Invalid parameters to function board_can_place_value()");
}


bool
board_is_valid (struct board *board)
{
  for (board_pos y = 0; y < 9; ++y)
    for (board_pos x = 0; x < 9; ++x)
      if (
            board_has_value (board, x, y) &&
            ! board_can_place_value (
                board,
                x,
                y,
                BOARD_ELEM (board, x, y)->value
              )
      )
        return false;
  return true;
}
