#ifndef WFC_H
#define WFC_H

/* status codes */
#define WFC_OK 0
#define WFC_ERROR 1

/* to the color of a pixel */
typedef struct {
    unsigned char r, g, b;
} WFC_Color;

/* struct to store the image data */
typedef struct {
    WFC_Color *data;
    int height, width;
    int status_code; /* a WFC status code */
} WFC_Bitmap;

/*
 * read and parse a PPM3 P3 file
 *
 * errors if the given file is invalid or doesn't exist, errors are shown using
 * the status_code of the return value
 */
WFC_Bitmap WFC_read_file(const char *filename);

#endif
