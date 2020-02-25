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
#ifdef OPTIMIZE
  return true;
#else
  return  x >= 0 &&
          x <  9 &&
          y >= 0 &&
          y <  9 ;
#endif
}

/**
 * Check if a given element value is within acceptable bounds
 */
static inline bool
is_valid_value (element_value value)
{
#ifdef OPTIMIZE
  return true;
#else
  return  value >= 0 &&
          value <  9 ;
#endif
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
  const struct board *board,
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
  const struct board *board,
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
  const struct board *board,
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
    unsigned short potential = elem->potential;
    while (potential != 0)
    {
      if ((potential & 1) == 1)
        ++(elem->complexity);
      potential >>= 1;
    }

    /* Store complexity if element is the simplest */
    if (elem->complexity < board->complexity)
      board->complexity = elem->complexity;
  }
  else ERROR("Invalid parameters to function board_update_marks()");
}


bool
board_can_quad_set_value (
  struct board *board,
  board_pos x,
  board_pos y,
  element_value value
)
{
  if (is_in_bounds (x, y) && is_valid_value (value))
  {
    struct board_element *elem = BOARD_ELEM (board, x, y);

    /* Compute quadrant bases */
    board_pos quad_x = TO_QUAD (x);
    board_pos quad_y = TO_QUAD (y);

    /* Compute sub-quadrant positions */
    board_pos simp_x = x % 3;
    board_pos simp_y = y % 3;


    bool next = false;

    /* Check along x-axis */
    for (board_pos base_x = 0; base_x < 9; base_x += 3)
    {
      next = false;
      if (base_x != quad_x)
      {
        for (board_pos check_y = 0; check_y < 3 && ! next; ++check_y)
          if (check_y != simp_y)
            for (board_pos check_x = 0; check_x < 3 && ! next; ++check_x)
            {
              board_pos target_x = base_x + check_x;
              board_pos target_y = quad_y + check_y;

              bool has_value = board_has_value (board, target_x, target_y);

              /* Check if a quadrant can contain the given value */
              if (
                  (
                    has_value &&
                    BOARD_ELEM (board, target_x, target_y)->value == value
                  ) ||
                  (
                    ! has_value &&
                    board_is_marked (board, target_x, target_y, value)
                  )
              )
              {
                next = true;
                break;
              }
            }
        if (! next) 
          return false;
      }
    }

    /* Check along y-axis */
    for (board_pos base_y = 0; base_y < 9; base_y += 3)
    {
      next = false;
      if (base_y != quad_y)
      {
        for (board_pos check_x = 0; check_x < 3 && ! next; ++check_x)
          if (check_x != simp_x)
            for (board_pos check_y = 0; check_y < 3 && ! next; ++check_y)
            {
              board_pos target_x = quad_x + check_x;
              board_pos target_y = base_y + check_y;

              bool has_value = board_has_value (board, target_x, target_y);

              /* Check if a quadrant can contain the given value */
              if (
                  (
                    has_value &&
                    BOARD_ELEM (board, target_x, target_y)->value == value
                  ) ||
                  (
                    ! has_value &&
                    board_is_marked (board, target_x, target_y, value)
                  )
              )
              {
                next = true;
                break;
              }
            }
        if (! next) 
          return false;
      }
    }

    return true;
  }
  else ERROR("Invalid parameters to function board_can_quad_set_value()");
}


/**
 * Compute amount of unset board elements in a quadrant with a given potential
 * bit field value marked.
 */
struct count_result
board_count_quad_potentials (
  struct board *board,
  board_pos x,
  board_pos y,
  element_value value
)
{
  if (is_in_bounds (x, y) && is_valid_value (value))
  {
    board_pos quad_x = TO_QUAD (x);
    board_pos quad_y = TO_QUAD (y);

    struct count_result result;
    result.count = 0;
    result.unique = NULL;
    result.is_set = false;

    unsigned char count = 0;
    for (board_pos check_y = 0; check_y < 3; ++check_y)
      for (board_pos check_x = 0; check_x < 3; ++check_x)
      {
        board_pos target_x = quad_x + check_x;
        board_pos target_y = quad_y + check_y;

        bool has_value = board_has_value (board, target_x, target_y);

        if (! has_value && board_is_marked (board, target_x, target_y, value))
        {
          ++result.count;
          if (result.count == 1)
            result.unique = BOARD_ELEM (board, target_x, target_y);
          else
            result.unique = NULL;
        }
        else if (
                  has_value &&
                  board_get_value (board, target_x, target_y) == value
        )
        {
          result.count = 1;
          result.is_set = true;
          return result;
        }

      }

    return result;
  }
  else ERROR("Invalid parameters to function board_count_quad_potentials()");
}


bool
board_update_marks_by_quad (
  struct board *board,
  board_pos x,
  board_pos y
)
{
  if (is_in_bounds (x, y))
  {
    bool changed = false;

    for (element_value value = 0; value < 9; ++value)
    {
      struct count_result result =
        board_count_quad_potentials (
          board,
          x,
          y,
          value
        );

      /* No need to check populated values */
      if (result.is_set)
        continue;

      /* If there is only one element that can hold a value, mark it */
      if (result.unique != NULL && result.unique->potential != (1 << value))
      {
        changed |= true;
        result.unique->potential = 1 << value;
        result.unique->complexity = 1;
        board->complexity = 1;
      }
    }
    return changed;
  }
  else ERROR("Invalid parameters to function board_update_marks_by_quad()");
}


bool
board_update_marks_by_excl (
  struct board *board,
  board_pos x,
  board_pos y
)
{
  if (is_in_bounds (x, y))
  {
    bool changed = false;

    struct board_element *elem = BOARD_ELEM (board, x, y);
    unsigned short potential = elem->potential;
    unsigned char value = 0;

    /* Invert bit field for element removal */
    elem->potential ^= 0x1FF;

    while (potential != 0)
    {
      if ((potential & 1) == 1)
      {
        /* If setting value would make another quad unsolvable, un-flip bit */
        if (! board_can_quad_set_value (board, x, y, value))
        {
          changed |= true;
          elem->potential |= 1 << value;
        }
      }

      ++value;
      potential >>= 1;
    }

    /* Revert bit field inversion */
    elem->potential ^= 0x1FF;
    return changed;
  }
  else ERROR("Invalid parameters to function board_update_marks_by_excl()");
}


void
board_update_all_marks (struct board *board)
{
  for (board_pos y = 0; y < 9; ++y)
    for (board_pos x = 0; x < 9; ++x)
      if (! board_has_value (board, x, y))
        board_update_marks (board, x, y);

  bool changed;
  do
  {
    changed = false;

    /* Update marks by exclusion analysis */
    for (board_pos y = 0; y < 9; ++y)
      for (board_pos x = 0; x < 9; ++x)
        if (! board_has_value (board, x, y))
          changed |= board_update_marks_by_excl (board, x, y);

    /* Update marks by quad potential analysis */
    for (board_pos y = 0; y < 9; y += 3)
      for (board_pos x = 0; x < 9; x += 3)
        changed |= board_update_marks_by_quad (board, x, y);

  } while (changed);
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

      /* Update quad */
      board_pos quad_x = TO_QUAD (x);
      board_pos quad_y = TO_QUAD (y);

      for (board_pos unmark_y = 0; unmark_y < 3; ++unmark_y)
        for (board_pos unmark_x = 0; unmark_x < 3; ++unmark_x)
        {
          board_pos target_x = quad_x + unmark_x;
          board_pos target_y = quad_y + unmark_y;

          /* Unmark value for all unset elements in quad */
          if (
              (target_x != x || target_y != y) &&
              !board_has_value (board, target_x, target_y)
          )
            board_unmark (board, target_x, target_y, value);
        }

        /* Set value */
      board_set (board, x, y, value); 

      /* Update board complexity */
      return board_refresh_complexity (board);
    }
    else return false;
  }
  else ERROR("Invalid parameters to function board_place()");
}


struct board *
board_place_speculative (
  const struct board *board,
  board_pos x,
  board_pos y,
  element_value value
)
{
  if (is_in_bounds (x, y) && is_valid_value (value))
  {
    /* Ensure value can be placed*/
    if (board_can_place_value (board, x, y, value))
    {
      /* Create duplicate and place value */
      struct board *board_duplicate = malloc (sizeof (struct board));
      board_copy (board, board_duplicate);

      if (! board_place (board_duplicate, x, y, value))
      {
        free (board_duplicate);
        return NULL;
      }

      return board_duplicate;
    }
    else return NULL;
  }
  else ERROR("Invalid parameters to function board_place_speculative()");
}


bool
board_refresh_complexity (struct board *board)
{
  board_update_all_marks (board);

  board->complexity = 10;
  for (board_pos y = 0; y < 9; ++y)
    for (board_pos x = 0; x < 9; ++x)
      if (! board_has_value (board, x, y))
      {
        struct board_element *elem = BOARD_ELEM (board, x, y);
        if (elem->complexity < board->complexity)
        {
          /* If complexity is somehow 0, we have an invalid board state */
          if (elem->complexity == 0)
            return false;

          board->complexity = elem->complexity;

          /* Short-circuit function on comlpexity=1, since it can't go lower */
          if (board->complexity == 1)
            return true;
        }
      }

  /* If there are no complex board elements, board is solved */
  if (board->complexity == 10)
    board->complexity = 0;

  return true;
}


void
board_copy (const struct board *board_from, struct board *board_to)
{
  memcpy (board_to, board_from, sizeof(struct board));
}
