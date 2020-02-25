#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include "board.h"


/**
 * ANSI control codes
 */
#define COLOUR_RED    "\e[31m"
#define COLOUR_RESET  "\e[0m"
#define CLEAR         "\033[2J"



/**
 * How many boards to allocate by default for board spec
 * Average depth for highly complex boards is between 10 and 12
 */
#define DEFAULT_DEPTH 10

/**
 * How many boards to allocate per depth increase
 * Higher values mean fewer reallocations, by with greater resource costs
 */
#define DEPTH_INCREMENT 3


struct boards_table {
  struct board **board_specs;
  unsigned long long max_depth;
};


void
tables_ensure_depth (struct boards_table *board_spec, unsigned long long depth)
{
  if (board_spec->max_depth <= depth)
  {
    /* Compute new max depth */
    unsigned long long new_max;
    if (board_spec->max_depth == 0)
      new_max = DEFAULT_DEPTH;
    else
      new_max = board_spec->max_depth + DEPTH_INCREMENT;

    /* Disregard NULL return value. What's it gonna do, segfault? :P */
    board_spec->board_specs = realloc (
        board_spec->board_specs,
        sizeof (struct board *) * new_max
    );

    /* Allocate boards */
    for (unsigned long long l = board_spec->max_depth; l < new_max; ++l)
      board_spec->board_specs[l] = malloc (sizeof (struct board));

    /* Update max depth */
    board_spec->max_depth = new_max;
  }
}

bool
simplify (struct boards_table *board_spec, unsigned long long depth);


struct board_file
{
  int fd;
  void *data;
};


/**
 * Determines if the given char is a valid character for defining
 * a board element
 */
static inline bool
is_valid_def (char def)
{
  return  def == ' '    ||
          (
            def >= '0'  &&
            def <= '9'
          );
}

/**
 * Load a board-definition file and return file descriptor
 */
static struct board_file
load_board_file (const char *path)
{
  struct board_file file;
  file.fd = -1;
  file.data = NULL;

  /* Open file */
  int fd = open (path, 0);
  if (fd < 0)
    return file;

  /* Map 89 bytes of file to memory */
  void *region = mmap (NULL, 89, PROT_READ, MAP_SHARED, fd, 0);
  if (region == -1)
  {
    close (fd);
    return file;
  }

  /*
    Ensure data in file is formatted correctly:
      1) Every 10th character is ignored (row-terminator)
      2) Each character must be in range '0'-'9' or ' '
  */
  char *data = region;
  for (unsigned i = 1; i <= 89; ++i)
    if (!((i % 10) == 0) && !is_valid_def (data[i - 1]))
    {
      munmap (region, 89);
      close (fd);
      return file;
    }

  /* Save file data to struct and return */
  file.fd = fd;
  file.data = region;

  return file;
}

/**
 * Free all resources linked to an opened board definition file
 */
static void
close_board_file (struct board_file file)
{
  close (file.fd);
  munmap (file.data, 89);
}

static void
copy_to_board (struct board_file file, struct board *board)
{
  // Clear board
  board_init (board);

  char *data = file.data;
  for (unsigned i = 1; i <= 89; ++i)
    if ((i % 10) != 0)
    {
      board_pos x = i % 10;
      board_pos y = i / 10;

      if (data[i - 1] != ' ')
        board_set (board, x - 1, y, data[i - 1] - '0' - 1);
    }
}


static void
ansi_set_cursor (unsigned y, unsigned x)
{
  printf ("\033[%u;%uH", x + 1, y + 1);
}


static void
ansi_clear_screen ()
{
  puts (CLEAR);
}


static void
print_board (const struct board *board, const struct board *compare, unsigned whence_x, unsigned whence_y)
{
  for (board_pos y = 0; y < 9; ++y)
  {
    /* Print row */
    for (board_pos x = 0; x < 9; ++x)
    {
      ansi_set_cursor (whence_x + (x * 2), whence_y + (y * 2));

      /* Print board element */
      if (board_has_value (board, x, y))
      {
        if (compare != NULL && ! board_has_value (compare, x, y))
          printf (COLOUR_RED "%u" COLOUR_RESET, board_get_value (board, x, y) + 1);
        else
          printf ("%u", board_get_value (board, x, y) + 1);
      }
      else
        fputs (" ", stdout);

      /* Print column element delimiter */
      if (x < 8)
        fputs("|", stdout);
    }
    
    /* Print row element delimiter */
    if (y < 8)
    {
      for (board_pos x = 0; x < 17; ++x)
      {
        ansi_set_cursor (whence_x + x, whence_y + (y * 2 + 1));
        if ((x & 1) == 0)
          fputs ("-", stdout);
        else
          fputs ("+", stdout);
      }
    }
  }
}


/**
 * Compute first potential value of a given element
 */
element_value
first_potential_value (struct board_element *element, struct board *board, bool *error)
{
  unsigned short potential = element->potential;
  if (potential == 0)
  {
    *error = true;
    return 0;
  }

  *error = false;
  
  element_value value = 0;
  while (potential != 0)
  {
    if (potential & 1 == 1)
      return value;
    ++value;
    potential >>= 1;
  }
  
  /* Unreachable */
  abort();
}


/**
 * Reduce away all elements on board with complexity=1 until none remain
 */
bool
simplify (struct boards_table *board_specs, unsigned long long depth)
{
  /* Get current table */
  struct board *board = board_specs->board_specs[depth];

  bool error;
  /* Reduce using low-complexity computation */
  while (board->complexity == 1)
  {
    for (board_pos y = 0; y < 9; ++y)
      for (board_pos x = 0; x < 9; ++x)
        if (! board_has_value (board, x, y))
        {
          struct board_element *elem = BOARD_ELEM (board, x, y);
          if (elem->complexity == 1)
          {
            element_value value = first_potential_value (elem, board, &error);
            if (error) return false;

            board_place (board, x, y, value);
          }
        }
  }

  /* Attempt to reduce with speculative placement */
  if (board->complexity > 1)
  {
    for (board_pos y = 0; y < 9; ++y)
      for (board_pos x = 0; x < 9; ++x)
      {
        struct board_element *elem = BOARD_ELEM (board, x, y);
        /* Find a simplest element on the board*/
        if (
            ! board_has_value (board, x, y) &&
            elem->complexity == board->complexity
        )
          for (element_value value = 0; value < 9; ++value)
          {
            /* Try speculative placement of each potential value and recurse */
            tables_ensure_depth (board_specs, depth + 1);
            if ((elem->potential & (1 << value)) != 0)
            {
              struct board *board_spec =
                board_place_speculative (
                  board,
                  board_specs->board_specs[depth + 1],
                  x,
                  y,
                  value
                );

              /* If speculative placement failed, try another value */
              if (board_spec == NULL)
                continue;

              /* Found solution */
              if (simplify (board_specs, depth + 1) && board_spec->complexity == 0)
              {
                board_copy (board_spec, board);
                x = 9;
                y = 9;
                value = 9;
              }
            }
          }
      }
  }
  return true;
}


int
main (int argc, char **argv, char **env)
{
  if (argc != 2) return -1;
  
  struct board_file file = load_board_file (argv[1]);
  if (file.fd == -1 || file.data == NULL)
    return -1;

  /* Allocate board */
  struct board original;

  struct boards_table boards;
  boards.max_depth = 0;
  boards.board_specs = NULL;
  tables_ensure_depth (&boards, 0);

  struct board *root_board = boards.board_specs[0];

  copy_to_board (file, &original);
  board_copy (&original, boards.board_specs[0]);

  close_board_file (file);

  board_update_all_marks (root_board);
  
  ansi_clear_screen ();

  print_board (root_board, NULL, 0, 0);

  if (! board_is_valid (root_board))
  {
    puts ("Supplied board is not valid!");
    return 1;
  }

  ansi_set_cursor (0, 18);
  puts("Reducing...");

  /* Profiler start time */
  clock_t start_clk = clock ();
  
  simplify (&boards, 0);

  /* Profiler end time */
  clock_t end_clk = clock ();

  print_board (root_board, &original, 21, 0);
 
  ansi_set_cursor (0, 18);
  printf ("Simplification took %Lf seconds\n", ((long double)(end_clk - start_clk))/CLOCKS_PER_SEC);

  return 0;
}
