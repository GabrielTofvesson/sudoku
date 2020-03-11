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
  memset (board, 0, sizeof board->elements);
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
  struct metadata *meta = BOARD_QUAD (board, qx, qy);
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
      meta_has_value (BOARD_QUAD (board, TO_QUAD (x), TO_QUAD (y)), value)
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
board_is_valid (struct board *board)
{
  for (board_pos y = 0; y < 9; ++y)
    for (board_pos x = 0; x < 9; ++x)
      if (
            board_has_value (board, x, y) &&
            ! board_meta_can_set (
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
    elem->potential |= BOARD_QUAD (board, TO_QUAD (x), TO_QUAD (y))->marked;
    elem->potential |= BOARD_ROW (board, y)->marked;
    elem->potential |= BOARD_COL (board, x)->marked;

//  for (board_pos check_x = 0; check_x < 9; ++check_x)
//    if (check_x != x && board_has_value (board, check_x, y))
//      elem->potential |= (1 << BOARD_ELEM (board, check_x, y)->value);

//  /* Check y-axis */
//  for (board_pos check_y = 0; check_y < 9; ++check_y)
//    if (check_y != y && board_has_value (board, x, check_y))
//      elem->potential |= (1 << BOARD_ELEM (board, x, check_y)->value);

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

static bool
element_shares_value (struct board *board, board_pos x, board_pos y, element_value value)
{
  struct board_element *elem = BOARD_ELEM (board, x, y);
  return (elem->has_value && elem->value == value) || (! elem->has_value && (elem->potential & (1 << value) != 0));
}

static inline bool
field_is_potential (unsigned short field, element_value value)
{
  return (field >> (value * 2)) & 3 < 2;
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

static void
field_populate_values(struct board *board, board_pos x, board_pos y, unsigned short *field)
{
  struct board_element *elem = BOARD_ELEM (board, x, y);
  if (elem->has_value)
  {
    field_invalidate (field, elem->value);
  }
  else
  {
    unsigned short potential = elem->potential;
    element_value value = 0;
    while (potential != 0)
    {
      if (potential & 1 == 1)
        field_increment (field, value);

      potential >>= 1;
      ++value;
    }
  }
}

static bool
board_update_marks_by_rows (struct board *board)
{
  bool changed = false;

  for (board_pos y = 0; y < 9; ++y)
  {
    unsigned short field = 0;
    for (board_pos x = 0; x < 9; ++x)
      field_populate_values (board, x, y, &field);

    element_value value = 0;
    while (field != 0)
    {
      if (field & 3 == 1)
      {
        for (board_pos x = 0; x < 9; ++x)
          if (element_shares_value (board, x, y, value))
          {
            struct board_element *elem = BOARD_ELEM (board, x ,y);
            elem->complexity = 1;
            changed |= elem->potential != (elem->potential = (1 << value));
          }
      }

      field >>= 2;
      ++value;
    }
  }
  return changed;
}

static bool
board_update_marks_by_cols (struct board *board)
{
  bool changed = false;

  for (board_pos x = 0; x < 9; ++x)
  {
    unsigned short field = 0;
    for (board_pos y = 0; y < 9; ++y)
      field_populate_values (board, x, y, &field);

    element_value value = 0;
    while (field != 0)
    {
      if (field & 3 == 1)
      {
        for (board_pos y = 0; y < 9; ++y)
          if (element_shares_value (board, x, y, value))
          {
            struct board_element *elem = BOARD_ELEM (board, x ,y);
            elem->complexity = 1;
            changed |= elem->potential != (elem->potential = (1 << value));
          }
      }

      field >>= 2;
      ++value;
    }
  }
  return changed;
}


void
board_update_all_marks (struct board *board)
{
  for (board_pos y = 0; y < 9; ++y)
    for (board_pos x = 0; x < 9; ++x)
      if (! board_has_value (board, x, y))
        board_update_marks (board, x, y);

  return;
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

    changed |= board_update_marks_by_rows (board);
    changed |= board_update_marks_by_cols (board);

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

      /* Update metadata */
      meta_set_value (BOARD_QUAD (board, quad_x, quad_y), value);
      meta_set_value (BOARD_ROW  (board, y), value);
      meta_set_value (BOARD_COL  (board, x), value);

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

      if (! board_place (board_duplicate, x, y, value))
        return NULL;

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
