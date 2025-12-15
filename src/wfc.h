#ifndef WFC_H
#define WFC_H

#include <stdbool.h>

// status codes
#define WFC_OK 0
#define WFC_ERROR 1

// stores a location
typedef struct {
    size_t x, y;
} WFC_Point;

// to the color of a pixel
typedef struct {
    unsigned char r, g, b;
} WFC_Color;

// struct to store the image data
typedef struct {
    WFC_Color *data;
    size_t height, width;
    bool had_error;
} WFC_Bitmap;

/*
 * read and parse a PPM3 P3 file
 *
 * sets had_error to true in case of an error
 */
WFC_Bitmap WFC_read_image(const char *filename);

// the amount of directions
#define WFC_DIRECTION_COUNT 4

// stores a direction
typedef enum {
    WFC_DIR_UP = 0,
    WFC_DIR_DOWN = 1,
    WFC_DIR_LEFT = 2,
    WFC_DIR_RIGHT = 3,
} WFC_Dir;

// stores the neighbors of a specific point, the indices are a direction
typedef struct {
    WFC_Point *neighbors[WFC_DIRECTION_COUNT];
    size_t lengths[WFC_DIRECTION_COUNT];
    size_t capacities[WFC_DIRECTION_COUNT];
    bool had_error;
} WFC_Neighbors;

// free the heap allocated data from WFC_Neighbors
void free_neighbors(WFC_Neighbors *neighbors);

// hashmap with a point as key and its neighbors as value
typedef struct {
    WFC_Point key;
    WFC_Neighbors value;
    bool active;
} WFC_NeighborMapEntry;

// stores the value of a entry and if there was an error
typedef struct {
    WFC_NeighborMapEntry *data;
    size_t capacity;
    size_t length;
    bool had_error;
} WFC_NeighborMap;

/*
 * extracts the patterns from an image
 *
 * sets had_error to true in case of an error
 */
WFC_NeighborMap WFC_extract_patterns(WFC_Bitmap bitmap, size_t region_size);

/*
 * create a new image based on a patterns of a bitmap
 *
 * output_size is the size in tiles, the size of the bitmap will be
 * region_size * output_size
 */
WFC_Bitmap WFC_Solve(
    WFC_NeighborMap map,
    WFC_Bitmap bitmap,
    size_t region_size,
    WFC_Point output_size
);

/*
 * read and parse a PPM3 P3 file
 *
 * returns WFC_ERROR in case of an error
 */
int WFC_write_image(const char *filename, WFC_Bitmap bitmap);

#endif
