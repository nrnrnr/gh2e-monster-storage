#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
        // Use pnmtoplainpnm to convert input to plain PBM
        char command[1024];
        snprintf(command, sizeof(command), "pnmtoplainpnm %s", filename);
        input_fd = popen(command, "r");
        if (!input_fd) {
            fprintf(stderr, "Error: Failed to open input file through pnmtoplainpnm\n");
            exit(1);
        }
    } else {
        input_fd = stdin;
    }
    
    // Read input bitmap
    pbm_bitmap *input = pbm_read(input_fd);
    if (!input) {
        fprintf(stderr, "Error: Failed to read PBM input\n");
        if (filename) pclose(input_fd);
        exit(1);
    }
    
    if (filename) {
        pclose(input_fd);
    }
    
    // Calculate delta in pixels
    double delta = dpi * mm / 25.4;
    
    // Embiggen the bitmap
    pbm_bitmap *output = embiggen(input, delta);
    if (!output) {
        fprintf(stderr, "Error: Failed to embiggen bitmap\n");
        pbm_free(input);
        exit(1);
    }
    
    // Write output
    pbm_write(stdout, output);
    
    // Cleanup
    pbm_free(input);
    pbm_free(output);
    
    return 0;
}