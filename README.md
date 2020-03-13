# Sudoku solver
A simple sudoku problem solver because I got bored of solving them manually.

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


## Compiling and running
To test the functionality, simply run `gcc -o sudoku main.c board.c`, then
`./sudoku [-v[v]] {filename}`, where the given file is formatted according to
the aforementioned specifications.

## Optimization
An optimization directive has been included in `board.c` to allow for the near
complete removal of boundary checks and error conditions. To enable this
optimization, simply add `-DOPTIMIZE` to your compiler flags. For insane speeds,
don't forget the `-O3` gcc flag.

## Live status output
If you would like to get a live status output of speculative value placement,
include the `-v` for verbose-mode solving (or `-vv` for more verbosity) when
running the sudoku program. **Please note:** Most boards will be solved in a
matter of milliseconds (at most) on modern machines, so these verbosity options
are really only useful on much slower devices.

## TODO

* Cleaner code

  * Move printing to separate file

  * Move board solver algorithm to separate file

* Better error messages

* Makefile
