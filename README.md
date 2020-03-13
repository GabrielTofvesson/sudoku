# Sudoku solver
A simple sudoku problem solver because I got bored of solving them manually.

## WASM Adaptation
This branch has been specially adapted to work with WebAssembly: specifically,
Emcripten. To compile, simply run `emcc main.c board.c -s WASM=1 -o sudoku.html`
and then open the generated html file in a browser that supports WASM.
**NOTE:** Emscripten has a terrible default *stdin*, so to enter a board into
the program, one must enter it as one line, press **OK**, then, when the second
input text box appears, press **Cancel**.

## File format
The sudoku solver accepts a file for the following format:
* Each row consists of 9 characters (+ newline)

* Each character on a row (aside from newline) must be '0'-'9' or ' ' (if value is unknown)

* There must be a total of 9 rows

For example, a file might look as follows:
```
 23456789
2 4567891
34 678912
456 89123
5678 1234
67891 345
789123 56
8912345 7
91234567 
```

or

```
123456789
2 456 891
3456789  
456 89123
5678 12 4
6  91 345
789123456
8 12345 7
912345678
```

## Optimization
An optimization directive has been included in `board.c` to allow for the near
complete removal of boundary checks and error conditions. To enable this
optimization, simply add `-DOPTIMIZE` to your compiler flags.

## TODO

* Optimizations

  * Packed structures

  * Better structure layouts

  * Optimize speculative value placement error-condition checks

* Cleaner code

  * Move printing to separate file

  * Move board solver algorithm to separate file

* Better error messages

* Makefile
