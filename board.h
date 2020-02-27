/**
 * Sudoku board header file
 *
 * Created by Gabriel Tofvesson
 */



/**
 * Simple definition of a board element. Vaild values range: 0-8
 */
struct board_element {
  unsigned char has_value : 1;        /* Whether element has a decided value */

  union {
    unsigned char  value     : 4;     /* Value of element */
    unsigned short potential : 9;     /* Bitfield of possible values */
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
