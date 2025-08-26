#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "embiggen.h"

// Create new bitmap with given dimensions
pbm_bitmap *pbm_new(int width, int height) {
    pbm_bitmap *bitmap = malloc(sizeof(pbm_bitmap));
    if (!bitmap) return NULL;
    
    bitmap->width = width;
    bitmap->height = height;
    
    // Allocate array of column pointers
    bitmap->pixels = malloc(width * sizeof(bool*));
    if (!bitmap->pixels) {
        free(bitmap);
        return NULL;
    }
    
    // Allocate each column
    for (int x = 0; x < width; x++) {
        bitmap->pixels[x] = calloc(height, sizeof(bool));
        if (!bitmap->pixels[x]) {
            // Clean up on failure
            for (int i = 0; i < x; i++) {
                free(bitmap->pixels[i]);
            }
            free(bitmap->pixels);
            free(bitmap);
            return NULL;
        }
    }
    
    return bitmap;
}

// Free bitmap memory
void pbm_free(pbm_bitmap *bitmap) {
    if (!bitmap) return;
    
    if (bitmap->pixels) {
        for (int x = 0; x < bitmap->width; x++) {
            free(bitmap->pixels[x]);
        }
        free(bitmap->pixels);
    }
    free(bitmap);
}

// Read PBM from file descriptor
pbm_bitmap *pbm_read(FILE *fd) {
    char line[256];
    char format[8];
    int width, height;
    
    // Read header
    if (!fgets(line, sizeof(line), fd)) {
        fprintf(stderr, "Error reading PBM header\n");
        return NULL;
    }
    
    // Parse format
    if (sscanf(line, "%7s %d %d", format, &width, &height) == 3) {
        // Header on one line
    } else if (sscanf(line, "%7s", format) == 1) {
        // Format on separate line, read dimensions
        if (!fgets(line, sizeof(line), fd)) {
            fprintf(stderr, "Error reading PBM dimensions\n");
            return NULL;
        }
        if (sscanf(line, "%d %d", &width, &height) != 2) {
            fprintf(stderr, "Error parsing PBM dimensions\n");
            return NULL;
        }
    } else {
        fprintf(stderr, "Error parsing PBM header\n");
        return NULL;
    }
    
    // Validate format
    if (strcmp(format, "P1") != 0) {
        fprintf(stderr, "Need plain PBM format (P1), got %s\n", format);
        return NULL;
    }
    
    // Skip comments
    int c;
    while ((c = fgetc(fd)) != EOF) {
        if (c == '#') {
            // Skip comment line
            while ((c = fgetc(fd)) != EOF && c != '\n');
        } else if (isspace(c)) {
            continue;
        } else {
            ungetc(c, fd);
            break;
        }
    }
    
    // Create bitmap
    pbm_bitmap *bitmap = pbm_new(width, height);
    if (!bitmap) {
        fprintf(stderr, "Failed to allocate bitmap\n");
        return NULL;
    }
    
    // Read pixel data
    int row = 0, col = 0;
    while ((c = fgetc(fd)) != EOF) {
        if (c == '0' || c == '1') {
            if (row >= height || col >= width) {
                fprintf(stderr, "Too many pixels in PBM data\n");
                pbm_free(bitmap);
                return NULL;
            }
            
            // Note: Lua uses [col][row], x=col, y=row indexing
            bitmap->pixels[col][row] = (c == '1');
            col++;
            if (col >= width) {
                col = 0;
                row++;
            }
        } else if (isspace(c)) {
            continue;
        } else {
            fprintf(stderr, "Invalid character in PBM data: %c\n", c);
            pbm_free(bitmap);
            return NULL;
        }
    }
    
    if (row != height || col != 0) {
        fprintf(stderr, "Incomplete PBM data: expected %d pixels, got %d\n", 
                width * height, row * width + col);
        pbm_free(bitmap);
        return NULL;
    }
    
    return bitmap;
}

// Write PBM to file descriptor
void pbm_write(FILE *fd, pbm_bitmap *bitmap) {
    fprintf(fd, "P1 %d %d\n", bitmap->width, bitmap->height);
    
    for (int y = 0; y < bitmap->height; y++) {
        for (int x = 0; x < bitmap->width; x++) {
            if ((x + 1) % 65 == 64) {
                fputc('\n', fd);
            }
            fputc(bitmap->pixels[x][y] ? '1' : '0', fd);
        }
        fputc('\n', fd);
    }
}

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
    if (!coords) return NULL;
    
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

// Embiggen bitmap by delta pixels
pbm_bitmap *embiggen(pbm_bitmap *input, double delta) {
    fprintf(stderr, "Embiggening at delta %g\n", delta);
    
    // Calculate offset coordinates
    int coord_count;
    coord_pair *coords = calculate_offsets(delta, &coord_count);
    if (!coords) {
        fprintf(stderr, "Failed to calculate coordinates\n");
        return NULL;
    }
    
    // Create output bitmap
    pbm_bitmap *output = pbm_new(input->width, input->height);
    if (!output) {
        free(coords);
        return NULL;
    }
    
    // For each pixel in output
    long tests = 0;
    for (int col = 0; col < input->width; col++) {
        for (int row = 0; row < input->height; row++) {
            output->pixels[col][row] = false;
            
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
                if (x >= 0 && x < input->width && y >= 0 && y < input->height) {
                    if (input->pixels[x][y]) {
                        output->pixels[col][row] = true;
                        break;
                    }
                }
            }
        }
    }
    
    free(coords);
    return output;
}