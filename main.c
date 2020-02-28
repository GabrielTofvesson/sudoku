#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "board.h"

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
print_board (struct board *board)
{
  for (board_pos y = 0; y < 9; ++y)
  {
    /* Print row */
    for (board_pos x = 0; x < 9; ++x)
    {
      /* Print board element */
      if (board_has_value (board, x, y))
        printf ("%u", board_get_value (board, x, y) + 1);
      else
        fputs (" ", stdout);

      /* Print column element delimiter */
      if (x < 8)
        fputs("|", stdout);
    }
    
    fputs("\n", stdout);
    
    /* Print row element delimiter */
    if (y < 8)
    {
      for (board_pos x = 0; x < 17; ++x)
      {
        if ((x & 1) == 0)
          fputs ("-", stdout);
        else
          fputs ("+", stdout);
      }
      fputs ("\n", stdout);
    }
  }
}


/**
 * Compute first potential value of a given element
 */
element_value
first_potential_value (struct board_element *element, struct board *board)
{
  unsigned short potential = element->potential;
  if (potential == 0)
  {
    print_board (board);
    abort(); /* Error */
  }
  
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
void
simplify (struct board *board)
{
  while (board->complexity == 1)
  {
    for (board_pos y = 0; y < 9; ++y)
      for (board_pos x = 0; x < 9; ++x)
        if (! board_has_value (board, x, y))
        {
          struct board_element *elem = BOARD_ELEM (board, x, y);
          if (elem->complexity == 1)
          {
            board_place (board, x, y, first_potential_value (elem, board));
          }
        }
  }
}


int
main (int argc, char **argv, char **env)
{
  if (argc != 2) return -1;
  
  struct board_file file = load_board_file (argv[1]);
  if (file.fd == -1 || file.data == NULL)
    return -1;

  struct board board;
  copy_to_board (file, &board);

  close_board_file (file);

  board_update_all_marks (&board);

  print_board (&board);

  if (board_is_valid (&board))
    printf ("Board is valid!\nBoard complexity: %u\n", board.complexity);
  else
    puts ("Board is invalid!");

  puts("\nReducing...");
  simplify (&board);

  print_board (&board);
 
  if (board_is_valid (&board))
    printf ("Board is valid!\nBoard complexity: %u\n", board.complexity);
  else
    puts ("Board is invalid!");

  return 0;
}
