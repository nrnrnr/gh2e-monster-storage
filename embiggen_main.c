#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netpbm/pbm.h>
#include <netpbm/pm.h>
#include "embiggen.h"

void usage(const char *progname) {
    fprintf(stderr, "Usage: %s -dpi <dpi> -mm <mm> [file]\n", progname);
    fprintf(stderr, "  -dpi <num>  dots per inch (default: 300)\n");
    fprintf(stderr, "  -mm <num>   delta in millimeters (required)\n");
    fprintf(stderr, "  [file]      input PBM file (default: stdin)\n");
    exit(1);
}

int main(int argc, char *argv[]) {
    double dpi = 300.0;
    double mm = -1.0;  // Required parameter
    char *filename = NULL;
    
    // Initialize netpbm
    pbm_init(&argc, argv);
    
    // Manual argument parsing to handle -dpi and -mm
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-dpi") == 0 && i + 1 < argc) {
            dpi = atof(argv[++i]);
        } else if (strcmp(argv[i], "-mm") == 0 && i + 1 < argc) {
            mm = atof(argv[++i]);
        } else if (argv[i][0] != '-') {
            // Non-option argument is filename
            filename = argv[i];
        } else {
            usage(argv[0]);
        }
    }
    
    // Validate required parameters
    if (mm < 0) {
        fprintf(stderr, "Error: -mm parameter is required\n");
        usage(argv[0]);
    }
    
    // Open input file
    FILE *input_fd;
    if (filename) {
        input_fd = fopen(filename, "rb");
        if (!input_fd) {
            pm_error("Failed to open input file: %s", filename);
        }
    } else {
        input_fd = stdin;
    }
    
    // Read input bitmap
    int cols, rows;
    bit **input = pbm_read_file(input_fd, &cols, &rows);
    
    if (filename) {
        fclose(input_fd);
    }
    
    // Calculate delta in pixels
    double delta = dpi * mm / 25.4;
    
    // Embiggen the bitmap
    int out_cols, out_rows;
    bit **output = embiggen(input, cols, rows, delta, &out_cols, &out_rows);
    
    // Write output
    pbm_write_file(stdout, output, out_cols, out_rows);
    
    // Cleanup
    pbm_freearray(input, rows);
    pbm_freearray(output, out_rows);
    
    return 0;
}