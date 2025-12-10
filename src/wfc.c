#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wfc.h"
#include "wfc_hashmap.h"

int read_header(FILE *file, WFC_Bitmap *result) {
    char header[3];
    if (fscanf(file, "%2s", header) != 1 || strcmp(header, "P3") != 0) {
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

    size_t i = 0;
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

WFC_Color bitmap_get(WFC_Bitmap bitmap, size_t x, size_t y) {
    return bitmap.data[y * bitmap.width + x];
}

WFC_Bitmap extract_region(WFC_Bitmap bitmap, size_t region_size, size_t x, size_t y) {
    WFC_Bitmap result = {
        .data = NULL,
        .height = region_size,
        .width = region_size,
        .status_code = WFC_ERROR,
    };

    result.data = malloc(sizeof(*result.data) * result.width * result.height);
    if (result.data == NULL) {
        fprintf(stderr, "malloc failed\n");
        goto cleanup;
    }

    for (size_t y_offset = 0; y_offset < region_size; y_offset++) {
        for (size_t x_offset = 0; x_offset < region_size; x_offset++) {
            size_t new_x = x + x_offset;
            size_t new_y = y + y_offset;
            result.data[y_offset * region_size + x_offset] =
                bitmap_get(bitmap, new_x, new_y);
        }
    }

    result.status_code = WFC_OK;

cleanup:
    if (result.status_code != WFC_OK) {
        free(result.data);
        result.data = NULL;
    }

    return result;
}

// TODO: WFC_Neighbors is defined in wfc_hashmap.h, not the best place for it.
// move it to a seperate file, or to wfc.h?
WFC_Neighbors find_neighbors(WFC_Bitmap bitmap, size_t region_size, size_t x, size_t y) {
    int offsets[4][2] = {
        {-region_size, 0},
        {region_size, 0},
        {0, region_size},
        {0, -region_size},
    };

    for (int i = 0; i < 4; i++) {
        printf("%d %d\n", offsets[i][0], offsets[i][1]);
    }

    // TODO: finish this function
}

WFC_Patterns WFC_extract_patterns(WFC_Bitmap bitmap, size_t region_size) {
    WFC_Patterns result = {
        .patterns = NULL,
        .region_size = region_size,
        .status_code = WFC_ERROR
    };

    WFC_PosMap posmap = new_wfc_posmap();

    for (size_t y = 0; y < bitmap.height - region_size; y++) {
        for (size_t x = 0; x < bitmap.width - region_size; x++) {
            WFC_Bitmap region = extract_region(bitmap, region_size, x, y);
            WFC_Point pattern_location = { .x = x, .y = y };

            WFC_PosMapResult location =
                wfc_posmap_find_or_insert(&posmap, region, pattern_location);
            pattern_location = location.value;

            WFC_Bitmap *neighbors = find_neighbors(bitmap, region_size, x, y);

            // TODO:
            // * find neighbors
            // * get index of neighbors (move code block above in seperate fucntion)
            // * create a hashmap that stores the patterns (remove WFC_Pattern struct in favor of hashmap)
            // * profit

            // TODO: debug info: remove
            int identifier = pattern_location.y * bitmap.width + pattern_location.x;
            printf("%d %d -> %d\n", x, y, identifier);
            for (size_t y = 0; y < region.height; y++) {
                for (size_t x = 0; x < region.width; x++) {
                    WFC_Color c = region.data[y*region.width+x];
                    printf("%d ", c.r/255);
                }
                printf("\n");
            }
            printf("\n");

        }
    }

    wfc_posmap_free(&posmap);

cleanup:
    return result;
}
