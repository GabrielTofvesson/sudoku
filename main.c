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



struct args {
  bool valid;
  unsigned verbosity : 2;
  char *file_name;
};


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
    {
      board_spec->board_specs[l] = malloc (sizeof (struct board));
      board_make_links (board_spec->board_specs[l]);
    }

    /* Update max depth */
    board_spec->max_depth = new_max;
  }
}

bool
simplify (
  struct boards_table *board_spec,
  unsigned long long depth,
  unsigned long long *counter,
  unsigned verbosity
);


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
  if (region == (void*)-1)
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
        board_place (board, x - 1, y, data[i - 1] - '0' - 1);
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


void
ansi_cursor_show (bool show)
{
  if (show)
    fputs("\e[?25h", stdout);
  else
    fputs("\e[?25l", stdout);
}

static void
print_board_verbose (
  const struct board *board,
  unsigned whence_x,
  unsigned whence_y
)
{
  for (board_pos y = 0; y < 9; ++y)
  {
    for (board_pos x = 0; x < 9; ++x)
    {
      for (element_value vy = 0; vy < 3; ++vy)
        for (element_value vx = 0; vx < 3; ++vx)
        {
          element_value check = vx + (vy * 3);

          ansi_set_cursor (whence_x + (x * 4) + vx, whence_y + (y * 4) + vy);
          
          if (board_has_value (board, x, y))
            printf ("%u", board_get_value (board, x, y) + 1);
          else if (board_is_marked (board, x, y, check))
            printf (COLOUR_RED "%u" COLOUR_RESET, check + 1);
          else
            fputs (" ", stdout);

          if (vx == 2 && x != 8)
            fputs ("|", stdout);
        }
    }
    
    ansi_set_cursor (0, (y * 4) + 3);
    if (y != 8)
    {
      for (unsigned i = 0; i < (4 * 9) - 1; ++i)
        if ((i + 1) % 4 == 0)
          fputs ("+", stdout);
        else
          fputs ("-", stdout);
    }
  }

  fflush (stdout);
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
    if ((potential & 1) == 1)
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
#ifdef NOVERB
bool
simplify (
  struct boards_table *board_specs,
  unsigned long long depth
)
#else
bool
simplify (
  struct boards_table *board_specs,
  unsigned long long depth,
  unsigned long long *counter,
  unsigned verbosity
)
#endif
{
  /* Get current table */
  struct board *board = board_specs->board_specs[depth];

#ifndef NOVERB
  if (verbosity > 0)
  {
    if (((*counter) & (0xFFFF >> (4 * (4 - verbosity)))) == 0)
    {
      print_board_verbose (board, 0, 0);
      ansi_set_cursor (0, 35);
      printf ("Iteration: %llu", *counter);
    }
    *counter += 1;
  }
#endif


  bool error;

  unsigned count = 0;

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

            ++count;

            if (! board_place (board, x, y, value))
              return false;
          }
        }

    board_refresh_complexity (board);
  }

  /* Attempt to reduce with speculative placement */
  if (board->complexity > 1)
  {
    for (board_pos y = 0; y < 9; ++y)
      for (board_pos x = 0; x < 9; ++x)
      {
        struct board_element *elem = BOARD_ELEM (board, x, y);
        /* Find a simplest element on the board */
        if (
            ! elem->has_value &&
            elem->complexity == board->complexity
        )
          for (element_value value = 0; value < 9; ++value)
          {
            /* Try speculative placement of each potential value and recurse */
            tables_ensure_depth (board_specs, depth + 1);
            if (elem_is_marked (elem, value))
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
              {
                if (! elem_unmark (elem, value))
                  return false;
                continue;
              }

              /* Found solution */

              if (
#ifdef NOVERB
                  simplify (
                    board_specs,
                    depth + 1
                  ) &&
#else
                  simplify (
                    board_specs,
                    depth + 1,
                    counter,
                    verbosity
                  ) &&
#endif
                  board_spec->complexity == 0)
              {
                board_copy (board_spec, board);
                x = 9;
                y = 9;
                value = 9;
              }
              else if (! elem_unmark (elem, value))
                return false;
            }
          }
      }
  }
  return true;
}


struct args
argparse (int argc, char **argv)
{
  struct args result;
  result.file_name = NULL;
  result.valid = true;
  result.verbosity = 0;
  if (argc < 2)
  {
    result.valid = false;
    return result;
  }

  for (int i = 1; i < argc; ++i)
    if (strncmp (argv[i], "-", 1) == 0)
    {
      if (result.verbosity != 0)
      {
        result.valid = false;
        return result;
      }
      if (strcmp (argv[i], "-v") == 0)
        result.verbosity = 1;
      else if (strcmp (argv[i], "-vv") == 0)
        result.verbosity = 2;
      else
      {
        result.valid = false;
        return result;
      }
    }
    else if (result.file_name == NULL)
      result.file_name = argv[i];
    else
    {
      result.valid = false;
      return result;
    }
  return result;
}



int
main (int argc, char **argv, char **env)
{
  struct args args = argparse (argc, argv);
  if (! args.valid)
  {
    fputs ("Badly formatted arguments! Usage:\n\t./sudoku [-v[v]] {file name}\n", stderr);
    return 1;
  }
  
  struct board_file file = load_board_file (args.file_name);
  if (file.fd == -1 || file.data == NULL)
    return -1;

  ansi_cursor_show (false);

  /* Allocate boards */
  struct board original;

  struct boards_table boards;
  boards.max_depth = 0;
  boards.board_specs = NULL;
  tables_ensure_depth (&boards, 0);

  struct board *root_board = boards.board_specs[0];

  copy_to_board (file, &original);
  board_copy (&original, boards.board_specs[0]);

  close_board_file (file);



  ansi_clear_screen ();
  
  if (! board_is_valid (root_board))
  {
    fputs ("Supplied board is not valid!\n", stderr);

    ansi_cursor_show (true);
    
    return 1;
  }
  

  if (args.verbosity == 0)
    puts ("Simplifying...");


  board_refresh_complexity (root_board);

  /* Profiler start time */
  clock_t start_clk = clock ();

  unsigned long long counter = 0;
  
  simplify (&boards, 0, &counter, args.verbosity);

  /* Profiler end time */
  clock_t end_clk = clock ();

  ansi_clear_screen ();

  if (root_board->complexity == 0)
  {
    print_board (&original, NULL, 0, 0);
    print_board (root_board, &original, 21, 0);
    ansi_set_cursor (0, 18);
  }
  else
  {
    print_board_verbose (root_board, 0, 0);
    ansi_set_cursor (0, 36);
  }
  printf ("Simplification took %Lf seconds\n", ((long double)(end_clk - start_clk))/CLOCKS_PER_SEC);


  ansi_cursor_show (true);

  return 0;
}
