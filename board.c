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
  board->complexity = 9;
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
    if (! board_is_marked (board, x, y, value))
    {
      elem->potential |= 1 << value;
      ++(elem->complexity);
    }
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

    if (board_is_marked (board, x, y, value))
    {
      /* Shift bit to correct place and then invert first 9 bits */
      elem->potential &= (1 << value) ^ 0x1FF;
      --(elem->complexity);
      if (elem->complexity < board->complexity)
        board->complexity = elem->complexity;
    }
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


void
board_update_marks (
  struct board *board,
  board_pos x,
  board_pos y
)
{
  if (is_in_bounds (x, y))
  {
    struct board_element *elem = BOARD_ELEM (board, x, y);

    /* Mark all values as impossible */
    elem->potential = 0;
    elem->complexity = 0;

    /* Check x-axis */
    for (board_pos check_x = 0; check_x < 9; ++check_x)
      if (check_x != x && board_has_value (board, check_x, y))
        elem->potential |= (1 << BOARD_ELEM (board, check_x, y)->value);

    /* Check y-axis */
    for (board_pos check_y = 0; check_y < 9; ++check_y)
      if (check_y != y && board_has_value (board, x, check_y))
        elem->potential |= (1 << BOARD_ELEM (board, x, check_y)->value);

    /* Invert matches */
    elem->potential ^= 0x1FF;

    /* Count marked bits */
    unsigned char potential = elem->potential;
    while (potential != 0)
    {
      if (potential & 1 == 1)
        ++(elem->complexity);
      potential >>= 1;
    }

    /* Store complexity if element is the simplest */
    if (elem->complexity < board->complexity)
      board->complexity = elem->complexity;
  }
  else ERROR("Invalid parameters to function board_update_marks()");
}


void
board_update_all_marks (struct board *board)
{
  for (board_pos y = 0; y < 9; ++y)
    for (board_pos x = 0; x < 9; ++x)
      if (! board_has_value (board, x, y))
        board_update_marks (board, x, y);
}


bool
board_place (
  struct board *board,
  board_pos x,
  board_pos y,
  element_value value
)
{
  if (is_in_bounds (x, y) && is_valid_value (value))
  {
    if (board_can_place_value (board, x, y, value))
    {
      /* Unmark x-axis */
      for (board_pos unmark_x = 0; unmark_x < 9; ++unmark_x)
        if (unmark_x != x && ! board_has_value (board, unmark_x, y))
          board_unmark (board, unmark_x, y, value);

      /* Unmark y-axis */
      for (board_pos unmark_y = 0; unmark_y < 9; ++unmark_y)
        if (unmark_y != y && ! board_has_value (board, x, unmark_y))
          board_unmark (board, x, unmark_y, value);

      /* Set value */
      board_set (board, x, y, value); 
      
      return true;
    }
    else return false;
  }
  else ERROR("Invalid parameters to function board_place()");
}
