#ifndef EMBIGGEN_H
#define EMBIGGEN_H

#include <stdio.h>
#include <netpbm/pbm.h>

// Function prototypes using netpbm types
bit **pbm_read_file(FILE *fp, int *cols, int *rows);
void pbm_write_file(FILE *fp, bit **bits, int cols, int rows);
bit **embiggen(bit **input, int cols, int rows, double delta, int *out_cols, int *out_rows);

#endif // EMBIGGEN_H