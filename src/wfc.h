#ifndef WFC_H
#define WFC_H

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
    int status_code;
} WFC_Bitmap;

/*
 * read and parse a PPM3 P3 file
 */
WFC_Bitmap WFC_read_file(const char *filename);

// stores a region and the possible neighbours
typedef struct {
    WFC_Point location; // index of the pattern
    WFC_Point *up, *down, *left, *right; // possible neighbours
} WFC_Pattern;

// stores all possible neighbours
typedef struct {
    WFC_Pattern *patterns;
    size_t region_size;
    int status_code;
} WFC_Patterns;

/*
 * extracts the patterns from an image
 *
 * returns NULL in case of an error
 */
WFC_Patterns WFC_extract_patterns(WFC_Bitmap bitmap, size_t region_size);

#endif
