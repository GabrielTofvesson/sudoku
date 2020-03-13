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
 * Raises an error if the given condition is not met
 * The error will be the given reason
 */
#ifdef OPTIMIZE
#define ASSERT(cond, reason)
#else
#define ASSERT(cond, reason)              \
{                                         \
  if (! cond) ERROR ((reason));           \
}
#endif


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
meta_init (struct metadata *meta)
{
  memset (meta, 0, sizeof (struct metadata));
}


void
board_init (struct board *board)
{
  struct board_element defval;
  defval.has_value = false;
  defval.potential = 0x1FF;
  defval.complexity = 9;

  for (size_t t = 0; t < (sizeof (board->elements) / sizeof (struct board_element)); ++t)
    memcpy (&board->elements[t], &defval, sizeof (struct board_element));

  board->complexity = 9;
  
  for (unsigned i = 0; i < 9; ++i)
  {
    meta_init (&board->meta_quad[i]);
    meta_init (&board->meta_row[i]);
    meta_init (&board->meta_col[i]);
  }
}


bool
meta_has_value (const struct metadata *meta, element_value value)
{
  return ((meta->marked >> value) & 1) == 1;
}


void
meta_set_value (struct metadata *meta, element_value value)
{
  meta->marked |= 1 << value;
}


void
meta_clear_values (struct metadata *meta)
{
  meta->marked = 0;
}


static inline void
meta_mark (struct metadata *meta, element_value value, unsigned index)
{
  meta_set_value (meta, value);

  unsigned char count = meta->unique[value].count;
  if (count == 0)
  {
    meta->unique[value].count = 1;
    meta->unique[value].index = index;
  }
  else
  {
    meta->unique[value].count = 2;
  }
}


void
board_meta_quad_refresh (struct board *board, board_pos qx, board_pos qy)
{
  struct metadata *meta = BOARD_QUAD (board, qx * 3, qy * 3);
  board_pos quad_base_x = qx * 3;
  board_pos quad_base_y = qy * 3;

  meta_clear_values (meta);

  for (board_pos off_y = 0; off_y < 3; ++off_y)
    for (board_pos off_x = 0; off_x < 3; ++off_x)
    {
      struct board_element *elem =
        BOARD_ELEM (board, quad_base_x + off_x, quad_base_y + off_y);

      if (elem->has_value)
        meta_mark (meta, elem->value, (off_y * 3) + off_x);
    }
}


void
board_meta_row_refresh (struct board *board, board_pos y)
{
  struct metadata *meta = BOARD_ROW (board, y);
  
  meta_clear_values (meta);

  for (board_pos x = 0; x < 9; ++x)
  {
    struct board_element *elem = BOARD_ELEM (board, x, y);

    if (elem->has_value)
      meta_mark (meta, elem->value, x);
  }
}


void
board_meta_col_refresh (struct board *board, board_pos x)
{
  struct metadata *meta = BOARD_COL (board, x);
  
  meta_clear_values (meta);

  for (board_pos y = 0; y < 9; ++y)
  {
    struct board_element *elem = BOARD_ELEM (board, x, y);

    if (elem->has_value)
      meta_mark (meta, elem->value, y);
  }
}


bool
board_meta_can_set (
  const struct board *board,
  board_pos x,
  board_pos y,
  element_value value
)
{
  if (is_in_bounds (x, y) && is_valid_value (value))
  {
    return ! (
      meta_has_value (BOARD_ROW (board, y), value) ||
      meta_has_value (BOARD_COL (board, x), value) ||
      meta_has_value (BOARD_QUAD (board, x, y), value)
    );
  }
  else ERROR("Invalid parameters to function board_meta_can_set()");
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
    ASSERT (
      board_meta_can_set (board, x, y, value),
      "Attempt to set impossible value on board"
    );

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
    ASSERT (
      ! board_has_value (board, x, y),
      "Attempt to mark element with value"
    );

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
    ASSERT (
      ! board_has_value (board, x, y),
      "Attempt to mark element with value"
    );

    struct board_element *elem = BOARD_ELEM (board, x, y);

    if (board_is_marked (board, x, y, value))
    {
      /* Shift bit to correct place and then invert first 9 bits */
      elem->potential ^= (1 << value);
      --(elem->complexity);
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
  const struct board *board,
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
board_is_valid (struct board *board)
{
  for (board_pos y = 0; y < 9; ++y)
    for (board_pos x = 0; x < 9; ++x)
      if (
            !board_has_value (board, x, y) &&
            BOARD_ELEM (board, x, y)->potential == 0
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
    elem->potential |= BOARD_QUAD (board, x, y)->marked;
    elem->potential |= BOARD_ROW (board, y)->marked;
    elem->potential |= BOARD_COL (board, x)->marked;

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


static inline bool
field_is_potential (unsigned short field, element_value value)
{
  return ((field >> (value * 2)) & 3) < 2;
}


static inline void
field_invalidate (unsigned short *field, element_value value)
{
  *field |= 2 << (value * 2);
}


static inline void
field_increment (unsigned short *field, element_value value)
{
  if (field_is_potential (*field, value))
    *field += (1 << (value * 2));
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
    if (board_meta_can_set (board, x, y, value))
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

      /* Update metadata */
      meta_set_value (BOARD_QUAD (board, x, y), value);
      meta_set_value (BOARD_ROW  (board, y), value);
      meta_set_value (BOARD_COL  (board, x), value);

      return true;
    }
    else return false;
  }
  else ERROR("Invalid parameters to function board_place()");
}


struct board *
board_place_speculative (
  const struct board *board,
  struct board *board_duplicate,
  board_pos x,
  board_pos y,
  element_value value
)
{
  if (is_in_bounds (x, y) && is_valid_value (value))
  {
    /* Ensure value can be placed*/
    if (board_meta_can_set (board, x, y, value))
    {
      /* Create duplicate and place value */
      board_copy (board, board_duplicate);

      if (! board_place (board_duplicate, x, y, value) || ! board_is_valid (board_duplicate))
        return NULL;

      board_refresh_complexity (board_duplicate);

      return board_duplicate;
    }
    else return NULL;
  }
  else ERROR("Invalid parameters to function board_place_speculative()");
}


bool
board_refresh_complexity (struct board *board)
{
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
