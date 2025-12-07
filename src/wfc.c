#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wfc.h"

WFC_Bitmap WFC_read_file(const char *filename) {
    WFC_Bitmap result = { .data = NULL,
        .height = 0,
        .width = 0,
        .status_code = WFC_ERROR
    };

    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "failed to open file\n");
        goto cleanup;
    }

    char header[3];
    if (fscanf(file, "%2s", header) != 1) {
        fprintf(stderr, "invalid PPM file\n");
        goto cleanup;
    }
    if (strcmp(header, "P3") != 0) {
        fprintf(stderr, "invalid PPM file\n");
        goto cleanup;
    }

    int width, height;
    if (fscanf(file, "%d %d", &width, &height) != 2) {
        fprintf(stderr, "invalid PPM file\n");
        goto cleanup;
    }
    result.width = width;
    result.height = height;

    const int size = width * height;
    result.data = (WFC_Color*)malloc(sizeof(WFC_Color) * size);
    if (!result.data) {
        fprintf(stderr, "malloc failed\n");
        goto cleanup;
    }

    int i = 0;
    int r,g,b;
    while (fscanf(file, "%d %d %d", &r, &b, &g) == 3) {
        if (i >= size) {
            fprintf(stderr, "invalid PPM file\n");
            goto cleanup;
        }
        WFC_Color c = { .r = r, .b = b, .g = g };
        result.data[i] = c;
        i++;
    }

    if (i != size || !feof(file)) {
        fprintf(stderr, "invalid PPM file\n");
        goto cleanup;
    }

    result.status_code = WFC_OK;

cleanup:
    if (file) fclose(file);

    if (result.status_code != WFC_OK) {
        if (result.data) free(result.data);
    }

    return result;
}

int WFC_extract_patterns(WFC_Bitmap input) {
    return WFC_ERROR;
}
