#ifndef EMBIGGEN_H
#define EMBIGGEN_H

#include <stdio.h>
#include <stdbool.h>

// PBM bitmap structure
typedef struct {
    int width;
    int height;
    bool **pixels;  // pixels[x][y] - indexed by column, then row
} pbm_bitmap;

// Coordinate pair for offset calculations
typedef struct {
    int x;
    int y;
} coord_pair;

// Function prototypes
pbm_bitmap *pbm_read(FILE *fd);
void pbm_write(FILE *fd, pbm_bitmap *bitmap);
void pbm_free(pbm_bitmap *bitmap);
pbm_bitmap *embiggen(pbm_bitmap *input, double delta);

// Helper functions
pbm_bitmap *pbm_new(int width, int height);
coord_pair *calculate_offsets(double delta, int *count);
int coord_compare(const void *a, const void *b);

#endif // EMBIGGEN_H