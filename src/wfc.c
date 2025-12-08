#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wfc.h"

int read_header(FILE *file, WFC_Bitmap *result) {
    char header[3];
    if (fscanf(file, "%2s", &header) != 1 || strcmp(header, "P3") != 0) {
        fprintf(stderr, "unsupported PPM format\n");
        return WFC_ERROR;
    }

    int width, height, maxval;
    if (
        fscanf(file, "%d %d %d", &width, &height, &maxval) != 3
        || width <= 0 
        || height <= 0
        || maxval != 255
    ) {
        fprintf(stderr, "invalid PPM file\n");
        return WFC_ERROR;
    }
    result->width = width;
    result->height = height;

    return WFC_OK;
}

int read_colors(FILE *file, WFC_Bitmap *result) {
    const size_t size = result->width * result->height;
    result->data = malloc(sizeof(*result->data) * size);
    if (!result->data) {
        fprintf(stderr, "malloc failed\n");
        return WFC_ERROR;
    }

    int i = 0;
    int r, g, b;
    while (fscanf(file, "%d %d %d", &r, &g, &b) == 3) {
        if (i >= size) {
            fprintf(stderr, "invalid PPM file\n");
            return WFC_ERROR;
        }
        WFC_Color c = { .r = r, .b = b, .g = g };
        result->data[i] = c;
        i++;
    }

    if (i != size || !feof(file)) {
        fprintf(stderr, "invalid PPM file\n");
        return WFC_ERROR;
    }

    return WFC_OK;
}

WFC_Bitmap WFC_read_file(const char *filename) {
    WFC_Bitmap result = {
        .data = NULL,
        .height = 0,
        .width = 0,
        .status_code = WFC_ERROR
    };

    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "failed to open file\n");
        goto cleanup;
    }

    if (read_header(file, &result) != WFC_OK) {
        goto cleanup;
    }

    if (read_colors(file, &result) != WFC_OK) {
        goto cleanup;
    }

    result.status_code = WFC_OK;

cleanup:
    if (file) {
        fclose(file);
    }

    if (result.status_code != WFC_OK) {
        free(result.data);
        result.data = NULL;
    }

    return result;
}
