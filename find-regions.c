#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pam.h>

typedef struct {
    int left, top, right, bottom;
    int pixel_count;
    int center_x, bottom_y;
    int rounded_x, rounded_y;
    int region_number;
} BoundingBox;

typedef struct {
    int x, y;
} Point;

typedef struct {
    Point* points;
    int capacity;
    int size;
} Stack;

static Stack* stack_create(int capacity) {
    Stack* s = malloc(sizeof(Stack));
    s->points = malloc(capacity * sizeof(Point));
    s->capacity = capacity;
    s->size = 0;
    return s;
}

static void stack_destroy(Stack* s) {
    free(s->points);
    free(s);
}

static void stack_push(Stack* s, int x, int y) {
    if (s->size >= s->capacity) {
        s->capacity *= 2;
        s->points = realloc(s->points, s->capacity * sizeof(Point));
    }
    s->points[s->size].x = x;
    s->points[s->size].y = y;
    s->size++;
}

static int stack_pop(Stack* s, int* x, int* y) {
    if (s->size == 0) return 0;
    s->size--;
    *x = s->points[s->size].x;
    *y = s->points[s->size].y;
    return 1;
}

static void flood_fill(unsigned char** bitmap, int** visited, int width, int height, 
                      int start_x, int start_y, BoundingBox* bbox, int region_id) {
    Stack* stack = stack_create(1000);
    
    bbox->left = bbox->right = start_x;
    bbox->top = bbox->bottom = start_y;
    bbox->pixel_count = 0;
    
    stack_push(stack, start_x, start_y);
    
    while (stack->size > 0) {
        int x, y;
        if (!stack_pop(stack, &x, &y)) break;
        
        if (x < 0 || x >= width || y < 0 || y >= height) continue;
        if (visited[y][x] != 0 || bitmap[y][x] == 0) continue;
        
        visited[y][x] = region_id;
        bbox->pixel_count++;
        
        if (x < bbox->left) bbox->left = x;
        if (x > bbox->right) bbox->right = x;
        if (y < bbox->top) bbox->top = y;
        if (y > bbox->bottom) bbox->bottom = y;
        
        // 4-connectivity
        stack_push(stack, x + 1, y);
        stack_push(stack, x - 1, y);
        stack_push(stack, x, y + 1);
        stack_push(stack, x, y - 1);
    }
    
    stack_destroy(stack);
}

int main(int argc, char* argv[]) {
    struct pam inpam;
    tuple* tuplerow;
    unsigned char** bitmap;
    int** visited;
    BoundingBox* bboxes;
    int num_regions = 0;
    int max_regions = 1000;
    
    if (argc != 3) {
        fprintf(stderr, "Usage: %s input.pbm coordinates.txt\n", argv[0]);
        return 1;
    }
    
    pm_init(argv[0], 0);
    
    FILE* input = fopen(argv[1], "r");
    if (!input) {
        fprintf(stderr, "Cannot open %s\n", argv[1]);
        return 1;
    }
    
    pnm_readpaminit(input, &inpam, PAM_STRUCT_SIZE(tuple_type));
    
    if (inpam.format != PBM_FORMAT) {
        fprintf(stderr, "Input must be PBM format\n");
        return 1;
    }
    
    // Allocate bitmap and visited arrays
    bitmap = malloc(inpam.height * sizeof(unsigned char*));
    visited = malloc(inpam.height * sizeof(int*));
    for (int y = 0; y < inpam.height; y++) {
        bitmap[y] = malloc(inpam.width * sizeof(unsigned char));
        visited[y] = calloc(inpam.width, sizeof(int));
    }
    
    // Read the entire image into memory
    tuplerow = pnm_allocpamrow(&inpam);
    for (int y = 0; y < inpam.height; y++) {
        pnm_readpamrow(&inpam, tuplerow);
        for (int x = 0; x < inpam.width; x++) {
            bitmap[y][x] = (tuplerow[x][0] == PAM_BLACK) ? 1 : 0;
        }
    }
    fclose(input);
    pnm_freepamrow(tuplerow);
    
    bboxes = malloc(max_regions * sizeof(BoundingBox));
    
    // Find connected components
    for (int y = 0; y < inpam.height; y++) {
        for (int x = 0; x < inpam.width; x++) {
            if (bitmap[y][x] == 1 && visited[y][x] == 0) {
                if (num_regions >= max_regions) {
                    max_regions *= 2;
                    bboxes = realloc(bboxes, max_regions * sizeof(BoundingBox));
                }
                
                BoundingBox bbox;
                flood_fill(bitmap, visited, inpam.width, inpam.height, 
                          x, y, &bbox, num_regions + 1);
                
                if (bbox.pixel_count < 100) {
                    fprintf(stderr, "Tiny region %d: %d pixels, bbox (%d,%d)-(%d,%d)\n",
                           num_regions + 1, bbox.pixel_count, 
                           bbox.left, bbox.top, bbox.right, bbox.bottom);
                } else {
                    bboxes[num_regions] = bbox;
                    num_regions++;
                }
            }
        }
    }
    
    // Calculate positioning info for each region
    for (int i = 0; i < num_regions; i++) {
        bboxes[i].center_x = (bboxes[i].left + bboxes[i].right) / 2;
        bboxes[i].bottom_y = bboxes[i].bottom;
        
        // Round to nearest multiple of 100
        bboxes[i].rounded_x = ((bboxes[i].center_x + 50) / 100) * 100;
        bboxes[i].rounded_y = ((bboxes[i].bottom_y + 50) / 100) * 100;
    }
    
    // Sort regions by rows (rounded_y) then by columns (rounded_x)
    for (int i = 0; i < num_regions - 1; i++) {
        for (int j = i + 1; j < num_regions; j++) {
            int swap = 0;
            if (bboxes[i].rounded_y > bboxes[j].rounded_y) {
                swap = 1;
            } else if (bboxes[i].rounded_y == bboxes[j].rounded_y && 
                       bboxes[i].rounded_x > bboxes[j].rounded_x) {
                swap = 1;
            }
            
            if (swap) {
                BoundingBox temp = bboxes[i];
                bboxes[i] = bboxes[j];
                bboxes[j] = temp;
            }
        }
    }
    
    // Assign sequential region numbers
    for (int i = 0; i < num_regions; i++) {
        bboxes[i].region_number = i + 1;
    }
    
    // Write coordinates to file
    FILE* coords = fopen(argv[2], "w");
    if (!coords) {
        fprintf(stderr, "Cannot create %s\n", argv[2]);
        return 1;
    }
    
    for (int i = 0; i < num_regions; i++) {
        int center_y = (bboxes[i].top + bboxes[i].bottom) / 2;
        fprintf(coords, "origin=\"%s\" name=%02d centerx=%d centery=%d left=%d top=%d right=%d bottom=%d\n",
               argv[1],
               bboxes[i].region_number, bboxes[i].center_x, center_y,
               bboxes[i].left, bboxes[i].top, 
               bboxes[i].right, bboxes[i].bottom);
    }
    fclose(coords);
    
    fprintf(stderr, "Found %d regions\n", num_regions);
    
    // Cleanup
    for (int y = 0; y < inpam.height; y++) {
        free(bitmap[y]);
        free(visited[y]);
    }
    free(bitmap);
    free(visited);
    free(bboxes);
    
    return 0;
}
