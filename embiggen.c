#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <netpbm/pbm.h>
#include <netpbm/pm.h>
#include "embiggen.h"

// Coordinate pair for offset calculations
typedef struct {
    int x;
    int y;
} coord_pair;

// Comparison function for coordinate sorting (by distance from origin, then lexicographic)
int coord_compare(const void *a, const void *b) {
    const coord_pair *c1 = (const coord_pair *)a;
    const coord_pair *c2 = (const coord_pair *)b;
    
    int d1 = c1->x * c1->x + c1->y * c1->y;
    int d2 = c2->x * c2->x + c2->y * c2->y;
    
    if (d1 > d2) return -1;  // Larger distance first (reverse order)
    if (d2 > d1) return 1;
    
    // Same distance, sort by x then y
    if (c1->x != c2->x) return c1->x - c2->x;
    return c1->y - c2->y;
}

// Calculate coordinate offsets within delta radius
coord_pair *calculate_offsets(double delta, int *count) {
    int max_coords = (int)((delta + 1) * (delta + 1) * 4 + 10);  // Generous estimate
    coord_pair *coords = malloc(max_coords * sizeof(coord_pair));
    if (!coords) pm_error("Failed to allocate coordinate array");
    
    double d2 = delta * delta;
    int n = 0;
    
    // Add origin
    coords[n++] = (coord_pair){0, 0};
    
    // Add all coordinates within delta distance
    for (int x = 0; x <= (int)delta; x++) {
        for (int y = 0; y <= (int)delta; y++) {
            double squared = x * x + y * y;
            if (squared > 0 && squared <= d2) {
                coords[n++] = (coord_pair){x, y};
                if (y != 0) coords[n++] = (coord_pair){x, -y};
                if (x != 0) {
                    coords[n++] = (coord_pair){-x, y};
                    if (y != 0) coords[n++] = (coord_pair){-x, -y};
                }
            }
        }
    }
    
    fprintf(stderr, "Precalculated %d coordinates\n", n);
    
    // Sort coordinates by distance (largest first)
    fprintf(stderr, "Sorting coordinates...");
    qsort(coords, n, sizeof(coord_pair), coord_compare);
    fprintf(stderr, " sorted!\n");
    
    *count = n;
    return coords;
}

// Read PBM file using netpbm library
bit **pbm_read_file(FILE *fp, int *cols, int *rows) {
    return pbm_readpbm(fp, cols, rows);
}

// Write PBM file using netpbm library (compact format)
void pbm_write_file(FILE *fp, bit **bits, int cols, int rows) {
    pbm_writepbm(fp, bits, cols, rows, 0);  // 0 = use compact format
}

// Embiggen bitmap by delta pixels
bit **embiggen(bit **input, int cols, int rows, double delta, int *out_cols, int *out_rows) {
    fprintf(stderr, "Embiggening at delta %g\n", delta);
    
    // Calculate offset coordinates
    int coord_count;
    coord_pair *coords = calculate_offsets(delta, &coord_count);
    
    // Create output bitmap (same dimensions)
    bit **output = pbm_allocarray(cols, rows);
    *out_cols = cols;
    *out_rows = rows;
    
    // For each pixel in output
    long tests = 0;
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            output[row][col] = PBM_WHITE;
            
            // Check each offset coordinate
            for (int i = 0; i < coord_count; i++) {
                tests++;
                if (tests % 10000000 == 0) {
                    fprintf(stderr, "Test (%d, %d)\n", col, row);
                    fflush(stderr);
                }
                
                int x = col + coords[i].x;
                int y = row + coords[i].y;
                
                // Check bounds and pixel value
                if (x >= 0 && x < cols && y >= 0 && y < rows) {
                    if (input[y][x] == PBM_BLACK) {
                        output[row][col] = PBM_BLACK;
                        break;
                    }
                }
            }
        }
    }
    
    free(coords);
    return output;
}